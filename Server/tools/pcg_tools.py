"""
PCG (Procedural Content Generation) Tools for Unreal MCP.

This module exposes @mcp.tool() wrappers for every Phase 1 PCG command
implemented in FUnrealMCPPCGCommands on the C++ side. Wrappers forward
arguments as snake_case JSON to the command dispatcher via TCP.

Phase 1 coverage (24 tools):
  - Graph CRUD: create/delete/duplicate/rename/list/save_pcg_graph
  - Node discovery: list_pcg_node_types, get_pcg_node_schema
  - Session: begin_pcg_edit, end_pcg_edit
  - Construction: add/delete/move_pcg_node, connect/disconnect pins,
    auto_layout_pcg_graph
  - Property I/O: set/get_pcg_node_property, add/remove/clear array item
  - Introspection: get_pcg_graph_snapshot, get_pcg_node_info, list_pcg_node_pins

Phase 2 (component application, spec round-trip, subgraphs) ships later.
"""

import logging
from typing import Dict, List, Any, Optional
from mcp.server.fastmcp import FastMCP, Context

# Get logger
logger = logging.getLogger("UnrealMCP")


def register_pcg_tools(mcp: FastMCP):
    """Register all Phase 1 PCG tools with the MCP server."""

    # ========================================================================
    # Graph Asset CRUD
    # ========================================================================

    @mcp.tool()
    def create_pcg_graph(
        ctx: Context,
        name: str,
        path: str = "/Game/PCG"
    ) -> Dict[str, Any]:
        """
        Create a new PCG Graph asset.

        Args:
            name: Name for the new PCG Graph asset
            path: Content Browser folder path (default "/Game/PCG")

        Returns:
            Response with graph_path and name on success
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            logger.info(f"Creating PCG graph '{name}' at {path}")
            response = unreal.send_command("create_pcg_graph", {"name": name, "path": path})
            if not response:
                return {"success": False, "message": "No response from Unreal Engine"}
            return response
        except Exception as e:
            logger.error(f"Error creating PCG graph: {e}")
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def delete_pcg_graph(
        ctx: Context,
        path: str
    ) -> Dict[str, Any]:
        """
        Delete a PCG Graph asset from the Content Browser.

        Args:
            path: Object path of the PCG Graph to delete (e.g. "/Game/PCG/Foo.Foo")

        Returns:
            Response with the deleted path on success
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            logger.info(f"Deleting PCG graph at {path}")
            response = unreal.send_command("delete_pcg_graph", {"path": path})
            if not response:
                return {"success": False, "message": "No response from Unreal Engine"}
            return response
        except Exception as e:
            logger.error(f"Error deleting PCG graph: {e}")
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def duplicate_pcg_graph(
        ctx: Context,
        src_path: str,
        dst_path: str
    ) -> Dict[str, Any]:
        """
        Duplicate a PCG Graph asset to a new object path.

        Args:
            src_path: Source object path (e.g. "/Game/PCG/Foo.Foo")
            dst_path: Destination object path (e.g. "/Game/PCG/FooCopy.FooCopy")

        Returns:
            Response with src_path and dst_path on success
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            logger.info(f"Duplicating PCG graph {src_path} -> {dst_path}")
            response = unreal.send_command("duplicate_pcg_graph", {
                "src_path": src_path,
                "dst_path": dst_path,
            })
            if not response:
                return {"success": False, "message": "No response from Unreal Engine"}
            return response
        except Exception as e:
            logger.error(f"Error duplicating PCG graph: {e}")
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def rename_pcg_graph(
        ctx: Context,
        old_path: str,
        new_name: str
    ) -> Dict[str, Any]:
        """
        Rename a PCG Graph asset in place (keeps the parent package directory).

        Args:
            old_path: Current object path (e.g. "/Game/PCG/Foo.Foo")
            new_name: New asset name (directory is preserved)

        Returns:
            Response with old_path and new_path on success
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            logger.info(f"Renaming PCG graph {old_path} -> {new_name}")
            response = unreal.send_command("rename_pcg_graph", {
                "old_path": old_path,
                "new_name": new_name,
            })
            if not response:
                return {"success": False, "message": "No response from Unreal Engine"}
            return response
        except Exception as e:
            logger.error(f"Error renaming PCG graph: {e}")
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def list_pcg_graphs(
        ctx: Context,
        path_filter: Optional[str] = None
    ) -> Dict[str, Any]:
        """
        List PCG Graph assets in the project, optionally scoped to a folder.

        Args:
            path_filter: Optional content-path prefix (e.g. "/Game/PCG/_smoke").
                         When None, all PCG graphs are returned.

        Returns:
            Response with pcg_graphs array and count
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            params: Dict[str, Any] = {}
            if path_filter is not None:
                params["path_filter"] = path_filter
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("list_pcg_graphs", params)
            if not response:
                return {"success": False, "message": "No response from Unreal Engine"}
            return response
        except Exception as e:
            logger.error(f"Error listing PCG graphs: {e}")
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def save_pcg_graph(
        ctx: Context,
        path: str
    ) -> Dict[str, Any]:
        """
        Save a PCG Graph asset to disk. Works even if the dirty flag has
        already been cleared (explicit flush after a batch of edits).

        Args:
            path: Object path of the PCG Graph (e.g. "/Game/PCG/Foo.Foo")

        Returns:
            Response with saved path on success
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            logger.info(f"Saving PCG graph at {path}")
            response = unreal.send_command("save_pcg_graph", {"path": path})
            if not response:
                return {"success": False, "message": "No response from Unreal Engine"}
            return response
        except Exception as e:
            logger.error(f"Error saving PCG graph: {e}")
            return {"success": False, "message": str(e)}

    # ========================================================================
    # Node Type Discovery
    # ========================================================================

    @mcp.tool()
    def list_pcg_node_types(
        ctx: Context,
        category: Optional[str] = None
    ) -> Dict[str, Any]:
        """
        List every PCG node type exposed to the library, optionally filtered
        by category. Category values come from EPCGSettingsType (e.g. "Sampler",
        "Filter", "Spawner").

        Args:
            category: Optional EPCGSettingsType enum name filter.

        Returns:
            Response with node_types array and count
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            params: Dict[str, Any] = {}
            if category is not None:
                params["category"] = category
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("list_pcg_node_types", params)
            if not response:
                return {"success": False, "message": "No response from Unreal Engine"}
            return response
        except Exception as e:
            logger.error(f"Error listing PCG node types: {e}")
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def get_pcg_node_schema(
        ctx: Context,
        node_type: str
    ) -> Dict[str, Any]:
        """
        Get the full schema (properties + pins + category) for a PCG node type.

        Args:
            node_type: Short type name (e.g. "SurfaceSampler", "TransformPoints")

        Returns:
            Response with type_name, class_path, display_name, category,
            pins (input/output arrays), and properties array
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("get_pcg_node_schema", {"node_type": node_type})
            if not response:
                return {"success": False, "message": "No response from Unreal Engine"}
            return response
        except Exception as e:
            logger.error(f"Error getting PCG node schema: {e}")
            return {"success": False, "message": str(e)}

    # ========================================================================
    # Edit Session (batched mutations)
    # ========================================================================

    @mcp.tool()
    def begin_pcg_edit(
        ctx: Context,
        graph_path: str
    ) -> Dict[str, Any]:
        """
        Begin a batched edit session for a PCG Graph. Defers editor
        notifications until end_pcg_edit so a run of mutation commands
        does not spam the editor with per-op repaints.

        Args:
            graph_path: Object path of the PCG Graph (e.g. "/Game/PCG/Foo.Foo")

        Returns:
            Response with graph_path on success
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            logger.info(f"Beginning PCG edit session for {graph_path}")
            response = unreal.send_command("begin_pcg_edit", {"graph_path": graph_path})
            if not response:
                return {"success": False, "message": "No response from Unreal Engine"}
            return response
        except Exception as e:
            logger.error(f"Error beginning PCG edit: {e}")
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def end_pcg_edit(
        ctx: Context,
        auto_layout: bool = False,
        save: bool = False
    ) -> Dict[str, Any]:
        """
        End the active PCG edit session, fire a single structural notification,
        and optionally auto-layout and/or save the graph.

        Args:
            auto_layout: If True, run auto_layout_pcg_graph before closing.
            save: If True, save the graph asset after closing.

        Returns:
            Response with closed graph_path, auto_layout flag, saved flag
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            logger.info(f"Ending PCG edit session (auto_layout={auto_layout}, save={save})")
            response = unreal.send_command("end_pcg_edit", {
                "auto_layout": auto_layout,
                "save": save,
            })
            if not response:
                return {"success": False, "message": "No response from Unreal Engine"}
            return response
        except Exception as e:
            logger.error(f"Error ending PCG edit: {e}")
            return {"success": False, "message": str(e)}

    # ========================================================================
    # Graph Construction (nodes, edges, layout)
    # ========================================================================

    @mcp.tool()
    def add_pcg_node(
        ctx: Context,
        graph_path: str,
        node_type: str,
        position: Optional[List[float]] = None,
        node_id: Optional[str] = None,
        properties: Optional[Dict[str, Any]] = None
    ) -> Dict[str, Any]:
        """
        Add a node to a PCG graph inside an active edit session.

        Args:
            graph_path: Path of the graph being edited (must match the active session)
            node_type: Short type name (e.g. "SurfaceSampler", "TransformPoints")
            position: Optional [x, y] viewport coordinates. Defaults to [0, 0].
            node_id: Optional session-local id. When omitted, a "<type>_<counter>"
                     id is generated.
            properties: Optional dict of property_name -> JSON value. Unsupported
                        types are collected in skipped_properties in the response.

        Returns:
            Response with node_id, node_type, position, applied_properties,
            skipped_properties
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            params: Dict[str, Any] = {
                "graph_path": graph_path,
                "node_type": node_type,
                "position": position if position is not None else [0, 0],
                "properties": properties if properties is not None else {},
            }
            if node_id is not None:
                params["node_id"] = node_id
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            logger.info(f"Adding PCG node {node_type} to {graph_path}")
            response = unreal.send_command("add_pcg_node", params)
            if not response:
                return {"success": False, "message": "No response from Unreal Engine"}
            return response
        except Exception as e:
            logger.error(f"Error adding PCG node: {e}")
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def delete_pcg_node(
        ctx: Context,
        graph_path: str,
        node_id: str
    ) -> Dict[str, Any]:
        """
        Delete a node from a PCG graph inside an active edit session.
        The reserved $input/$output nodes cannot be deleted.

        Args:
            graph_path: Path of the graph being edited (must match active session)
            node_id: Session-local node id

        Returns:
            Response with deleted_node_id on success
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            logger.info(f"Deleting PCG node {node_id} from {graph_path}")
            response = unreal.send_command("delete_pcg_node", {
                "graph_path": graph_path,
                "node_id": node_id,
            })
            if not response:
                return {"success": False, "message": "No response from Unreal Engine"}
            return response
        except Exception as e:
            logger.error(f"Error deleting PCG node: {e}")
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def move_pcg_node(
        ctx: Context,
        graph_path: str,
        node_id: str,
        x: float,
        y: float
    ) -> Dict[str, Any]:
        """
        Move a node to a new viewport position inside an active edit session.

        Args:
            graph_path: Path of the graph being edited (must match active session)
            node_id: Session-local node id
            x: New X coordinate (integer, converted from float)
            y: New Y coordinate (integer, converted from float)

        Returns:
            Response with node_id and position array
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("move_pcg_node", {
                "graph_path": graph_path,
                "node_id": node_id,
                "x": x,
                "y": y,
            })
            if not response:
                return {"success": False, "message": "No response from Unreal Engine"}
            return response
        except Exception as e:
            logger.error(f"Error moving PCG node: {e}")
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def connect_pcg_nodes(
        ctx: Context,
        graph_path: str,
        from_node: str,
        to_node: str,
        from_pin: Optional[str] = None,
        to_pin: Optional[str] = None
    ) -> Dict[str, Any]:
        """
        Connect two PCG nodes by edge. Pin labels are auto-resolved when
        omitted — the first compatible unconnected pin on each side wins.

        Args:
            graph_path: Path of the graph being edited (must match active session)
            from_node: Source node id ("$input" is valid)
            to_node: Destination node id ("$output" is valid)
            from_pin: Optional source pin label. Auto-picks when omitted.
            to_pin: Optional destination pin label. Auto-picks when omitted.

        Returns:
            Response with from_node, from_pin, to_node, to_pin, and:
              - edge_created: always True on success (handler errors out if
                either pin cannot be resolved before reaching the success path)
              - broke_existing_edge: True if the new edge displaced a
                pre-existing edge on a single-connection pin, False otherwise
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            params: Dict[str, Any] = {
                "graph_path": graph_path,
                "from_node": from_node,
                "to_node": to_node,
            }
            if from_pin is not None:
                params["from_pin"] = from_pin
            if to_pin is not None:
                params["to_pin"] = to_pin
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            logger.info(f"Connecting PCG {from_node} -> {to_node} in {graph_path}")
            response = unreal.send_command("connect_pcg_nodes", params)
            if not response:
                return {"success": False, "message": "No response from Unreal Engine"}
            return response
        except Exception as e:
            logger.error(f"Error connecting PCG nodes: {e}")
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def disconnect_pcg_pins(
        ctx: Context,
        graph_path: str,
        from_node: str,
        from_pin: str,
        to_node: str,
        to_pin: str
    ) -> Dict[str, Any]:
        """
        Remove a specific edge between two PCG node pins.

        Args:
            graph_path: Path of the graph being edited (must match active session)
            from_node: Source node id
            from_pin: Source pin label
            to_node: Destination node id
            to_pin: Destination pin label

        Returns:
            Response with the four endpoint fields and a disconnected flag
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("disconnect_pcg_pins", {
                "graph_path": graph_path,
                "from_node": from_node,
                "from_pin": from_pin,
                "to_node": to_node,
                "to_pin": to_pin,
            })
            if not response:
                return {"success": False, "message": "No response from Unreal Engine"}
            return response
        except Exception as e:
            logger.error(f"Error disconnecting PCG pins: {e}")
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def auto_layout_pcg_graph(
        ctx: Context,
        graph_path: str
    ) -> Dict[str, Any]:
        """
        Apply topological-column layout to a PCG graph: longest-path layering
        assigns columns, nodes stack in insertion order within each column.

        Args:
            graph_path: Path of the graph being edited (must match active session)

        Returns:
            Response with node_count and column_count
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            logger.info(f"Auto-laying out PCG graph {graph_path}")
            response = unreal.send_command("auto_layout_pcg_graph", {"graph_path": graph_path})
            if not response:
                return {"success": False, "message": "No response from Unreal Engine"}
            return response
        except Exception as e:
            logger.error(f"Error auto-laying PCG graph: {e}")
            return {"success": False, "message": str(e)}

    # ========================================================================
    # Property Manipulation
    # ========================================================================

    @mcp.tool()
    def set_pcg_node_property(
        ctx: Context,
        graph_path: str,
        node_id: str,
        property_path: str,
        value: Any
    ) -> Dict[str, Any]:
        """
        Set a property on a PCG node. property_path supports dotted/bracketed
        nested access (e.g. "Mesh.Entries[2].Descriptor.StaticMesh").

        Args:
            graph_path: Path of the graph being edited (must match active session)
            node_id: Session-local node id
            property_path: Dotted path into the node's settings
            value: JSON value to marshal into the target property

        Returns:
            Response with node_id, property_path, applied flag
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("set_pcg_node_property", {
                "graph_path": graph_path,
                "node_id": node_id,
                "property_path": property_path,
                "value": value,
            })
            if not response:
                return {"success": False, "message": "No response from Unreal Engine"}
            return response
        except Exception as e:
            logger.error(f"Error setting PCG node property: {e}")
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def get_pcg_node_property(
        ctx: Context,
        graph_path: str,
        node_id: str,
        property_path: str
    ) -> Dict[str, Any]:
        """
        Get a property value from a PCG node. Round-trips through the marshaler
        so the returned JSON shape matches what set_pcg_node_property accepts.

        Args:
            graph_path: Path of the graph being edited (must match active session)
            node_id: Session-local node id
            property_path: Dotted path into the node's settings

        Returns:
            Response with node_id, property_path, and the serialized value
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("get_pcg_node_property", {
                "graph_path": graph_path,
                "node_id": node_id,
                "property_path": property_path,
            })
            if not response:
                return {"success": False, "message": "No response from Unreal Engine"}
            return response
        except Exception as e:
            logger.error(f"Error getting PCG node property: {e}")
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def add_pcg_array_item(
        ctx: Context,
        graph_path: str,
        node_id: str,
        array_path: str,
        value: Optional[Any] = None
    ) -> Dict[str, Any]:
        """
        Append a new element to a PCG node array property. When value is
        provided, the marshaler fills the new element; otherwise the element
        is default-initialized.

        Args:
            graph_path: Path of the graph being edited (must match active session)
            node_id: Session-local node id
            array_path: Dotted path to the array property
            value: Optional JSON value marshaled into the new element

        Returns:
            Response with new_index and new_size
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            params: Dict[str, Any] = {
                "graph_path": graph_path,
                "node_id": node_id,
                "array_path": array_path,
            }
            if value is not None:
                params["value"] = value
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("add_pcg_array_item", params)
            if not response:
                return {"success": False, "message": "No response from Unreal Engine"}
            return response
        except Exception as e:
            logger.error(f"Error adding PCG array item: {e}")
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def remove_pcg_array_item(
        ctx: Context,
        graph_path: str,
        node_id: str,
        array_path: str,
        index: int
    ) -> Dict[str, Any]:
        """
        Remove a single element from a PCG node array property by index.

        Args:
            graph_path: Path of the graph being edited (must match active session)
            node_id: Session-local node id
            array_path: Dotted path to the array property
            index: Zero-based index of the element to remove

        Returns:
            Response with removed_index and new_size
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("remove_pcg_array_item", {
                "graph_path": graph_path,
                "node_id": node_id,
                "array_path": array_path,
                "index": index,
            })
            if not response:
                return {"success": False, "message": "No response from Unreal Engine"}
            return response
        except Exception as e:
            logger.error(f"Error removing PCG array item: {e}")
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def clear_pcg_array(
        ctx: Context,
        graph_path: str,
        node_id: str,
        array_path: str
    ) -> Dict[str, Any]:
        """
        Remove all elements from a PCG node array property.

        Args:
            graph_path: Path of the graph being edited (must match active session)
            node_id: Session-local node id
            array_path: Dotted path to the array property

        Returns:
            Response with cleared flag
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("clear_pcg_array", {
                "graph_path": graph_path,
                "node_id": node_id,
                "array_path": array_path,
            })
            if not response:
                return {"success": False, "message": "No response from Unreal Engine"}
            return response
        except Exception as e:
            logger.error(f"Error clearing PCG array: {e}")
            return {"success": False, "message": str(e)}

    # ========================================================================
    # Introspection / Round-trip
    # ========================================================================

    @mcp.tool()
    def get_pcg_graph_snapshot(
        ctx: Context,
        graph_path: str
    ) -> Dict[str, Any]:
        """
        Serialize the full state of a PCG graph for round-tripping or diffing.
        Non-default properties are included per node; CDO-equal
        non-overridable fields are elided to keep snapshots compact.

        Args:
            graph_path: Path of the graph (session-aware: uses in-memory state
                       when the graph is the active edit session target)

        Returns:
            Response with nodes array, edges array, node_count, edge_count
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("get_pcg_graph_snapshot", {"graph_path": graph_path})
            if not response:
                return {"success": False, "message": "No response from Unreal Engine"}
            return response
        except Exception as e:
            logger.error(f"Error getting PCG graph snapshot: {e}")
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def get_pcg_node_info(
        ctx: Context,
        graph_path: str,
        node_id: str
    ) -> Dict[str, Any]:
        """
        Get full detail for a single PCG node: type, position, non-default
        properties, and live pin state (edges with connected endpoints).

        Args:
            graph_path: Path of the graph (session-aware)
            node_id: Session-local node id or reserved "$input"/"$output"

        Returns:
            Response with node_id, type, position, properties,
            input_pins array, output_pins array
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("get_pcg_node_info", {
                "graph_path": graph_path,
                "node_id": node_id,
            })
            if not response:
                return {"success": False, "message": "No response from Unreal Engine"}
            return response
        except Exception as e:
            logger.error(f"Error getting PCG node info: {e}")
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def list_pcg_node_pins(
        ctx: Context,
        graph_path: str,
        node_id: str
    ) -> Dict[str, Any]:
        """
        List the input and output pin state for a PCG node. Pin-only subset of
        get_pcg_node_info, useful for debugging connection issues without
        pulling the full property payload.

        Args:
            graph_path: Path of the graph (session-aware)
            node_id: Session-local node id or reserved "$input"/"$output"

        Returns:
            Response with node_id, input_pins array, output_pins array
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("list_pcg_node_pins", {
                "graph_path": graph_path,
                "node_id": node_id,
            })
            if not response:
                return {"success": False, "message": "No response from Unreal Engine"}
            return response
        except Exception as e:
            logger.error(f"Error listing PCG node pins: {e}")
            return {"success": False, "message": str(e)}
