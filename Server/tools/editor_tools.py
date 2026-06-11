"""
Editor Tools for Unreal MCP.

This module provides tools for controlling the Unreal Editor viewport and other editor functionality.
"""

import logging
from typing import Dict, List, Any, Optional
from mcp.server.fastmcp import FastMCP, Context

# Get logger
logger = logging.getLogger("UnrealMCP")

def register_editor_tools(mcp: FastMCP):
    """Register editor tools with the MCP server."""
    
    @mcp.tool()
    def get_actors_in_level(ctx: Context) -> List[Dict[str, Any]]:
        """Get a list of all actors in the current level."""
        from unreal_mcp_server import get_unreal_connection
        
        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.warning("Failed to connect to Unreal Engine")
                return []
                
            response = unreal.send_command("get_actors_in_level", {})
            
            if not response:
                logger.warning("No response from Unreal Engine")
                return []
                
            # Log the complete response for debugging
            logger.info(f"Complete response from Unreal: {response}")
            
            # Check response format
            if "result" in response and "actors" in response["result"]:
                actors = response["result"]["actors"]
                logger.info(f"Found {len(actors)} actors in level")
                return actors
            elif "actors" in response:
                actors = response["actors"]
                logger.info(f"Found {len(actors)} actors in level")
                return actors
                
            logger.warning(f"Unexpected response format: {response}")
            return []
            
        except Exception as e:
            logger.error(f"Error getting actors: {e}")
            return []

    @mcp.tool()
    def find_actors_by_name(ctx: Context, pattern: str) -> List[str]:
        """Find actors by name pattern."""
        from unreal_mcp_server import get_unreal_connection
        
        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.warning("Failed to connect to Unreal Engine")
                return []
                
            response = unreal.send_command("find_actors_by_name", {
                "pattern": pattern
            })
            
            if not response:
                return []
                
            return response.get("actors", [])
            
        except Exception as e:
            logger.error(f"Error finding actors: {e}")
            return []
    
    @mcp.tool()
    def spawn_actor(
        ctx: Context,
        name: str,
        type: str,
        location: List[float] = [0.0, 0.0, 0.0],
        rotation: List[float] = [0.0, 0.0, 0.0],
        static_mesh: str = None,
        intensity: float = None,
        color: List[float] = None,
        temperature: float = None,
        attenuation_radius: float = None,
        source_radius: float = None,
        soft_source_radius: float = None,
        cast_shadows: bool = None,
        inner_cone_angle: float = None,
        outer_cone_angle: float = None,
        source_width: float = None,
        source_height: float = None,
        barn_door_angle: float = None,
        barn_door_length: float = None,
        scale: List[float] = None
    ) -> Dict[str, Any]:
        """Create a new actor in the current level.

        Args:
            ctx: The MCP context
            name: The name to give the new actor (must be unique)
            type: The type of actor to create (e.g. StaticMeshActor, PointLight, SpotLight, RectLight, DirectionalLight, CameraActor)
            location: The [x, y, z] world location to spawn at
            rotation: The [pitch, yaw, roll] rotation in degrees
            static_mesh: Asset path for StaticMeshActor (default: /Engine/BasicShapes/Cube.Cube)
            intensity: Light intensity in candelas
            color: Light color as [r, g, b] (0-1 range)
            temperature: Color temperature in Kelvin (enables temperature mode)
            attenuation_radius: Light falloff radius
            source_radius: Soft shadow source radius (Point/SpotLight)
            soft_source_radius: Additional soft shadow radius (Point/SpotLight)
            cast_shadows: Whether the light casts shadows
            inner_cone_angle: SpotLight inner cone angle in degrees
            outer_cone_angle: SpotLight outer cone angle in degrees
            source_width: RectLight width in cm
            source_height: RectLight height in cm
            barn_door_angle: RectLight barn door angle
            barn_door_length: RectLight barn door length
            scale: Actor scale as [x, y, z]

        Returns:
            Dict containing the created actor's properties
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            # Normalize common type aliases
            type_map = {
                "STATICMESHACTOR": "StaticMeshActor",
                "POINTLIGHT": "PointLight",
                "SPOTLIGHT": "SpotLight",
                "RECTLIGHT": "RectLight",
                "DIRECTIONALLIGHT": "DirectionalLight",
                "CAMERAACTOR": "CameraActor",
            }
            actor_type = type_map.get(type.upper(), type)

            # Ensure all parameters are properly formatted
            params = {
                "name": name,
                "type": actor_type,
                "location": location,
                "rotation": rotation
            }

            # Forward optional params
            if static_mesh is not None:
                params["static_mesh"] = static_mesh
            if intensity is not None:
                params["intensity"] = intensity
            if color is not None:
                params["color"] = color
            if temperature is not None:
                params["temperature"] = temperature
            if attenuation_radius is not None:
                params["attenuation_radius"] = attenuation_radius
            if source_radius is not None:
                params["source_radius"] = source_radius
            if soft_source_radius is not None:
                params["soft_source_radius"] = soft_source_radius
            if cast_shadows is not None:
                params["cast_shadows"] = cast_shadows
            if inner_cone_angle is not None:
                params["inner_cone_angle"] = inner_cone_angle
            if outer_cone_angle is not None:
                params["outer_cone_angle"] = outer_cone_angle
            if source_width is not None:
                params["source_width"] = source_width
            if source_height is not None:
                params["source_height"] = source_height
            if barn_door_angle is not None:
                params["barn_door_angle"] = barn_door_angle
            if barn_door_length is not None:
                params["barn_door_length"] = barn_door_length
            if scale is not None:
                params["scale"] = scale
            
            # Validate location and rotation formats
            for param_name in ["location", "rotation"]:
                param_value = params[param_name]
                if not isinstance(param_value, list) or len(param_value) != 3:
                    logger.error(f"Invalid {param_name} format: {param_value}. Must be a list of 3 float values.")
                    return {"success": False, "message": f"Invalid {param_name} format. Must be a list of 3 float values."}
                # Ensure all values are float
                params[param_name] = [float(val) for val in param_value]
            
            logger.info(f"Creating actor '{name}' of type '{type}' with params: {params}")
            response = unreal.send_command("spawn_actor", params)
            
            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}
            
            # Log the complete response for debugging
            logger.info(f"Actor creation response: {response}")
            
            # Handle error responses correctly
            if response.get("status") == "error":
                error_message = response.get("error", "Unknown error")
                logger.error(f"Error creating actor: {error_message}")
                return {"success": False, "message": error_message}
            
            return response
            
        except Exception as e:
            error_msg = f"Error creating actor: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}
    
    @mcp.tool()
    def delete_actor(ctx: Context, name: str) -> Dict[str, Any]:
        """Delete an actor by name."""
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("delete_actor", {
                "name": name
            })
            return response or {}

        except Exception as e:
            logger.error(f"Error deleting actor: {e}")
            return {}

    @mcp.tool()
    def list_static_mesh_actors(
        ctx: Context,
        persistent_only: bool = True,
        folder_filter: str = "",
    ) -> Dict[str, Any]:
        """Enumerate StaticMeshActors in the current world with their mesh asset path and folder.

        Useful for categorizing actors by what mesh they reference (e.g. moving every
        SMA that uses /Game/.../SM_Couch_* into a "Props/Seating" folder).

        Args:
            persistent_only: Only return actors that live in the persistent level
                (skip level-instance content). Default True.
            folder_filter: If non-empty, only return actors whose current Folder Path
                equals this value exactly (e.g. "Geometry/Static"). Default "" (no filter).

        Returns:
            {"count": int, "actors": [{"name", "folder_path", "mesh_path", "mesh_name", "location": [x,y,z]}, ...]}
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            params = {"persistent_only": persistent_only}
            if folder_filter:
                params["folder_filter"] = folder_filter
            response = unreal.send_command("list_static_mesh_actors", params)
            return response or {}
        except Exception as e:
            logger.error(f"Error in list_static_mesh_actors: {e}")
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def set_actor_folders(
        ctx: Context,
        actor_to_folder: Dict[str, str],
        save: bool = True,
    ) -> Dict[str, Any]:
        """Set Outliner folder paths on persistent-level actors by name.

        Args:
            actor_to_folder: Dict mapping actor name (as shown in Outliner / GetName())
                to target folder path. Folders can be nested with "/".
            save: Save the persistent level afterwards. Default True.

        Returns:
            {"changed": int, "not_found": int, "missing_actor_names": [...], "saved": bool}
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("set_actor_folders", {
                "actor_to_folder": actor_to_folder,
                "save": save,
            })
            return response or {}
        except Exception as e:
            logger.error(f"Error in set_actor_folders: {e}")
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def organize_outliner_by_class(
        ctx: Context,
        class_to_folder: Dict[str, str],
        only_if_empty: bool = True,
        skip_level_instance_content: bool = True,
        match_parent_classes: bool = True,
        save: bool = True,
    ) -> Dict[str, Any]:
        """Bulk-assign Outliner folder paths to persistent-level actors by class.

        Iterates every actor in the current persistent level, looks up its class name
        in ``class_to_folder``, and sets the outliner Folder Path to the mapped value.
        Optionally walks the class chain so subclasses inherit a folder mapping
        (e.g. ``"Light"`` matches PointLight, SpotLight, etc.).

        Args:
            class_to_folder: Dict like {"StaticMeshActor": "Geometry",
                "DecalActor": "Decals", "LevelInstance": "Houses", ...}.
                Folder paths can be nested with ``/`` (e.g. ``"FX/Niagara"``).
            only_if_empty: Only set folder on actors currently at the root
                (folder path empty or "None"). Default True — preserves existing
                organization.
            skip_level_instance_content: Skip actors that live inside a loaded
                level instance (i.e. not in the persistent level). Default True
                to avoid touching sublevel-owned actors.
            match_parent_classes: If exact class name isn't in the mapping,
                walk up the class chain. Default True.
            save: Save the persistent level after re-foldering. Default True.

        Returns:
            {
              "total_changed": int,
              "skipped_level_instance_content": int,
              "skipped_already_in_folder": int,
              "skipped_no_class_match": int,
              "saved": bool,
              "by_folder": {"FolderName": count, ...}
            }
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            params = {
                "class_to_folder": class_to_folder,
                "only_if_empty": only_if_empty,
                "skip_level_instance_content": skip_level_instance_content,
                "match_parent_classes": match_parent_classes,
                "save": save,
            }
            response = unreal.send_command("organize_outliner_by_class", params)
            return response or {}

        except Exception as e:
            logger.error(f"Error in organize_outliner_by_class: {e}")
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def fix_null_static_mesh_actors(
        ctx: Context,
        map_paths: List[str] = None,
        include_persistent: bool = True,
        save: bool = True,
        restore_original: bool = True,
    ) -> Dict[str, Any]:
        """Sweep persistent + given sublevels and delete every StaticMeshActor with a NULL StaticMesh, saving each modified map.

        Walks the current persistent level first (when include_persistent=True), then loads each
        sublevel path in turn, scans for StaticMeshActor with no StaticMesh assigned, destroys them,
        and saves the map. Finally restores the originally open map when restore_original=True.

        Use this to clean Map Check warnings of the form
        "StaticMeshActor_NNN Static mesh actor has NULL StaticMesh property" without manually
        switching maps in the editor.

        Args:
            map_paths: Long package paths of sublevel maps to fix
                (e.g. ["/Game/ModernMansions/Maps/Buildings/House01", ...]).
                If None or empty, only the persistent level is processed (when include_persistent).
            include_persistent: Also fix the currently loaded persistent level. Default True.
            save: Save each map after deletion. Default True.
            restore_original: After processing, reload the original persistent map. Default True.

        Returns:
            {
              "total_deleted": int,
              "original_map": "/Game/...",
              "per_map": [
                {"map_path": ..., "deleted_count": int, "saved": bool, "deleted_names": [...], "error": "..."}
              ]
            }
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            params: Dict[str, Any] = {
                "include_persistent": include_persistent,
                "save": save,
                "restore_original": restore_original,
            }
            if map_paths:
                params["map_paths"] = map_paths

            response = unreal.send_command("fix_null_static_mesh_actors", params)
            return response or {}

        except Exception as e:
            logger.error(f"Error in fix_null_static_mesh_actors: {e}")
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def set_actor_transform(
        ctx: Context,
        name: str,
        location: List[float]  = None,
        rotation: List[float]  = None,
        scale: List[float] = None
    ) -> Dict[str, Any]:
        """Set the transform of an actor."""
        from unreal_mcp_server import get_unreal_connection
        
        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
                
            params = {"name": name}
            if location is not None:
                params["location"] = location
            if rotation is not None:
                params["rotation"] = rotation
            if scale is not None:
                params["scale"] = scale
                
            response = unreal.send_command("set_actor_transform", params)
            return response or {}
            
        except Exception as e:
            logger.error(f"Error setting transform: {e}")
            return {}
    
    @mcp.tool()
    def get_actor_properties(ctx: Context, name: str) -> Dict[str, Any]:
        """Get all properties of an actor."""
        from unreal_mcp_server import get_unreal_connection
        
        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
                
            response = unreal.send_command("get_actor_properties", {
                "name": name
            })
            return response or {}
            
        except Exception as e:
            logger.error(f"Error getting properties: {e}")
            return {}

    @mcp.tool()
    def set_actor_property(
        ctx: Context,
        name: str,
        property_name: str,
        property_value,
    ) -> Dict[str, Any]:
        """
        Set a property on an actor.
        
        Args:
            name: Name of the actor
            property_name: Name of the property to set
            property_value: Value to set the property to
            
        Returns:
            Dict containing response from Unreal with operation status
        """
        from unreal_mcp_server import get_unreal_connection
        
        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
                
            response = unreal.send_command("set_actor_property", {
                "name": name,
                "property_name": property_name,
                "property_value": property_value
            })
            
            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}
            
            logger.info(f"Set actor property response: {response}")
            return response
            
        except Exception as e:
            error_msg = f"Error setting actor property: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    # @mcp.tool() commented out because it's buggy
    def focus_viewport(
        ctx: Context,
        target: str = None,
        location: List[float] = None,
        distance: float = 1000.0,
        orientation: List[float] = None
    ) -> Dict[str, Any]:
        """
        Focus the viewport on a specific actor or location.
        
        Args:
            target: Name of the actor to focus on (if provided, location is ignored)
            location: [X, Y, Z] coordinates to focus on (used if target is None)
            distance: Distance from the target/location
            orientation: Optional [Pitch, Yaw, Roll] for the viewport camera
            
        Returns:
            Response from Unreal Engine
        """
        from unreal_mcp_server import get_unreal_connection
        
        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
                
            params = {}
            if target:
                params["target"] = target
            elif location:
                params["location"] = location
            
            if distance:
                params["distance"] = distance
                
            if orientation:
                params["orientation"] = orientation
                
            response = unreal.send_command("focus_viewport", params)
            return response or {}
            
        except Exception as e:
            logger.error(f"Error focusing viewport: {e}")
            return {"status": "error", "message": str(e)}

    @mcp.tool()
    def spawn_blueprint_actor(
        ctx: Context,
        blueprint_name: str,
        actor_name: str,
        location: List[float] = [0.0, 0.0, 0.0],
        rotation: List[float] = [0.0, 0.0, 0.0]
    ) -> Dict[str, Any]:
        """Spawn an actor from a Blueprint.
        
        Args:
            ctx: The MCP context
            blueprint_name: Name of the Blueprint to spawn from
            actor_name: Name to give the spawned actor
            location: The [x, y, z] world location to spawn at
            rotation: The [pitch, yaw, roll] rotation in degrees
            
        Returns:
            Dict containing the spawned actor's properties
        """
        from unreal_mcp_server import get_unreal_connection
        
        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            
            # Ensure all parameters are properly formatted
            params = {
                "blueprint_name": blueprint_name,
                "actor_name": actor_name,
                "location": location or [0.0, 0.0, 0.0],
                "rotation": rotation or [0.0, 0.0, 0.0]
            }
            
            # Validate location and rotation formats
            for param_name in ["location", "rotation"]:
                param_value = params[param_name]
                if not isinstance(param_value, list) or len(param_value) != 3:
                    logger.error(f"Invalid {param_name} format: {param_value}. Must be a list of 3 float values.")
                    return {"success": False, "message": f"Invalid {param_name} format. Must be a list of 3 float values."}
                # Ensure all values are float
                params[param_name] = [float(val) for val in param_value]
            
            logger.info(f"Spawning blueprint actor with params: {params}")
            response = unreal.send_command("spawn_blueprint_actor", params)
            
            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}
            
            logger.info(f"Spawn blueprint actor response: {response}")
            return response
            
        except Exception as e:
            error_msg = f"Error spawning blueprint actor: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @mcp.tool()
    def get_editor_context(ctx: Context) -> Dict[str, Any]:
        """
        Get the current Unreal Editor context in one call.
        Returns selected actors, open asset editors, active tab, current level, and PIE state.
        Use this before any context-sensitive operation to understand what the user is looking at.
        """
        from unreal_mcp_server import get_unreal_connection
        unreal = get_unreal_connection()
        if not unreal:
            return {"success": False, "error": "Not connected to Unreal Engine"}
        return unreal.send_command("get_editor_context", {})

    @mcp.tool()
    def get_viewport_camera(ctx: Context) -> Dict[str, Any]:
        """
        Get the active Level Editor viewport camera position, rotation, FOV and view mode.
        """
        from unreal_mcp_server import get_unreal_connection
        unreal = get_unreal_connection()
        if not unreal:
            return {"success": False, "error": "Not connected to Unreal Engine"}
        return unreal.send_command("get_viewport_camera", {})

    @mcp.tool()
    def take_editor_screenshot(ctx: Context) -> Dict[str, Any]:
        """
        Capture the active Level Editor viewport as a base64-encoded PNG image.
        Returns: width, height, image_base64 (PNG). Editor must be open with a visible 3D viewport.
        Does NOT require PIE — captures the editor scene directly.
        """
        from unreal_mcp_server import get_unreal_connection
        unreal = get_unreal_connection()
        if not unreal:
            return {"success": False, "error": "Not connected to Unreal Engine"}
        return unreal.send_command("take_editor_screenshot", {})

    @mcp.tool()
    def get_cvar(ctx: Context, name: str) -> Dict[str, Any]:
        """
        Get the current value of an Unreal Engine console variable (CVar).
        Useful for reading project debug flags and engine settings.
        Args:
            name: CVar name, e.g. "r.ScreenPercentage" or "MyGame.Debug.Enabled"
        """
        from unreal_mcp_server import get_unreal_connection
        unreal = get_unreal_connection()
        if not unreal:
            return {"success": False, "error": "Not connected to Unreal Engine"}
        return unreal.send_command("get_cvar", {"name": name})

    @mcp.tool()
    def set_cvar(ctx: Context, name: str, value: str) -> Dict[str, Any]:
        """
        Set an Unreal Engine console variable (CVar) value.
        Args:
            name: CVar name, e.g. "r.ScreenPercentage" or "MyGame.Debug.Enabled"
            value: New value as string, e.g. "1" or "0" or "100.0"
        """
        from unreal_mcp_server import get_unreal_connection
        unreal = get_unreal_connection()
        if not unreal:
            return {"success": False, "error": "Not connected to Unreal Engine"}
        return unreal.send_command("set_cvar", {"name": name, "value": value})

    @mcp.tool()
    def get_actor_material_info(ctx: Context, name: str) -> Dict[str, Any]:
        """Inspect all materials assigned to an actor's primitive components.

        Returns each component's material slots with material name, asset path,
        class (Material / MaterialInstanceConstant / MaterialInstanceDynamic),
        instance type, and base material info.

        Args:
            name: The actor name in the level.
        """
        from unreal_mcp_server import get_unreal_connection
        unreal = get_unreal_connection()
        if not unreal:
            return {"success": False, "error": "Not connected to Unreal Engine"}
        return unreal.send_command("get_actor_material_info", {"name": name})

    @mcp.tool()
    def set_actor_material(
        ctx: Context,
        name: str,
        material_path: str,
        slot_index: int = 0
    ) -> Dict[str, Any]:
        """Assign a material to an actor's mesh component.

        Args:
            name: The actor name in the level.
            material_path: Asset path of the material (e.g. /Game/Materials/M_MyMat).
            slot_index: Material slot index (default 0).
        """
        from unreal_mcp_server import get_unreal_connection
        unreal = get_unreal_connection()
        if not unreal:
            return {"success": False, "error": "Not connected to Unreal Engine"}
        return unreal.send_command("set_actor_material", {
            "name": name,
            "material_path": material_path,
            "slot_index": slot_index
        })

    logger.info("Editor tools registered successfully")
