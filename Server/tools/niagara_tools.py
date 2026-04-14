"""
Niagara particle system creation and editing tools for Unreal MCP.

Provides comprehensive Niagara system management via MCP commands:
- Asset Management: Create, delete, list, compile, save systems
- Emitter Management: Add, remove, configure emitters
- Renderer Configuration: Sprite, mesh, ribbon, light renderers
- Module Operations: Spawn, update, particle stacks
- Parameter System: User parameters, bindings
- Material Integration: Particle materials, dynamic bindings
"""

import logging
from mcp.server.fastmcp import FastMCP, Context
from typing import List, Optional, Dict, Any

# Get logger
logger = logging.getLogger("UnrealMCP")


def register_niagara_tools(mcp: FastMCP):
    """Register all Niagara-related MCP tools."""

    #=========================================================================
    # Sprint 1: Asset Management Commands
    #=========================================================================

    @mcp.tool()
    def create_niagara_system(
        ctx: Context,
        system_name: str,
        path: str = "/Game/FX",
        create_default_emitter: bool = True
    ) -> Dict[str, Any]:
        """
        Create a new Niagara System asset.

        Args:
            system_name: Name for the new system
            path: Content path (default: /Game/FX)
            create_default_emitter: Create a default emitter (default: True)

        Returns:
            Dict with success status and system info
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.warning("Failed to connect to Unreal Engine")
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("create_niagara_system", {
                "system_name": system_name,
                "path": path,
                "create_default_emitter": create_default_emitter
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error creating Niagara system: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def delete_niagara_system(
        ctx: Context,
        system_name: str
    ) -> Dict[str, Any]:
        """
        Delete a Niagara System asset.

        Args:
            system_name: Name of system to delete

        Returns:
            Dict with success status
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("delete_niagara_system", {
                "system_name": system_name
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error deleting Niagara system: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def get_niagara_system_info(
        ctx: Context,
        system_name: str
    ) -> Dict[str, Any]:
        """
        Get information about a Niagara System.

        Args:
            system_name: Name of system to query

        Returns:
            Dict with system properties and emitter list
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("get_niagara_system_info", {
                "system_name": system_name
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error getting Niagara system info: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def list_niagara_systems(
        ctx: Context,
        path: str = "/Game",
        recursive: bool = True
    ) -> Dict[str, Any]:
        """
        List Niagara Systems in a path.

        Args:
            path: Content path to search (default: /Game)
            recursive: Search recursively (default: True)

        Returns:
            Dict with list of systems
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("list_niagara_systems", {
                "path": path,
                "recursive": recursive
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error listing Niagara systems: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def open_niagara_system(
        ctx: Context,
        system_name: str
    ) -> Dict[str, Any]:
        """
        Open a Niagara System for editing.

        Args:
            system_name: Name or path of system to open

        Returns:
            Dict with system info
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("open_niagara_system", {
                "system_name": system_name
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error opening Niagara system: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def compile_niagara_system(
        ctx: Context,
        system_name: str
    ) -> Dict[str, Any]:
        """
        Request compilation of a Niagara System.

        Args:
            system_name: Name of system to compile

        Returns:
            Dict with success status
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("compile_niagara_system", {
                "system_name": system_name
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error compiling Niagara system: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def save_niagara_system(
        ctx: Context,
        system_name: str
    ) -> Dict[str, Any]:
        """
        Save a Niagara System asset.

        Args:
            system_name: Name of system to save

        Returns:
            Dict with success status
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("save_niagara_system", {
                "system_name": system_name
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error saving Niagara system: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def set_niagara_system_properties(
        ctx: Context,
        system_name: str,
        fixed_bounds: Optional[List[float]] = None,
        warmup_time: Optional[float] = None,
        determinism: Optional[bool] = None,
        emitter_name: Optional[str] = None,
        spawn_rate: Optional[float] = None,
        lifetime: Optional[float] = None,
        size: Optional[float] = None,
        initial_velocity: Optional[List[float]] = None,
        color: Optional[List[float]] = None
    ) -> Dict[str, Any]:
        """
        Set Niagara System properties.

        Args:
            system_name: Target system
            fixed_bounds: [MinX, MinY, MinZ, MaxX, MaxY, MaxZ] (optional)
            warmup_time: System warmup time in seconds (optional)
            determinism: Enable deterministic simulation (optional)
            emitter_name: Target emitter for emitter-level properties (optional)
            spawn_rate: Particles spawned per second (optional, requires emitter_name)
            lifetime: Particle lifetime in seconds (optional, requires emitter_name)
            size: Particle size (optional, requires emitter_name)
            initial_velocity: [X, Y, Z] initial velocity (optional, requires emitter_name)
            color: [R, G, B, A] particle color 0-1 (optional, requires emitter_name)

        Returns:
            Dict with success status
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            params = {"system_name": system_name}
            if fixed_bounds is not None:
                params["fixed_bounds"] = fixed_bounds
            if warmup_time is not None:
                params["warmup_time"] = warmup_time
            if determinism is not None:
                params["determinism"] = determinism
            if emitter_name is not None:
                params["emitter_name"] = emitter_name
            if spawn_rate is not None:
                params["spawn_rate"] = spawn_rate
            if lifetime is not None:
                params["lifetime"] = lifetime
            if size is not None:
                params["size"] = size
            if initial_velocity is not None:
                params["initial_velocity"] = initial_velocity
            if color is not None:
                params["color"] = color

            response = unreal.send_command("set_niagara_system_properties", params)

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error setting Niagara system properties: {e}")
            return {"status": "error", "error": str(e)}

    #=========================================================================
    # Sprint 2: Emitter Management Commands
    #=========================================================================

    @mcp.tool()
    def add_emitter_to_system(
        ctx: Context,
        system_name: str,
        emitter_name: str,
        template_name: Optional[str] = None
    ) -> Dict[str, Any]:
        """
        Add an emitter to a Niagara System.

        Args:
            system_name: Target system
            emitter_name: Name for the new emitter
            template_name: Template emitter to copy from (optional)

        Returns:
            Dict with emitter info
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            params = {
                "system_name": system_name,
                "emitter_name": emitter_name
            }
            if template_name:
                params["template_name"] = template_name

            response = unreal.send_command("add_emitter_to_system", params)

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error adding emitter: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def remove_emitter_from_system(
        ctx: Context,
        system_name: str,
        emitter_name: str
    ) -> Dict[str, Any]:
        """
        Remove an emitter from a Niagara System.

        Args:
            system_name: Target system
            emitter_name: Name of emitter to remove

        Returns:
            Dict with success status
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("remove_emitter_from_system", {
                "system_name": system_name,
                "emitter_name": emitter_name
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error removing emitter: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def duplicate_emitter(
        ctx: Context,
        system_name: str,
        source_emitter: str,
        new_name: str
    ) -> Dict[str, Any]:
        """
        Duplicate an emitter within a system.

        Args:
            system_name: Target system
            source_emitter: Emitter to duplicate
            new_name: Name for the duplicate

        Returns:
            Dict with new emitter info
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("duplicate_emitter", {
                "system_name": system_name,
                "source_emitter": source_emitter,
                "new_name": new_name
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error duplicating emitter: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def enable_emitter(
        ctx: Context,
        system_name: str,
        emitter_name: str
    ) -> Dict[str, Any]:
        """
        Enable an emitter.

        Args:
            system_name: Target system
            emitter_name: Emitter to enable

        Returns:
            Dict with success status
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("enable_emitter", {
                "system_name": system_name,
                "emitter_name": emitter_name
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error enabling emitter: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def disable_emitter(
        ctx: Context,
        system_name: str,
        emitter_name: str
    ) -> Dict[str, Any]:
        """
        Disable an emitter.

        Args:
            system_name: Target system
            emitter_name: Emitter to disable

        Returns:
            Dict with success status
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("disable_emitter", {
                "system_name": system_name,
                "emitter_name": emitter_name
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error disabling emitter: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def rename_emitter(
        ctx: Context,
        system_name: str,
        emitter_name: str,
        new_name: str
    ) -> Dict[str, Any]:
        """
        Rename an emitter.

        Args:
            system_name: Target system
            emitter_name: Current emitter name
            new_name: New name for the emitter

        Returns:
            Dict with success status
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("rename_emitter", {
                "system_name": system_name,
                "emitter_name": emitter_name,
                "new_name": new_name
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error renaming emitter: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def get_emitter_info(
        ctx: Context,
        system_name: str,
        emitter_name: str
    ) -> Dict[str, Any]:
        """
        Get emitter information.

        Args:
            system_name: Target system
            emitter_name: Emitter to query

        Returns:
            Dict with emitter properties
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("get_emitter_info", {
                "system_name": system_name,
                "emitter_name": emitter_name
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error getting emitter info: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def list_emitters(
        ctx: Context,
        system_name: str
    ) -> Dict[str, Any]:
        """
        List all emitters in a system.

        Args:
            system_name: Target system

        Returns:
            Dict with list of emitters
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("list_emitters", {
                "system_name": system_name
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error listing emitters: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def set_emitter_mode(
        ctx: Context,
        system_name: str,
        emitter_name: str,
        mode: str = "Standard"
    ) -> Dict[str, Any]:
        """
        Set emitter simulation mode.

        Args:
            system_name: Target system
            emitter_name: Target emitter
            mode: "Standard" or "Stateless"

        Returns:
            Dict with success status
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("set_emitter_mode", {
                "system_name": system_name,
                "emitter_name": emitter_name,
                "mode": mode
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error setting emitter mode: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def isolate_emitter(
        ctx: Context,
        system_name: str,
        emitter_name: str,
        isolated: bool = True
    ) -> Dict[str, Any]:
        """
        Toggle emitter isolation for debugging.

        Args:
            system_name: Target system
            emitter_name: Emitter to isolate
            isolated: Whether to isolate (default: True)

        Returns:
            Dict with success status
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("isolate_emitter", {
                "system_name": system_name,
                "emitter_name": emitter_name,
                "isolated": isolated
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error isolating emitter: {e}")
            return {"status": "error", "error": str(e)}

    #=========================================================================
    # Sprint 3: Renderer Configuration Commands
    #=========================================================================

    @mcp.tool()
    def add_sprite_renderer(
        ctx: Context,
        system_name: str,
        emitter_name: str,
        material: Optional[str] = None,
        alignment: str = "Unaligned",
        facing_mode: str = "FaceCamera"
    ) -> Dict[str, Any]:
        """
        Add a sprite renderer to an emitter.

        Args:
            system_name: Target system
            emitter_name: Target emitter
            material: Material path (optional)
            alignment: Unaligned, VelocityAligned, CustomAlignment (default: Unaligned)
            facing_mode: FaceCamera, FaceCameraPlane, CustomFacingVector, etc. (default: FaceCamera)

        Returns:
            Dict with renderer info
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            params = {
                "system_name": system_name,
                "emitter_name": emitter_name,
                "alignment": alignment,
                "facing_mode": facing_mode
            }
            if material:
                params["material"] = material

            response = unreal.send_command("add_sprite_renderer", params)

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error adding sprite renderer: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def configure_sprite_renderer(
        ctx: Context,
        system_name: str,
        emitter_name: str,
        renderer_index: int = 0,
        material: Optional[str] = None,
        sub_image_size: Optional[List[float]] = None
    ) -> Dict[str, Any]:
        """
        Configure sprite renderer properties.

        Args:
            system_name: Target system
            emitter_name: Target emitter
            renderer_index: Renderer index (default: 0)
            material: Material path (optional)
            sub_image_size: [X, Y] sub-image size for flipbooks (optional)

        Returns:
            Dict with success status
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            params = {
                "system_name": system_name,
                "emitter_name": emitter_name,
                "renderer_index": renderer_index
            }
            if material:
                params["material"] = material
            if sub_image_size:
                params["sub_image_size"] = sub_image_size

            response = unreal.send_command("configure_sprite_renderer", params)

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error configuring sprite renderer: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def add_mesh_renderer(
        ctx: Context,
        system_name: str,
        emitter_name: str,
        mesh: Optional[str] = None,
        material: Optional[str] = None
    ) -> Dict[str, Any]:
        """
        Add a mesh renderer to an emitter.

        Args:
            system_name: Target system
            emitter_name: Target emitter
            mesh: Static mesh path (optional)
            material: Material path (optional)

        Returns:
            Dict with renderer info
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            params = {
                "system_name": system_name,
                "emitter_name": emitter_name
            }
            if mesh:
                params["mesh"] = mesh
            if material:
                params["material"] = material

            response = unreal.send_command("add_mesh_renderer", params)

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error adding mesh renderer: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def configure_mesh_renderer(
        ctx: Context,
        system_name: str,
        emitter_name: str,
        renderer_index: int = 0,
        facing_mode: Optional[str] = None
    ) -> Dict[str, Any]:
        """
        Configure mesh renderer properties.

        Args:
            system_name: Target system
            emitter_name: Target emitter
            renderer_index: Renderer index (default: 0)
            facing_mode: Default, Velocity, CameraPosition, CameraPlane (optional)

        Returns:
            Dict with success status
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            params = {
                "system_name": system_name,
                "emitter_name": emitter_name,
                "renderer_index": renderer_index
            }
            if facing_mode:
                params["facing_mode"] = facing_mode

            response = unreal.send_command("configure_mesh_renderer", params)

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error configuring mesh renderer: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def add_ribbon_renderer(
        ctx: Context,
        system_name: str,
        emitter_name: str,
        material: Optional[str] = None
    ) -> Dict[str, Any]:
        """
        Add a ribbon renderer to an emitter.

        Args:
            system_name: Target system
            emitter_name: Target emitter
            material: Material path (optional)

        Returns:
            Dict with renderer info
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            params = {
                "system_name": system_name,
                "emitter_name": emitter_name
            }
            if material:
                params["material"] = material

            response = unreal.send_command("add_ribbon_renderer", params)

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error adding ribbon renderer: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def configure_ribbon_renderer(
        ctx: Context,
        system_name: str,
        emitter_name: str,
        renderer_index: int = 0,
        tessellation_factor: Optional[int] = None
    ) -> Dict[str, Any]:
        """
        Configure ribbon renderer properties.

        Args:
            system_name: Target system
            emitter_name: Target emitter
            renderer_index: Renderer index (default: 0)
            tessellation_factor: Tessellation factor (optional)

        Returns:
            Dict with success status
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            params = {
                "system_name": system_name,
                "emitter_name": emitter_name,
                "renderer_index": renderer_index
            }
            if tessellation_factor is not None:
                params["tessellation_factor"] = tessellation_factor

            response = unreal.send_command("configure_ribbon_renderer", params)

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error configuring ribbon renderer: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def add_light_renderer(
        ctx: Context,
        system_name: str,
        emitter_name: str
    ) -> Dict[str, Any]:
        """
        Add a light renderer to an emitter.

        Args:
            system_name: Target system
            emitter_name: Target emitter

        Returns:
            Dict with renderer info
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("add_light_renderer", {
                "system_name": system_name,
                "emitter_name": emitter_name
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error adding light renderer: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def configure_light_renderer(
        ctx: Context,
        system_name: str,
        emitter_name: str,
        renderer_index: int = 0,
        radius_scale: Optional[float] = None,
        intensity_scale: Optional[float] = None,
        use_inverse_squared_falloff: Optional[bool] = None
    ) -> Dict[str, Any]:
        """
        Configure light renderer properties.

        Args:
            system_name: Target system
            emitter_name: Target emitter
            renderer_index: Renderer index (default: 0)
            radius_scale: Light radius scale (optional)
            intensity_scale: Light intensity scale (optional)
            use_inverse_squared_falloff: Use inverse squared falloff (optional)

        Returns:
            Dict with success status
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            params = {
                "system_name": system_name,
                "emitter_name": emitter_name,
                "renderer_index": renderer_index
            }
            if radius_scale is not None:
                params["radius_scale"] = radius_scale
            if intensity_scale is not None:
                params["intensity_scale"] = intensity_scale
            if use_inverse_squared_falloff is not None:
                params["use_inverse_squared_falloff"] = use_inverse_squared_falloff

            response = unreal.send_command("configure_light_renderer", params)

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error configuring light renderer: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def remove_renderer(
        ctx: Context,
        system_name: str,
        emitter_name: str,
        renderer_index: int
    ) -> Dict[str, Any]:
        """
        Remove a renderer from an emitter.

        Args:
            system_name: Target system
            emitter_name: Target emitter
            renderer_index: Renderer index to remove

        Returns:
            Dict with success status
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("remove_renderer", {
                "system_name": system_name,
                "emitter_name": emitter_name,
                "renderer_index": renderer_index
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error removing renderer: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def get_renderers(
        ctx: Context,
        system_name: str,
        emitter_name: str
    ) -> Dict[str, Any]:
        """
        Get all renderers for an emitter.

        Args:
            system_name: Target system
            emitter_name: Target emitter

        Returns:
            Dict with list of renderers
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("get_renderers", {
                "system_name": system_name,
                "emitter_name": emitter_name
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error getting renderers: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def set_renderer_material(
        ctx: Context,
        system_name: str,
        emitter_name: str,
        renderer_index: int,
        material: str
    ) -> Dict[str, Any]:
        """
        Set renderer material.

        Args:
            system_name: Target system
            emitter_name: Target emitter
            renderer_index: Renderer index
            material: Material path

        Returns:
            Dict with success status
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("set_renderer_material", {
                "system_name": system_name,
                "emitter_name": emitter_name,
                "renderer_index": renderer_index,
                "material": material
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error setting renderer material: {e}")
            return {"status": "error", "error": str(e)}

    #=========================================================================
    # Sprint 5: Parameter System Commands
    #=========================================================================

    @mcp.tool()
    def expose_user_parameter(
        ctx: Context,
        system_name: str,
        param_name: str,
        param_type: str,
        default_value: Optional[Any] = None
    ) -> Dict[str, Any]:
        """
        Expose a user parameter on a Niagara System.

        Args:
            system_name: Target system
            param_name: Parameter name
            param_type: Float, Int32, Bool, Vector, Color
            default_value: Default value (optional)

        Returns:
            Dict with parameter info
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            params = {
                "system_name": system_name,
                "param_name": param_name,
                "param_type": param_type
            }
            if default_value is not None:
                params["default_value"] = default_value

            response = unreal.send_command("expose_user_parameter", params)

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error exposing user parameter: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def set_user_parameter_default(
        ctx: Context,
        system_name: str,
        param_name: str,
        value: Any
    ) -> Dict[str, Any]:
        """
        Set user parameter default value.

        Args:
            system_name: Target system
            param_name: Parameter name
            value: New default value

        Returns:
            Dict with success status
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("set_user_parameter_default", {
                "system_name": system_name,
                "param_name": param_name,
                "value": value
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error setting user parameter default: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def get_user_parameters(
        ctx: Context,
        system_name: str
    ) -> Dict[str, Any]:
        """
        Get all user parameters for a system.

        Args:
            system_name: Target system

        Returns:
            Dict with list of parameters
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("get_user_parameters", {
                "system_name": system_name
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error getting user parameters: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def remove_user_parameter(
        ctx: Context,
        system_name: str,
        param_name: str
    ) -> Dict[str, Any]:
        """
        Remove a user parameter.

        Args:
            system_name: Target system
            param_name: Parameter to remove

        Returns:
            Dict with success status
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("remove_user_parameter", {
                "system_name": system_name,
                "param_name": param_name
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error removing user parameter: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def get_parameter_bindings(
        ctx: Context,
        system_name: str,
        emitter_name: str
    ) -> Dict[str, Any]:
        """
        Get all rapid iteration parameters (module inputs) for an emitter.

        Args:
            system_name: Target system
            emitter_name: Target emitter

        Returns:
            Dict with list of all parameters, their types, and current values
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("get_parameter_bindings", {
                "system_name": system_name,
                "emitter_name": emitter_name
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error getting parameter bindings: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def set_emitter_parameter(
        ctx: Context,
        system_name: str,
        emitter_name: str,
        parameter_name: str,
        value: Any
    ) -> Dict[str, Any]:
        """
        Set any emitter parameter by name (universal setter for module inputs).

        Args:
            system_name: Target system
            emitter_name: Target emitter
            parameter_name: Full parameter name (e.g., "Constants.Fountain.Spawn Rate.Spawn Rate")
            value: Parameter value (type depends on parameter: float, int, bool, [array])

        Returns:
            Dict with success status
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("set_emitter_parameter", {
                "system_name": system_name,
                "emitter_name": emitter_name,
                "parameter_name": parameter_name,
                "value": value
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error setting emitter parameter: {e}")
            return {"status": "error", "error": str(e)}

    #=========================================================================
    # Sprint 6: Material Integration Commands
    #=========================================================================

    @mcp.tool()
    def create_particle_material(
        ctx: Context,
        material_name: str,
        path: str = "/Game/Materials",
        blend_mode: str = "Translucent",
        for_sprites: bool = True,
        for_meshes: bool = False,
        for_ribbons: bool = False
    ) -> Dict[str, Any]:
        """
        Create a particle-compatible material.

        Args:
            material_name: Name for the new material
            path: Content path (default: /Game/Materials)
            blend_mode: Opaque, Translucent, Additive, Modulate (default: Translucent)
            for_sprites: Enable for sprite particles (default: True)
            for_meshes: Enable for mesh particles (default: False)
            for_ribbons: Enable for ribbon particles (default: False)

        Returns:
            Dict with material info
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("create_particle_material", {
                "material_name": material_name,
                "path": path,
                "blend_mode": blend_mode,
                "for_sprites": for_sprites,
                "for_meshes": for_meshes,
                "for_ribbons": for_ribbons
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error creating particle material: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def assign_material_to_renderer(
        ctx: Context,
        system_name: str,
        emitter_name: str,
        renderer_index: int,
        material: str
    ) -> Dict[str, Any]:
        """
        Assign material to a renderer.

        Args:
            system_name: Target system
            emitter_name: Target emitter
            renderer_index: Renderer index
            material: Material path

        Returns:
            Dict with success status
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("assign_material_to_renderer", {
                "system_name": system_name,
                "emitter_name": emitter_name,
                "renderer_index": renderer_index,
                "material": material
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error assigning material to renderer: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def create_particle_material_instance(
        ctx: Context,
        instance_name: str,
        parent_material: str,
        path: str = "/Game/Materials"
    ) -> Dict[str, Any]:
        """
        Create a material instance for particles.

        Args:
            instance_name: Name for the instance
            parent_material: Parent material path
            path: Content path (default: /Game/Materials)

        Returns:
            Dict with instance info
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("create_particle_material_instance", {
                "instance_name": instance_name,
                "parent_material": parent_material,
                "path": path
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error creating particle material instance: {e}")
            return {"status": "error", "error": str(e)}
