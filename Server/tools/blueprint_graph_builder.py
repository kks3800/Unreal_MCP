"""
Blueprint Graph Intelligence Engine.

Transforms declarative Blueprint graph specifications into optimized
execute_blueprint_batch operations with $N references and automatic layout.

This module has NO dependencies on MCP or the Unreal connection — it is pure
graph logic that can be tested and used independently.
"""

import logging
from typing import List, Dict, Any, Optional, Tuple, Set
from collections import defaultdict, deque

logger = logging.getLogger("UnrealMCP")


# ============================================================================
# CONSTANTS
# ============================================================================

EXEC_COL_SPACING = 400   # Horizontal spacing between exec-flow columns
DATA_COL_OFFSET = -250   # Data nodes placed left of their first consumer
ROW_SPACING = 200         # Vertical spacing between nodes in the same column
DATA_ROW_OFFSET = 150     # Vertical offset for data nodes below their consumer


# ============================================================================
# DIRECT COMMAND NODE TYPES
# ============================================================================

# Node types that map directly to add_* commands (no search needed)
DIRECT_NODE_COMMANDS: Dict[str, str] = {
    "Branch": "add_branch_node",
    "Sequence": "add_sequence_node",
    "ForEachLoop": "add_for_each_loop_node",
    "WhileLoop": "add_while_loop_node",
    "Gate": "add_gate_node",
    "DoOnce": "add_do_once_node",
    "FlipFlop": "add_flip_flop_node",
    "Delay": "add_delay_node",
    "SwitchOnInt": "add_switch_on_int_node",
    "SwitchOnString": "add_switch_on_string_node",
    "SwitchOnEnum": "add_switch_on_enum_node",
    "MakeArray": "add_make_array_node",
    "MakeMap": "add_make_map_node",
    "MakeSet": "add_make_set_node",
    "Comment": "add_comment_node",
    "Reroute": "add_reroute_node",
    "SpawnActor": "add_spawn_actor_node",
    "ConstructObject": "add_construct_object_node",
    "FormatText": "add_format_text_node",
    "Select": "add_select_node",
    "Timeline": "add_timeline_node",
}


# ============================================================================
# DATA CLASSES
# ============================================================================

class BPNode:
    """Internal representation of a Blueprint graph node."""

    __slots__ = (
        "name", "node_type", "props", "column", "row", "x", "y",
        "is_exec", "op_index",
    )

    def __init__(self, name: str, node_type: str, props: Optional[Dict[str, Any]] = None):
        self.name = name
        self.node_type = node_type
        self.props: Dict[str, Any] = props or {}
        self.column: int = -1
        self.row: int = -1
        self.x: int = 0
        self.y: int = 0
        self.is_exec: bool = False   # True if node has exec pins
        self.op_index: int = -1      # Index in the generated op list


class BPConnection:
    """A directed edge between two Blueprint nodes."""

    __slots__ = ("from_node", "from_pin", "to_node", "to_pin")

    def __init__(self, from_node: str, from_pin: str, to_node: str, to_pin: str):
        self.from_node = from_node
        self.from_pin = from_pin
        self.to_node = to_node
        self.to_pin = to_pin


# ============================================================================
# BUILDER
# ============================================================================

class BlueprintGraphBuilder:
    """
    Builds Blueprint graphs from declarative specs.

    Generates an execute_blueprint_batch operation list with $N.node_id
    references, automatic layout, and action resolution.

    Usage::

        builder = BlueprintGraphBuilder()
        builder.load_spec(nodes, connections, pin_defaults, blueprint_name, graph_name)
        ops = builder.generate_ops()
        # Pass ops to execute_blueprint_batch
    """

    def __init__(self):
        self.nodes: Dict[str, BPNode] = {}
        self.connections: List[BPConnection] = []
        self.pin_defaults: Dict[str, Dict[str, str]] = {}  # node_name -> {pin: value}
        self.blueprint_name: str = ""
        self.graph_name: str = "EventGraph"

        # Search cache: keyword -> {action_name, owner_path, spawner_index}
        self._search_cache: Dict[str, Dict[str, Any]] = {}

        # Adjacency for layout (exec-flow)
        self._exec_adj: Dict[str, List[str]] = defaultdict(list)
        self._exec_reverse: Dict[str, Set[str]] = defaultdict(set)

        # Data-flow adjacency
        self._data_adj: Dict[str, List[str]] = defaultdict(list)

    def reset(self) -> None:
        self.nodes.clear()
        self.connections.clear()
        self.pin_defaults.clear()
        self._exec_adj.clear()
        self._exec_reverse.clear()
        self._data_adj.clear()

    # ------------------------------------------------------------------
    # SPEC LOADING
    # ------------------------------------------------------------------

    def load_spec(
        self,
        nodes: Dict[str, Dict[str, Any]],
        connections: List[List[str]],
        pin_defaults: Optional[Dict[str, Dict[str, str]]] = None,
        blueprint_name: str = "",
        graph_name: str = "EventGraph",
    ) -> None:
        """
        Load a Blueprint graph specification.

        Args:
            nodes: Dict mapping symbolic names to node specs.
                   Each spec has:
                   - "type": Node type. One of:
                     * A direct command type (e.g. "Branch", "ForEachLoop")
                     * "Function" — calls a function (needs "target" and "function_name")
                     * "VariableGet" — gets a variable (needs "variable_name")
                     * "VariableSet" — sets a variable (needs "variable_name")
                     * "Event" — event node (needs "event_name")
                     * "CustomEvent" — custom event (needs "event_name")
                     * "ComponentRef" — self component reference (needs "component_name")
                     * "SelfRef" — self reference
                     * "CastTo" — cast node (needs "class_name")
                     * "MacroInstance" — places a macro by search (needs "search" keyword)
                     * "SearchAction" — searches and places any action (needs "search", optional "class_filter")
                   - Other fields depend on type (see above)
                   - "position": Optional [x, y] override

            connections: List of connection specs. Each is a list of 4 strings:
                        [from_node.pin, to_node.pin] in "NodeName.PinName" format,
                        OR [from_node, from_pin, to_node, to_pin] as 4 separate strings.

            pin_defaults: Optional dict of {node_name: {pin_name: default_value}}.

            blueprint_name: Target Blueprint name.
            graph_name: Target graph (default "EventGraph").
        """
        self.reset()
        self.blueprint_name = blueprint_name
        self.graph_name = graph_name
        self.pin_defaults = pin_defaults or {}

        # Register nodes
        for name, spec in nodes.items():
            node_type = spec.get("type", "SearchAction")
            node = BPNode(name, node_type, dict(spec))
            self.nodes[name] = node

        # Parse connections
        for conn_spec in connections:
            from_node, from_pin, to_node, to_pin = self._parse_connection(conn_spec)
            conn = BPConnection(from_node, from_pin, to_node, to_pin)
            self.connections.append(conn)

            # Classify as exec or data flow
            pin_lower = from_pin.lower()
            if pin_lower in ("then", "execute", "exec", "completed",
                             "loopbody", "loop body"):
                self._exec_adj[from_node].append(to_node)
                self._exec_reverse[to_node].add(from_node)
                self.nodes[from_node].is_exec = True
                if to_node in self.nodes:
                    self.nodes[to_node].is_exec = True
            else:
                self._data_adj[from_node].append(to_node)

    def _parse_connection(self, spec: list) -> Tuple[str, str, str, str]:
        """Parse connection spec into (from_node, from_pin, to_node, to_pin)."""
        if len(spec) == 4:
            return spec[0], spec[1], spec[2], spec[3]
        elif len(spec) == 2:
            # "NodeName.PinName" format
            from_parts = spec[0].rsplit(".", 1)
            to_parts = spec[1].rsplit(".", 1)
            if len(from_parts) != 2 or len(to_parts) != 2:
                raise ValueError(f"Connection spec must be 'Node.Pin' format: {spec}")
            return from_parts[0], from_parts[1], to_parts[0], to_parts[1]
        else:
            raise ValueError(f"Connection spec must have 2 or 4 elements: {spec}")

    # ------------------------------------------------------------------
    # LAYOUT
    # ------------------------------------------------------------------

    def compute_layout(self) -> None:
        """
        Assign positions to all nodes using exec-flow BFS layout.

        Exec-connected nodes are laid out left-to-right by BFS layer.
        Data-only nodes are positioned near their first consumer.
        """
        # Find exec roots (exec nodes with no exec predecessors)
        exec_roots = [
            name for name, node in self.nodes.items()
            if node.is_exec and name not in self._exec_reverse
        ]

        # BFS to assign columns for exec-flow nodes
        visited: Set[str] = set()
        columns: Dict[str, int] = {}

        queue = deque()
        for root in exec_roots:
            queue.append((root, 0))
            columns[root] = 0
            visited.add(root)

        while queue:
            name, col = queue.popleft()
            for successor in self._exec_adj.get(name, []):
                if successor not in visited and successor in self.nodes:
                    new_col = col + 1
                    columns[successor] = max(columns.get(successor, 0), new_col)
                    visited.add(successor)
                    queue.append((successor, new_col))

        # Group exec nodes by column for row assignment
        col_groups: Dict[int, List[str]] = defaultdict(list)
        for name, col in columns.items():
            self.nodes[name].column = col
            col_groups[col].append(name)

        # Assign rows within each column
        for col, names in col_groups.items():
            for row_idx, name in enumerate(names):
                self.nodes[name].row = row_idx
                self.nodes[name].x = col * EXEC_COL_SPACING
                self.nodes[name].y = row_idx * ROW_SPACING

        # Position data-only nodes (not in exec flow)
        data_nodes = [
            name for name in self.nodes
            if name not in columns
        ]

        for name in data_nodes:
            node = self.nodes[name]
            # Find which exec node consumes this data node
            consumers = self._data_adj.get(name, [])
            if consumers:
                first_consumer = consumers[0]
                if first_consumer in self.nodes:
                    consumer = self.nodes[first_consumer]
                    node.x = consumer.x + DATA_COL_OFFSET
                    node.y = consumer.y + DATA_ROW_OFFSET
                    node.column = consumer.column
                    continue
            # No consumer found — place after all exec nodes
            max_x = max((n.x for n in self.nodes.values()), default=0)
            node.x = max_x + DATA_COL_OFFSET
            node.y = len(data_nodes) * ROW_SPACING

        # Apply user-specified position overrides
        for name, node in self.nodes.items():
            pos = node.props.get("position")
            if pos and isinstance(pos, (list, tuple)) and len(pos) >= 2:
                node.x = int(pos[0])
                node.y = int(pos[1])

    # ------------------------------------------------------------------
    # OP GENERATION
    # ------------------------------------------------------------------

    def generate_ops(self) -> List[Dict[str, Any]]:
        """
        Generate execute_blueprint_batch operations.

        Returns a list of operation dicts ready for execute_blueprint_batch.
        Node references use $N.node_id syntax.
        """
        self.compute_layout()

        ops: List[Dict[str, Any]] = []
        name_to_op_index: Dict[str, int] = {}

        # Phase 1: Create all nodes
        for name, node in self.nodes.items():
            op = self._node_to_op(name, node)
            if op:
                node.op_index = len(ops)
                name_to_op_index[name] = len(ops)
                ops.append(op)

        # Phase 2: Set pin default values
        for name, defaults in self.pin_defaults.items():
            if name not in name_to_op_index:
                continue
            idx = name_to_op_index[name]
            for pin_name, value in defaults.items():
                ops.append({
                    "op": "set_pin_default_value",
                    "node_id": f"${idx}.node_id",
                    "pin_name": pin_name,
                    "default_value": str(value),
                    "graph_name": self.graph_name,
                })

        # Phase 3: Connect nodes
        for conn in self.connections:
            from_idx = name_to_op_index.get(conn.from_node)
            to_idx = name_to_op_index.get(conn.to_node)
            if from_idx is None or to_idx is None:
                logger.warning(
                    f"Skipping connection {conn.from_node}.{conn.from_pin} -> "
                    f"{conn.to_node}.{conn.to_pin}: node not found"
                )
                continue

            ops.append({
                "op": "connect_blueprint_nodes",
                "source_node_id": f"${from_idx}.node_id",
                "source_pin": conn.from_pin,
                "target_node_id": f"${to_idx}.node_id",
                "target_pin": conn.to_pin,
                "graph_name": self.graph_name,
            })

        return ops

    def _node_to_op(self, name: str, node: BPNode) -> Optional[Dict[str, Any]]:
        """Convert a BPNode to a batch operation dict."""
        node_type = node.node_type
        position = [node.x, node.y]

        # Direct command types (Branch, Sequence, etc.)
        if node_type in DIRECT_NODE_COMMANDS:
            op: Dict[str, Any] = {
                "op": DIRECT_NODE_COMMANDS[node_type],
                "position": position,
                "graph_name": self.graph_name,
            }
            # Pass through extra props
            for key in ("enum_type", "text_format", "class_name"):
                if key in node.props:
                    op[key] = node.props[key]
            return op

        # Function call
        if node_type == "Function":
            op = {
                "op": "add_blueprint_function_node",
                "target": node.props.get("target", "self"),
                "function_name": node.props["function_name"],
                "node_position": f"[{position[0]},{position[1]}]",
            }
            if "params" in node.props:
                op["params"] = node.props["params"]
            return op

        # Variable get/set
        if node_type == "VariableGet":
            return {
                "op": "add_variable_get_node",
                "variable_name": node.props["variable_name"],
                "position": position,
                "graph_name": self.graph_name,
            }
        if node_type == "VariableSet":
            return {
                "op": "add_variable_set_node",
                "variable_name": node.props["variable_name"],
                "position": position,
                "graph_name": self.graph_name,
            }

        # Event nodes
        if node_type == "Event":
            return {
                "op": "add_blueprint_event_node",
                "event_name": node.props["event_name"],
                "node_position": f"[{position[0]},{position[1]}]",
            }
        if node_type == "CustomEvent":
            return {
                "op": "add_custom_event_node",
                "event_name": node.props["event_name"],
                "position": position,
                "graph_name": self.graph_name,
            }

        # Component reference
        if node_type == "ComponentRef":
            return {
                "op": "add_blueprint_get_self_component_reference",
                "component_name": node.props["component_name"],
                "node_position": f"[{position[0]},{position[1]}]",
            }

        # Self reference
        if node_type == "SelfRef":
            return {
                "op": "add_blueprint_self_reference",
                "node_position": f"[{position[0]},{position[1]}]",
            }

        # Cast node
        if node_type == "CastTo":
            return {
                "op": "add_cast_node",
                "class_name": node.props["class_name"],
                "position": position,
                "graph_name": self.graph_name,
            }

        # Search-based placement (MacroInstance, SearchAction)
        if node_type in ("MacroInstance", "SearchAction"):
            search_kw = node.props.get("search", "")
            if not search_kw:
                logger.error(f"Node '{name}' type '{node_type}' requires 'search' keyword")
                return None
            return {
                "op": "search_and_place_action",
                "search_keyword": search_kw,
                "class_filter": node.props.get("class_filter", ""),
                "search_index": node.props.get("search_index", 0),
                "position": position,
                "graph_name": self.graph_name,
            }

        # Fallback: treat as SearchAction with the type as keyword
        logger.info(f"Node '{name}': treating type '{node_type}' as search keyword")
        return {
            "op": "search_and_place_action",
            "search_keyword": node_type,
            "class_filter": node.props.get("class_filter", ""),
            "search_index": node.props.get("search_index", 0),
            "position": position,
            "graph_name": self.graph_name,
        }
