"""
Input Simulation & Screenshot Tools for Unreal MCP.

Provides PIE control, viewport capture, widget discovery,
and mouse/keyboard input injection for automated UI testing.
"""

import logging
from typing import Dict, Any, Optional
from mcp.server.fastmcp import FastMCP, Context

logger = logging.getLogger("UnrealMCP")


def register_input_tools(mcp: FastMCP):
    """Register input simulation and screenshot tools with the MCP server."""

    # ================================================================
    # PIE CONTROL
    # ================================================================

    @mcp.tool()
    def start_pie(ctx: Context) -> Dict[str, Any]:
        """
        Start Play In Editor (PIE) session.
        Call is_pie_active() after a short delay to confirm PIE is ready.
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("start_pie", {})
            return response or {"success": False}
        except Exception as e:
            logger.error(f"Error starting PIE: {e}")
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def stop_pie(ctx: Context) -> Dict[str, Any]:
        """Stop the current Play In Editor (PIE) session."""
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("stop_pie", {})
            return response or {"success": False}
        except Exception as e:
            logger.error(f"Error stopping PIE: {e}")
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def is_pie_active(ctx: Context) -> Dict[str, Any]:
        """Check if a Play In Editor (PIE) session is currently running."""
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"active": False, "message": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("is_pie_active", {})
            return response or {"active": False}
        except Exception as e:
            logger.error(f"Error checking PIE: {e}")
            return {"active": False, "message": str(e)}

    # ================================================================
    # SCREENSHOT
    # ================================================================

    @mcp.tool()
    def take_game_screenshot(
        ctx: Context,
        filepath: Optional[str] = None
    ) -> Dict[str, Any]:
        """
        Capture a screenshot of the game window (PIE) including all UI.
        Uses native window capture so both 3D scene and Slate/UMG widgets
        are included in the screenshot.

        Args:
            filepath: Optional file path for the screenshot (.png).
                      Defaults to Saved/Screenshots/mcp_screenshot.png

        Returns:
            Dict with filepath, width, height of the saved screenshot.
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            params = {}
            if filepath:
                params["filepath"] = filepath

            response = unreal.send_command("take_game_screenshot", params)
            return response or {"success": False}
        except Exception as e:
            logger.error(f"Error taking screenshot: {e}")
            return {"success": False, "message": str(e)}

    # ================================================================
    # WIDGET DISCOVERY
    # ================================================================

    @mcp.tool()
    def find_widget(
        ctx: Context,
        widget_name: str
    ) -> Dict[str, Any]:
        """
        Find a UMG widget by name and return its screen-space bounds.
        Works during PIE — searches all active UUserWidget trees.

        Args:
            widget_name: The name of the widget to find (e.g., "OptionsButton", "MasterVolumeSlider")

        Returns:
            Dict with found, x, y, width, height, center_x, center_y
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"found": False, "message": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("find_widget_bounds", {
                "widget_name": widget_name
            })
            return response or {"found": False}
        except Exception as e:
            logger.error(f"Error finding widget: {e}")
            return {"found": False, "message": str(e)}

    @mcp.tool()
    def list_visible_widgets(
        ctx: Context,
        root_class: Optional[str] = None
    ) -> Dict[str, Any]:
        """
        List all visible UMG widgets with their screen-space positions.
        Useful for understanding what's on screen before clicking.

        Args:
            root_class: Optional filter — only include widgets from root widgets
                       whose class name contains this string (e.g., "MainMenu", "OptionsMenu")

        Returns:
            Dict with count and widgets array (each with name, class, x, y, width, height)
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"count": 0, "widgets": [], "message": "Failed to connect"}

            params = {}
            if root_class:
                params["root_class"] = root_class

            response = unreal.send_command("list_visible_widgets", params)
            return response or {"count": 0, "widgets": []}
        except Exception as e:
            logger.error(f"Error listing widgets: {e}")
            return {"count": 0, "widgets": [], "message": str(e)}

    # ================================================================
    # INPUT SIMULATION
    # ================================================================

    @mcp.tool()
    def click_widget(
        ctx: Context,
        widget_name: str
    ) -> Dict[str, Any]:
        """
        Find a widget by name and simulate a mouse click on its center.
        Combines find_widget + click in one call.

        Args:
            widget_name: Name of the widget to click (e.g., "OptionsButton")

        Returns:
            Dict with success, x, y of the click position
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("simulate_click", {
                "widget_name": widget_name
            })
            return response or {"success": False}
        except Exception as e:
            logger.error(f"Error clicking widget: {e}")
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def click_at(
        ctx: Context,
        x: float,
        y: float
    ) -> Dict[str, Any]:
        """
        Simulate a mouse click at screen coordinates (x, y).

        Args:
            x: X screen coordinate (pixels)
            y: Y screen coordinate (pixels)

        Returns:
            Dict with success status
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("simulate_click", {
                "x": x,
                "y": y
            })
            return response or {"success": False}
        except Exception as e:
            logger.error(f"Error clicking: {e}")
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def move_mouse(
        ctx: Context,
        x: float,
        y: float
    ) -> Dict[str, Any]:
        """
        Move the mouse cursor to screen coordinates (x, y).

        Args:
            x: X screen coordinate (pixels)
            y: Y screen coordinate (pixels)
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("simulate_mouse_move", {
                "x": x,
                "y": y
            })
            return response or {"success": False}
        except Exception as e:
            logger.error(f"Error moving mouse: {e}")
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def press_key(
        ctx: Context,
        key: str,
        shift: bool = False,
        ctrl: bool = False,
        alt: bool = False
    ) -> Dict[str, Any]:
        """
        Simulate a keyboard key press (down + up).

        Args:
            key: Key name (e.g., "Escape", "Enter", "SpaceBar", "A", "Tab",
                 "Up", "Down", "Left", "Right", "F1")
            shift: Hold Shift modifier
            ctrl: Hold Ctrl modifier
            alt: Hold Alt modifier
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            params = {"key": key}
            if shift:
                params["shift"] = True
            if ctrl:
                params["ctrl"] = True
            if alt:
                params["alt"] = True

            response = unreal.send_command("simulate_key_press", params)
            return response or {"success": False}
        except Exception as e:
            logger.error(f"Error pressing key: {e}")
            return {"success": False, "message": str(e)}
