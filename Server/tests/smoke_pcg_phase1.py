"""
Phase 1 end-to-end smoke test for PCG MCP commands.

Exercises 21 of the 24 Phase 1 commands against a live Unreal Editor with
the UnrealMCP plugin compiled. Uses the raw UnrealConnection class (not the
@mcp.tool() wrappers) so the script is self-contained and does not require
a running FastMCP server process.

The 3 array-item commands (add_pcg_array_item, remove_pcg_array_item,
clear_pcg_array) are intentionally NOT exercised here — they need a node
type with a default-populated array property, which Phase 2's
StaticMeshSpawner smoke test will provide.

Response envelope contract from UnrealMCPBridge:
  - success:  {"status": "success", "result": {"success": true, ...payload...}}
  - failure:  {"status": "error",  "error":  "...", "result": {...}}

Handler fields (pcg_graphs, nodes, edges, properties, value, ...) live under
the nested "result" dict — NEVER at the top level. Use payload() to extract.

Run: python Plugins/UnrealMCP/Server/tests/smoke_pcg_phase1.py
"""

import os
import sys

# Add the Server dir to sys.path so `from unreal_mcp_server import ...` resolves.
SERVER_DIR = os.path.normpath(os.path.join(os.path.dirname(__file__), ".."))
if SERVER_DIR not in sys.path:
    sys.path.insert(0, SERVER_DIR)

from unreal_mcp_server import UnrealConnection  # noqa: E402


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

GRAPH_PATH = "/Game/PCG/_smoke/Phase1Smoke.Phase1Smoke"
COPY_PATH = "/Game/PCG/_smoke/Phase1Smoke_Copy.Phase1Smoke_Copy"
RENAMED_PATH = "/Game/PCG/_smoke/Phase1Smoke_Renamed.Phase1Smoke_Renamed"


def is_success(response):
    """
    The UnrealMCPBridge wraps handler responses as
      {"status": "success", "result": {"success": True, ...fields...}}
    on success, and
      {"status": "error", "error": "...", "result": {...}}
    on error (send_command remaps internal success:false into status:error).
    """
    if not isinstance(response, dict):
        return False
    if response.get("status") != "success":
        return False
    # Defensive: inner payload should also report success, but tolerate
    # missing inner flag if the top-level status is "success".
    inner = response.get("result")
    if isinstance(inner, dict) and inner.get("success") is False:
        return False
    return True


def payload(response):
    """
    Extract the handler payload dict from an envelope response. All handler
    fields (pcg_graphs, nodes, edges, properties, value, ...) live under
    this nested "result" object — NEVER at the envelope's top level.
    """
    if not isinstance(response, dict):
        return {}
    return response.get("result") or {}


def extract_error(response):
    """
    Pull a human-readable error string from either the envelope top level
    (where send_command's normalization puts it) or from the inner payload
    (where the handler emits it when status hasn't been remapped).
    """
    if not isinstance(response, dict):
        return str(response)
    inner = payload(response)
    return (
        response.get("error")
        or inner.get("error")
        or response.get("message")
        or inner.get("message")
        or str(response)
    )


def make_send(conn):
    """Build a send() closure that logs and returns the raw response."""
    def send(cmd, **params):
        response = conn.send_command(cmd, params)
        if response is None:
            print(f"[no-response] {cmd}")
            return {"status": "error", "error": "no response"}
        if is_success(response):
            print(f"[ok]    {cmd}")
        else:
            print(f"[error] {cmd}: {extract_error(response)}")
        return response
    return send


def assert_success(response, context):
    if not is_success(response):
        raise AssertionError(f"{context}: {extract_error(response)}")


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main():
    conn = UnrealConnection()
    if not conn.connect():
        print("FAIL: Could not connect to Unreal Engine (editor not running?)")
        sys.exit(1)
    # send_command reconnects per-call; close the probe socket.
    conn._close_socket()

    send = make_send(conn)

    # Cleanup any leftover from a previous run. Errors are expected and
    # ignored — the assets may not exist yet.
    print("\n--- Pre-test cleanup ---")
    send("end_pcg_edit", save=False)
    send("delete_pcg_graph", path=GRAPH_PATH)
    send("delete_pcg_graph", path=COPY_PATH)
    send("delete_pcg_graph", path=RENAMED_PATH)

    try:
        print("\n--- 1. Graph CRUD (create/list) ---")
        response = send("create_pcg_graph", name="Phase1Smoke", path="/Game/PCG/_smoke")
        assert_success(response, "create_pcg_graph")

        response = send("list_pcg_graphs", path_filter="/Game/PCG/_smoke")
        assert_success(response, "list_pcg_graphs")
        pcg_graphs = payload(response).get("pcg_graphs") or []
        found = any(g.get("name") == "Phase1Smoke" for g in pcg_graphs)
        if not found:
            raise AssertionError(
                f"list_pcg_graphs did not include 'Phase1Smoke'. Got: {pcg_graphs}")

        print("\n--- 2. Node type discovery ---")
        response = send("list_pcg_node_types")
        assert_success(response, "list_pcg_node_types")
        node_types_data = payload(response)
        node_types = node_types_data.get("node_types") or []
        type_names = {entry.get("type_name") for entry in node_types}
        # SurfaceSampler + TransformPoints are staples of PCG; if both exist
        # we're confident the reflection walk worked.
        for required in ("SurfaceSampler", "TransformPoints"):
            if required not in type_names:
                raise AssertionError(
                    f"list_pcg_node_types missing '{required}'. Sample: {sorted(type_names)[:20]}")
        count = node_types_data.get("count") or len(node_types)
        if count < 20:
            raise AssertionError(f"Expected at least 20 PCG node types, got {count}")

        print("\n--- 3. Schema introspection ---")
        response = send("get_pcg_node_schema", node_type="SurfaceSampler")
        assert_success(response, "get_pcg_node_schema SurfaceSampler")
        props = payload(response).get("properties") or []
        prop_names = {p.get("name") for p in props}
        # PointsPerSquaredMeter is a canonical SurfaceSampler knob; its absence
        # would indicate reflection is missing user-facing properties.
        if "PointsPerSquaredMeter" not in prop_names:
            raise AssertionError(
                f"SurfaceSampler schema missing 'PointsPerSquaredMeter'. Got: {sorted(prop_names)[:20]}")

        print("\n--- 4. Begin edit session ---")
        response = send("begin_pcg_edit", graph_path=GRAPH_PATH)
        assert_success(response, "begin_pcg_edit")

        print("\n--- 5. Add nodes ---")
        # Scalar-only properties for Phase 1. Struct/array properties are
        # exercised by Phase 2's smoke test with explicit marshaler coverage.
        response = send(
            "add_pcg_node",
            graph_path=GRAPH_PATH,
            node_type="SurfaceSampler",
            position=[200, 0],
            node_id="sampler",
            properties={"PointsPerSquaredMeter": 0.5},
        )
        assert_success(response, "add_pcg_node sampler")

        response = send(
            "add_pcg_node",
            graph_path=GRAPH_PATH,
            node_type="TransformPoints",
            position=[500, 0],
            node_id="xform",
        )
        assert_success(response, "add_pcg_node xform")

        print("\n--- 6. Connect edges ($input -> sampler -> xform -> $output) ---")
        for from_node, to_node in [
            ("$input", "sampler"),
            ("sampler", "xform"),
            ("xform", "$output"),
        ]:
            response = send(
                "connect_pcg_nodes",
                graph_path=GRAPH_PATH,
                from_node=from_node,
                to_node=to_node,
            )
            assert_success(response, f"connect_pcg_nodes {from_node}->{to_node}")

        print("\n--- 7. Graph snapshot + structure verification ---")
        response = send("get_pcg_graph_snapshot", graph_path=GRAPH_PATH)
        assert_success(response, "get_pcg_graph_snapshot")
        snapshot_data = payload(response)
        snapshot_nodes = snapshot_data.get("nodes") or []
        user_nodes = [n for n in snapshot_nodes if n.get("id") not in ("$input", "$output")]
        if len(user_nodes) != 2:
            raise AssertionError(
                f"Snapshot expected 2 user nodes, got {len(user_nodes)}: "
                f"{[n.get('id') for n in snapshot_nodes]}")
        edge_count = snapshot_data.get("edge_count")
        if edge_count != 3:
            raise AssertionError(f"Snapshot expected 3 edges, got {edge_count}")

        print("\n--- 8. Property round-trip (set -> get -> verify) ---")
        response = send(
            "set_pcg_node_property",
            graph_path=GRAPH_PATH,
            node_id="sampler",
            property_path="PointsPerSquaredMeter",
            value=1.0,
        )
        assert_success(response, "set_pcg_node_property")

        response = send(
            "get_pcg_node_property",
            graph_path=GRAPH_PATH,
            node_id="sampler",
            property_path="PointsPerSquaredMeter",
        )
        assert_success(response, "get_pcg_node_property")
        got_value = payload(response).get("value")
        if got_value is None or abs(float(got_value) - 1.0) > 1e-6:
            raise AssertionError(f"Property round-trip: expected 1.0, got {got_value}")

        print("\n--- 9. Node info + pin state ---")
        response = send("get_pcg_node_info", graph_path=GRAPH_PATH, node_id="sampler")
        assert_success(response, "get_pcg_node_info sampler")
        node_info = payload(response)
        if node_info.get("type") != "SurfaceSampler":
            raise AssertionError(
                f"Node info: expected type SurfaceSampler, got {node_info.get('type')}")
        output_pins = node_info.get("output_pins") or []
        if not output_pins:
            raise AssertionError("SurfaceSampler should have at least one output pin")

        response = send("list_pcg_node_pins", graph_path=GRAPH_PATH, node_id="xform")
        assert_success(response, "list_pcg_node_pins xform")

        print("\n--- 10. Move node ---")
        response = send(
            "move_pcg_node",
            graph_path=GRAPH_PATH,
            node_id="xform",
            x=600,
            y=100,
        )
        assert_success(response, "move_pcg_node")

        print("\n--- 11. Auto-layout ---")
        response = send("auto_layout_pcg_graph", graph_path=GRAPH_PATH)
        assert_success(response, "auto_layout_pcg_graph")

        print("\n--- 12. Disconnect + reconnect ---")
        # Pin labels depend on the node's declared PinProperties. disconnect
        # returns success even when the edge does not exist, so a failed
        # disconnect here would not short-circuit the test — the reconnect
        # step below is what verifies the pair actually works.
        send(
            "disconnect_pcg_pins",
            graph_path=GRAPH_PATH,
            from_node="sampler",
            from_pin="Out",
            to_node="xform",
            to_pin="In",
        )
        response = send(
            "connect_pcg_nodes",
            graph_path=GRAPH_PATH,
            from_node="sampler",
            to_node="xform",
        )
        assert_success(response, "reconnect sampler -> xform")

        print("\n--- 13. Delete node ---")
        response = send("delete_pcg_node", graph_path=GRAPH_PATH, node_id="xform")
        assert_success(response, "delete_pcg_node xform")

        print("\n--- 14. End session + save ---")
        response = send("end_pcg_edit", save=True)
        assert_success(response, "end_pcg_edit")

        print("\n--- 15. Save (explicit flush) ---")
        response = send("save_pcg_graph", path=GRAPH_PATH)
        assert_success(response, "save_pcg_graph")

        print("\n--- 16. Duplicate ---")
        response = send(
            "duplicate_pcg_graph",
            src_path=GRAPH_PATH,
            dst_path=COPY_PATH,
        )
        assert_success(response, "duplicate_pcg_graph")

        print("\n--- 17. Rename ---")
        response = send(
            "rename_pcg_graph",
            old_path=COPY_PATH,
            new_name="Phase1Smoke_Renamed",
        )
        assert_success(response, "rename_pcg_graph")

        print("\n--- 18. Final cleanup ---")
        response = send("delete_pcg_graph", path=GRAPH_PATH)
        assert_success(response, "delete_pcg_graph GRAPH_PATH")
        response = send("delete_pcg_graph", path=RENAMED_PATH)
        assert_success(response, "delete_pcg_graph RENAMED_PATH")

        print("\n========================================")
        print("PHASE 1 SMOKE TEST PASSED")
        print("========================================")

    except AssertionError as e:
        print(f"\n!!! ASSERTION FAILED: {e}")
        # Best-effort cleanup so the next run starts from a clean state.
        send("end_pcg_edit", save=False)
        send("delete_pcg_graph", path=GRAPH_PATH)
        send("delete_pcg_graph", path=COPY_PATH)
        send("delete_pcg_graph", path=RENAMED_PATH)
        sys.exit(1)


if __name__ == "__main__":
    main()
