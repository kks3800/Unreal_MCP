"""
Material Graph Intelligence Engine.

Transforms declarative graph specifications into optimized batch operations
with automatic layout, grouping, named reroutes, and anti-spaghetti placement.

This module has NO dependencies on MCP or the Unreal connection — it is pure
graph logic that can be tested and used independently.
"""

import logging
import math
from typing import List, Dict, Any, Optional, Tuple, Set
from collections import defaultdict, deque

logger = logging.getLogger("UnrealMCP")


# ============================================================================
# CONSTANTS
# ============================================================================

NODE_WIDTH = 300      # Approximate node width in UE material editor
NODE_HEIGHT = 200     # Approximate node height
COL_SPACING = 350     # Horizontal spacing between columns
ROW_SPACING = 220     # Vertical spacing between rows in the same column
GROUP_PADDING = 80    # Padding around comment boxes
COMMENT_HEADER = 40   # Extra height for comment box title

# Insert a named reroute when a single output fans out to more than this many targets.
REROUTE_FANOUT_THRESHOLD = 2

# Named reroute colors by semantic type (RGBA, 0-1 range)
REROUTE_COLORS: Dict[str, List[float]] = {
    "texture":  [0.2, 0.8, 0.2, 1.0],   # Green for textures
    "scalar":   [0.3, 0.5, 1.0, 1.0],   # Blue for scalars
    "vector":   [1.0, 0.6, 0.1, 1.0],   # Orange for vectors
    "default":  [0.7, 0.7, 0.7, 1.0],   # Gray for unknown
}

# Node types that are classified as texture-related
TEXTURE_NODE_TYPES: Set[str] = {
    "TextureSample",
    "TextureSampleParameter2D",
    "TextureObject",
    "TextureObjectParameter",
}

SCALAR_NODE_TYPES: Set[str] = {"Constant", "ScalarParameter"}
VECTOR_NODE_TYPES: Set[str] = {"Constant3Vector", "Constant4Vector", "VectorParameter"}

# Prefix used to distinguish function-output pseudo-nodes from regular nodes
OUTPUT_PREFIX = "output:"


# ============================================================================
# DATA CLASSES
# ============================================================================

class GraphNode:
    """Internal representation of a single material graph node."""

    def __init__(self, name: str, node_type: str, props: Optional[Dict[str, Any]] = None):
        self.name = name
        self.node_type = node_type
        self.props: Dict[str, Any] = props or {}
        self.column: int = -1   # Assigned during layout (Sugiyama column)
        self.row: float = -1    # Assigned during layout (barycenter value before rounding)
        self.x: int = 0         # Final pixel X position
        self.y: int = 0         # Final pixel Y position
        self.group: Optional[str] = None  # Comment-box group this node belongs to
        self.is_input: bool = False        # True for function input nodes
        self.is_output: bool = False       # True for function output pseudo-nodes

    def semantic_type(self) -> str:
        """Return a semantic category used to colour named reroutes."""
        if self.node_type in TEXTURE_NODE_TYPES:
            return "texture"
        if self.is_input:
            input_type = self.props.get("input_type", "") or self.props.get("type", "")
            if "Texture" in input_type:
                return "texture"
            if input_type == "Scalar":
                return "scalar"
        if self.node_type in SCALAR_NODE_TYPES:
            return "scalar"
        if self.node_type in VECTOR_NODE_TYPES:
            return "vector"
        return "default"


class GraphConnection:
    """Internal representation of a directed edge in the material graph."""

    def __init__(self, from_node: str, to_node: str, to_input: str,
                 from_output: str = ""):
        self.from_node = from_node
        self.to_node = to_node
        self.to_input = to_input
        self.from_output = from_output


# ============================================================================
# BUILDER
# ============================================================================

class MaterialGraphBuilder:
    """
    Builds material / material-function graphs from declarative specs.

    Provides automatic layered layout (Sugiyama-style), comment-box grouping,
    and named-reroute insertion for fan-out connections.

    Usage::

        builder = MaterialGraphBuilder()
        builder.load_function_spec(inputs, outputs, nodes, connections)
        builder.compute_layout()
        ops = builder.generate_function_ops(function_name, inputs, outputs)
        # Pass `ops` to execute_function_batch via the MCP connection.
    """

    def __init__(self):
        self.nodes: Dict[str, GraphNode] = {}
        self.connections: List[GraphConnection] = []
        self.groups: Dict[str, List[str]] = {}       # group_name -> [node_names]
        self.output_connections: List[Dict[str, str]] = []  # material output pins

        # Directed-graph adjacency (forward and reverse)
        self._adjacency: Dict[str, List[str]] = defaultdict(list)
        self._reverse_adj: Dict[str, List[str]] = defaultdict(list)

        # Fan-out tracking: _fanout[from_node][from_output] = [to_node, ...]
        self._fanout: Dict[str, Dict[str, List[str]]] = defaultdict(
            lambda: defaultdict(list)
        )

    # ------------------------------------------------------------------
    # RESET
    # ------------------------------------------------------------------

    def reset(self) -> None:
        """Clear all internal state for a fresh load."""
        self.nodes.clear()
        self.connections.clear()
        self.groups.clear()
        self.output_connections.clear()
        self._adjacency.clear()
        self._reverse_adj.clear()
        self._fanout.clear()

    # ------------------------------------------------------------------
    # SPEC LOADING — FUNCTION GRAPHS
    # ------------------------------------------------------------------

    def load_function_spec(
        self,
        inputs: List[Dict[str, Any]],
        outputs: List[Dict[str, Any]],
        nodes: List[Dict[str, Any]],
        connections: List[Dict[str, Any]],
        groups: Optional[List[Dict[str, Any]]] = None,
    ) -> None:
        """
        Load a material function specification.

        Function inputs are treated as source nodes (column 0).
        Function outputs are stored with the ``output:`` prefix so they
        never collide with regular node names.
        """
        self.reset()

        # Register function inputs
        for inp in inputs:
            name = inp["name"]
            node = GraphNode(name, "FunctionInput", inp)
            node.is_input = True
            node.column = 0
            self.nodes[name] = node

        # Register function outputs (keyed with output: prefix)
        for out in outputs:
            name = out["name"]
            out_key = OUTPUT_PREFIX + name
            node = GraphNode(out_key, "FunctionOutput", out)
            node.is_output = True
            node.column = 999  # Will be recalculated; outputs must be rightmost
            self.nodes[out_key] = node

        # Register regular interior nodes
        for n in nodes:
            name = n["name"]
            node = GraphNode(name, n["type"], n)
            self.nodes[name] = node

        # Parse connections and build adjacency structures
        for c in connections:
            from_node = c["from"]
            to_raw = c["to"]
            to_input = c.get("input", "")
            from_output = c.get("output", "")

            # Normalise "output:Name" destinations — they are already prefixed
            to_node = to_raw  # kept as-is; output: prefix is canonical

            conn = GraphConnection(from_node, to_node, to_input, from_output)
            self.connections.append(conn)
            self._adjacency[from_node].append(to_node)
            self._reverse_adj[to_node].append(from_node)

            fanout_key = from_output or "__default__"
            self._fanout[from_node][fanout_key].append(to_node)

        # Load manually-specified comment groups
        if groups:
            for g in groups:
                self.groups[g["name"]] = list(g["nodes"])

    # ------------------------------------------------------------------
    # SPEC LOADING — MATERIAL GRAPHS
    # ------------------------------------------------------------------

    def load_material_spec(
        self,
        nodes: List[Dict[str, Any]],
        connections: List[Dict[str, Any]],
        material_outputs: Optional[List[Dict[str, Any]]] = None,
        groups: Optional[List[Dict[str, Any]]] = None,
    ) -> None:
        """Load a material graph specification."""
        self.reset()

        for n in nodes:
            name = n["name"]
            node = GraphNode(name, n["type"], n)
            self.nodes[name] = node

        for c in connections:
            from_node = c["from"]
            to_node = c["to"]
            to_input = c.get("input", "")
            from_output = c.get("output", "")

            conn = GraphConnection(from_node, to_node, to_input, from_output)
            self.connections.append(conn)
            self._adjacency[from_node].append(to_node)
            self._reverse_adj[to_node].append(from_node)

            fanout_key = from_output or "__default__"
            self._fanout[from_node][fanout_key].append(to_node)

        if material_outputs:
            self.output_connections = list(material_outputs)

        if groups:
            for g in groups:
                self.groups[g["name"]] = list(g["nodes"])

    # ------------------------------------------------------------------
    # LAYOUT — Sugiyama-style layered graph drawing
    # ------------------------------------------------------------------

    def compute_layout(self) -> None:
        """Compute pixel positions for all nodes using layered graph drawing."""
        self._assign_columns()
        self._assign_rows()
        self._compute_positions()

    def _assign_columns(self) -> None:
        """
        Assign each node to a column (layer) using the longest-path algorithm.

        Sources (no predecessors, or marked is_input) start at column 0.
        Each successor is placed at max(current_col, predecessor_col + 1).
        Output nodes are forced to the rightmost column + 1.
        """
        sources: List[str] = []
        for name, node in self.nodes.items():
            if node.is_input:
                node.column = 0
                sources.append(name)
            elif not self._reverse_adj.get(name):
                node.column = 0
                sources.append(name)

        visited: Set[str] = set()
        queue: deque = deque(sources)
        while queue:
            name = queue.popleft()
            if name in visited:
                continue
            visited.add(name)
            node = self.nodes.get(name)
            if node is None:
                continue

            for successor in self._adjacency.get(name, []):
                succ_node = self.nodes.get(successor)
                if succ_node is not None:
                    proposed = node.column + 1
                    if proposed > succ_node.column:
                        succ_node.column = proposed
                    queue.append(successor)

        # Force output nodes to the column after the current maximum
        max_col = max((n.column for n in self.nodes.values() if not n.is_output), default=0)
        for node in self.nodes.values():
            if node.is_output:
                node.column = max_col + 1

    def _assign_rows(self) -> None:
        """
        Assign rows within each column using the barycenter heuristic.

        Column 0 (inputs) is sorted by ``sort_priority`` from the spec.
        Each subsequent column sorts nodes by the mean row-index of their
        predecessors, which reduces edge crossings.
        """
        columns: Dict[int, List[GraphNode]] = defaultdict(list)
        for node in self.nodes.values():
            columns[node.column].append(node)

        sorted_col_indices = sorted(columns.keys())

        # Column 0: sort by explicit priority, then assign integer rows
        if sorted_col_indices:
            col0_nodes = columns[sorted_col_indices[0]]
            col0_nodes.sort(key=lambda n: n.props.get("sort_priority", 0))
            for i, node in enumerate(col0_nodes):
                node.row = float(i)

        # Remaining columns: barycenter → sort → integer rows
        for col_idx in range(1, len(sorted_col_indices)):
            col = sorted_col_indices[col_idx]
            nodes_in_col = columns[col]

            for node in nodes_in_col:
                pred_rows = [
                    self.nodes[pred].row
                    for pred in self._reverse_adj.get(node.name, [])
                    if pred in self.nodes and self.nodes[pred].row >= 0
                ]
                node.row = (sum(pred_rows) / len(pred_rows)) if pred_rows else 0.0

            nodes_in_col.sort(key=lambda n: n.row)
            for i, node in enumerate(nodes_in_col):
                node.row = float(i)

    def _compute_positions(self) -> None:
        """
        Convert column / row indices to pixel coordinates.

        The tallest column determines the total height; all columns are
        vertically centred relative to that height.
        """
        columns: Dict[int, List[GraphNode]] = defaultdict(list)
        for node in self.nodes.values():
            columns[node.column].append(node)

        if not columns:
            return

        max_rows = max(len(nodes) for nodes in columns.values())
        center_y = (max_rows * ROW_SPACING) / 2.0

        for col, nodes in columns.items():
            nodes.sort(key=lambda n: n.row)
            col_height = len(nodes) * ROW_SPACING
            start_y = center_y - col_height / 2.0

            for i, node in enumerate(nodes):
                node.x = col * COL_SPACING
                node.y = int(start_y + i * ROW_SPACING)

    # ------------------------------------------------------------------
    # AUTO-GROUPING
    # ------------------------------------------------------------------

    def auto_detect_groups(self) -> None:
        """
        Detect logical comment-box groups from node connectivity and semantics.

        If the caller already provided groups via the spec, this method does
        nothing so manual groupings are never overwritten.
        """
        if self.groups:
            return  # Respect user-provided groups

        columns: Dict[int, List[GraphNode]] = defaultdict(list)
        for node in self.nodes.values():
            if not node.is_input and not node.is_output:
                columns[node.column].append(node)

        # Always group function inputs and outputs separately
        input_nodes = [n.name for n in self.nodes.values() if n.is_input]
        if input_nodes:
            self.groups["Inputs"] = input_nodes

        output_nodes = [n.name for n in self.nodes.values() if n.is_output]
        if output_nodes:
            self.groups["Outputs"] = output_nodes

        # Merge adjacent columns when their connectivity is tight
        sorted_cols = sorted(columns.keys())
        i = 0
        while i < len(sorted_cols):
            col = sorted_cols[i]
            group_node_names = [n.name for n in columns[col]]

            # Greedily absorb the next column if all its nodes are
            # exclusively fed by nodes in the current group
            if i + 1 < len(sorted_cols):
                next_col = sorted_cols[i + 1]
                next_nodes = columns[next_col]
                all_connected = all(
                    any(n.name in self._adjacency.get(prev, [])
                        for prev in group_node_names)
                    for n in next_nodes
                )
                if all_connected and len(next_nodes) <= 4:
                    group_node_names.extend(n.name for n in next_nodes)
                    i += 1  # Skip the absorbed column

            if len(group_node_names) >= 2:
                group_name = self._semantic_group_name(group_node_names, col)
                self.groups[group_name] = group_node_names

            i += 1

    def _semantic_group_name(self, node_names: List[str], col: int) -> str:
        """Derive a human-readable group name from the node types inside it."""
        types = {self.nodes[n].node_type for n in node_names if n in self.nodes}
        if types & TEXTURE_NODE_TYPES:
            base = "Texture Sampling"
        elif "Lerp" in types:
            base = "Blending"
        elif types & {"Multiply", "Add", "Subtract", "Divide"}:
            base = "Math Operations"
        else:
            base = f"Stage {col}"

        # Deduplicate: append a counter if the name is already taken
        name = base
        counter = 2
        while name in self.groups:
            name = f"{base} {counter}"
            counter += 1
        return name

    def generate_comment_ops(self, op_type: str = "add_material_node") -> List[Dict[str, Any]]:
        """
        Generate batch operations that add Comment nodes around each group.

        Args:
            op_type: The MCP batch op name to use (``"add_material_node"`` or
                     ``"add_function_node"``).

        Returns:
            List of batch-op dicts ready to be included in the operations list.
        """
        ops: List[Dict[str, Any]] = []
        for group_name, node_names in self.groups.items():
            group_nodes = [self.nodes[n] for n in node_names if n in self.nodes]
            if not group_nodes:
                continue

            min_x = min(n.x for n in group_nodes) - GROUP_PADDING
            min_y = min(n.y for n in group_nodes) - GROUP_PADDING - COMMENT_HEADER
            max_x = max(n.x for n in group_nodes) + NODE_WIDTH + GROUP_PADDING
            max_y = max(n.y for n in group_nodes) + NODE_HEIGHT + GROUP_PADDING

            ops.append({
                "op": op_type,
                "node_type": "Comment",
                "node_name": group_name,
                "position": [min_x, min_y],
                # Width/height are informational; UE auto-sizes comment boxes
                "comment_width": max_x - min_x,
                "comment_height": max_y - min_y,
            })

        return ops

    # ------------------------------------------------------------------
    # NAMED REROUTE INSERTION
    # ------------------------------------------------------------------

    def insert_named_reroutes(self) -> Tuple[List[Dict[str, Any]], List[GraphConnection]]:
        """
        Detect fan-out patterns and insert named reroute pairs.

        For each output that fans out to more than ``REROUTE_FANOUT_THRESHOLD``
        targets, a ``NamedRerouteDeclaration`` node is inserted (connected to
        the source) and one ``NamedRerouteUsage`` node is added per target.
        The original direct connections are replaced with
        declaration → usage → target edges.

        Returns:
            A tuple of (new_node_ops, new_connections_added).
            The builder's ``self.connections`` list is mutated in place.
        """
        reroute_ops: List[Dict[str, Any]] = []
        new_connections: List[GraphConnection] = []
        connections_to_remove: Set[int] = set()

        for from_node, outputs in list(self._fanout.items()):
            for from_output, targets in outputs.items():
                if len(targets) <= REROUTE_FANOUT_THRESHOLD:
                    continue

                source = self.nodes.get(from_node)
                if source is None:
                    continue

                actual_output = from_output if from_output != "__default__" else ""
                output_suffix = f"_{from_output}" if actual_output else ""
                reroute_name = f"Reroute_{from_node}{output_suffix}"

                sem_type = source.semantic_type()
                color = REROUTE_COLORS.get(sem_type, REROUTE_COLORS["default"])

                # Declaration sits slightly to the right of its source
                reroute_x = source.x + int(COL_SPACING * 0.6)
                reroute_y = source.y

                reroute_ops.append({
                    "op": "add_function_node",   # Caller overwrites "op" as needed
                    "node_type": "NamedRerouteDeclaration",
                    "node_name": reroute_name,
                    "position": [reroute_x, reroute_y],
                    "node_color": color,
                })

                # Source → declaration
                new_connections.append(GraphConnection(
                    from_node, reroute_name, "", actual_output
                ))

                # One usage node per original target
                for i, target in enumerate(targets):
                    usage_name = f"{reroute_name}_use{i}"
                    target_node = self.nodes.get(target)
                    usage_y = target_node.y if target_node is not None else reroute_y + i * 50
                    usage_x = reroute_x + 100

                    reroute_ops.append({
                        "op": "add_function_node",
                        "node_type": "NamedRerouteUsage",
                        "node_name": usage_name,
                        "position": [usage_x, usage_y],
                    })

                    # Link the usage node to its declaration
                    reroute_ops.append({
                        "op": "link_named_reroute_usage",
                        "usage_node": usage_name,
                        "declaration_node": reroute_name,
                    })

                    # Find and retire the original connection to this target
                    for ci, conn in enumerate(self.connections):
                        actual_from_out = conn.from_output or "__default__"
                        if (conn.from_node == from_node
                                and actual_from_out == from_output
                                and conn.to_node == target):
                            connections_to_remove.add(ci)
                            new_connections.append(GraphConnection(
                                usage_name, target, conn.to_input, ""
                            ))
                            break

        # Apply removals and append new connections
        self.connections = [
            c for i, c in enumerate(self.connections)
            if i not in connections_to_remove
        ]
        self.connections.extend(new_connections)

        return reroute_ops, new_connections

    # ------------------------------------------------------------------
    # VALIDATION
    # ------------------------------------------------------------------

    def validate(self) -> List[Dict[str, str]]:
        """
        Check the graph specification for common authoring mistakes.

        Returns:
            A list of issue dicts, each with ``type``, ``message``, and
            optional ``node`` / ``nodes`` keys.  An empty list means the
            graph is valid.
        """
        issues: List[Dict[str, str]] = []

        connected_nodes: Set[str] = set()
        for conn in self.connections:
            connected_nodes.add(conn.from_node)
            connected_nodes.add(conn.to_node)

        for name, node in self.nodes.items():
            if name not in connected_nodes and not node.is_input and not node.is_output:
                issues.append({
                    "type": "orphaned_node",
                    "node": name,
                    "message": f"Node '{name}' has no connections",
                })

        # Function outputs must have at least one incoming connection
        output_targets = {c.to_node for c in self.connections}
        for name, node in self.nodes.items():
            if node.is_output and name not in output_targets:
                display_name = name[len(OUTPUT_PREFIX):] if name.startswith(OUTPUT_PREFIX) else name
                issues.append({
                    "type": "disconnected_output",
                    "node": name,
                    "message": f"Output '{display_name}' has no input connection",
                })

        return issues

    # ------------------------------------------------------------------
    # BATCH OP GENERATION — FUNCTION GRAPHS
    # ------------------------------------------------------------------

    def generate_function_ops(
        self,
        function_name: str,
        inputs: List[Dict[str, Any]],
        outputs: List[Dict[str, Any]],
        auto_layout: bool = True,
        auto_group: bool = True,
        auto_reroute: bool = True,
    ) -> List[Dict[str, Any]]:
        """
        Generate the full ordered list of batch operations for a function graph.

        Order: inputs → outputs → interior nodes → reroutes → connections →
               comment boxes.

        Args:
            function_name: Full content path of the function asset.
            inputs: Original inputs spec (used to preserve order / metadata).
            outputs: Original outputs spec.
            auto_layout: Include computed node positions in the ops.
            auto_group: Detect and emit comment-box groups.
            auto_reroute: Emit named reroute pairs for high-fan-out outputs.

        Returns:
            List of batch-op dicts consumable by ``execute_function_batch``.
        """
        ops: List[Dict[str, Any]] = []

        # 1. Function inputs (with position baked in when auto_layout is on)
        for i, inp in enumerate(inputs):
            name = inp["name"]
            op: Dict[str, Any] = {
                "op": "add_function_input",
                "input_name": name,
                "input_type": inp.get("type", "Scalar"),
                "sort_priority": inp.get("priority", i),
            }
            if "description" in inp:
                op["description"] = inp["description"]
            if "default" in inp:
                op["use_preview_value_as_default"] = True
                op["preview_value"] = inp["default"]
            # Bake layout position directly into the input op
            if auto_layout and name in self.nodes:
                node = self.nodes[name]
                op["position"] = [node.x, node.y]
            ops.append(op)

        # 2. Function outputs (with position baked in when auto_layout is on)
        for i, out in enumerate(outputs):
            name = out["name"]
            out_key = OUTPUT_PREFIX + name
            op = {
                "op": "add_function_output",
                "output_name": name,
                "sort_priority": out.get("priority", i),
            }
            if "description" in out:
                op["description"] = out["description"]
            # Bake layout position directly into the output op
            if auto_layout and out_key in self.nodes:
                node = self.nodes[out_key]
                op["position"] = [node.x, node.y]
            ops.append(op)

        # 3. Interior nodes (exclude input/output pseudo-nodes)
        for name, node in self.nodes.items():
            if node.is_input or node.is_output:
                continue
            op = {
                "op": "add_function_node",
                "node_type": node.node_type,
                "node_name": name,
            }
            for key in ("value", "color", "texture_path", "group",
                         "node_color", "code", "output_type"):
                if key in node.props:
                    op[key] = node.props[key]

            if auto_layout:
                op["position"] = [node.x, node.y]
            elif "position" in node.props:
                op["position"] = node.props["position"]

            ops.append(op)

        # 4. Named reroutes (inserts new nodes + mutates self.connections)
        if auto_reroute:
            reroute_ops, _ = self.insert_named_reroutes()
            for rop in reroute_ops:
                if rop["op"] != "link_named_reroute_usage":
                    rop["op"] = "add_function_node"
            ops.extend(reroute_ops)

        # 5. Connections
        for conn in self.connections:
            to_node = conn.to_node
            to_input = conn.to_input

            # Strip the output: prefix → connect to the actual function output node
            if to_node.startswith(OUTPUT_PREFIX):
                to_node = to_node[len(OUTPUT_PREFIX):]
                to_input = ""  # Function outputs use their default (single) input pin

            op = {
                "op": "connect_function_nodes",
                "from_node": conn.from_node,
                "to_node": to_node,
                "to_input": to_input,
            }
            if conn.from_output:
                op["from_output"] = conn.from_output
            ops.append(op)

        # 6. Comment boxes
        if auto_group:
            self.auto_detect_groups()
        if self.groups:
            comment_ops = self.generate_comment_ops(op_type="add_function_node")
            ops.extend(comment_ops)

        return ops

    # ------------------------------------------------------------------
    # BATCH OP GENERATION — MATERIAL GRAPHS
    # ------------------------------------------------------------------

    def generate_material_ops(
        self,
        material_name: str,
        nodes: List[Dict[str, Any]],
        auto_layout: bool = True,
        auto_group: bool = True,
        auto_reroute: bool = True,
    ) -> List[Dict[str, Any]]:
        """
        Generate the full ordered list of batch operations for a material graph.

        Order: nodes → reroutes → interior connections → material output
        connections → comment boxes → batch position update.

        Args:
            material_name: Full content path of the material asset.
            nodes: Original nodes spec (used for metadata).
            auto_layout: Include computed node positions.
            auto_group: Detect and emit comment-box groups.
            auto_reroute: Emit named reroute pairs for high-fan-out outputs.

        Returns:
            List of batch-op dicts consumable by ``execute_material_batch``.
        """
        ops: List[Dict[str, Any]] = []

        # 1. Material nodes
        for name, node in self.nodes.items():
            op: Dict[str, Any] = {
                "op": "add_material_node",
                "node_type": node.node_type,
                "node_name": name,
            }
            for key in ("value", "color", "texture_path", "code", "output_type",
                         "inputs", "function_name", "node_color", "group"):
                if key in node.props:
                    op[key] = node.props[key]

            if auto_layout:
                op["position"] = [node.x, node.y]
            elif "position" in node.props:
                op["position"] = node.props["position"]

            ops.append(op)

        # 2. Named reroutes (mutates self.connections)
        if auto_reroute:
            reroute_ops, _ = self.insert_named_reroutes()
            for rop in reroute_ops:
                if rop["op"] != "link_named_reroute_usage":
                    rop["op"] = "add_material_node"
            ops.extend(reroute_ops)

        # 3. Interior connections
        for conn in self.connections:
            op = {
                "op": "connect_material_nodes",
                "from_node": conn.from_node,
                "to_node": conn.to_node,
                "to_input": conn.to_input,
            }
            if conn.from_output:
                op["from_output"] = conn.from_output
            ops.append(op)

        # 4. Material output pin connections
        for mc in self.output_connections:
            ops.append({
                "op": "connect_to_material_output",
                "from_node": mc["from"],
                "material_property": mc["property"],
                "from_output": mc.get("output", ""),
            })

        # 5. Comment boxes
        if auto_group:
            self.auto_detect_groups()
        if self.groups:
            comment_ops = self.generate_comment_ops(op_type="add_material_node")
            ops.extend(comment_ops)

        # 6. Batch position update (single op for all nodes)
        if auto_layout and self.nodes:
            position_nodes = [
                {"name": name, "x": node.x, "y": node.y}
                for name, node in self.nodes.items()
            ]
            ops.append({
                "op": "set_material_node_position",
                "nodes": position_nodes,
            })

        return ops

    # ------------------------------------------------------------------
    # ANALYSIS — existing graphs
    # ------------------------------------------------------------------

    def load_from_existing(self, graph_data: Dict[str, Any]) -> None:
        """
        Populate the builder from ``get_material_nodes`` response data.

        Handles both ``position: [x, y]`` list format and flat ``x``/``y``
        fields so it is compatible with different MCP command versions.
        """
        self.reset()

        nodes_data = graph_data.get("nodes", [])
        for n in nodes_data:
            name = n.get("name") or n.get("node_name", "")
            node_type = n.get("type") or n.get("node_type", "")
            node = GraphNode(name, node_type, n)

            pos = n.get("position")
            if isinstance(pos, list) and len(pos) >= 2:
                node.x = int(pos[0])
                node.y = int(pos[1])
            else:
                node.x = int(n.get("x", 0))
                node.y = int(n.get("y", 0))

            self.nodes[name] = node

        connections_data = graph_data.get("connections", [])
        for c in connections_data:
            conn = GraphConnection(
                c.get("from_node", ""),
                c.get("to_node", ""),
                c.get("to_input", ""),
                c.get("from_output", ""),
            )
            self.connections.append(conn)
            self._adjacency[conn.from_node].append(conn.to_node)
            self._reverse_adj[conn.to_node].append(conn.from_node)

            fanout_key = conn.from_output or "__default__"
            self._fanout[conn.from_node][fanout_key].append(conn.to_node)

    def analyze(self) -> Dict[str, Any]:
        """
        Inspect an existing graph and return a structured report.

        Checks for: orphaned nodes, disconnected outputs, overlapping nodes,
        missing comment boxes, and excessive fan-out.

        Returns:
            Dict with ``node_count``, ``connection_count``, ``issues`` (list),
            and ``issue_count``.
        """
        issues: List[Dict[str, Any]] = []
        issues.extend(self.validate())

        # Detect overlapping nodes
        node_list = list(self.nodes.values())
        for idx_a in range(len(node_list)):
            for idx_b in range(idx_a + 1, len(node_list)):
                n_a = node_list[idx_a]
                n_b = node_list[idx_b]
                if (abs(n_a.x - n_b.x) < NODE_WIDTH * 0.8
                        and abs(n_a.y - n_b.y) < NODE_HEIGHT * 0.8):
                    issues.append({
                        "type": "overlapping",
                        "nodes": [n_a.name, n_b.name],
                        "message": (
                            f"Nodes '{n_a.name}' and '{n_b.name}' overlap"
                        ),
                    })

        # Detect excessive fan-out
        for from_node, outputs in self._fanout.items():
            for from_output, targets in outputs.items():
                if len(targets) > REROUTE_FANOUT_THRESHOLD:
                    issues.append({
                        "type": "needs_reroute",
                        "node": from_node,
                        "output": from_output,
                        "fanout": len(targets),
                        "message": (
                            f"'{from_node}' fans out to {len(targets)} targets"
                            " — consider a named reroute"
                        ),
                    })

        # Detect absence of comment boxes on large graphs
        has_comments = any(n.node_type == "Comment" for n in self.nodes.values())
        non_comment_count = sum(
            1 for n in self.nodes.values() if n.node_type != "Comment"
        )
        if not has_comments and non_comment_count > 6:
            issues.append({
                "type": "no_groups",
                "message": (
                    f"Graph has {non_comment_count} nodes but no comment boxes"
                    " for organisation"
                ),
            })

        return {
            "node_count": len(self.nodes),
            "connection_count": len(self.connections),
            "issues": issues,
            "issue_count": len(issues),
        }

    def generate_fix_ops(
        self,
        target_key: str = "material_name",
        relayout: bool = True,
        add_reroutes: bool = True,
        add_groups: bool = True,
    ) -> List[Dict[str, Any]]:
        """
        Generate batch operations to remedy issues detected by ``analyze()``.

        Args:
            target_key: ``"material_name"`` or ``"function_name"`` — controls
                        which op type is used for node additions.
            relayout: Recompute all node positions.
            add_reroutes: Insert named reroutes where fan-out exceeds threshold.
            add_groups: Add comment boxes for detected node groups.

        Returns:
            List of batch-op dicts (may be empty if no fixes are needed).
        """
        ops: List[Dict[str, Any]] = []

        is_material = target_key == "material_name"
        add_node_op = "add_material_node" if is_material else "add_function_node"

        if relayout:
            self.compute_layout()
            position_nodes = [
                {"name": name, "x": node.x, "y": node.y}
                for name, node in self.nodes.items()
                if node.node_type != "Comment"
            ]
            if position_nodes:
                if is_material:
                    ops.append({
                        "op": "set_material_node_position",
                        "nodes": position_nodes,
                    })
                else:
                    # Function graphs: emit individual set_node_position ops
                    for entry in position_nodes:
                        ops.append({
                            "op": "set_node_position",
                            "node_name": entry["name"],
                            "x": entry["x"],
                            "y": entry["y"],
                        })

        if add_reroutes:
            reroute_ops, _ = self.insert_named_reroutes()
            for rop in reroute_ops:
                if rop["op"] != "link_named_reroute_usage":
                    rop["op"] = add_node_op
            ops.extend(reroute_ops)

        if add_groups:
            self.auto_detect_groups()
            comment_ops = self.generate_comment_ops(op_type=add_node_op)
            ops.extend(comment_ops)

        return ops
