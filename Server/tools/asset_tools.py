"""
Asset Tools for Unreal MCP.

Provides 17 MCP tools for analyzing, organizing, moving, and renaming
project assets inside the Unreal Editor content browser.
"""

import logging
from typing import Dict, List, Any, Optional
from mcp.server.fastmcp import FastMCP, Context

logger = logging.getLogger("UnrealMCP")


def register_asset_tools(mcp: FastMCP):
    """Register asset management tools with the MCP server."""

    # -------------------------------------------------------------------------
    # Group A: Discovery (read-only)
    # -------------------------------------------------------------------------

    @mcp.tool()
    def list_assets(
        ctx: Context,
        directory_path: str = "/Game",
        recursive: bool = True,
        asset_class: str = "",
        name_pattern: str = "",
    ) -> Dict[str, Any]:
        """List all assets in a directory.

        Args:
            ctx: MCP context.
            directory_path: Content browser path to search (default "/Game").
            recursive: Whether to search subdirectories.
            asset_class: Optional class name filter (e.g. "StaticMesh", "Material").
            name_pattern: Optional wildcard name filter (e.g. "WBP_*", "*Crafting*"). Supports * and ? wildcards.

        Returns:
            Dict with 'assets' array of {path, name, class} and 'count'.
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "error": "Failed to connect to Unreal Engine"}

            params = {
                "directory_path": directory_path,
                "recursive": recursive,
            }
            if asset_class:
                params["asset_class"] = asset_class
            if name_pattern:
                params["name_pattern"] = name_pattern

            response = unreal.send_command("list_assets", params)
            if not response:
                return {"success": False, "error": "No response from Unreal Engine"}

            if response.get("status") == "error":
                return {"success": False, "error": response.get("error", "Unknown error")}

            result = response.get("result", response)
            return result

        except Exception as e:
            logger.error(f"Error in list_assets: {e}")
            return {"success": False, "error": str(e)}

    @mcp.tool()
    def get_asset_info(
        ctx: Context,
        asset_path: str,
    ) -> Dict[str, Any]:
        """Get detailed information about a single asset.

        Args:
            ctx: MCP context.
            asset_path: Full asset path (e.g. "/Game/Meshes/SM_Ball").

        Returns:
            Dict with name, class, package_path, disk_size_bytes, tags, etc.
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("get_asset_info", {
                "asset_path": asset_path,
            })
            if not response:
                return {"success": False, "error": "No response from Unreal Engine"}

            if response.get("status") == "error":
                return {"success": False, "error": response.get("error", "Unknown error")}

            return response.get("result", response)

        except Exception as e:
            logger.error(f"Error in get_asset_info: {e}")
            return {"success": False, "error": str(e)}

    @mcp.tool()
    def find_asset_references(
        ctx: Context,
        asset_path: str,
        load_to_confirm: bool = False,
    ) -> Dict[str, Any]:
        """Find all assets that reference the given asset.

        Args:
            ctx: MCP context.
            asset_path: Full asset path to find referencers for.
            load_to_confirm: If True, load packages to confirm references (slower but more accurate).

        Returns:
            Dict with 'referencers' array and 'count'.
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("find_asset_references", {
                "asset_path": asset_path,
                "load_to_confirm": load_to_confirm,
            })
            if not response:
                return {"success": False, "error": "No response from Unreal Engine"}

            if response.get("status") == "error":
                return {"success": False, "error": response.get("error", "Unknown error")}

            return response.get("result", response)

        except Exception as e:
            logger.error(f"Error in find_asset_references: {e}")
            return {"success": False, "error": str(e)}

    @mcp.tool()
    def get_folder_structure(
        ctx: Context,
        directory_path: str = "/Game",
        max_depth: int = -1,
        include_asset_counts: bool = False,
    ) -> Dict[str, Any]:
        """Get the folder structure of a content browser directory as a nested tree.

        Args:
            ctx: MCP context.
            directory_path: Root path to start from (default "/Game").
            max_depth: Maximum recursion depth (-1 for unlimited).
            include_asset_counts: If True, include asset count per folder.

        Returns:
            Dict with nested 'tree' object {name, path, children, asset_count}.
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "error": "Failed to connect to Unreal Engine"}

            params: Dict[str, Any] = {"directory_path": directory_path}
            if max_depth >= 0:
                params["max_depth"] = max_depth
            if include_asset_counts:
                params["include_asset_counts"] = True

            response = unreal.send_command("get_folder_structure", params)
            if not response:
                return {"success": False, "error": "No response from Unreal Engine"}

            if response.get("status") == "error":
                return {"success": False, "error": response.get("error", "Unknown error")}

            return response.get("result", response)

        except Exception as e:
            logger.error(f"Error in get_folder_structure: {e}")
            return {"success": False, "error": str(e)}

    @mcp.tool()
    def get_asset_class(
        ctx: Context,
        asset_path: str,
    ) -> Dict[str, Any]:
        """Get the class and parent class of an asset.

        Args:
            ctx: MCP context.
            asset_path: Full asset path.

        Returns:
            Dict with 'class_name' and 'parent_class'.
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("get_asset_class", {
                "asset_path": asset_path,
            })
            if not response:
                return {"success": False, "error": "No response from Unreal Engine"}

            if response.get("status") == "error":
                return {"success": False, "error": response.get("error", "Unknown error")}

            return response.get("result", response)

        except Exception as e:
            logger.error(f"Error in get_asset_class: {e}")
            return {"success": False, "error": str(e)}

    # -------------------------------------------------------------------------
    # Group B: Organization (write)
    # -------------------------------------------------------------------------

    @mcp.tool()
    def rename_asset(
        ctx: Context,
        source_path: str,
        destination_path: str,
        protected_paths: List[str] = [],
    ) -> Dict[str, Any]:
        """Rename an asset (change its name within the same or different folder).

        Creates a redirector at the old path so existing references still resolve.
        Run fix_redirectors afterwards to clean up.

        Args:
            ctx: MCP context.
            source_path: Current full asset path.
            destination_path: New full asset path.
            protected_paths: List of directory paths that must not be modified.

        Returns:
            Dict with success, old_path, new_path.
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("rename_asset", {
                "source_path": source_path,
                "destination_path": destination_path,
                "protected_paths": protected_paths,
            })
            if not response:
                return {"success": False, "error": "No response from Unreal Engine"}

            if response.get("status") == "error":
                return {"success": False, "error": response.get("error", "Unknown error")}

            return response.get("result", response)

        except Exception as e:
            logger.error(f"Error in rename_asset: {e}")
            return {"success": False, "error": str(e)}

    @mcp.tool()
    def move_asset(
        ctx: Context,
        source_path: str,
        destination_path: str,
        protected_paths: List[str] = [],
    ) -> Dict[str, Any]:
        """Move an asset to a new location.

        Equivalent to rename_asset (in UE, move == rename to new path).
        Creates a redirector at the old path.

        Args:
            ctx: MCP context.
            source_path: Current full asset path.
            destination_path: New full asset path.
            protected_paths: List of directory paths that must not be modified.

        Returns:
            Dict with success, old_path, new_path.
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("move_asset", {
                "source_path": source_path,
                "destination_path": destination_path,
                "protected_paths": protected_paths,
            })
            if not response:
                return {"success": False, "error": "No response from Unreal Engine"}

            if response.get("status") == "error":
                return {"success": False, "error": response.get("error", "Unknown error")}

            return response.get("result", response)

        except Exception as e:
            logger.error(f"Error in move_asset: {e}")
            return {"success": False, "error": str(e)}

    @mcp.tool()
    def move_directory(
        ctx: Context,
        source_path: str,
        destination_path: str,
        protected_paths: List[str] = [],
    ) -> Dict[str, Any]:
        """Move an entire directory and all its contents to a new location.

        Args:
            ctx: MCP context.
            source_path: Current directory path (e.g. "/Game/OldFolder").
            destination_path: New directory path (e.g. "/Game/NewFolder").
            protected_paths: List of directory paths that must not be modified.

        Returns:
            Dict with success and assets_moved count.
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("move_directory", {
                "source_path": source_path,
                "destination_path": destination_path,
                "protected_paths": protected_paths,
            })
            if not response:
                return {"success": False, "error": "No response from Unreal Engine"}

            if response.get("status") == "error":
                return {"success": False, "error": response.get("error", "Unknown error")}

            return response.get("result", response)

        except Exception as e:
            logger.error(f"Error in move_directory: {e}")
            return {"success": False, "error": str(e)}

    @mcp.tool()
    def make_directory(
        ctx: Context,
        directory_path: str,
    ) -> Dict[str, Any]:
        """Create a new directory in the content browser.

        Args:
            ctx: MCP context.
            directory_path: Path for the new directory (e.g. "/Game/Meshes/Props").

        Returns:
            Dict with success, path, and already_existed.
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("make_directory", {
                "directory_path": directory_path,
            })
            if not response:
                return {"success": False, "error": "No response from Unreal Engine"}

            if response.get("status") == "error":
                return {"success": False, "error": response.get("error", "Unknown error")}

            return response.get("result", response)

        except Exception as e:
            logger.error(f"Error in make_directory: {e}")
            return {"success": False, "error": str(e)}

    @mcp.tool()
    def duplicate_asset(
        ctx: Context,
        source_path: str,
        destination_path: str,
    ) -> Dict[str, Any]:
        """Duplicate an asset to a new location.

        Args:
            ctx: MCP context.
            source_path: Asset to duplicate.
            destination_path: Where to place the copy.

        Returns:
            Dict with success.
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("duplicate_asset", {
                "source_path": source_path,
                "destination_path": destination_path,
            })
            if not response:
                return {"success": False, "error": "No response from Unreal Engine"}

            if response.get("status") == "error":
                return {"success": False, "error": response.get("error", "Unknown error")}

            return response.get("result", response)

        except Exception as e:
            logger.error(f"Error in duplicate_asset: {e}")
            return {"success": False, "error": str(e)}

    @mcp.tool()
    def save_asset(
        ctx: Context,
        asset_path: str,
        only_if_dirty: bool = True,
    ) -> Dict[str, Any]:
        """Save a single asset to disk.

        Args:
            ctx: MCP context.
            asset_path: Asset to save.
            only_if_dirty: If True, only save when the asset has unsaved changes.

        Returns:
            Dict with success.
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("save_asset", {
                "asset_path": asset_path,
                "only_if_dirty": only_if_dirty,
            })
            if not response:
                return {"success": False, "error": "No response from Unreal Engine"}

            if response.get("status") == "error":
                return {"success": False, "error": response.get("error", "Unknown error")}

            return response.get("result", response)

        except Exception as e:
            logger.error(f"Error in save_asset: {e}")
            return {"success": False, "error": str(e)}

    @mcp.tool()
    def save_directory(
        ctx: Context,
        directory_path: str,
        recursive: bool = True,
        only_if_dirty: bool = True,
    ) -> Dict[str, Any]:
        """Save all assets in a directory to disk.

        Args:
            ctx: MCP context.
            directory_path: Directory whose contents to save.
            recursive: Whether to include subdirectories.
            only_if_dirty: If True, only save assets with unsaved changes.

        Returns:
            Dict with success.
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("save_directory", {
                "directory_path": directory_path,
                "recursive": recursive,
                "only_if_dirty": only_if_dirty,
            })
            if not response:
                return {"success": False, "error": "No response from Unreal Engine"}

            if response.get("status") == "error":
                return {"success": False, "error": response.get("error", "Unknown error")}

            return response.get("result", response)

        except Exception as e:
            logger.error(f"Error in save_directory: {e}")
            return {"success": False, "error": str(e)}

    # -------------------------------------------------------------------------
    # Group C: Safety
    # -------------------------------------------------------------------------

    @mcp.tool()
    def validate_asset_move(
        ctx: Context,
        source_path: str,
        destination_path: str,
        protected_paths: List[str] = [],
    ) -> Dict[str, Any]:
        """Validate whether an asset can be safely moved.

        Checks source existence, destination conflicts, protected paths, and
        reports all referencers that will need redirector cleanup.

        Args:
            ctx: MCP context.
            source_path: Current asset path.
            destination_path: Proposed new path.
            protected_paths: Directories that must not be modified.

        Returns:
            Dict with can_move, referencers, is_protected, destination_exists, warnings.
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("validate_asset_move", {
                "source_path": source_path,
                "destination_path": destination_path,
                "protected_paths": protected_paths,
            })
            if not response:
                return {"success": False, "error": "No response from Unreal Engine"}

            if response.get("status") == "error":
                return {"success": False, "error": response.get("error", "Unknown error")}

            return response.get("result", response)

        except Exception as e:
            logger.error(f"Error in validate_asset_move: {e}")
            return {"success": False, "error": str(e)}

    @mcp.tool()
    def fix_redirectors(
        ctx: Context,
        directory_path: str = "/Game",
        recursive: bool = True,
    ) -> Dict[str, Any]:
        """Fix up asset redirectors left behind by move/rename operations.

        Finds all ObjectRedirectors in the directory, updates all referencers
        to point to the final destination, then deletes the redirectors.

        Args:
            ctx: MCP context.
            directory_path: Directory to scan for redirectors.
            recursive: Whether to include subdirectories.

        Returns:
            Dict with success and redirectors_fixed count.
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("fix_redirectors", {
                "directory_path": directory_path,
                "recursive": recursive,
            })
            if not response:
                return {"success": False, "error": "No response from Unreal Engine"}

            if response.get("status") == "error":
                return {"success": False, "error": response.get("error", "Unknown error")}

            return response.get("result", response)

        except Exception as e:
            logger.error(f"Error in fix_redirectors: {e}")
            return {"success": False, "error": str(e)}

    @mcp.tool()
    def is_protected_path(
        ctx: Context,
        path: str,
        protected_paths: List[str] = [],
    ) -> Dict[str, Any]:
        """Check if a path falls within any of the protected directories.

        Args:
            ctx: MCP context.
            path: The path to check.
            protected_paths: List of protected directory prefixes.

        Returns:
            Dict with is_protected and reason.
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("is_protected_path", {
                "path": path,
                "protected_paths": protected_paths,
            })
            if not response:
                return {"success": False, "error": "No response from Unreal Engine"}

            if response.get("status") == "error":
                return {"success": False, "error": response.get("error", "Unknown error")}

            return response.get("result", response)

        except Exception as e:
            logger.error(f"Error in is_protected_path: {e}")
            return {"success": False, "error": str(e)}

    # -------------------------------------------------------------------------
    # Group D: Analysis / Batch
    # -------------------------------------------------------------------------

    @mcp.tool()
    def analyze_folder_organization(
        ctx: Context,
        directory_path: str = "/Game",
        protected_paths: List[str] = [],
    ) -> Dict[str, Any]:
        """Analyze a directory for organization issues.

        Checks naming conventions (SM_, M_, T_ prefixes), folder placement
        (meshes in /Meshes/, materials in /Materials/, etc.), and finds
        empty folders. Protected paths are skipped during analysis.

        Args:
            ctx: MCP context.
            directory_path: Directory to analyze.
            protected_paths: Marketplace/third-party folders to skip.

        Returns:
            Dict with total_assets, assets_by_class, misplaced_assets,
            naming_issues, empty_folders.
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("analyze_folder_organization", {
                "directory_path": directory_path,
                "protected_paths": protected_paths,
            })
            if not response:
                return {"success": False, "error": "No response from Unreal Engine"}

            if response.get("status") == "error":
                return {"success": False, "error": response.get("error", "Unknown error")}

            return response.get("result", response)

        except Exception as e:
            logger.error(f"Error in analyze_folder_organization: {e}")
            return {"success": False, "error": str(e)}

    @mcp.tool()
    def batch_move_assets(
        ctx: Context,
        moves: List[Dict[str, str]],
        dry_run: bool = False,
        protected_paths: List[str] = [],
    ) -> Dict[str, Any]:
        """Move multiple assets in a single batch operation.

        Validates ALL moves first. If any validation fails, reports it in
        the results. In dry_run mode, only validation is performed.

        Args:
            ctx: MCP context.
            moves: Array of {source_path, destination_path} objects.
            dry_run: If True, only validate without executing moves.
            protected_paths: Directories that must not be modified.

        Returns:
            Dict with results array, total_moved, total_failed.
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("batch_move_assets", {
                "moves": moves,
                "dry_run": dry_run,
                "protected_paths": protected_paths,
            })
            if not response:
                return {"success": False, "error": "No response from Unreal Engine"}

            if response.get("status") == "error":
                return {"success": False, "error": response.get("error", "Unknown error")}

            return response.get("result", response)

        except Exception as e:
            logger.error(f"Error in batch_move_assets: {e}")
            return {"success": False, "error": str(e)}
