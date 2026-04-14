"""
Blackboard data asset management tools for Unreal MCP.

Provides Blackboard creation, deletion, listing, and key manipulation:
- Asset Management: Create, delete, list, info, save blackboard assets
- Key Management: Add, remove, modify blackboard keys
"""

import logging
from mcp.server.fastmcp import FastMCP, Context
from typing import List, Optional, Dict, Any

# Get logger
logger = logging.getLogger("UnrealMCP")


def register_blackboard_tools(mcp: FastMCP):
    """Register all Blackboard-related MCP tools."""

    #=========================================================================
    # Asset Management Commands
    #=========================================================================

    @mcp.tool()
    def create_blackboard(
        ctx: Context,
        bb_name: str,
        path: str = "/Game/AI",
        parent_bb: str = ""
    ) -> Dict[str, Any]:
        """
        Create a new Blackboard Data asset.

        Args:
            bb_name: Name for the new blackboard
            path: Content path (default: /Game/AI)
            parent_bb: Parent blackboard name for key inheritance (optional)

        Returns:
            Dict with success status and blackboard info
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.warning("Failed to connect to Unreal Engine")
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("create_blackboard", {
                "bb_name": bb_name,
                "path": path,
                "parent_bb": parent_bb
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error creating blackboard: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def delete_blackboard(
        ctx: Context,
        bb_name: str
    ) -> Dict[str, Any]:
        """
        Delete a Blackboard Data asset.

        Args:
            bb_name: Name of blackboard to delete

        Returns:
            Dict with success status
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("delete_blackboard", {
                "bb_name": bb_name
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error deleting blackboard: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def list_blackboards(
        ctx: Context,
        path: str = "/Game",
        recursive: bool = True
    ) -> Dict[str, Any]:
        """
        List Blackboard Data assets in a path.

        Args:
            path: Content path to search (default: /Game)
            recursive: Search subdirectories (default: True)

        Returns:
            Dict with list of blackboard assets
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("list_blackboards", {
                "path": path,
                "recursive": recursive
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error listing blackboards: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def get_blackboard_info(
        ctx: Context,
        bb_name: str
    ) -> Dict[str, Any]:
        """
        Get information about a Blackboard asset including all keys.

        Args:
            bb_name: Name of blackboard to query

        Returns:
            Dict with blackboard properties and key list
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("get_blackboard_info", {
                "bb_name": bb_name
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error getting blackboard info: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def save_blackboard(
        ctx: Context,
        bb_name: str
    ) -> Dict[str, Any]:
        """
        Save a Blackboard Data asset to disk.

        Args:
            bb_name: Name of blackboard to save

        Returns:
            Dict with success status
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("save_blackboard", {
                "bb_name": bb_name
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error saving blackboard: {e}")
            return {"status": "error", "error": str(e)}

    #=========================================================================
    # Key Management Commands
    #=========================================================================

    @mcp.tool()
    def add_blackboard_key(
        ctx: Context,
        bb_name: str,
        key_name: str,
        key_type: str = "Bool",
        description: str = "",
        instance_synced: bool = False
    ) -> Dict[str, Any]:
        """
        Add a key to a Blackboard asset.

        Args:
            bb_name: Target blackboard name
            key_name: Name for the new key
            key_type: Key type - Bool, Float, Int, Vector, Object, String, Class, Enum, Name, Rotator
            description: Optional key description
            instance_synced: Whether to sync across instances

        Returns:
            Dict with success status and key info
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("add_blackboard_key", {
                "bb_name": bb_name,
                "key_name": key_name,
                "key_type": key_type,
                "description": description,
                "instance_synced": instance_synced
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error adding blackboard key: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def remove_blackboard_key(
        ctx: Context,
        bb_name: str,
        key_name: str
    ) -> Dict[str, Any]:
        """
        Remove a key from a Blackboard asset.

        Args:
            bb_name: Target blackboard name
            key_name: Name of key to remove

        Returns:
            Dict with success status
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("remove_blackboard_key", {
                "bb_name": bb_name,
                "key_name": key_name
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error removing blackboard key: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def modify_blackboard_key(
        ctx: Context,
        bb_name: str,
        key_name: str,
        new_name: str = "",
        description: str = "",
        instance_synced: bool = False
    ) -> Dict[str, Any]:
        """
        Modify an existing Blackboard key.

        Args:
            bb_name: Target blackboard name
            key_name: Key to modify
            new_name: New name for the key (optional, empty = no rename)
            description: Updated description (optional)
            instance_synced: Updated sync setting

        Returns:
            Dict with success status and updated key info
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("modify_blackboard_key", {
                "bb_name": bb_name,
                "key_name": key_name,
                "new_name": new_name,
                "description": description,
                "instance_synced": instance_synced
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error modifying blackboard key: {e}")
            return {"status": "error", "error": str(e)}
