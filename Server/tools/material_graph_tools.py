"""
High-level material graph building tools for Unreal MCP.

These tools expose a declarative interface that lets Claude describe an entire
material or material-function graph in one call.  The MaterialGraphBuilder
engine handles auto-layout, comment-box grouping, and named reroute insertion
before forwarding a single optimised batch to the C++ MCP bridge.

Registered tools:
    build_function_graph    — Create a complete material function from a spec
    build_material_graph    — Create a complete material from a spec
    build_material          — Build or overwrite a complete material in one atomic call
    get_material_preview    — Get a rendered preview thumbnail as base64 PNG
    analyze_material_graph  — Inspect an existing material for layout issues
    fix_material_graph      — Auto-fix layout, reroutes, and groups in-place
"""

import logging
from typing import Dict, List, Any, Optional

from mcp.server.fastmcp import FastMCP, Context

logger = logging.getLogger("UnrealMCP")


def register_material_graph_tools(mcp: FastMCP) -> None:
    """Register high-level material graph building tools with the MCP server."""

    # ------------------------------------------------------------------
    # build_function_graph
    # ------------------------------------------------------------------

    @mcp.tool()
    def build_function_graph(
        ctx: Context,
        function_name: str,
        inputs: List[Dict[str, Any]],
        outputs: List[Dict[str, Any]],
        nodes: List[Dict[str, Any]],
        connections: List[Dict[str, Any]],
        groups: Optional[List[Dict[str, Any]]] = None,
        create_function: bool = True,
        description: str = "",
        expose_to_library: bool = True,
        library_categories: Optional[List[str]] = None,
        auto_layout: bool = True,
        auto_group: bool = True,
        auto_reroute: bool = True,
        auto_save: bool = False,
    ) -> Dict[str, Any]:
        """
        Build a complete material function graph in one declarative call.

        Handles asset creation, inputs/outputs, all interior nodes, wiring,
        automatic node layout, comment-box grouping, and named reroutes for
        fan-out connections — all from a single specification dict.

        Args:
            function_name: Full content path, e.g.
                           ``"/Game/Materials/Functions/MF_MyFunc"``.
            inputs: Function input descriptors. Each dict supports:
                    ``name`` (str, required), ``type`` (str, e.g. "Scalar" /
                    "Texture2D" / "Vector3" / "Boolean"), ``description`` (str),
                    ``default`` (float — sets preview value), ``priority`` (int).
            outputs: Function output descriptors. Each dict supports:
                     ``name`` (str, required), ``description`` (str),
                     ``priority`` (int).
            nodes: Interior graph nodes. Each dict supports:
                   ``name`` (str, required), ``type`` (str, required — e.g.
                   "TextureCoordinate", "Multiply", "Add", "Subtract", "Lerp",
                   "Clamp", "TextureSample", "Constant", "ComponentMask",
                   "DotProduct", "Sqrt", "LinearInterpolate", "If",
                   "MaterialFunctionCall", "NamedRerouteDeclaration",
                   "NamedRerouteUsage", …), plus optional ``value``,
                   ``color``, ``texture_path``, ``position`` ([x, y]), etc.
            connections: Wiring list. Each dict supports:
                         ``from`` (str, required — node name or input name),
                         ``to`` (str, required — node name or ``"output:Name"``
                         for function outputs), ``input`` (str — destination pin
                         name, e.g. "A", "B", "Alpha", "UVs", "Tex"),
                         ``output`` (str — source pin, e.g. "R", "G", "RGB",
                         or ``""`` for the default output).
            groups: Optional manual comment-box groups. Each dict:
                    ``name`` (str), ``nodes`` (list[str]).
                    If omitted and ``auto_group=True``, groups are inferred.
            create_function: Create the function asset before building
                             (default: True).
            description: Function description / tooltip text.
            expose_to_library: Show in the material function library
                               (default: True).
            library_categories: List of category strings for library
                                 organisation.
            auto_layout: Compute optimal node positions with the layered-graph
                         algorithm (default: True).
            auto_group: Detect and create comment-box groups automatically
                        (default: True).  Ignored when ``groups`` is provided.
            auto_reroute: Insert named reroutes where a single output feeds
                          more than 2 targets (default: True).
            auto_save: Save the function asset after building (default: False).

        Returns:
            Dict with keys:
            ``success`` (bool), ``operations_executed`` (int),
            ``operations_failed`` (int), ``total_operations`` (int),
            ``issues`` (list), ``node_count`` (int), ``input_count`` (int),
            ``output_count`` (int), ``groups_created`` (int),
            ``details`` (list).

        Example::

            build_function_graph(
                function_name="/Game/LandscapeMaterial/MF_LandscapeLayer",
                inputs=[
                    {"name": "BaseColor", "type": "Texture2D",
                     "description": "Base colour texture"},
                    {"name": "Tiling", "type": "Scalar", "default": 4.0},
                ],
                outputs=[
                    {"name": "Color", "description": "Final colour output"},
                ],
                nodes=[
                    {"name": "TexCoord", "type": "TextureCoordinate"},
                    {"name": "MulUV",    "type": "Multiply"},
                    {"name": "Sample",   "type": "TextureSample"},
                ],
                connections=[
                    {"from": "TexCoord", "to": "MulUV",   "input": "A"},
                    {"from": "Tiling",   "to": "MulUV",   "input": "B"},
                    {"from": "MulUV",    "to": "Sample",  "input": "UVs"},
                    {"from": "BaseColor","to": "Sample",  "input": "Tex"},
                    {"from": "Sample",   "to": "output:Color"},
                ],
            )
        """
        from unreal_mcp_server import get_unreal_connection
        from tools.material_graph_builder import MaterialGraphBuilder

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "error": "Failed to connect to Unreal Engine"}

            builder = MaterialGraphBuilder()
            builder.load_function_spec(inputs, outputs, nodes, connections, groups)

            if auto_layout:
                builder.compute_layout()

            issues = builder.validate()

            ops = builder.generate_function_ops(
                function_name,
                inputs,
                outputs,
                auto_layout=auto_layout,
                auto_group=auto_group,
                auto_reroute=auto_reroute,
            )

            # Create the function asset first (ignored gracefully if it exists)
            if create_function:
                name_parts = function_name.rsplit("/", 1)
                create_params: Dict[str, Any] = {
                    "function_name": name_parts[-1],
                    "path": name_parts[0] if len(name_parts) > 1 else "/Game/Materials/Functions",
                    "description": description,
                    "expose_to_library": expose_to_library,
                }
                if library_categories:
                    create_params["library_categories"] = library_categories

                create_resp = unreal.send_command("create_material_function", create_params)
                logger.info("build_function_graph: create_material_function response: %s", create_resp)
                if not create_resp:
                    return {"success": False, "error": "create_material_function returned None"}

            # Execute the full batch
            batch_params: Dict[str, Any] = {
                "function_name": function_name,
                "operations": ops,
                "auto_save": auto_save,
            }
            response = unreal.send_command("execute_function_batch", batch_params)

            if not response:
                return {"success": False, "error": "No response from execute_function_batch"}

            # Check for top-level error from bridge or send_command
            if response.get("status") == "error" and "result" not in response:
                return {"success": False, "error": response.get("error", "Unknown batch error"), "raw_response": response}

            # Unwrap the bridge envelope: actual batch data is inside "result"
            batch = response.get("result", response)

            # Propagate error from batch if present
            batch_error = batch.get("error", "")

            return {
                "success": batch.get("success", False),
                "error": batch_error,
                "operations_executed": batch.get("success_count", 0),
                "operations_failed": batch.get("fail_count", 0),
                "total_operations": batch.get("total_operations", len(ops)),
                "issues": issues,
                "node_count": len(nodes),
                "input_count": len(inputs),
                "output_count": len(outputs),
                "groups_created": len(builder.groups),
                "details": batch.get("results", []),
            }

        except Exception as exc:
            logger.error("build_function_graph error: %s", exc, exc_info=True)
            return {"success": False, "error": str(exc)}

    # ------------------------------------------------------------------
    # build_material_graph
    # ------------------------------------------------------------------

    @mcp.tool()
    def build_material_graph(
        ctx: Context,
        material_name: str,
        nodes: List[Dict[str, Any]],
        connections: List[Dict[str, Any]],
        material_outputs: Optional[List[Dict[str, Any]]] = None,
        groups: Optional[List[Dict[str, Any]]] = None,
        create_material: bool = True,
        domain: str = "Surface",
        blend_mode: str = "Opaque",
        shading_model: str = "DefaultLit",
        two_sided: bool = False,
        properties: Optional[Dict[str, Any]] = None,
        auto_layout: bool = True,
        auto_group: bool = True,
        auto_reroute: bool = True,
        auto_recompile: bool = True,
        auto_save: bool = False,
        return_details: bool = False,
    ) -> Dict[str, Any]:
        """
        Build a complete material graph in one declarative call.

        Handles asset creation, all expression nodes, wiring to material output
        pins, automatic layout, comment-box grouping, and named reroute insertion.

        Args:
            material_name: Full content path, e.g.
                           ``"/Game/Materials/M_MyMaterial"``.
            nodes: Expression nodes. Each dict supports:
                   ``name`` (str, required), ``type`` (str, required — e.g.
                   "TextureSampleParameter2D", "ScalarParameter",
                   "VectorParameter", "Multiply", "Add", "Lerp", "Clamp",
                   "ComponentMask", "MaterialFunctionCall", "Constant",
                   "Constant3Vector", …), plus optional ``value``,
                   ``color``, ``texture_path``, ``function_name``,
                   ``position`` ([x, y]).
            connections: Interior wiring. Each dict:
                         ``from`` (str), ``to`` (str), ``input`` (str pin
                         name), ``output`` (str pin name, default ``""``).
            material_outputs: Connections to the material result node. Each:
                              ``from`` (str node name), ``property`` (str —
                              "BaseColor", "Normal", "Roughness", "Metallic",
                              "Emissive", "Opacity", "AmbientOcclusion", …),
                              ``output`` (str pin name, default ``""``).
            groups: Optional manual comment-box groups.
            create_material: Create the material asset before building
                             (default: True).
            domain: Material domain (Surface, DeferredDecal, UI, …).
            blend_mode: Blend mode (Opaque, Masked, Translucent, …).
            shading_model: Shading model (DefaultLit, Unlit, …).
            two_sided: Enable two-sided rendering.
            properties: Additional material property overrides dict.
            auto_layout: Compute optimal node positions (default: True).
            auto_group: Detect comment-box groups automatically (default: True).
            auto_reroute: Insert named reroutes for fan-out > 2 (default: True).
            auto_recompile: Recompile the material after building (default: True).
            auto_save: Save the asset after building (default: False).

        Returns:
            Dict with keys: ``success``, ``operations_executed``,
            ``operations_failed``, ``total_operations``, ``issues``,
            ``node_count``, ``groups_created``, ``details``.
        """
        from unreal_mcp_server import get_unreal_connection
        from tools.material_graph_builder import MaterialGraphBuilder

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "error": "Failed to connect to Unreal Engine"}

            builder = MaterialGraphBuilder()
            builder.load_material_spec(nodes, connections, material_outputs, groups)

            if auto_layout:
                builder.compute_layout()

            issues = builder.validate()

            ops = builder.generate_material_ops(
                material_name,
                nodes,
                auto_layout=auto_layout,
                auto_group=auto_group,
                auto_reroute=auto_reroute,
            )

            # Create the material asset (ignored gracefully if it already exists)
            if create_material:
                name_parts = material_name.rsplit("/", 1)
                create_params: Dict[str, Any] = {
                    "material_name": name_parts[-1],
                    "path": name_parts[0] if len(name_parts) > 1 else "/Game/Materials",
                    "domain": domain,
                    "blend_mode": blend_mode,
                    "shading_model": shading_model,
                    "two_sided": two_sided,
                }
                if properties:
                    create_params["properties"] = properties

                create_resp = unreal.send_command("create_material", create_params)
                if not create_resp or not create_resp.get("success", False):
                    logger.warning(
                        "build_material_graph: create_material returned: %s",
                        create_resp,
                    )

            # Execute the full batch
            batch_params: Dict[str, Any] = {
                "material_name": material_name,
                "operations": ops,
                "auto_recompile": auto_recompile,
                "auto_save": auto_save,
            }
            response = unreal.send_command("execute_material_batch", batch_params)

            if not response:
                return {"success": False, "error": "No response from execute_material_batch"}

            # Unwrap the bridge envelope: actual batch data is inside "result"
            batch = response.get("result", response)

            result: Dict[str, Any] = {
                "success": batch.get("success", False),
                "operations_executed": batch.get("success_count", 0),
                "operations_failed": batch.get("fail_count", 0),
                "total_operations": batch.get("total_operations", len(ops)),
                "issues": issues,
                "node_count": len(nodes),
                "groups_created": len(builder.groups),
            }
            if return_details:
                result["details"] = batch.get("results", [])
            return result

        except Exception as exc:
            logger.error("build_material_graph error: %s", exc, exc_info=True)
            return {"success": False, "error": str(exc)}

    # ------------------------------------------------------------------
    # analyze_material_graph
    # ------------------------------------------------------------------

    @mcp.tool()
    def analyze_material_graph(
        ctx: Context,
        material_name: str,
    ) -> Dict[str, Any]:
        """
        Analyse an existing material graph and report layout issues.

        Checks for: orphaned nodes (no connections), disconnected function
        outputs, overlapping nodes, missing comment-box organisation, and
        fan-out connections that should use named reroutes.

        Args:
            material_name: Material short name or full content path.

        Returns:
            Dict with keys: ``success``, ``node_count``, ``connection_count``,
            ``issues`` (list of issue dicts), ``issue_count`` (int).

        Each issue dict contains at minimum ``type`` and ``message``.
        Additional keys vary by type:
        - ``"orphaned_node"`` → ``node`` (str)
        - ``"disconnected_output"`` → ``node`` (str)
        - ``"overlapping"`` → ``nodes`` (list[str])
        - ``"needs_reroute"`` → ``node`` (str), ``output`` (str),
          ``fanout`` (int)
        - ``"no_groups"`` — no extra keys
        """
        from unreal_mcp_server import get_unreal_connection
        from tools.material_graph_builder import MaterialGraphBuilder

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("get_material_hierarchy", {"material_name": material_name})
            if not response:
                return {"success": False, "error": "Failed to read material graph"}

            builder = MaterialGraphBuilder()
            data = response.get("result", response)
            builder.load_from_existing(data)

            analysis = builder.analyze()
            analysis["success"] = True
            return analysis

        except Exception as exc:
            logger.error("analyze_material_graph error: %s", exc, exc_info=True)
            return {"success": False, "error": str(exc)}

    # ------------------------------------------------------------------
    # fix_material_graph
    # ------------------------------------------------------------------

    @mcp.tool()
    def fix_material_graph(
        ctx: Context,
        material_name: str,
        relayout: bool = True,
        add_reroutes: bool = True,
        add_groups: bool = True,
        auto_recompile: bool = True,
    ) -> Dict[str, Any]:
        """
        Automatically fix layout issues in an existing material graph.

        Reads the current graph, generates the necessary correction ops, and
        executes them as a single batch.  Can independently recompute node
        positions, add named reroutes for fan-out connections, and add
        comment-box groups.

        Args:
            material_name: Material short name or full content path.
            relayout: Recompute all node positions with the layered-graph
                      algorithm (default: True).
            add_reroutes: Insert named reroute pairs wherever a single output
                          feeds more than 2 targets (default: True).
            add_groups: Detect node groups and add comment boxes (default: True).
            auto_recompile: Recompile the material after applying fixes
                            (default: True).

        Returns:
            Dict with keys: ``success``, ``operations_executed``,
            ``operations_failed``, ``fixes_applied`` (int — total ops
            generated), and optionally ``message`` when no fixes were needed.
        """
        from unreal_mcp_server import get_unreal_connection
        from tools.material_graph_builder import MaterialGraphBuilder

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("get_material_hierarchy", {"material_name": material_name})
            if not response:
                return {"success": False, "error": "Failed to read material graph"}

            builder = MaterialGraphBuilder()
            data = response.get("result", response)
            builder.load_from_existing(data)

            ops = builder.generate_fix_ops(
                target_key="material_name",
                relayout=relayout,
                add_reroutes=add_reroutes,
                add_groups=add_groups,
            )

            if not ops:
                return {
                    "success": True,
                    "message": "No fixes were needed",
                    "operations_executed": 0,
                    "fixes_applied": 0,
                }

            batch_params: Dict[str, Any] = {
                "material_name": material_name,
                "operations": ops,
                "auto_recompile": auto_recompile,
            }
            batch_resp = unreal.send_command("execute_material_batch", batch_params)

            if not batch_resp:
                return {"success": False, "error": "No response from execute_material_batch"}

            # Unwrap the bridge envelope
            batch = batch_resp.get("result", batch_resp)

            return {
                "success": batch.get("success", False),
                "operations_executed": batch.get("success_count", 0),
                "operations_failed": batch.get("fail_count", 0),
                "fixes_applied": len(ops),
            }

        except Exception as exc:
            logger.error("fix_material_graph error: %s", exc, exc_info=True)
            return {"success": False, "error": str(exc)}

    # ------------------------------------------------------------------
    # build_material
    # ------------------------------------------------------------------

    @mcp.tool()
    def build_material(
        ctx: Context,
        material_name: str,
        nodes: List[Dict[str, Any]],
        connections: List[Dict[str, Any]],
        outputs: Optional[List[Dict[str, Any]]] = None,
        domain: str = "Surface",
        blend_mode: str = "Opaque",
        shading_model: str = "DefaultLit",
        two_sided: bool = False,
        auto_recompile: bool = True,
        include_preview: bool = False,
    ) -> Dict[str, Any]:
        """
        Build or overwrite a complete material in one atomic call.

        Creates the asset if it does not exist, sets all expression nodes,
        wires connections and output pins, then recompiles — all in a single
        TCP round trip. Use this instead of build_material_graph for new materials
        or full overwrites.

        Args:
            material_name: Full content path, e.g. "/Game/Materials/M_Rock".
            nodes: Expression nodes. Each dict: "name" (required), "type" (required),
                   optional "value", "color", "texture_path", "position" ([x, y]).
            connections: Interior wiring. Each dict: "from", "to", "input", "output" (default "").
            outputs: Connections to material result node. Each dict: "from", "property"
                     (e.g. "BaseColor", "Normal", "Roughness"), "output" (default "").
            domain: Material domain (Surface, UI, DeferredDecal, ...).
            blend_mode: Blend mode (Opaque, Masked, Translucent, ...).
            shading_model: Shading model (DefaultLit, Unlit, ...).
            two_sided: Enable two-sided rendering.
            auto_recompile: Recompile after building (default True).
            include_preview: Include base64 PNG preview thumbnail in response.

        Returns:
            Dict with keys: success, material_path, nodes_created, connections_made,
            outputs_wired, compile_errors. If include_preview=True, also preview_base64.
        """
        from unreal_mcp_server import get_unreal_connection
        from tools.material_graph_builder import MaterialGraphBuilder

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "error": "Failed to connect to Unreal Engine"}

            # Run Python-side layout computation
            builder = MaterialGraphBuilder()
            builder.load_material_spec(nodes, connections, outputs or [], [])
            builder.compute_layout()
            computed_nodes = builder.get_nodes_with_positions() if hasattr(builder, "get_nodes_with_positions") else nodes

            resp = unreal.send_command("build_material", {
                "material_name": material_name,
                "domain": domain,
                "blend_mode": blend_mode,
                "shading_model": shading_model,
                "two_sided": two_sided,
                "nodes": computed_nodes,
                "connections": connections,
                "outputs": outputs or [],
                "auto_recompile": auto_recompile,
                "include_preview": include_preview,
            })
            if not resp:
                return {"success": False, "error": "No response from build_material"}
            return resp.get("result", resp)

        except Exception as exc:
            logger.error("build_material error: %s", exc, exc_info=True)
            return {"success": False, "error": str(exc)}

    # ------------------------------------------------------------------
    # get_material_preview
    # ------------------------------------------------------------------

    @mcp.tool()
    def get_material_preview(
        ctx: Context,
        material_name: str,
        width: int = 256,
        height: int = 256,
    ) -> Dict[str, Any]:
        """
        Get a rendered preview thumbnail of a material as a base64-encoded PNG.

        Returns the material sphere preview thumbnail that Unreal generates
        after each recompile. Recompile the material first if the preview
        is out of date.

        Args:
            material_name: Material short name or full content path.
            width: Desired preview width in pixels (default 256).
            height: Desired preview height in pixels (default 256).

        Returns:
            Dict with keys: success, width, height, preview_base64 (PNG as base64 string).
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "error": "Failed to connect to Unreal Engine"}

            return unreal.send_command("get_material_preview", {
                "material_name": material_name,
                "width": width,
                "height": height,
            })

        except Exception as exc:
            logger.error("get_material_preview error: %s", exc, exc_info=True)
            return {"success": False, "error": str(exc)}
