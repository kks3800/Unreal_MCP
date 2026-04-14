"""
Material creation and editing tools for Unreal MCP.

Provides full material node graph creation via MCP commands:
- Create materials and material instances
- Add and connect material expression nodes
- Set material properties (domain, blend mode, shading model, translucency, usage, etc.)
- Create and use material functions (reusable shader building blocks)
"""

import logging
from mcp.server.fastmcp import FastMCP, Context
from typing import List, Optional, Dict, Any

# Get logger
logger = logging.getLogger("UnrealMCP")


def register_material_tools(mcp: FastMCP):
    """Register all material-related MCP tools."""

    @mcp.tool()
    def create_material(
        ctx: Context,
        material_name: str,
        path: str = "/Game/Materials",
        domain: str = "Surface",
        blend_mode: str = "Opaque",
        shading_model: str = "DefaultLit",
        two_sided: bool = False,
        properties: Optional[Dict[str, Any]] = None
    ) -> Dict[str, Any]:
        """
        Create a new Material asset with optional extended properties.

        Args:
            material_name: Name for the new material
            path: Content path (default: /Game/Materials)
            domain: Surface, DeferredDecal, LightFunction, Volume, PostProcess, UI
            blend_mode: Opaque, Masked, Translucent, Additive, Modulate, AlphaComposite, AlphaHoldout
            shading_model: Unlit, DefaultLit, Subsurface, PreintegratedSkin, SubsurfaceProfile,
                          ClearCoat, TwoSidedFoliage, Hair, Cloth, Eye, SingleLayerWater, ThinTranslucent
            two_sided: Enable two-sided rendering
            properties: Dict of additional properties to set. Keys include:
                Core: opacity_mask_clip_value, is_thin_surface, use_material_attributes,
                      cast_ray_traced_shadows, dithered_lod_transition, allow_negative_emissive_color,
                      tangent_space_normal
                Translucency: translucency_lighting_mode (VolumetricNonDirectional/VolumetricDirectional/
                      VolumetricPerVertexNonDirectional/VolumetricPerVertexDirectional/Surface/
                      SurfacePerPixelLighting), directional_lighting_intensity, screen_space_reflections,
                      contact_shadows, apply_fogging, apply_cloud_fogging, compute_fog_per_pixel,
                      output_translucent_velocity, disable_depth_test, write_only_alpha,
                      enable_responsive_aa, translucency_pass (BeforeDOF/AfterDOF/AfterMotionBlur)
                Translucency Self-Shadowing: translucent_shadow_density_scale,
                      translucent_self_shadow_density_scale, translucent_self_shadow_second_density_scale,
                      translucent_self_shadow_second_opacity, translucent_backscattering_exponent,
                      translucent_multiple_scattering_extinction ([R,G,B]), translucent_shadow_start_offset
                Refraction: refraction_mode (IndexOfRefraction/PixelNormalOffset/2DOffset/None),
                      refraction_coverage_mode (CoverageAccountedFor/CoverageIgnored)
                Post-Process: blendable_location (SceneColorAfterTonemapping/SceneColorAfterDOF/
                      SceneColorBeforeDOF/SceneColorBeforeBloom/ReplacingTonemapper/SSRInput),
                      blendable_priority (int), blendable_output_alpha, enable_stencil_test,
                      stencil_compare (Less/LessEqual/Greater/GreaterEqual/Equal/NotEqual/Never/Always),
                      stencil_ref_value (0-255)
                Usage: used_with_skeletal_mesh, used_with_particle_sprites, used_with_niagara_sprites,
                      used_with_niagara_mesh_particles, used_with_niagara_ribbons,
                      used_with_static_lighting, used_with_morph_targets, used_with_spline_meshes,
                      used_with_instanced_static_meshes, used_with_clothing, used_with_water,
                      used_with_hair_strands, used_with_nanite, used_with_volumetric_cloud,
                      automatically_set_usage_in_editor
                Mobile: fully_rough, use_lightmap_directionality, use_alpha_to_coverage
                Advanced: num_customized_uvs, use_emissive_for_dynamic_area_lighting,
                      cast_dynamic_shadow_as_masked

        Returns:
            Dict with success status and material path
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.warning("Failed to connect to Unreal Engine")
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            params = {
                "material_name": material_name,
                "path": path,
                "domain": domain,
                "blend_mode": blend_mode,
                "shading_model": shading_model,
                "two_sided": two_sided
            }

            # Merge extended properties into params
            if properties:
                params.update(properties)

            response = unreal.send_command("create_material", params)

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error creating material: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def set_material_properties(
        ctx: Context,
        material_name: str,
        domain: Optional[str] = None,
        blend_mode: Optional[str] = None,
        shading_model: Optional[str] = None,
        two_sided: Optional[bool] = None,
        wireframe: Optional[bool] = None,
        properties: Optional[Dict[str, Any]] = None
    ) -> Dict[str, Any]:
        """
        Set material properties. Supports 60+ properties via the properties dict.

        Args:
            material_name: Target material name or path
            domain: Surface, DeferredDecal, LightFunction, Volume, PostProcess, UI
            blend_mode: Opaque, Masked, Translucent, Additive, Modulate, AlphaComposite, AlphaHoldout
            shading_model: Unlit, DefaultLit, Subsurface, ClearCoat, SingleLayerWater, ThinTranslucent, etc.
            two_sided: Enable two-sided rendering
            wireframe: Enable wireframe mode
            properties: Dict of additional properties. Same keys as create_material properties param.
                Examples:
                  {"screen_space_reflections": True, "translucency_lighting_mode": "Surface"}
                  {"used_with_water": True, "apply_fogging": True}
                  {"blendable_location": "SceneColorAfterDOF", "blendable_priority": 1}
                  {"refraction_mode": "PixelNormalOffset", "opacity_mask_clip_value": 0.333}

        Returns:
            Dict with success status
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.warning("Failed to connect to Unreal Engine")
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            params = {"material_name": material_name}
            if domain is not None:
                params["domain"] = domain
            if blend_mode is not None:
                params["blend_mode"] = blend_mode
            if shading_model is not None:
                params["shading_model"] = shading_model
            if two_sided is not None:
                params["two_sided"] = two_sided
            if wireframe is not None:
                params["wireframe"] = wireframe

            # Merge extended properties into params
            if properties:
                params.update(properties)

            response = unreal.send_command("set_material_properties", params)

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error setting material properties: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def add_material_node(
        ctx: Context,
        material_name: str,
        node_type: str,
        node_name: Optional[str] = None,
        position: Optional[List[float]] = None,
        value: Optional[float] = None,
        color: Optional[List[float]] = None,
        texture_path: Optional[str] = None,
        code: Optional[str] = None,
        output_type: Optional[str] = None,
        inputs: Optional[List[str]] = None,
        function_name: Optional[str] = None,
        node_color: Optional[List[float]] = None,
        group: Optional[str] = None
    ) -> Dict[str, Any]:
        """
        Add a material expression node to a material.

        Args:
            material_name: Target material name or path
            node_type: Expression type - Constants: Constant, Constant2Vector, Constant3Vector, Constant4Vector;
                      Parameters: ScalarParameter, VectorParameter, TextureSampleParameter2D;
                      Textures: TextureSample, TextureObject, TextureCoordinate;
                      Math: Add, Subtract, Multiply, Divide, Lerp, Clamp, OneMinus, Power, Sqrt, Abs, Frac, Floor, Ceil, Min, Max;
                      Trig: Sine, Cosine;
                      Animation: Time, Panner, Rotator;
                      Effects: Fresnel, DepthFade, Desaturation;
                      World: WorldPosition, CameraPosition, VertexNormalWS, PixelNormalWS, ActorPosition;
                      Vector: DotProduct, CrossProduct, Normalize, ComponentMask, AppendVector;
                      Logic: If, StaticBool, StaticSwitch;
                      Vertex: VertexColor, ParticleColor;
                      Comment: Comment;
                      Custom: Custom (HLSL code);
                      Reroute: Reroute, NamedRerouteDeclaration, NamedRerouteUsage;
                      Function: MaterialFunctionCall, FunctionCall
            node_name: Name for the node (sets ParameterName for parameter nodes)
            position: [X, Y] position in graph
            value: Default value for Constant/ScalarParameter nodes
            color: [R, G, B, A] for vector nodes (0.0 to 1.0)
            texture_path: Texture asset path for TextureSample/TextureSampleParameter2D nodes
            code: HLSL code for Custom nodes
            output_type: Return type for Custom nodes (float, float2, float3, float4)
            inputs: List of input names for Custom nodes
            function_name: Description/function name for Custom nodes
            node_color: [R, G, B, A] color for NamedRerouteDeclaration nodes (0.0 to 1.0)
            group: Parameter group name for material instance organization (e.g. "Layer 1 - Grass")

        Returns:
            Dict with success status and node name
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.warning("Failed to connect to Unreal Engine")
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            params = {
                "material_name": material_name,
                "node_type": node_type
            }
            if node_name is not None:
                params["node_name"] = node_name
            if position is not None:
                params["position"] = position
            if value is not None:
                params["value"] = value
            if color is not None:
                params["color"] = color
            if texture_path is not None:
                params["texture_path"] = texture_path
            if code is not None:
                params["code"] = code
            if output_type is not None:
                params["output_type"] = output_type
            if inputs is not None:
                params["inputs"] = inputs
            if function_name is not None:
                params["function_name"] = function_name
            if node_color is not None:
                params["node_color"] = node_color
            if group is not None:
                params["group"] = group

            response = unreal.send_command("add_material_node", params)

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error adding material node: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def set_material_node_property(
        ctx: Context,
        material_name: str,
        node_name: str,
        value: Optional[float] = None,
        color: Optional[List[float]] = None,
        sampler_type: Optional[str] = None
    ) -> Dict[str, Any]:
        """
        Set properties on an existing material expression node.

        Args:
            material_name: Target material name or path
            node_name: Name of the node to modify
            value: Scalar value (for Constant/ScalarParameter)
            color: [R, G, B, A] color value (for vector nodes)
            sampler_type: Texture sampler type (Color, Normal, LinearColor, Masks, Grayscale, Alpha, Data)

        Returns:
            Dict with success status
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.warning("Failed to connect to Unreal Engine")
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            params = {
                "material_name": material_name,
                "node_name": node_name
            }
            if value is not None:
                params["value"] = value
            if color is not None:
                params["color"] = color
            if sampler_type is not None:
                params["sampler_type"] = sampler_type

            response = unreal.send_command("set_material_node_property", params)

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error setting material node property: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def connect_material_nodes(
        ctx: Context,
        material_name: str,
        from_node: str,
        to_node: str,
        to_input: str,
        from_output: str = ""
    ) -> Dict[str, Any]:
        """
        Connect two material expression nodes.

        Args:
            material_name: Target material name or path
            from_node: Source node name
            to_node: Destination node name
            to_input: Input pin name on destination (A, B, Alpha, Coordinates, Base, Exponent, etc.)
            from_output: Output pin name on source (default: first output)

        Returns:
            Dict with success status
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.warning("Failed to connect to Unreal Engine")
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("connect_material_nodes", {
                "material_name": material_name,
                "from_node": from_node,
                "to_node": to_node,
                "to_input": to_input,
                "from_output": from_output
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error connecting material nodes: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def connect_to_material_output(
        ctx: Context,
        material_name: str,
        from_node: str,
        material_property: str,
        from_output: str = ""
    ) -> Dict[str, Any]:
        """
        Connect a material expression to a material output property.

        Args:
            material_name: Target material name or path
            from_node: Source node name
            material_property: Output property - BaseColor, Metallic, Specular, Roughness, Anisotropy,
                             Normal, Tangent, EmissiveColor, Opacity, OpacityMask,
                             WorldPositionOffset, SubsurfaceColor, AmbientOcclusion, Refraction
            from_output: Output pin name on source (default: first output)

        Returns:
            Dict with success status
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.warning("Failed to connect to Unreal Engine")
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("connect_to_material_output", {
                "material_name": material_name,
                "from_node": from_node,
                "material_property": material_property,
                "from_output": from_output
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error connecting to material output: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def delete_material_node(
        ctx: Context,
        material_name: str,
        node_name: str
    ) -> Dict[str, Any]:
        """
        Delete a material expression node.

        Args:
            material_name: Target material name or path
            node_name: Name of node to delete

        Returns:
            Dict with success status
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.warning("Failed to connect to Unreal Engine")
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("delete_material_node", {
                "material_name": material_name,
                "node_name": node_name
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error deleting material node: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def recompile_material(
        ctx: Context,
        material_name: str
    ) -> Dict[str, Any]:
        """
        Recompile a material after making changes.

        Args:
            material_name: Target material name or path

        Returns:
            Dict with success status
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.warning("Failed to connect to Unreal Engine")
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("recompile_material", {
                "material_name": material_name
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error recompiling material: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def create_material_instance(
        ctx: Context,
        instance_name: str,
        parent_material: str,
        path: str = "/Game/Materials"
    ) -> Dict[str, Any]:
        """
        Create a Material Instance Constant from a parent material.

        Args:
            instance_name: Name for the new material instance
            parent_material: Parent material name or path
            path: Content path (default: /Game/Materials)

        Returns:
            Dict with success status and instance path
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.warning("Failed to connect to Unreal Engine")
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("create_material_instance", {
                "instance_name": instance_name,
                "parent_material": parent_material,
                "path": path
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error creating material instance: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def set_material_instance_parameter(
        ctx: Context,
        instance_name: str,
        parameter_name: str,
        parameter_type: str,
        value
    ) -> Dict[str, Any]:
        """
        Set a parameter value on a Material Instance.

        Args:
            instance_name: Target material instance name or path
            parameter_name: Name of the parameter
            parameter_type: Scalar, Vector, or Texture
            value: For Scalar: float; For Vector: [R,G,B,A]; For Texture: texture asset path

        Returns:
            Dict with success status
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.warning("Failed to connect to Unreal Engine")
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("set_material_instance_parameter", {
                "instance_name": instance_name,
                "parameter_name": parameter_name,
                "parameter_type": parameter_type,
                "value": value
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error setting material instance parameter: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def get_material_instance_parameters(
        ctx: Context,
        instance_name: str
    ) -> Dict[str, Any]:
        """
        Get all overridden parameter values from a Material Instance.

        Args:
            instance_name: Material instance name or path

        Returns:
            Dict with scalar_parameters, vector_parameters, texture_parameters arrays
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.warning("Failed to connect to Unreal Engine")
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("get_material_instance_parameters", {
                "instance_name": instance_name
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error getting material instance parameters: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def get_material_nodes(
        ctx: Context,
        material_name: str
    ) -> Dict[str, Any]:
        """
        Get all expression nodes in a material.

        Args:
            material_name: Target material name or path

        Returns:
            Dict with success status and list of nodes
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.warning("Failed to connect to Unreal Engine")
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("get_material_nodes", {
                "material_name": material_name
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error getting material nodes: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def layout_material_nodes(
        ctx: Context,
        material_name: str
    ) -> Dict[str, Any]:
        """
        Auto-layout material expression nodes.

        Args:
            material_name: Target material name or path

        Returns:
            Dict with success status
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.warning("Failed to connect to Unreal Engine")
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("layout_material_nodes", {
                "material_name": material_name
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error laying out material nodes: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def link_named_reroute_usage(
        ctx: Context,
        material_name: str,
        usage_node: str,
        declaration_node: str
    ) -> Dict[str, Any]:
        """
        Link a NamedRerouteUsage node to a NamedRerouteDeclaration node.

        Args:
            material_name: Target material name or path
            usage_node: Name of the NamedRerouteUsage node to link
            declaration_node: Name of the NamedRerouteDeclaration to link to

        Returns:
            Dict with success status
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.warning("Failed to connect to Unreal Engine")
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("link_named_reroute_usage", {
                "material_name": material_name,
                "usage_node": usage_node,
                "declaration_node": declaration_node
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error linking named reroute usage: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def get_material_hierarchy(
        ctx: Context,
        material_name: str
    ) -> Dict[str, Any]:
        """
        Get complete material graph hierarchy with all connections.

        Returns the full node graph including:
        - All expression nodes with their inputs and outputs
        - All node-to-node connections (which output connects to which input)
        - Material output connections (EmissiveColor, Opacity, BaseColor, etc.)
        - Connection status for each input (connected or disconnected)

        Args:
            material_name: Target material name or path

        Returns:
            Dict with nodes, connections, and material_outputs arrays
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.warning("Failed to connect to Unreal Engine")
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("get_material_hierarchy", {
                "material_name": material_name
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error getting material hierarchy: {e}")
            return {"status": "error", "error": str(e)}

    # =========================================================================
    # Material Function Tools
    # =========================================================================

    @mcp.tool()
    def create_material_function(
        ctx: Context,
        function_name: str,
        path: str = "/Game/Materials/Functions",
        description: str = "",
        expose_to_library: bool = True,
        library_categories: Optional[List[str]] = None
    ) -> Dict[str, Any]:
        """
        Create a new Material Function asset (reusable shader building block).

        Args:
            function_name: Name for the new function (e.g. MF_WaterRipple)
            path: Content path (default: /Game/Materials/Functions)
            description: Description shown as tooltip in the material editor
            expose_to_library: Whether to show in the material function library browser
            library_categories: List of category strings for library organization (e.g. ["Water", "Effects"])

        Returns:
            Dict with success status and function path
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.warning("Failed to connect to Unreal Engine")
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            params = {
                "function_name": function_name,
                "path": path,
                "description": description,
                "expose_to_library": expose_to_library
            }
            if library_categories is not None:
                params["library_categories"] = library_categories

            response = unreal.send_command("create_material_function", params)

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error creating material function: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def add_function_input(
        ctx: Context,
        function_name: str,
        input_name: str,
        input_type: str = "Scalar",
        sort_priority: int = 0,
        description: str = "",
        use_preview_value_as_default: bool = False,
        preview_value: Optional[Any] = None,
        position: Optional[List[float]] = None
    ) -> Dict[str, Any]:
        """
        Add a typed input pin to a material function.

        Args:
            function_name: Material function name or path
            input_name: Name for the input pin (e.g. "UVs", "BaseColor", "Strength")
            input_type: Scalar, Vector2, Vector3, Vector4, Texture2D, TextureCube,
                       Texture2DArray, VolumeTexture, StaticBool, MaterialAttributes, TextureExternal, Bool
            sort_priority: Controls pin order (lower = earlier)
            description: Tooltip text for this input
            use_preview_value_as_default: Use preview value when input is unconnected
            preview_value: Default value - float for Scalar, or [X,Y,Z,W] array for vectors
            position: [X, Y] position in graph

        Returns:
            Dict with success status
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.warning("Failed to connect to Unreal Engine")
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            params = {
                "function_name": function_name,
                "input_name": input_name,
                "input_type": input_type,
                "sort_priority": sort_priority,
                "description": description,
                "use_preview_value_as_default": use_preview_value_as_default
            }
            if preview_value is not None:
                params["preview_value"] = preview_value
            if position is not None:
                params["position"] = position

            response = unreal.send_command("add_function_input", params)

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error adding function input: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def add_function_output(
        ctx: Context,
        function_name: str,
        output_name: str,
        sort_priority: int = 0,
        description: str = "",
        position: Optional[List[float]] = None
    ) -> Dict[str, Any]:
        """
        Add an output pin to a material function.

        Args:
            function_name: Material function name or path
            output_name: Name for the output pin (e.g. "Result", "Normal", "Mask")
            sort_priority: Controls pin order (lower = earlier)
            description: Tooltip text for this output
            position: [X, Y] position in graph

        Returns:
            Dict with success status
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.warning("Failed to connect to Unreal Engine")
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            params = {
                "function_name": function_name,
                "output_name": output_name,
                "sort_priority": sort_priority,
                "description": description
            }
            if position is not None:
                params["position"] = position

            response = unreal.send_command("add_function_output", params)

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error adding function output: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def add_function_node(
        ctx: Context,
        function_name: str,
        node_type: str,
        node_name: Optional[str] = None,
        position: Optional[List[float]] = None,
        value: Optional[float] = None,
        color: Optional[List[float]] = None,
        texture_path: Optional[str] = None,
        group: Optional[str] = None
    ) -> Dict[str, Any]:
        """
        Add an expression node inside a material function (same node types as add_material_node).

        Args:
            function_name: Material function name or path
            node_type: Expression type (same as add_material_node: Constant, Multiply, Add, etc.)
            node_name: Name/description for the node
            position: [X, Y] position in graph
            value: Default value for Constant/ScalarParameter
            color: [R, G, B, A] for vector nodes
            texture_path: Texture path for TextureSample
            group: Parameter group name for material instance organization

        Returns:
            Dict with success status and node name
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.warning("Failed to connect to Unreal Engine")
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            params = {
                "function_name": function_name,
                "node_type": node_type
            }
            if node_name is not None:
                params["node_name"] = node_name
            if position is not None:
                params["position"] = position
            if value is not None:
                params["value"] = value
            if color is not None:
                params["color"] = color
            if texture_path is not None:
                params["texture_path"] = texture_path
            if group is not None:
                params["group"] = group

            response = unreal.send_command("add_function_node", params)

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error adding function node: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def connect_function_nodes(
        ctx: Context,
        function_name: str,
        from_node: str,
        to_node: str,
        to_input: str,
        from_output: str = ""
    ) -> Dict[str, Any]:
        """
        Connect two nodes inside a material function.

        Args:
            function_name: Material function name or path
            from_node: Source node name
            to_node: Destination node name
            to_input: Input pin name on destination (A, B, Alpha, etc.)
            from_output: Output pin name on source (default: first output)

        Returns:
            Dict with success status
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.warning("Failed to connect to Unreal Engine")
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("connect_function_nodes", {
                "function_name": function_name,
                "from_node": from_node,
                "to_node": to_node,
                "to_input": to_input,
                "from_output": from_output
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error connecting function nodes: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def add_material_function_call(
        ctx: Context,
        material_name: str,
        function_path: str,
        node_name: Optional[str] = None,
        position: Optional[List[float]] = None
    ) -> Dict[str, Any]:
        """
        Add a MaterialFunctionCall node to a material that references an existing function.

        The response includes the function's input and output pin names so you can
        connect them using connect_material_nodes.

        Args:
            material_name: Target material name or path
            function_path: Path to the material function to call (e.g. /Game/Materials/Functions/MF_WaterRipple)
            node_name: Name/description for the function call node
            position: [X, Y] position in graph

        Returns:
            Dict with success status, node_name, and lists of input/output pins
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.warning("Failed to connect to Unreal Engine")
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            params = {
                "material_name": material_name,
                "function_path": function_path
            }
            if node_name is not None:
                params["node_name"] = node_name
            if position is not None:
                params["position"] = position

            response = unreal.send_command("add_material_function_call", params)

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error adding material function call: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def set_material_node_position(
        ctx: Context,
        material_name: str,
        nodes: Optional[List[Dict[str, Any]]] = None,
        node_name: Optional[str] = None,
        x: int = 0,
        y: int = 0
    ) -> Dict[str, Any]:
        """
        Set the position of one or more material expression nodes in the graph.

        Supports two modes:
        1. Single node: pass node_name, x, y
        2. Batch: pass nodes as a list of {"name": "NodeName", "x": 100, "y": 200}

        Batch mode is strongly preferred for repositioning multiple nodes (avoids N round-trips).

        Args:
            material_name: Target material name or path
            nodes: List of {name, x, y} dicts for batch repositioning
            node_name: Single node name (used when nodes is None)
            x: X position for single node mode
            y: Y position for single node mode

        Returns:
            Dict with success status and moved_count (batch) or node position (single)
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.warning("Failed to connect to Unreal Engine")
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            params = {"material_name": material_name}
            if nodes is not None:
                params["nodes"] = nodes
            else:
                if node_name is None:
                    return {"status": "error", "error": "Must provide either nodes array or node_name"}
                params["node_name"] = node_name
                params["x"] = x
                params["y"] = y

            response = unreal.send_command("set_material_node_position", params)

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error setting material node position: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def get_material_function_info(
        ctx: Context,
        function_name: str
    ) -> Dict[str, Any]:
        """
        Get detailed info about a material function including inputs, outputs, and nodes.

        Args:
            function_name: Material function name or path

        Returns:
            Dict with name, description, inputs (with types), outputs, nodes,
            expose_to_library flag, and library_categories
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.warning("Failed to connect to Unreal Engine")
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("get_material_function_info", {
                "function_name": function_name
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error getting material function info: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def execute_material_batch(
        ctx: Context,
        material_name: str,
        operations: List[Dict[str, Any]],
        auto_recompile: bool = True,
        auto_save: bool = False
    ) -> Dict[str, Any]:
        """
        Execute multiple material operations in a single call.

        Operations are executed sequentially. Use $N.field syntax to reference
        results from previous operations (zero-indexed). For example, $0.node_name
        references the node_name from operation 0's result.

        Args:
            material_name: Target material name or path
            operations: Array of operations, each with an "op" field:
                       [
                         {"op": "add_material_node", "node_type": "TextureCoordinate", "node_name": "TexCoord", "position": [0, 0]},
                         {"op": "add_material_node", "node_type": "Multiply", "node_name": "Mul_UV", "position": [200, 0]},
                         {"op": "connect_material_nodes", "from_node": "TexCoord", "to_node": "Mul_UV", "to_input": "A"}
                       ]
            auto_recompile: Recompile material after all operations (default: True)
            auto_save: Save the material after operations (default: False)

        Returns:
            Dict with success, success_count, fail_count, total_operations, and results list
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "error": "Failed to connect to Unreal Engine"}

            params = {
                "material_name": material_name,
                "operations": operations,
                "auto_recompile": auto_recompile,
                "auto_save": auto_save
            }

            response = unreal.send_command("execute_material_batch", params)
            if not response:
                return {"success": False, "error": "No response from Unreal Engine"}
            return response

        except Exception as e:
            logger.error(f"Error executing material batch: {e}")
            return {"success": False, "error": f"Error executing material batch: {e}"}

    @mcp.tool()
    def execute_function_batch(
        ctx: Context,
        function_name: str,
        operations: List[Dict[str, Any]],
        auto_save: bool = False
    ) -> Dict[str, Any]:
        """
        Execute multiple material function operations in a single call.

        Operations are executed sequentially. Use $N.field syntax to reference
        results from previous operations (zero-indexed).

        Args:
            function_name: Target material function name or path
            operations: Array of operations, each with an "op" field:
                       [
                         {"op": "add_function_input", "input_name": "BaseColor", "input_type": "Texture2D"},
                         {"op": "add_function_node", "node_type": "TextureSample", "node_name": "BC_Sample"},
                         {"op": "connect_function_nodes", "from_node": "BaseColor", "to_node": "BC_Sample", "to_input": "Tex"}
                       ]
            auto_save: Save the function after operations (default: False)

        Returns:
            Dict with success, success_count, fail_count, total_operations, and results list
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "error": "Failed to connect to Unreal Engine"}

            params = {
                "function_name": function_name,
                "operations": operations,
                "auto_save": auto_save
            }

            response = unreal.send_command("execute_function_batch", params)
            if not response:
                return {"success": False, "error": "No response from Unreal Engine"}
            return response

        except Exception as e:
            logger.error(f"Error executing function batch: {e}")
            return {"success": False, "error": f"Error executing function batch: {e}"}
