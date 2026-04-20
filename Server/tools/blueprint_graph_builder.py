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
    # Aliases so builders can use the shorter name matching the C++ short-name table
    "SwitchInt": "add_switch_on_int_node",
    "SwitchString": "add_switch_on_string_node",
    "SwitchEnum": "add_switch_on_enum_node",
    "MakeStruct": "add_make_struct_node",
    "BreakStruct": "add_break_struct_node",
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
    # Phases 5-7 additions
    "DynamicCast": "add_dynamic_cast_node",
    "CastTo": "add_dynamic_cast_node",
    "SelfRef": "add_blueprint_self_reference",
    "Self": "add_blueprint_self_reference",
    "ComponentBoundEvent": "add_component_bound_event_node",
}


# ============================================================================
# DATA CLASSES
# ============================================================================

class BPNode:
    """Internal representation of a Blueprint graph node."""

    __slots__ = (
        "name", "node_type", "props", "column", "row", "x", "y",
        "is_exec", "op_index", "existing_guid",
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
        # Phase 10 idempotency: if set, this node already exists in the graph
        # and we reuse its guid instead of emitting an add_* op.
        self.existing_guid: Optional[str] = None


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

        # Phase 10 idempotency: count of nodes reused from existing graph
        self.reused_count: int = 0

    def reset(self) -> None:
        self.nodes.clear()
        self.connections.clear()
        self.pin_defaults.clear()
        self._exec_adj.clear()
        self._exec_reverse.clear()
        self._data_adj.clear()
        self.reused_count = 0

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
    # IDEMPOTENCY (Phase 10)
    # ------------------------------------------------------------------

    def _node_signature(self, node: BPNode) -> Optional[Tuple[str, ...]]:
        """
        Compute a dedup signature for a spec node, or None to skip dedup.

        Only certain node types have well-defined identity:
          - Event nodes: event entry is a singleton per event name per graph
          - VariableGet / VariableSet: dedupe on variable name
          - Function: dedupe on (target, function_name)
          - SearchAction / MacroInstance: dedupe on resolved search keyword

        Nodes without a signature (Branch, Sequence, etc.) are never deduped —
        users legitimately spawn multiples.
        """
        t = node.node_type
        if t == "Event":
            return ("Event", str(node.props.get("event_name", "")).lower())
        if t == "CustomEvent":
            return ("CustomEvent", str(node.props.get("event_name", "")).lower())
        if t == "VariableGet":
            return ("VariableGet", str(node.props.get("variable_name", "")))
        if t == "VariableSet":
            return ("VariableSet", str(node.props.get("variable_name", "")))
        if t == "Function":
            return (
                "Function",
                str(node.props.get("target", "self")),
                str(node.props.get("function_name", "")),
            )
        if t in ("SearchAction", "MacroInstance"):
            kw = str(node.props.get("search", "")).lower()
            if not kw:
                return None
            return ("Search", kw, str(node.props.get("class_filter", "")))
        if t == "ComponentRef":
            return ("ComponentRef", str(node.props.get("component_name", "")))
        return None

    @staticmethod
    def _snapshot_node_signature(
        snap_node: Dict[str, Any]
    ) -> List[Tuple[str, ...]]:
        """
        Derive candidate signatures from a snapshot node so we can match against
        spec signatures.

        Snapshot shape (from C++ HandleGetGraphSnapshot): each node has
            { guid, title, class, pos_x, pos_y, pins: [...] }

        The class name and title carry the type identity for our purposes:
            - K2Node_Event: title like "Event BeginPlay" → event name = last word
            - K2Node_CustomEvent: title is the event name
            - K2Node_VariableGet: title is the variable name
            - K2Node_VariableSet: title is "SET" or "Set <Var>" — use pin hint
            - K2Node_CallFunction: title is the function display name

        We return a LIST of signatures because a single snapshot node can match
        multiple spec flavors (e.g. a function call may match "Function" or
        "SearchAction" with the same keyword).
        """
        node_class = str(snap_node.get("class", ""))
        title = str(snap_node.get("title", ""))
        title_lower = title.lower().strip()
        # Phase 10b: authoritative member reference added by the C++ snapshot
        # handler. When present it's reliable; fall back to title-strip for
        # older snapshots that don't have these fields.
        member_name = str(snap_node.get("member_name", "")).strip()
        member_parent = str(snap_node.get("member_parent", "")).strip()
        custom_fn_name = str(snap_node.get("custom_function_name", "")).strip()
        sigs: List[Tuple[str, ...]] = []

        if node_class == "K2Node_Event":
            # Prefer the authoritative event reference member name (e.g.
            # "ReceiveTick"); fall back to stripping the title.
            event_name = member_name or title
            if not member_name and title_lower.startswith("event "):
                event_name = title[len("event "):].strip()
            event_lower = event_name.lower()
            sigs.append(("Event", event_lower))
            # Some events carry the "Receive" prefix in the API but drop it in
            # UI — emit both to be robust.
            if event_lower.startswith("receive"):
                sigs.append(("Event", event_lower[len("receive"):]))
            else:
                sigs.append(("Event", "receive" + event_lower))
        elif node_class == "K2Node_CustomEvent":
            # Custom events carry their name in CustomFunctionName.
            name = custom_fn_name or member_name or title
            sigs.append(("CustomEvent", name.lower()))
        elif node_class == "K2Node_VariableGet":
            var_name = member_name if member_name else title
            if not member_name and title_lower.startswith("get "):
                var_name = title[len("get "):].strip()
            sigs.append(("VariableGet", var_name))
        elif node_class == "K2Node_VariableSet":
            var_name = member_name if member_name else title
            if not member_name and title_lower.startswith("set "):
                var_name = title[len("set "):].strip()
            sigs.append(("VariableSet", var_name))
        elif node_class == "K2Node_CallFunction":
            # When the C++ snapshot provides member_name + member_parent, the
            # spec-side ("Function", target, function_name) signature can match
            # exactly. Emit every plausible form the target could take.
            if member_name:
                if member_parent:
                    sigs.append(("Function", member_parent, member_name))
                    # Also try common short-name / U-prefix variants to match
                    # how specs often write the target (e.g. "KismetMathLibrary"
                    # instead of "UKismetMathLibrary"). FindCallableFunction on
                    # the C++ side strips "U"; we replicate here.
                    if member_parent.startswith("U"):
                        sigs.append(("Function", member_parent[1:], member_name))
                # Empty/self target fallback
                sigs.append(("Function", "self", member_name))
                sigs.append(("Function", "", member_name))
                # Search fallback (title-keyed)
                sigs.append(("Search", member_name.lower(), ""))
            # Legacy fallback when member fields absent — use title.
            else:
                sigs.append(("Search", title_lower, ""))
                sigs.append(("Function", "self", title))
        elif node_class == "K2Node_SpawnActorFromClass":
            sigs.append(("Search", "spawn actor", ""))
        return sigs

    def apply_graph_snapshot(self, snapshot: Optional[Dict[str, Any]]) -> None:
        """
        Cross-reference the spec against an existing graph snapshot to set
        `existing_guid` on nodes we want to reuse.

        `snapshot` is the dict returned by `get_graph_snapshot` — either the
        raw response, or the already-unwrapped `result` field. We handle both.
        Pass None to skip dedup.
        """
        if not snapshot:
            return

        # Unwrap nested layers. The raw get_graph_snapshot response shape is
        #   {status, result: {success, data: {graph_name, node_count, nodes: [...]}}}
        # Peel both "result" and "data" so callers can hand us the raw response
        # or any already-partially-unwrapped variant.
        for key in ("result", "data"):
            if (
                isinstance(snapshot, dict)
                and key in snapshot
                and isinstance(snapshot[key], dict)
                and "nodes" not in snapshot
            ):
                snapshot = snapshot[key]

        snap_nodes = snapshot.get("nodes") if isinstance(snapshot, dict) else None
        if not isinstance(snap_nodes, list):
            return

        # Build signature → guid map from snapshot (first-match wins)
        sig_to_guid: Dict[Tuple[str, ...], str] = {}
        for snap in snap_nodes:
            if not isinstance(snap, dict):
                continue
            guid = snap.get("guid")
            if not guid:
                continue
            for sig in self._snapshot_node_signature(snap):
                sig_to_guid.setdefault(sig, str(guid))

        # Match spec nodes against the signature map
        for name, node in self.nodes.items():
            sig = self._node_signature(node)
            if sig is None:
                continue
            existing = sig_to_guid.get(sig)
            if existing:
                node.existing_guid = existing
                self.reused_count += 1
                logger.info(
                    f"Reusing existing node for '{name}' ({node.node_type}) → {existing}"
                )

    # ------------------------------------------------------------------
    # OP GENERATION
    # ------------------------------------------------------------------

    def generate_ops(self) -> List[Dict[str, Any]]:
        """
        Generate execute_blueprint_batch operations.

        Returns a list of operation dicts ready for execute_blueprint_batch.
        Node references use $N.node_id syntax for newly-spawned nodes, or the
        literal guid string for reused nodes (Phase 10 idempotency).
        """
        self.compute_layout()

        ops: List[Dict[str, Any]] = []
        # name -> reference string used for op payloads.
        # For spawned nodes: "$<op_index>.node_id"
        # For reused nodes: the literal guid (no $ prefix; resolver passes it through)
        name_to_ref: Dict[str, str] = {}

        # Phase 1: Create all nodes (skip spawn for reused)
        for name, node in self.nodes.items():
            if node.existing_guid:
                # Already in the graph — no add_* op, just record the guid
                name_to_ref[name] = node.existing_guid
                continue

            op = self._node_to_op(name, node)
            if op:
                node.op_index = len(ops)
                name_to_ref[name] = f"${len(ops)}.node_id"
                ops.append(op)

        # Phase 2: Set pin default values.
        # BUGFIX (Phase 10): The C++ handler expects "node_guid" and "value".
        # Previous code sent "node_id" and "default_value" — both wrong, hence
        # the "Missing 'node_guid' parameter" error.
        for name, defaults in self.pin_defaults.items():
            ref = name_to_ref.get(name)
            if not ref:
                logger.warning(
                    f"Skipping pin_defaults for '{name}': node has no spawned/"
                    f"reused ref (was it filtered out of the spec?)"
                )
                continue
            for pin_name, value in defaults.items():
                ops.append({
                    "op": "set_pin_default_value",
                    "node_guid": ref,            # <-- was "node_id" (wrong key)
                    "pin_name": pin_name,
                    "value": str(value),         # <-- was "default_value"
                    "graph_name": self.graph_name,
                })

        # Phase 3: Connect nodes
        for conn in self.connections:
            from_ref = name_to_ref.get(conn.from_node)
            to_ref = name_to_ref.get(conn.to_node)
            if from_ref is None or to_ref is None:
                logger.warning(
                    f"Skipping connection {conn.from_node}.{conn.from_pin} -> "
                    f"{conn.to_node}.{conn.to_pin}: node not found"
                )
                continue

            ops.append({
                "op": "connect_blueprint_nodes",
                "source_node_id": from_ref,
                "source_pin": conn.from_pin,
                "target_node_id": to_ref,
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
                "node_position": position,
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
                "node_position": position,
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
                "node_position": position,
            }

        # Self reference
        if node_type == "SelfRef":
            return {
                "op": "add_blueprint_self_reference",
                "node_position": position,
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
