"""
UMG Tools for Unreal MCP.

This module provides tools for creating and manipulating UMG Widget Blueprints in Unreal Engine.
"""

import logging
from typing import Dict, List, Any
from mcp.server.fastmcp import FastMCP, Context

# Get logger
logger = logging.getLogger("UnrealMCP")

def register_umg_tools(mcp: FastMCP):
    """Register ALL UMG tools (convenience wrapper for all widget sub-modules)."""
    register_widget_core_tools(mcp)
    register_widget_commonui_tools(mcp)
    register_widget_batch_tools(mcp)
    register_widget_discovery_tools(mcp)
    register_widget_style_tools(mcp)
    register_widget_input_tools(mcp)
    register_widget_animation_tools(mcp)
    register_widget_commonui_ext_tools(mcp)


def register_widget_core_tools(mcp: FastMCP):
    """Register core widget tools: create, add basic widgets, layout, hierarchy, styling."""

    @mcp.tool()
    def create_umg_widget_blueprint(
        ctx: Context,
        widget_name: str,
        parent_class: str = "UserWidget",
        path: str = "/Game/UI"
    ) -> Dict[str, Any]:
        """
        Create a new UMG Widget Blueprint.
        
        Args:
            widget_name: Name of the widget blueprint to create
            parent_class: Parent class for the widget (default: UserWidget)
            path: Content browser path where the widget should be created
            
        Returns:
            Dict containing success status and widget path
        """
        from unreal_mcp_server import get_unreal_connection
        
        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            
            params = {
                "name": widget_name,
                "parent_class": parent_class,
                "path": path
            }
            
            logger.info(f"Creating UMG Widget Blueprint with params: {params}")
            response = unreal.send_command("create_umg_widget_blueprint", params)
            
            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}
            
            logger.info(f"Create UMG Widget Blueprint response: {response}")
            return response
            
        except Exception as e:
            error_msg = f"Error creating UMG Widget Blueprint: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @mcp.tool()
    def add_text_block_to_widget(
        ctx: Context,
        widget_name: str,
        text_block_name: str,
        text: str = "",
        position: List[float] = [0.0, 0.0],
        size: List[float] = [200.0, 50.0],
        font_size: int = 12,
        color: List[float] = [1.0, 1.0, 1.0, 1.0]
    ) -> Dict[str, Any]:
        """
        Add a Text Block widget to a UMG Widget Blueprint.
        
        Args:
            widget_name: Name of the target Widget Blueprint
            text_block_name: Name to give the new Text Block
            text: Initial text content
            position: [X, Y] position in the canvas panel
            size: [Width, Height] of the text block
            font_size: Font size in points
            color: [R, G, B, A] color values (0.0 to 1.0)
            
        Returns:
            Dict containing success status and text block properties
        """
        from unreal_mcp_server import get_unreal_connection
        
        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            
            params = {
                "blueprint_name": widget_name,
                "widget_name": text_block_name,
                "text": text,
                "position": position,
                "size": size,
                "font_size": font_size,
                "color": color
            }
            
            logger.info(f"Adding Text Block to widget with params: {params}")
            response = unreal.send_command("add_text_block_to_widget", params)
            
            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}
            
            logger.info(f"Add Text Block response: {response}")
            return response
            
        except Exception as e:
            error_msg = f"Error adding Text Block to widget: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @mcp.tool()
    def add_button_to_widget(
        ctx: Context,
        widget_name: str,
        button_name: str,
        text: str = "",
        position: List[float] = [0.0, 0.0],
        size: List[float] = [200.0, 50.0],
        font_size: int = 12,
        color: List[float] = [1.0, 1.0, 1.0, 1.0],
        background_color: List[float] = [0.1, 0.1, 0.1, 1.0]
    ) -> Dict[str, Any]:
        """
        Add a Button widget to a UMG Widget Blueprint.
        
        Args:
            widget_name: Name of the target Widget Blueprint
            button_name: Name to give the new Button
            text: Text to display on the button
            position: [X, Y] position in the canvas panel
            size: [Width, Height] of the button
            font_size: Font size for button text
            color: [R, G, B, A] text color values (0.0 to 1.0)
            background_color: [R, G, B, A] button background color values (0.0 to 1.0)
            
        Returns:
            Dict containing success status and button properties
        """
        from unreal_mcp_server import get_unreal_connection
        
        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            
            params = {
                "blueprint_name": widget_name,
                "widget_name": button_name,
                "text": text,
                "position": position,
                "size": size,
                "font_size": font_size,
                "color": color,
                "background_color": background_color
            }
            
            logger.info(f"Adding Button to widget with params: {params}")
            response = unreal.send_command("add_button_to_widget", params)
            
            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}
            
            logger.info(f"Add Button response: {response}")
            return response
            
        except Exception as e:
            error_msg = f"Error adding Button to widget: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @mcp.tool()
    def bind_widget_event(
        ctx: Context,
        widget_name: str,
        widget_component_name: str,
        event_name: str,
        function_name: str = ""
    ) -> Dict[str, Any]:
        """
        Bind an event on a widget component to a function.
        
        Args:
            widget_name: Name of the target Widget Blueprint
            widget_component_name: Name of the widget component (button, etc.)
            event_name: Name of the event to bind (OnClicked, etc.)
            function_name: Name of the function to create/bind to (defaults to f"{widget_component_name}_{event_name}")
            
        Returns:
            Dict containing success status and binding information
        """
        from unreal_mcp_server import get_unreal_connection
        
        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            
            # If no function name provided, create one from component and event names
            if not function_name:
                function_name = f"{widget_component_name}_{event_name}"
            
            params = {
                "widget_name": widget_name,
                "widget_component_name": widget_component_name,
                "event_name": event_name,
                "function_name": function_name
            }
            
            logger.info(f"Binding widget event with params: {params}")
            response = unreal.send_command("bind_widget_event", params)
            
            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}
            
            logger.info(f"Bind widget event response: {response}")
            return response
            
        except Exception as e:
            error_msg = f"Error binding widget event: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @mcp.tool()
    def add_widget_to_viewport(
        ctx: Context,
        widget_name: str,
        z_order: int = 0
    ) -> Dict[str, Any]:
        """
        Add a Widget Blueprint instance to the viewport.
        
        Args:
            widget_name: Name of the Widget Blueprint to add
            z_order: Z-order for the widget (higher numbers appear on top)
            
        Returns:
            Dict containing success status and widget instance information
        """
        from unreal_mcp_server import get_unreal_connection
        
        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            
            params = {
                "widget_name": widget_name,
                "z_order": z_order
            }
            
            logger.info(f"Adding widget to viewport with params: {params}")
            response = unreal.send_command("add_widget_to_viewport", params)
            
            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}
            
            logger.info(f"Add widget to viewport response: {response}")
            return response
            
        except Exception as e:
            error_msg = f"Error adding widget to viewport: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @mcp.tool()
    def set_text_block_binding(
        ctx: Context,
        widget_name: str,
        text_block_name: str,
        binding_property: str,
        binding_type: str = "Text"
    ) -> Dict[str, Any]:
        """
        Set up a property binding for a Text Block widget.
        
        Args:
            widget_name: Name of the target Widget Blueprint
            text_block_name: Name of the Text Block to bind
            binding_property: Name of the property to bind to
            binding_type: Type of binding (Text, Visibility, etc.)
            
        Returns:
            Dict containing success status and binding information
        """
        from unreal_mcp_server import get_unreal_connection
        
        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            
            params = {
                "widget_name": widget_name,
                "text_block_name": text_block_name,
                "binding_property": binding_property,
                "binding_type": binding_type
            }
            
            logger.info(f"Setting text block binding with params: {params}")
            response = unreal.send_command("set_text_block_binding", params)
            
            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}
            
            logger.info(f"Set text block binding response: {response}")
            return response
            
        except Exception as e:
            error_msg = f"Error setting text block binding: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    # ========================================================================
    # NEW UMG WIDGET TOOLS
    # ========================================================================

    @mcp.tool()
    def add_border_to_widget(
        ctx: Context,
        widget_name: str,
        border_name: str,
        background_color: List[float] = [0.1, 0.1, 0.1, 0.8],
        padding: float = 10.0,
        position: List[float] = [0.0, 0.0],
        size: List[float] = [200.0, 100.0]
    ) -> Dict[str, Any]:
        """
        Add a Border widget to a UMG Widget Blueprint for panel backgrounds.
        
        Args:
            widget_name: Name of the target Widget Blueprint
            border_name: Name to give the new Border
            background_color: [R, G, B, A] background color values (0.0 to 1.0)
            padding: Padding inside the border
            position: [X, Y] position in the canvas panel
            size: [Width, Height] of the border
            
        Returns:
            Dict containing success status and border properties
        """
        from unreal_mcp_server import get_unreal_connection
        
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            
            params = {
                "blueprint_name": widget_name,
                "widget_name": border_name,
                "background_color": background_color,
                "padding": padding,
                "position": position,
                "size": size
            }
            
            response = unreal.send_command("add_border_to_widget", params)
            return response if response else {"success": False, "message": "No response"}
            
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def add_image_to_widget(
        ctx: Context,
        widget_name: str,
        image_name: str,
        tint: List[float] = [1.0, 1.0, 1.0, 1.0],
        position: List[float] = [0.0, 0.0],
        size: List[float] = [64.0, 64.0]
    ) -> Dict[str, Any]:
        """
        Add an Image widget to a UMG Widget Blueprint (for avatars, icons, etc.).
        
        Args:
            widget_name: Name of the target Widget Blueprint
            image_name: Name to give the new Image
            tint: [R, G, B, A] tint color values (0.0 to 1.0)
            position: [X, Y] position in the canvas panel
            size: [Width, Height] of the image
            
        Returns:
            Dict containing success status and image properties
        """
        from unreal_mcp_server import get_unreal_connection
        
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            
            params = {
                "blueprint_name": widget_name,
                "widget_name": image_name,
                "tint": tint,
                "position": position,
                "size": size
            }
            
            response = unreal.send_command("add_image_to_widget", params)
            return response if response else {"success": False, "message": "No response"}
            
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def add_vertical_box_to_widget(
        ctx: Context,
        widget_name: str,
        box_name: str,
        position: List[float] = [0.0, 0.0],
        size: List[float] = [200.0, 300.0]
    ) -> Dict[str, Any]:
        """
        Add a VerticalBox layout widget to a UMG Widget Blueprint.
        
        Args:
            widget_name: Name of the target Widget Blueprint
            box_name: Name to give the new VerticalBox
            position: [X, Y] position in the canvas panel
            size: [Width, Height] of the box
            
        Returns:
            Dict containing success status
        """
        from unreal_mcp_server import get_unreal_connection
        
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            
            params = {
                "blueprint_name": widget_name,
                "widget_name": box_name,
                "position": position,
                "size": size
            }
            
            response = unreal.send_command("add_vertical_box_to_widget", params)
            return response if response else {"success": False, "message": "No response"}
            
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def add_horizontal_box_to_widget(
        ctx: Context,
        widget_name: str,
        box_name: str,
        position: List[float] = [0.0, 0.0],
        size: List[float] = [400.0, 50.0]
    ) -> Dict[str, Any]:
        """
        Add a HorizontalBox layout widget to a UMG Widget Blueprint.
        
        Args:
            widget_name: Name of the target Widget Blueprint
            box_name: Name to give the new HorizontalBox
            position: [X, Y] position in the canvas panel
            size: [Width, Height] of the box
            
        Returns:
            Dict containing success status
        """
        from unreal_mcp_server import get_unreal_connection
        
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            
            params = {
                "blueprint_name": widget_name,
                "widget_name": box_name,
                "position": position,
                "size": size
            }
            
            response = unreal.send_command("add_horizontal_box_to_widget", params)
            return response if response else {"success": False, "message": "No response"}
            
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def add_size_box_to_widget(
        ctx: Context,
        widget_name: str,
        box_name: str,
        width_override: float = 100.0,
        height_override: float = 100.0,
        position: List[float] = [0.0, 0.0]
    ) -> Dict[str, Any]:
        """
        Add a SizeBox widget with fixed dimensions to a UMG Widget Blueprint.
        
        Args:
            widget_name: Name of the target Widget Blueprint
            box_name: Name to give the new SizeBox
            width_override: Fixed width override
            height_override: Fixed height override
            position: [X, Y] position in the canvas panel
            
        Returns:
            Dict containing success status
        """
        from unreal_mcp_server import get_unreal_connection
        
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            
            params = {
                "blueprint_name": widget_name,
                "widget_name": box_name,
                "width_override": width_override,
                "height_override": height_override,
                "position": position
            }
            
            response = unreal.send_command("add_size_box_to_widget", params)
            return response if response else {"success": False, "message": "No response"}
            
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def set_widget_slot_properties(
        ctx: Context,
        widget_name: str,
        target_widget: str,
        position: List[float] = None,
        size: List[float] = None,
        anchors: List[float] = None,
        alignment: List[float] = None,
        auto_size: bool = None
    ) -> Dict[str, Any]:
        """
        Set slot properties for a widget (position, size, anchors, alignment).
        
        Args:
            widget_name: Name of the target Widget Blueprint
            target_widget: Name of the widget to modify
            position: [X, Y] position in the canvas panel
            size: [Width, Height] of the widget
            anchors: [MinX, MinY, MaxX, MaxY] anchors (0.0 to 1.0)
            alignment: [X, Y] alignment pivot (0.0 to 1.0)
            auto_size: Whether to auto-size the widget
            
        Returns:
            Dict containing success status
        """
        from unreal_mcp_server import get_unreal_connection
        
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            
            params = {
                "blueprint_name": widget_name,
                "widget_name": target_widget
            }
            
            if position is not None:
                params["position"] = position
            if size is not None:
                params["size"] = size
            if anchors is not None:
                params["anchors"] = anchors
            if alignment is not None:
                params["alignment"] = alignment
            if auto_size is not None:
                params["auto_size"] = auto_size
            
            response = unreal.send_command("set_widget_slot_properties", params)
            return response if response else {"success": False, "message": "No response"}
            
        except Exception as e:
            return {"success": False, "message": str(e)}

    # ========================================================================
    # PHASE 1: WIDGET PARENTING
    # ========================================================================

    @mcp.tool()
    def add_widget_to_parent(
        ctx: Context,
        widget_name: str,
        parent_widget: str,
        widget_type: str,
        new_widget_name: str
    ) -> Dict[str, Any]:
        """
        Add a widget as child of another widget (not just root canvas).
        
        Args:
            widget_name: Name of the target Widget Blueprint
            parent_widget: Name of the parent widget to add to
            widget_type: Type of widget (TextBlock, Border, Image, Button, VerticalBox, HorizontalBox, SizeBox)
            new_widget_name: Name for the new widget
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect"}
            response = unreal.send_command("add_widget_to_parent", {
                "blueprint_name": widget_name,
                "parent_widget": parent_widget,
                "widget_type": widget_type,
                "widget_name": new_widget_name
            })
            return response if response else {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def set_widget_parent(
        ctx: Context,
        widget_name: str,
        target_widget: str,
        new_parent: str
    ) -> Dict[str, Any]:
        """Reparent an existing widget to a new parent."""
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect"}
            response = unreal.send_command("set_widget_parent", {
                "blueprint_name": widget_name,
                "widget_name": target_widget,
                "new_parent": new_parent
            })
            return response if response else {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def get_widget_hierarchy(
        ctx: Context,
        widget_name: str
    ) -> Dict[str, Any]:
        """Get the full widget hierarchy tree."""
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect"}
            response = unreal.send_command("get_widget_hierarchy", {
                "blueprint_name": widget_name
            })
            return response if response else {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": str(e)}

    # ========================================================================
    # PHASE 2: TEXT STYLING
    # ========================================================================

    @mcp.tool()
    def set_text_block_style(
        ctx: Context,
        widget_name: str,
        text_block_name: str,
        text: str = None,
        font_size: float = None,
        font_path: str = None,
        color: List[float] = None,
        shadow_offset: List[float] = None,
        shadow_color: List[float] = None,
        justification: str = None
    ) -> Dict[str, Any]:
        """
        Set full text styling (font, shadow, justification).
        
        Args:
            widget_name: Name of the Widget Blueprint
            text_block_name: Name of the TextBlock widget
            text: Text content to display
            font_size: Font size in points
            font_path: Path to font asset (e.g., /Game/Fonts/Fredoka)
            color: [R, G, B, A] text color
            shadow_offset: [X, Y] shadow offset
            shadow_color: [R, G, B, A] shadow color
            justification: Left, Center, or Right
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect"}
            params = {"blueprint_name": widget_name, "widget_name": text_block_name}
            if text is not None: params["text"] = text
            if font_size is not None: params["font_size"] = font_size
            if font_path is not None: params["font_path"] = font_path
            if color is not None: params["color"] = color
            if shadow_offset is not None: params["shadow_offset"] = shadow_offset
            if shadow_color is not None: params["shadow_color"] = shadow_color
            if justification is not None: params["justification"] = justification
            response = unreal.send_command("set_text_block_style", params)
            return response if response else {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": str(e)}

    # ========================================================================
    # PHASE 3: BORDER STYLING
    # ========================================================================

    @mcp.tool()
    def set_border_style(
        ctx: Context,
        widget_name: str,
        border_name: str,
        background_color: List[float] = None,
        padding: List[float] = None,
        corner_radius: float = None
    ) -> Dict[str, Any]:
        """
        Set border styling (background color, padding, corner radius).
        
        Args:
            widget_name: Name of the Widget Blueprint
            border_name: Name of the Border widget
            background_color: [R, G, B, A] background color
            padding: [Left, Top, Right, Bottom] or single value for all sides
            corner_radius: Corner radius for rounded box mode
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect"}
            params = {"blueprint_name": widget_name, "widget_name": border_name}
            if background_color is not None: params["background_color"] = background_color
            if padding is not None: params["padding"] = padding
            if corner_radius is not None: params["corner_radius"] = corner_radius
            response = unreal.send_command("set_border_style", params)
            return response if response else {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": str(e)}

    # ========================================================================
    # PHASE 4: LAYOUT SLOTS
    # ========================================================================

    @mcp.tool()
    def set_vertical_box_slot(
        ctx: Context,
        widget_name: str,
        child_widget: str,
        horizontal_alignment: str = None,
        vertical_alignment: str = None,
        padding: List[float] = None
    ) -> Dict[str, Any]:
        """Set VerticalBox slot properties (alignment, padding)."""
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect"}
            params = {"blueprint_name": widget_name, "widget_name": child_widget}
            if horizontal_alignment: params["horizontal_alignment"] = horizontal_alignment
            if vertical_alignment: params["vertical_alignment"] = vertical_alignment
            if padding: params["padding"] = padding
            response = unreal.send_command("set_vertical_box_slot", params)
            return response if response else {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def set_horizontal_box_slot(
        ctx: Context,
        widget_name: str,
        child_widget: str,
        horizontal_alignment: str = None,
        vertical_alignment: str = None,
        padding: List[float] = None
    ) -> Dict[str, Any]:
        """Set HorizontalBox slot properties (alignment, padding)."""
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect"}
            params = {"blueprint_name": widget_name, "widget_name": child_widget}
            if horizontal_alignment: params["horizontal_alignment"] = horizontal_alignment
            if vertical_alignment: params["vertical_alignment"] = vertical_alignment
            if padding: params["padding"] = padding
            response = unreal.send_command("set_horizontal_box_slot", params)
            return response if response else {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": str(e)}

    # ========================================================================
    # PHASE 5-6: VISIBILITY & PROPERTIES
    # ========================================================================

    @mcp.tool()
    def set_widget_visibility(
        ctx: Context,
        widget_name: str,
        target_widget: str,
        visibility: str
    ) -> Dict[str, Any]:
        """
        Set widget visibility.
        
        Args:
            visibility: Visible, Hidden, Collapsed, HitTestInvisible, SelfHitTestInvisible
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect"}
            response = unreal.send_command("set_widget_visibility", {
                "blueprint_name": widget_name,
                "widget_name": target_widget,
                "visibility": visibility
            })
            return response if response else {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def set_widget_enabled(
        ctx: Context,
        widget_name: str,
        target_widget: str,
        is_enabled: bool
    ) -> Dict[str, Any]:
        """Set widget enabled state."""
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect"}
            response = unreal.send_command("set_widget_enabled", {
                "blueprint_name": widget_name,
                "widget_name": target_widget,
                "is_enabled": is_enabled
            })
            return response if response else {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def set_widget_opacity(
        ctx: Context,
        widget_name: str,
        target_widget: str,
        opacity: float
    ) -> Dict[str, Any]:
        """Set widget render opacity (0.0 to 1.0)."""
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect"}
            response = unreal.send_command("set_widget_opacity", {
                "blueprint_name": widget_name,
                "widget_name": target_widget,
                "opacity": opacity
            })
            return response if response else {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": str(e)}

    # ========================================================================
    # PHASE 7: BUTTON STYLING
    # ========================================================================

    @mcp.tool()
    def set_button_style(
        ctx: Context,
        widget_name: str,
        button_name: str,
        normal_color: List[float] = None,
        hovered_color: List[float] = None,
        pressed_color: List[float] = None
    ) -> Dict[str, Any]:
        """Set button style colors for normal/hover/pressed states."""
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect"}
            params = {"blueprint_name": widget_name, "widget_name": button_name}
            if normal_color: params["normal_color"] = normal_color
            if hovered_color: params["hovered_color"] = hovered_color
            if pressed_color: params["pressed_color"] = pressed_color
            response = unreal.send_command("set_button_style", params)
            return response if response else {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": str(e)}

    # ========================================================================
    # PHASE 8: IMAGE
    # ========================================================================

    @mcp.tool()
    def set_image_brush(
        ctx: Context,
        widget_name: str,
        image_name: str,
        texture_path: str = None,
        tint: List[float] = None
    ) -> Dict[str, Any]:
        """Set image brush from texture path and tint."""
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect"}
            params = {"blueprint_name": widget_name, "widget_name": image_name}
            if texture_path: params["texture_path"] = texture_path
            if tint: params["tint"] = tint
            response = unreal.send_command("set_image_brush", params)
            return response if response else {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": str(e)}

    # ========================================================================
    # PHASE 11: WIDGET VARIABLES
    # ========================================================================

    @mcp.tool()
    def expose_widget_as_variable(
        ctx: Context,
        widget_name: str,
        target_widget: str
    ) -> Dict[str, Any]:
        """Expose widget as blueprint variable for C++ binding."""
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect"}
            response = unreal.send_command("expose_widget_as_variable", {
                "blueprint_name": widget_name,
                "widget_name": target_widget
            })
            return response if response else {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": str(e)}

    # ========================================================================
    # PHASE 12: UTILITIES
    # ========================================================================

    @mcp.tool()
    def delete_widget(
        ctx: Context,
        widget_name: str,
        target_widget: str
    ) -> Dict[str, Any]:
        """Delete a widget from the tree."""
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect"}
            response = unreal.send_command("delete_widget", {
                "blueprint_name": widget_name,
                "widget_name": target_widget
            })
            return response if response else {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def rename_widget(
        ctx: Context,
        widget_name: str,
        target_widget: str,
        new_name: str
    ) -> Dict[str, Any]:
        """Rename a widget."""
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect"}
            response = unreal.send_command("rename_widget", {
                "blueprint_name": widget_name,
                "widget_name": target_widget,
                "new_name": new_name
            })
            return response if response else {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def get_widget_properties(
        ctx: Context,
        widget_name: str,
        target_widget: str
    ) -> Dict[str, Any]:
        """Get all properties of a widget."""
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect"}
            response = unreal.send_command("get_widget_properties", {
                "blueprint_name": widget_name,
                "widget_name": target_widget
            })
            return response if response else {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": str(e)}

    logger.info("Widget core tools registered")


def register_widget_commonui_tools(mcp: FastMCP):
    """Register Common UI widget tools: CommonUI text block, button, config."""

    @mcp.tool()
    def add_common_text_block(
        ctx: Context,
        widget_name: str,
        text_block_name: str,
        text: str = "",
        position: List[float] = [0.0, 0.0],
        size: List[float] = [200.0, 50.0]
    ) -> Dict[str, Any]:
        """
        Add a Common Text Block widget (from Common UI plugin) to a UMG Widget Blueprint.

        Args:
            widget_name: Name of the target Widget Blueprint
            text_block_name: Name to give the new Common Text Block
            text: Initial text content
            position: [X, Y] position in the canvas panel
            size: [Width, Height] of the text block

        Returns:
            Dict containing success status and widget properties
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            params = {
                "widget_name": widget_name,
                "text_block_name": text_block_name,
                "text": text,
                "position": position,
                "size": size
            }

            response = unreal.send_command("add_common_text_block", params)
            return response if response else {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def add_widget_blueprint(
        ctx: Context,
        widget_name: str,
        parent_widget: str,
        instance_name: str,
        blueprint_path: str,
        position: List[float] = [0.0, 0.0],
        size: List[float] = [200.0, 100.0]
    ) -> Dict[str, Any]:
        """
        Add an instance of another Widget Blueprint to a UMG Widget Blueprint.

        This is useful for embedding reusable widget components like custom buttons,
        player entry widgets, or any other Widget Blueprint asset.

        Args:
            widget_name: Name of the target Widget Blueprint to add the instance to
            parent_widget: Name of the parent panel widget to add the instance to
            instance_name: Name to give the widget instance in the hierarchy
            blueprint_path: Content path to the Widget Blueprint to instantiate
                           (e.g., "/Game/UI/WBP_CommonButton")
            position: [X, Y] position (only used if parent is CanvasPanel)
            size: [Width, Height] of the widget instance (only used if parent is CanvasPanel)

        Returns:
            Dict containing success status and widget properties
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            params = {
                "widget_name": widget_name,
                "parent_widget": parent_widget,
                "instance_name": instance_name,
                "blueprint_path": blueprint_path,
                "position": position,
                "size": size
            }

            response = unreal.send_command("add_widget_blueprint", params)
            return response if response else {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def add_common_button(
        ctx: Context,
        widget_name: str,
        parent_widget: str,
        button_name: str,
        position: List[float] = [0.0, 0.0],
        size: List[float] = [200.0, 50.0]
    ) -> Dict[str, Any]:
        """
        Add a Common Button widget to a UMG Widget Blueprint.

        Uses the configured Common Button blueprint path. Configure the path
        with set_common_ui_config first if using a custom button widget.

        Args:
            widget_name: Name of the target Widget Blueprint
            parent_widget: Name of the parent panel widget to add the button to
            button_name: Name to give the button instance
            position: [X, Y] position (only used if parent is CanvasPanel)
            size: [Width, Height] of the button (only used if parent is CanvasPanel)

        Returns:
            Dict containing success status and widget properties
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            params = {
                "widget_name": widget_name,
                "parent_widget": parent_widget,
                "instance_name": button_name,
                "position": position,
                "size": size
            }

            response = unreal.send_command("add_common_button", params)
            return response if response else {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def set_common_ui_config(
        ctx: Context,
        type_name: str = "",
        blueprint_path: str = ""
    ) -> Dict[str, Any]:
        """
        Configure Common UI widget paths for the MCP session.

        Args:
            type_name: The type identifier (e.g., "CommonButton", "CommonTextBlock")
            blueprint_path: Content path to the Widget Blueprint
                           (e.g., "/Game/Widgets/WBP_CommonButton")

        Returns:
            Dict containing success status
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            if not type_name or not blueprint_path:
                return {"success": False, "message": "Both type_name and blueprint_path are required"}

            params = {
                "type_name": type_name,
                "blueprint_path": blueprint_path
            }

            response = unreal.send_command("set_common_ui_config", params)
            return response if response else {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def get_common_ui_config(ctx: Context) -> Dict[str, Any]:
        """
        Get the current Common UI configuration.

        Returns:
            Dict containing the configured Common UI widget paths
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("get_common_ui_config", {})
            return response if response else {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": str(e)}

    logger.info("Widget Common UI tools registered")


def register_widget_batch_tools(mcp: FastMCP):
    """Register widget batch operation tools: begin/end edit, execute_batch."""

    @mcp.tool()
    def begin_widget_edit(
        ctx: Context,
        widget_name: str
    ) -> Dict[str, Any]:
        """
        Begin editing a widget (deferred compilation).

        Use this before making multiple changes to avoid recompiling after each change.
        Call end_widget_edit when done to compile and save.

        Args:
            widget_name: Name of the Widget Blueprint to begin editing
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect"}
            response = unreal.send_command("begin_widget_edit", {
                "widget_name": widget_name
            })
            return response if response else {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def end_widget_edit(
        ctx: Context,
        widget_name: str
    ) -> Dict[str, Any]:
        """
        End editing a widget (compile and save).

        Call this after begin_widget_edit and making changes to compile and save.

        Args:
            widget_name: Name of the Widget Blueprint to finish editing
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect"}
            response = unreal.send_command("end_widget_edit", {
                "widget_name": widget_name
            })
            return response if response else {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def execute_batch(
        ctx: Context,
        widget_name: str,
        operations: List[Dict[str, Any]]
    ) -> Dict[str, Any]:
        """
        Execute multiple widget operations in a single batch.

        Args:
            widget_name: Name of the Widget Blueprint
            operations: List of operation dicts, each with "command" and "params" keys

        Example:
            operations = [
                {"command": "add_text_block_to_widget", "params": {"widget_name": "MyText", "text": "Hello"}},
                {"command": "set_text_block_style", "params": {"widget_name": "MyText", "font_size": 24}}
            ]
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect"}
            response = unreal.send_command("execute_batch", {
                "blueprint_name": widget_name,
                "operations": operations
            })
            return response if response else {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": str(e)}

    logger.info("Widget batch tools registered")


def register_widget_discovery_tools(mcp: FastMCP):
    """Register widget discovery tools: find, list, move, resize, hierarchy navigation."""

    @mcp.tool()
    def clone_widget(
        ctx: Context,
        widget_name: str,
        source_widget: str,
        new_name: str
    ) -> Dict[str, Any]:
        """
        Clone a widget with a new name.

        Args:
            widget_name: Name of the Widget Blueprint
            source_widget: Name of the widget to clone
            new_name: Name for the cloned widget
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect"}
            response = unreal.send_command("clone_widget", {
                "blueprint_name": widget_name,
                "source_widget": source_widget,
                "new_name": new_name
            })
            return response if response else {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def find_widgets(
        ctx: Context,
        widget_name: str,
        widget_type: str = None,
        name_pattern: str = None
    ) -> Dict[str, Any]:
        """
        Find all widgets matching a type or name pattern.

        Args:
            widget_name: Name of the Widget Blueprint
            widget_type: Type of widget to find (TextBlock, Button, etc.)
            name_pattern: Name pattern to match (supports * wildcard)
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect"}
            params = {"blueprint_name": widget_name}
            if widget_type:
                params["widget_type"] = widget_type
            if name_pattern:
                params["name_pattern"] = name_pattern
            response = unreal.send_command("find_widgets", params)
            return response if response else {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def get_all_widgets(
        ctx: Context,
        widget_name: str
    ) -> Dict[str, Any]:
        """
        Get all widgets in the blueprint.

        Args:
            widget_name: Name of the Widget Blueprint
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect"}
            response = unreal.send_command("get_all_widgets", {
                "blueprint_name": widget_name
            })
            return response if response else {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def move_widget(
        ctx: Context,
        widget_name: str,
        target_widget: str,
        position: List[float]
    ) -> Dict[str, Any]:
        """
        Move widget position.

        Args:
            widget_name: Name of the Widget Blueprint
            target_widget: Name of the widget to move
            position: [X, Y] new position
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect"}
            response = unreal.send_command("move_widget", {
                "widget_name": widget_name,
                "target_widget": target_widget,
                "position": position
            })
            return response if response else {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def resize_widget(
        ctx: Context,
        widget_name: str,
        target_widget: str,
        size: List[float]
    ) -> Dict[str, Any]:
        """
        Resize a widget.

        Args:
            widget_name: Name of the Widget Blueprint
            target_widget: Name of the widget to resize
            size: [Width, Height] new size
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect"}
            response = unreal.send_command("resize_widget", {
                "blueprint_name": widget_name,
                "widget_name": target_widget,
                "size": size
            })
            return response if response else {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def get_widget_bounds(
        ctx: Context,
        widget_name: str,
        target_widget: str
    ) -> Dict[str, Any]:
        """
        Get widget position and size bounds.

        Args:
            widget_name: Name of the Widget Blueprint
            target_widget: Name of the widget
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect"}
            response = unreal.send_command("get_widget_bounds", {
                "blueprint_name": widget_name,
                "widget_name": target_widget
            })
            return response if response else {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def get_parent(
        ctx: Context,
        widget_name: str,
        target_widget: str
    ) -> Dict[str, Any]:
        """
        Get parent widget.

        Args:
            widget_name: Name of the Widget Blueprint
            target_widget: Name of the widget
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect"}
            response = unreal.send_command("get_parent", {
                "blueprint_name": widget_name,
                "widget_name": target_widget
            })
            return response if response else {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def get_children(
        ctx: Context,
        widget_name: str,
        target_widget: str
    ) -> Dict[str, Any]:
        """
        Get children widgets.

        Args:
            widget_name: Name of the Widget Blueprint
            target_widget: Name of the parent widget
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect"}
            response = unreal.send_command("get_children", {
                "blueprint_name": widget_name,
                "widget_name": target_widget
            })
            return response if response else {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def swap_widgets(
        ctx: Context,
        widget_name: str,
        widget_a: str,
        widget_b: str
    ) -> Dict[str, Any]:
        """
        Swap positions of two widgets.

        Args:
            widget_name: Name of the Widget Blueprint
            widget_a: Name of first widget
            widget_b: Name of second widget
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect"}
            response = unreal.send_command("swap_widgets", {
                "blueprint_name": widget_name,
                "widget_a": widget_a,
                "widget_b": widget_b
            })
            return response if response else {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": str(e)}

    logger.info("Widget discovery tools registered")


def register_widget_style_tools(mcp: FastMCP):
    """Register widget style tools: presets, bulk styling, style queries."""

    @mcp.tool()
    def create_style_preset(
        ctx: Context,
        preset_name: str,
        widget_type: str,
        properties: Dict[str, Any]
    ) -> Dict[str, Any]:
        """
        Create a reusable style preset.

        Args:
            preset_name: Name for the preset
            widget_type: Type of widget this preset applies to (TextBlock, Button, etc.)
            properties: Dict of properties to set (font_size, color, etc.)
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect"}
            response = unreal.send_command("create_style_preset", {
                "preset_name": preset_name,
                "widget_type": widget_type,
                "properties": properties
            })
            return response if response else {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def apply_preset(
        ctx: Context,
        widget_name: str,
        target_widget: str,
        preset_name: str
    ) -> Dict[str, Any]:
        """
        Apply a saved style preset to a widget.

        Args:
            widget_name: Name of the Widget Blueprint
            target_widget: Name of the widget to style
            preset_name: Name of the preset to apply
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect"}
            response = unreal.send_command("apply_preset", {
                "blueprint_name": widget_name,
                "widget_name": target_widget,
                "preset_name": preset_name
            })
            return response if response else {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def list_presets(
        ctx: Context,
        widget_type: str = None
    ) -> Dict[str, Any]:
        """
        List all available style presets.

        Args:
            widget_type: Optional filter by widget type
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect"}
            params = {}
            if widget_type:
                params["widget_type"] = widget_type
            response = unreal.send_command("list_presets", params)
            return response if response else {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def style_query(
        ctx: Context,
        widget_name: str,
        selector: str,
        properties: Dict[str, Any]
    ) -> Dict[str, Any]:
        """
        Query widgets with CSS-like selectors and apply styles.

        Args:
            widget_name: Name of the Widget Blueprint
            selector: CSS-like selector (e.g., "TextBlock", ".MyClass", "#MyId")
            properties: Properties to apply to matching widgets
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect"}
            response = unreal.send_command("style_query", {
                "blueprint_name": widget_name,
                "selector": selector,
                "properties": properties
            })
            return response if response else {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def apply_bulk_style(
        ctx: Context,
        widget_name: str,
        selector: str,
        properties: Dict[str, Any]
    ) -> Dict[str, Any]:
        """
        Apply style to multiple widgets matching a selector.

        Args:
            widget_name: Name of the Widget Blueprint
            selector: Pattern to match widget names (supports * wildcard)
            properties: Properties to apply
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect"}
            response = unreal.send_command("apply_bulk_style", {
                "blueprint_name": widget_name,
                "selector": selector,
                "properties": properties
            })
            return response if response else {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": str(e)}

    logger.info("Widget style tools registered")


def register_widget_input_tools(mcp: FastMCP):
    """Register widget input tools: progress bar, slider, checkbox, combo box, editable text, scroll box, grid, overlay, spacer, switcher, throbber, scale box, wrap box."""

    @mcp.tool()
    def add_progress_bar_to_widget(
        ctx: Context,
        widget_name: str,
        bar_name: str,
        percent: float = 0.5,
        fill_color: List[float] = None,
        position: List[float] = [0.0, 0.0],
        size: List[float] = [200.0, 20.0]
    ) -> Dict[str, Any]:
        """
        Add a ProgressBar widget to a UMG Widget Blueprint.

        Args:
            widget_name: Name of the target Widget Blueprint
            bar_name: Name to give the new ProgressBar
            percent: Initial fill percent (0.0 to 1.0)
            fill_color: [R, G, B, A] fill color values (0.0 to 1.0)
            position: [X, Y] position in the canvas panel
            size: [Width, Height] of the progress bar
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect"}
            params = {
                "blueprint_name": widget_name,
                "widget_name": bar_name,
                "percent": percent,
                "position": position,
                "size": size
            }
            if fill_color:
                params["fill_color"] = fill_color
            response = unreal.send_command("add_progress_bar_to_widget", params)
            return response if response else {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def set_progress_bar_percent(
        ctx: Context,
        widget_name: str,
        bar_name: str,
        percent: float
    ) -> Dict[str, Any]:
        """Set ProgressBar percent value (0.0 to 1.0)."""
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect"}
            response = unreal.send_command("set_progress_bar_percent", {
                "blueprint_name": widget_name,
                "widget_name": bar_name,
                "percent": percent
            })
            return response if response else {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def set_progress_bar_style(
        ctx: Context,
        widget_name: str,
        bar_name: str,
        fill_color: List[float] = None
    ) -> Dict[str, Any]:
        """Set ProgressBar style (fill color)."""
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect"}
            params = {"blueprint_name": widget_name, "widget_name": bar_name}
            if fill_color:
                params["fill_color"] = fill_color
            response = unreal.send_command("set_progress_bar_style", params)
            return response if response else {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def add_slider_to_widget(
        ctx: Context,
        widget_name: str,
        slider_name: str,
        value: float = 0.5,
        min_value: float = 0.0,
        max_value: float = 1.0,
        position: List[float] = [0.0, 0.0],
        size: List[float] = [200.0, 20.0]
    ) -> Dict[str, Any]:
        """
        Add a Slider widget to a UMG Widget Blueprint.

        Args:
            widget_name: Name of the target Widget Blueprint
            slider_name: Name to give the new Slider
            value: Initial value
            min_value: Minimum slider value
            max_value: Maximum slider value
            position: [X, Y] position in the canvas panel
            size: [Width, Height] of the slider
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect"}
            response = unreal.send_command("add_slider_to_widget", {
                "blueprint_name": widget_name,
                "widget_name": slider_name,
                "value": value,
                "min_value": min_value,
                "max_value": max_value,
                "position": position,
                "size": size
            })
            return response if response else {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def set_slider_value(
        ctx: Context,
        widget_name: str,
        slider_name: str,
        value: float
    ) -> Dict[str, Any]:
        """Set Slider value."""
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect"}
            response = unreal.send_command("set_slider_value", {
                "blueprint_name": widget_name,
                "widget_name": slider_name,
                "value": value
            })
            return response if response else {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def set_slider_range(
        ctx: Context,
        widget_name: str,
        slider_name: str,
        min_value: float = None,
        max_value: float = None
    ) -> Dict[str, Any]:
        """Set Slider range (min/max)."""
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect"}
            params = {"blueprint_name": widget_name, "widget_name": slider_name}
            if min_value is not None:
                params["min_value"] = min_value
            if max_value is not None:
                params["max_value"] = max_value
            response = unreal.send_command("set_slider_range", params)
            return response if response else {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def add_checkbox_to_widget(
        ctx: Context,
        widget_name: str,
        checkbox_name: str,
        is_checked: bool = False,
        position: List[float] = [0.0, 0.0]
    ) -> Dict[str, Any]:
        """
        Add a CheckBox widget to a UMG Widget Blueprint.

        Args:
            widget_name: Name of the target Widget Blueprint
            checkbox_name: Name to give the new CheckBox
            is_checked: Initial checked state
            position: [X, Y] position in the canvas panel
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect"}
            response = unreal.send_command("add_checkbox_to_widget", {
                "blueprint_name": widget_name,
                "widget_name": checkbox_name,
                "is_checked": is_checked,
                "position": position
            })
            return response if response else {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def set_checkbox_state(
        ctx: Context,
        widget_name: str,
        checkbox_name: str,
        is_checked: bool
    ) -> Dict[str, Any]:
        """Set CheckBox checked state."""
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect"}
            response = unreal.send_command("set_checkbox_state", {
                "blueprint_name": widget_name,
                "widget_name": checkbox_name,
                "is_checked": is_checked
            })
            return response if response else {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def add_combo_box_to_widget(
        ctx: Context,
        widget_name: str,
        combo_name: str,
        options: List[str] = None,
        selected_index: int = 0,
        position: List[float] = [0.0, 0.0],
        size: List[float] = [200.0, 30.0]
    ) -> Dict[str, Any]:
        """
        Add a ComboBox (dropdown) widget to a UMG Widget Blueprint.

        Args:
            widget_name: Name of the target Widget Blueprint
            combo_name: Name to give the new ComboBox
            options: List of string options
            selected_index: Initially selected index
            position: [X, Y] position in the canvas panel
            size: [Width, Height] of the combo box
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect"}
            params = {
                "blueprint_name": widget_name,
                "widget_name": combo_name,
                "selected_index": selected_index,
                "position": position,
                "size": size
            }
            if options:
                params["options"] = options
            response = unreal.send_command("add_combo_box_to_widget", params)
            return response if response else {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def set_combo_box_options(
        ctx: Context,
        widget_name: str,
        combo_name: str,
        options: List[str]
    ) -> Dict[str, Any]:
        """Set ComboBox options (clears existing and adds new)."""
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect"}
            response = unreal.send_command("set_combo_box_options", {
                "blueprint_name": widget_name,
                "widget_name": combo_name,
                "options": options
            })
            return response if response else {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def set_combo_box_selected(
        ctx: Context,
        widget_name: str,
        combo_name: str,
        selected_index: int
    ) -> Dict[str, Any]:
        """Set ComboBox selected index."""
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect"}
            response = unreal.send_command("set_combo_box_selected", {
                "blueprint_name": widget_name,
                "widget_name": combo_name,
                "selected_index": selected_index
            })
            return response if response else {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def add_editable_text_to_widget(
        ctx: Context,
        widget_name: str,
        text_name: str,
        text: str = "",
        hint_text: str = "",
        position: List[float] = [0.0, 0.0],
        size: List[float] = [200.0, 30.0]
    ) -> Dict[str, Any]:
        """
        Add an EditableTextBox widget to a UMG Widget Blueprint.

        Args:
            widget_name: Name of the target Widget Blueprint
            text_name: Name to give the new EditableTextBox
            text: Initial text content
            hint_text: Placeholder text when empty
            position: [X, Y] position in the canvas panel
            size: [Width, Height] of the text box
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect"}
            response = unreal.send_command("add_editable_text_to_widget", {
                "blueprint_name": widget_name,
                "widget_name": text_name,
                "text": text,
                "hint_text": hint_text,
                "position": position,
                "size": size
            })
            return response if response else {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def set_editable_text_value(
        ctx: Context,
        widget_name: str,
        text_name: str,
        text: str
    ) -> Dict[str, Any]:
        """Set EditableTextBox text value."""
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect"}
            response = unreal.send_command("set_editable_text_value", {
                "blueprint_name": widget_name,
                "widget_name": text_name,
                "text": text
            })
            return response if response else {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def add_scroll_box_to_widget(
        ctx: Context,
        widget_name: str,
        scroll_name: str,
        orientation: str = "Vertical",
        position: List[float] = [0.0, 0.0],
        size: List[float] = [200.0, 300.0]
    ) -> Dict[str, Any]:
        """
        Add a ScrollBox widget to a UMG Widget Blueprint.

        Args:
            widget_name: Name of the target Widget Blueprint
            scroll_name: Name to give the new ScrollBox
            orientation: "Vertical" or "Horizontal"
            position: [X, Y] position in the canvas panel
            size: [Width, Height] of the scroll box
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect"}
            response = unreal.send_command("add_scroll_box_to_widget", {
                "blueprint_name": widget_name,
                "widget_name": scroll_name,
                "orientation": orientation,
                "position": position,
                "size": size
            })
            return response if response else {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def set_scroll_box_offset(
        ctx: Context,
        widget_name: str,
        scroll_name: str,
        offset: float
    ) -> Dict[str, Any]:
        """Set ScrollBox scroll offset."""
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect"}
            response = unreal.send_command("set_scroll_box_offset", {
                "blueprint_name": widget_name,
                "widget_name": scroll_name,
                "offset": offset
            })
            return response if response else {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def add_overlay_to_widget(
        ctx: Context,
        widget_name: str,
        overlay_name: str,
        position: List[float] = [0.0, 0.0],
        size: List[float] = [200.0, 200.0]
    ) -> Dict[str, Any]:
        """
        Add an Overlay widget (stacked children) to a UMG Widget Blueprint.

        Args:
            widget_name: Name of the target Widget Blueprint
            overlay_name: Name to give the new Overlay
            position: [X, Y] position in the canvas panel
            size: [Width, Height] of the overlay
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect"}
            response = unreal.send_command("add_overlay_to_widget", {
                "blueprint_name": widget_name,
                "widget_name": overlay_name,
                "position": position,
                "size": size
            })
            return response if response else {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def add_grid_panel_to_widget(
        ctx: Context,
        widget_name: str,
        grid_name: str,
        position: List[float] = [0.0, 0.0],
        size: List[float] = [300.0, 300.0]
    ) -> Dict[str, Any]:
        """
        Add a GridPanel widget to a UMG Widget Blueprint.

        Args:
            widget_name: Name of the target Widget Blueprint
            grid_name: Name to give the new GridPanel
            position: [X, Y] position in the canvas panel
            size: [Width, Height] of the grid panel
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect"}
            response = unreal.send_command("add_grid_panel_to_widget", {
                "blueprint_name": widget_name,
                "widget_name": grid_name,
                "position": position,
                "size": size
            })
            return response if response else {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def set_grid_slot(
        ctx: Context,
        widget_name: str,
        target_widget: str,
        row: int = 0,
        column: int = 0,
        row_span: int = 1,
        column_span: int = 1
    ) -> Dict[str, Any]:
        """
        Set GridPanel slot properties (row/column) for a child widget.

        Args:
            widget_name: Name of the Widget Blueprint
            target_widget: Name of the child widget in the grid
            row: Row index
            column: Column index
            row_span: Number of rows to span
            column_span: Number of columns to span
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect"}
            response = unreal.send_command("set_grid_slot", {
                "blueprint_name": widget_name,
                "widget_name": target_widget,
                "row": row,
                "column": column,
                "row_span": row_span,
                "column_span": column_span
            })
            return response if response else {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def add_uniform_grid_panel_to_widget(
        ctx: Context,
        widget_name: str,
        grid_name: str,
        slot_padding: float = 0.0,
        position: List[float] = [0.0, 0.0],
        size: List[float] = [300.0, 300.0]
    ) -> Dict[str, Any]:
        """
        Add a UniformGridPanel widget to a UMG Widget Blueprint.

        Args:
            widget_name: Name of the target Widget Blueprint
            grid_name: Name to give the new UniformGridPanel
            slot_padding: Padding between grid cells
            position: [X, Y] position in the canvas panel
            size: [Width, Height] of the grid panel
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect"}
            response = unreal.send_command("add_uniform_grid_panel_to_widget", {
                "blueprint_name": widget_name,
                "widget_name": grid_name,
                "slot_padding": slot_padding,
                "position": position,
                "size": size
            })
            return response if response else {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def add_spacer_to_widget(
        ctx: Context,
        widget_name: str,
        spacer_name: str,
        size: List[float] = [100.0, 100.0],
        position: List[float] = [0.0, 0.0]
    ) -> Dict[str, Any]:
        """
        Add a Spacer widget to a UMG Widget Blueprint.

        Args:
            widget_name: Name of the target Widget Blueprint
            spacer_name: Name to give the new Spacer
            size: [Width, Height] of the spacer
            position: [X, Y] position in the canvas panel
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect"}
            response = unreal.send_command("add_spacer_to_widget", {
                "blueprint_name": widget_name,
                "widget_name": spacer_name,
                "size": size,
                "position": position
            })
            return response if response else {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def add_widget_switcher_to_widget(
        ctx: Context,
        widget_name: str,
        switcher_name: str,
        active_index: int = 0,
        position: List[float] = [0.0, 0.0],
        size: List[float] = [200.0, 200.0]
    ) -> Dict[str, Any]:
        """
        Add a WidgetSwitcher widget to a UMG Widget Blueprint.

        Args:
            widget_name: Name of the target Widget Blueprint
            switcher_name: Name to give the new WidgetSwitcher
            active_index: Initially active child index
            position: [X, Y] position in the canvas panel
            size: [Width, Height] of the switcher
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect"}
            response = unreal.send_command("add_widget_switcher_to_widget", {
                "blueprint_name": widget_name,
                "widget_name": switcher_name,
                "active_index": active_index,
                "position": position,
                "size": size
            })
            return response if response else {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def set_active_widget_index(
        ctx: Context,
        widget_name: str,
        switcher_name: str,
        active_index: int
    ) -> Dict[str, Any]:
        """Set WidgetSwitcher active child index."""
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect"}
            response = unreal.send_command("set_active_widget_index", {
                "blueprint_name": widget_name,
                "widget_name": switcher_name,
                "active_index": active_index
            })
            return response if response else {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def add_throbber_to_widget(
        ctx: Context,
        widget_name: str,
        throbber_name: str,
        number_of_pieces: int = 6,
        position: List[float] = [0.0, 0.0]
    ) -> Dict[str, Any]:
        """
        Add a Throbber widget (loading indicator) to a UMG Widget Blueprint.

        Args:
            widget_name: Name of the target Widget Blueprint
            throbber_name: Name to give the new Throbber
            number_of_pieces: Number of animated pieces
            position: [X, Y] position in the canvas panel
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect"}
            response = unreal.send_command("add_throbber_to_widget", {
                "blueprint_name": widget_name,
                "widget_name": throbber_name,
                "number_of_pieces": number_of_pieces,
                "position": position
            })
            return response if response else {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def add_circular_throbber_to_widget(
        ctx: Context,
        widget_name: str,
        throbber_name: str,
        number_of_pieces: int = 6,
        position: List[float] = [0.0, 0.0]
    ) -> Dict[str, Any]:
        """
        Add a CircularThrobber widget to a UMG Widget Blueprint.

        Args:
            widget_name: Name of the target Widget Blueprint
            throbber_name: Name to give the new CircularThrobber
            number_of_pieces: Number of animated pieces
            position: [X, Y] position in the canvas panel
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect"}
            response = unreal.send_command("add_circular_throbber_to_widget", {
                "blueprint_name": widget_name,
                "widget_name": throbber_name,
                "number_of_pieces": number_of_pieces,
                "position": position
            })
            return response if response else {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def add_scale_box_to_widget(
        ctx: Context,
        widget_name: str,
        scale_box_name: str,
        stretch: str = "ScaleToFit",
        position: List[float] = [0.0, 0.0],
        size: List[float] = [200.0, 200.0]
    ) -> Dict[str, Any]:
        """
        Add a ScaleBox widget to a UMG Widget Blueprint.

        Args:
            widget_name: Name of the target Widget Blueprint
            scale_box_name: Name to give the new ScaleBox
            stretch: Stretch mode (None, Fill, ScaleToFit, ScaleToFill)
            position: [X, Y] position in the canvas panel
            size: [Width, Height] of the scale box
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect"}
            response = unreal.send_command("add_scale_box_to_widget", {
                "blueprint_name": widget_name,
                "widget_name": scale_box_name,
                "stretch": stretch,
                "position": position,
                "size": size
            })
            return response if response else {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def add_wrap_box_to_widget(
        ctx: Context,
        widget_name: str,
        wrap_box_name: str,
        inner_slot_padding: float = 0.0,
        position: List[float] = [0.0, 0.0],
        size: List[float] = [300.0, 200.0]
    ) -> Dict[str, Any]:
        """
        Add a WrapBox widget to a UMG Widget Blueprint.

        Args:
            widget_name: Name of the target Widget Blueprint
            wrap_box_name: Name to give the new WrapBox
            inner_slot_padding: Padding between wrapped items
            position: [X, Y] position in the canvas panel
            size: [Width, Height] of the wrap box
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect"}
            response = unreal.send_command("add_wrap_box_to_widget", {
                "blueprint_name": widget_name,
                "widget_name": wrap_box_name,
                "inner_slot_padding": inner_slot_padding,
                "position": position,
                "size": size
            })
            return response if response else {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": str(e)}

    logger.info("Widget input tools registered")


def register_widget_animation_tools(mcp: FastMCP):
    """Register widget animation tools: create, tracks, keyframes, playback control."""

    @mcp.tool()
    def create_widget_animation(
        ctx: Context,
        widget_name: str,
        animation_name: str,
        duration: float = 1.0,
        frame_rate: int = 30
    ) -> Dict[str, Any]:
        """
        Create a new widget animation in a UMG Widget Blueprint.

        Args:
            widget_name: Name of the target Widget Blueprint
            animation_name: Name for the new animation
            duration: Animation duration in seconds (default: 1.0)
            frame_rate: Animation frame rate (default: 30)

        Returns:
            Dict containing success status and animation details
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            params = {
                "blueprint_name": widget_name,
                "animation_name": animation_name,
                "duration": duration,
                "frame_rate": frame_rate
            }

            response = unreal.send_command("create_widget_animation", params)
            return response if response else {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def delete_widget_animation(
        ctx: Context,
        widget_name: str,
        animation_name: str
    ) -> Dict[str, Any]:
        """
        Delete a widget animation from a UMG Widget Blueprint.

        Args:
            widget_name: Name of the target Widget Blueprint
            animation_name: Name of the animation to delete

        Returns:
            Dict containing success status
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("delete_widget_animation", {
                "blueprint_name": widget_name,
                "animation_name": animation_name
            })
            return response if response else {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def get_widget_animations(
        ctx: Context,
        widget_name: str
    ) -> Dict[str, Any]:
        """
        Get all animations in a UMG Widget Blueprint.

        Args:
            widget_name: Name of the target Widget Blueprint

        Returns:
            Dict containing list of animations with their properties
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("get_widget_animations", {
                "blueprint_name": widget_name
            })
            return response if response else {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def add_animation_float_track(
        ctx: Context,
        widget_name: str,
        animation_name: str,
        target_widget: str,
        property_name: str
    ) -> Dict[str, Any]:
        """
        Add a float track to an animation (e.g., for opacity).

        Args:
            widget_name: Name of the Widget Blueprint containing the animation
            animation_name: Name of the animation to add the track to
            target_widget: Name of the widget to animate
            property_name: Name of the float property to animate (e.g., "RenderOpacity")

        Returns:
            Dict containing success status and track details
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("add_animation_float_track", {
                "blueprint_name": widget_name,
                "animation_name": animation_name,
                "widget_name": target_widget,
                "property_name": property_name
            })
            return response if response else {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def add_animation_color_track(
        ctx: Context,
        widget_name: str,
        animation_name: str,
        target_widget: str,
        property_name: str
    ) -> Dict[str, Any]:
        """
        Add a color track to an animation.

        Args:
            widget_name: Name of the Widget Blueprint containing the animation
            animation_name: Name of the animation to add the track to
            target_widget: Name of the widget to animate
            property_name: Name of the color property to animate (e.g., "ColorAndOpacity")

        Returns:
            Dict containing success status and track details
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("add_animation_color_track", {
                "blueprint_name": widget_name,
                "animation_name": animation_name,
                "widget_name": target_widget,
                "property_name": property_name
            })
            return response if response else {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def add_animation_transform_track(
        ctx: Context,
        widget_name: str,
        animation_name: str,
        target_widget: str
    ) -> Dict[str, Any]:
        """
        Add a transform track to an animation for RenderTransform.

        Args:
            widget_name: Name of the Widget Blueprint containing the animation
            animation_name: Name of the animation to add the track to
            target_widget: Name of the widget to animate

        Returns:
            Dict containing success status and track details
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("add_animation_transform_track", {
                "blueprint_name": widget_name,
                "animation_name": animation_name,
                "widget_name": target_widget
            })
            return response if response else {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def add_float_keyframe(
        ctx: Context,
        widget_name: str,
        animation_name: str,
        target_widget: str,
        property_name: str,
        time: float,
        value: float
    ) -> Dict[str, Any]:
        """
        Add a float keyframe to an animation track.

        Args:
            widget_name: Name of the Widget Blueprint containing the animation
            animation_name: Name of the animation
            target_widget: Name of the widget being animated
            property_name: Name of the property being animated
            time: Time in seconds for the keyframe
            value: Float value at this keyframe

        Returns:
            Dict containing success status
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("add_float_keyframe", {
                "blueprint_name": widget_name,
                "animation_name": animation_name,
                "widget_name": target_widget,
                "property_name": property_name,
                "time": time,
                "value": value
            })
            return response if response else {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def add_color_keyframe(
        ctx: Context,
        widget_name: str,
        animation_name: str,
        target_widget: str,
        property_name: str,
        time: float,
        color: List[float]
    ) -> Dict[str, Any]:
        """
        Add a color keyframe to an animation track.

        Args:
            widget_name: Name of the Widget Blueprint containing the animation
            animation_name: Name of the animation
            target_widget: Name of the widget being animated
            property_name: Name of the property being animated
            time: Time in seconds for the keyframe
            color: [R, G, B, A] color values (0.0-1.0)

        Returns:
            Dict containing success status
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("add_color_keyframe", {
                "blueprint_name": widget_name,
                "animation_name": animation_name,
                "widget_name": target_widget,
                "property_name": property_name,
                "time": time,
                "color": color
            })
            return response if response else {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def add_transform_keyframe(
        ctx: Context,
        widget_name: str,
        animation_name: str,
        target_widget: str,
        time: float,
        translation: List[float] = [0.0, 0.0],
        rotation: List[float] = [0.0],
        scale: List[float] = [1.0, 1.0]
    ) -> Dict[str, Any]:
        """
        Add a transform keyframe to an animation track.

        Args:
            widget_name: Name of the Widget Blueprint containing the animation
            animation_name: Name of the animation
            target_widget: Name of the widget being animated
            time: Time in seconds for the keyframe
            translation: [X, Y] translation offset
            rotation: [Angle] rotation in degrees (or [X, Y, Z] for 3D)
            scale: [X, Y] scale factors

        Returns:
            Dict containing success status
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("add_transform_keyframe", {
                "blueprint_name": widget_name,
                "animation_name": animation_name,
                "widget_name": target_widget,
                "time": time,
                "translation": translation,
                "rotation": rotation,
                "scale": scale
            })
            return response if response else {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def play_animation(
        ctx: Context,
        widget_name: str,
        animation_name: str,
        start_time: float = 0.0,
        num_loops: int = 1,
        playback_speed: float = 1.0,
        play_mode: str = "Forward"
    ) -> Dict[str, Any]:
        """
        Play an animation on a widget instance.

        Note: This is a runtime operation - the widget must be instantiated.

        Args:
            widget_name: Name of the Widget Blueprint
            animation_name: Name of the animation to play
            start_time: Start time in seconds (default: 0.0)
            num_loops: Number of times to loop (0 = infinite, default: 1)
            playback_speed: Playback speed multiplier (default: 1.0)
            play_mode: "Forward", "Reverse", or "PingPong" (default: "Forward")

        Returns:
            Dict containing animation playback information
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("play_animation", {
                "blueprint_name": widget_name,
                "animation_name": animation_name,
                "start_time": start_time,
                "num_loops": num_loops,
                "playback_speed": playback_speed,
                "play_mode": play_mode
            })
            return response if response else {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def pause_animation(
        ctx: Context,
        animation_name: str
    ) -> Dict[str, Any]:
        """
        Pause an animation.

        Note: This is a runtime operation.

        Args:
            animation_name: Name of the animation to pause

        Returns:
            Dict containing success status
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("pause_animation", {
                "animation_name": animation_name
            })
            return response if response else {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def stop_animation(
        ctx: Context,
        animation_name: str
    ) -> Dict[str, Any]:
        """
        Stop an animation.

        Note: This is a runtime operation.

        Args:
            animation_name: Name of the animation to stop

        Returns:
            Dict containing success status
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("stop_animation", {
                "animation_name": animation_name
            })
            return response if response else {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def set_animation_time(
        ctx: Context,
        animation_name: str,
        time: float
    ) -> Dict[str, Any]:
        """
        Set the current playback time of an animation.

        Note: This is a runtime operation.

        Args:
            animation_name: Name of the animation
            time: Time in seconds to jump to

        Returns:
            Dict containing success status
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("set_animation_time", {
                "animation_name": animation_name,
                "time": time
            })
            return response if response else {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def get_animation_state(
        ctx: Context,
        widget_name: str,
        animation_name: str
    ) -> Dict[str, Any]:
        """
        Get the current state of an animation.

        Args:
            widget_name: Name of the Widget Blueprint
            animation_name: Name of the animation

        Returns:
            Dict containing animation state information (duration, frame rate, tracks, bindings)
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("get_animation_state", {
                "blueprint_name": widget_name,
                "animation_name": animation_name
            })
            return response if response else {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": str(e)}

    logger.info("Widget animation tools registered")


def register_widget_commonui_ext_tools(mcp: FastMCP):
    """Register extended Common UI widget tools: border, activatable, list/tile/tree view, rotator, carousel, video player, etc."""

    @mcp.tool()
    def add_common_border(
        ctx: Context,
        widget_name: str,
        parent_widget: str,
        border_name: str,
        position: List[float] = [0.0, 0.0],
        size: List[float] = [200.0, 200.0]
    ) -> Dict[str, Any]:
        """
        Add a Common Border widget (from Common UI plugin) to a UMG Widget Blueprint.

        Args:
            widget_name: Name of the target Widget Blueprint
            parent_widget: Name of the parent panel widget
            border_name: Name to give the new Common Border
            position: [X, Y] position (if parent is CanvasPanel)
            size: [Width, Height] of the border

        Returns:
            Dict containing success status and widget properties
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("add_common_border", {
                "widget_name": widget_name,
                "border_name": border_name,
                "position": position,
                "size": size
            })
            return response if response else {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def add_common_activatable_widget(
        ctx: Context,
        widget_name: str,
        parent_widget: str,
        instance_name: str,
        position: List[float] = [0.0, 0.0],
        size: List[float] = [400.0, 300.0]
    ) -> Dict[str, Any]:
        """
        Add a Common Activatable Widget (from Common UI plugin) to a UMG Widget Blueprint.

        Activatable widgets integrate with Common UI's activation stack system.

        Args:
            widget_name: Name of the target Widget Blueprint
            parent_widget: Name of the parent panel widget
            instance_name: Name to give the new widget
            position: [X, Y] position (if parent is CanvasPanel)
            size: [Width, Height] of the widget

        Returns:
            Dict containing success status and widget properties
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("add_common_activatable_widget", {
                "widget_name": widget_name,
                "activatable_name": instance_name,
                "position": position,
                "size": size
            })
            return response if response else {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def add_common_button_base(
        ctx: Context,
        widget_name: str,
        parent_widget: str,
        button_name: str,
        position: List[float] = [0.0, 0.0],
        size: List[float] = [200.0, 50.0]
    ) -> Dict[str, Any]:
        """
        Add a Common Button Base widget (from Common UI plugin) to a UMG Widget Blueprint.

        This is the base class for Common UI buttons with input handling support.

        Args:
            widget_name: Name of the target Widget Blueprint
            parent_widget: Name of the parent panel widget
            button_name: Name to give the new button
            position: [X, Y] position (if parent is CanvasPanel)
            size: [Width, Height] of the button

        Returns:
            Dict containing success status and widget properties
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("add_common_button_base", {
                "widget_name": widget_name,
                "button_name": button_name,
                "position": position,
                "size": size
            })
            return response if response else {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def add_common_numeric_text_block(
        ctx: Context,
        widget_name: str,
        parent_widget: str,
        text_name: str,
        value: float = 0.0,
        position: List[float] = [0.0, 0.0],
        size: List[float] = [100.0, 30.0]
    ) -> Dict[str, Any]:
        """
        Add a Common Numeric Text Block widget (from Common UI plugin).

        Specialized text block for displaying numeric values with formatting.

        Args:
            widget_name: Name of the target Widget Blueprint
            parent_widget: Name of the parent panel widget
            text_name: Name to give the new text block
            value: Initial numeric value
            position: [X, Y] position (if parent is CanvasPanel)
            size: [Width, Height] of the text block

        Returns:
            Dict containing success status and widget properties
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("add_common_numeric_text_block", {
                "widget_name": widget_name,
                "text_block_name": text_name,
                "value": value,
                "position": position,
                "size": size
            })
            return response if response else {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def add_common_rich_text_block(
        ctx: Context,
        widget_name: str,
        parent_widget: str,
        text_name: str,
        text: str = "",
        position: List[float] = [0.0, 0.0],
        size: List[float] = [200.0, 50.0]
    ) -> Dict[str, Any]:
        """
        Add a Common Rich Text Block widget (from Common UI plugin).

        Rich text block with support for inline styles and formatting tags.

        Args:
            widget_name: Name of the target Widget Blueprint
            parent_widget: Name of the parent panel widget
            text_name: Name to give the new text block
            text: Initial text content (supports rich text markup)
            position: [X, Y] position (if parent is CanvasPanel)
            size: [Width, Height] of the text block

        Returns:
            Dict containing success status and widget properties
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("add_common_rich_text_block", {
                "widget_name": widget_name,
                "text_block_name": text_name,
                "text": text,
                "position": position,
                "size": size
            })
            return response if response else {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def add_common_lazy_image(
        ctx: Context,
        widget_name: str,
        parent_widget: str,
        image_name: str,
        position: List[float] = [0.0, 0.0],
        size: List[float] = [100.0, 100.0]
    ) -> Dict[str, Any]:
        """
        Add a Common Lazy Image widget (from Common UI plugin).

        Image widget that supports lazy loading of textures.

        Args:
            widget_name: Name of the target Widget Blueprint
            parent_widget: Name of the parent panel widget
            image_name: Name to give the new image
            position: [X, Y] position (if parent is CanvasPanel)
            size: [Width, Height] of the image

        Returns:
            Dict containing success status and widget properties
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("add_common_lazy_image", {
                "widget_name": widget_name,
                "image_name": image_name,
                "position": position,
                "size": size
            })
            return response if response else {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def add_common_list_view(
        ctx: Context,
        widget_name: str,
        parent_widget: str,
        list_name: str,
        orientation: str = "Vertical",
        position: List[float] = [0.0, 0.0],
        size: List[float] = [300.0, 400.0]
    ) -> Dict[str, Any]:
        """
        Add a Common List View widget (from Common UI plugin).

        Virtualized list view for displaying large data sets efficiently.

        Args:
            widget_name: Name of the target Widget Blueprint
            parent_widget: Name of the parent panel widget
            list_name: Name to give the new list view
            orientation: "Vertical" or "Horizontal"
            position: [X, Y] position (if parent is CanvasPanel)
            size: [Width, Height] of the list view

        Returns:
            Dict containing success status and widget properties
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("add_common_list_view", {
                "widget_name": widget_name,
                "list_view_name": list_name,
                "orientation": orientation,
                "position": position,
                "size": size
            })
            return response if response else {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def add_common_tile_view(
        ctx: Context,
        widget_name: str,
        parent_widget: str,
        tile_name: str,
        position: List[float] = [0.0, 0.0],
        size: List[float] = [400.0, 300.0]
    ) -> Dict[str, Any]:
        """
        Add a Common Tile View widget (from Common UI plugin).

        Grid-style virtualized view for displaying items in tiles.

        Args:
            widget_name: Name of the target Widget Blueprint
            parent_widget: Name of the parent panel widget
            tile_name: Name to give the new tile view
            position: [X, Y] position (if parent is CanvasPanel)
            size: [Width, Height] of the tile view

        Returns:
            Dict containing success status and widget properties
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("add_common_tile_view", {
                "widget_name": widget_name,
                "tile_view_name": tile_name,
                "position": position,
                "size": size
            })
            return response if response else {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def add_common_tree_view(
        ctx: Context,
        widget_name: str,
        parent_widget: str,
        tree_name: str,
        position: List[float] = [0.0, 0.0],
        size: List[float] = [300.0, 400.0]
    ) -> Dict[str, Any]:
        """
        Add a Common Tree View widget (from Common UI plugin).

        Hierarchical tree view for displaying nested data.

        Args:
            widget_name: Name of the target Widget Blueprint
            parent_widget: Name of the parent panel widget
            tree_name: Name to give the new tree view
            position: [X, Y] position (if parent is CanvasPanel)
            size: [Width, Height] of the tree view

        Returns:
            Dict containing success status and widget properties
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("add_common_tree_view", {
                "widget_name": widget_name,
                "tree_view_name": tree_name,
                "position": position,
                "size": size
            })
            return response if response else {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def add_common_rotator(
        ctx: Context,
        widget_name: str,
        parent_widget: str,
        rotator_name: str,
        position: List[float] = [0.0, 0.0],
        size: List[float] = [200.0, 50.0]
    ) -> Dict[str, Any]:
        """
        Add a Common Rotator widget (from Common UI plugin).

        Rotator widget for cycling through options with left/right navigation.

        Args:
            widget_name: Name of the target Widget Blueprint
            parent_widget: Name of the parent panel widget
            rotator_name: Name to give the new rotator
            position: [X, Y] position (if parent is CanvasPanel)
            size: [Width, Height] of the rotator

        Returns:
            Dict containing success status and widget properties
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("add_common_rotator", {
                "widget_name": widget_name,
                "rotator_name": rotator_name,
                "position": position,
                "size": size
            })
            return response if response else {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def add_common_action_widget(
        ctx: Context,
        widget_name: str,
        parent_widget: str,
        action_name: str,
        position: List[float] = [0.0, 0.0],
        size: List[float] = [100.0, 50.0]
    ) -> Dict[str, Any]:
        """
        Add a Common Action Widget (from Common UI plugin).

        Widget that displays bound input action icons/text.

        Args:
            widget_name: Name of the target Widget Blueprint
            parent_widget: Name of the parent panel widget
            action_name: Name to give the new action widget
            position: [X, Y] position (if parent is CanvasPanel)
            size: [Width, Height] of the widget

        Returns:
            Dict containing success status and widget properties
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("add_common_action_widget", {
                "widget_name": widget_name,
                "action_widget_name": action_name,
                "position": position,
                "size": size
            })
            return response if response else {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def add_common_animated_switcher(
        ctx: Context,
        widget_name: str,
        parent_widget: str,
        switcher_name: str,
        active_index: int = 0,
        position: List[float] = [0.0, 0.0],
        size: List[float] = [300.0, 200.0]
    ) -> Dict[str, Any]:
        """
        Add a Common Animated Switcher widget (from Common UI plugin).

        Widget switcher with built-in transition animations.

        Args:
            widget_name: Name of the target Widget Blueprint
            parent_widget: Name of the parent panel widget
            switcher_name: Name to give the new switcher
            active_index: Initially active child index
            position: [X, Y] position (if parent is CanvasPanel)
            size: [Width, Height] of the switcher

        Returns:
            Dict containing success status and widget properties
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("add_common_animated_switcher", {
                "widget_name": widget_name,
                "switcher_name": switcher_name,
                "active_index": active_index,
                "position": position,
                "size": size
            })
            return response if response else {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def add_common_widget_carousel(
        ctx: Context,
        widget_name: str,
        parent_widget: str,
        carousel_name: str,
        position: List[float] = [0.0, 0.0],
        size: List[float] = [400.0, 200.0]
    ) -> Dict[str, Any]:
        """
        Add a Common Widget Carousel (from Common UI plugin).

        Carousel widget for horizontal scrolling through widgets.

        Args:
            widget_name: Name of the target Widget Blueprint
            parent_widget: Name of the parent panel widget
            carousel_name: Name to give the new carousel
            position: [X, Y] position (if parent is CanvasPanel)
            size: [Width, Height] of the carousel

        Returns:
            Dict containing success status and widget properties
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("add_common_widget_carousel", {
                "widget_name": widget_name,
                "carousel_name": carousel_name,
                "position": position,
                "size": size
            })
            return response if response else {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def add_common_load_guard(
        ctx: Context,
        widget_name: str,
        parent_widget: str,
        guard_name: str,
        position: List[float] = [0.0, 0.0],
        size: List[float] = [200.0, 200.0]
    ) -> Dict[str, Any]:
        """
        Add a Common Load Guard widget (from Common UI plugin).

        Widget that shows loading state while content is being loaded.

        Args:
            widget_name: Name of the target Widget Blueprint
            parent_widget: Name of the parent panel widget
            guard_name: Name to give the new load guard
            position: [X, Y] position (if parent is CanvasPanel)
            size: [Width, Height] of the load guard

        Returns:
            Dict containing success status and widget properties
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("add_common_load_guard", {
                "widget_name": widget_name,
                "load_guard_name": guard_name,
                "position": position,
                "size": size
            })
            return response if response else {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def add_common_video_player(
        ctx: Context,
        widget_name: str,
        parent_widget: str,
        player_name: str,
        position: List[float] = [0.0, 0.0],
        size: List[float] = [640.0, 360.0]
    ) -> Dict[str, Any]:
        """
        Add a Common Video Player widget (from Common UI plugin).

        Video player widget with playback controls.

        Args:
            widget_name: Name of the target Widget Blueprint
            parent_widget: Name of the parent panel widget
            player_name: Name to give the new video player
            position: [X, Y] position (if parent is CanvasPanel)
            size: [Width, Height] of the video player

        Returns:
            Dict containing success status and widget properties
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("add_common_video_player", {
                "widget_name": widget_name,
                "video_player_name": player_name,
                "position": position,
                "size": size
            })
            return response if response else {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def add_common_date_time_text_block(
        ctx: Context,
        widget_name: str,
        parent_widget: str,
        text_name: str,
        position: List[float] = [0.0, 0.0],
        size: List[float] = [150.0, 30.0]
    ) -> Dict[str, Any]:
        """
        Add a Common Date Time Text Block widget (from Common UI plugin).

        Text block specialized for displaying date/time values.

        Args:
            widget_name: Name of the target Widget Blueprint
            parent_widget: Name of the parent panel widget
            text_name: Name to give the new text block
            position: [X, Y] position (if parent is CanvasPanel)
            size: [Width, Height] of the text block

        Returns:
            Dict containing success status and widget properties
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("add_common_date_time_text_block", {
                "widget_name": widget_name,
                "text_block_name": text_name,
                "position": position,
                "size": size
            })
            return response if response else {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def add_analog_slider(
        ctx: Context,
        widget_name: str,
        parent_widget: str,
        slider_name: str,
        value: float = 0.5,
        position: List[float] = [0.0, 0.0],
        size: List[float] = [200.0, 40.0]
    ) -> Dict[str, Any]:
        """
        Add an Analog Slider widget (from Common UI plugin).

        Slider widget optimized for analog stick input.

        Args:
            widget_name: Name of the target Widget Blueprint
            parent_widget: Name of the parent panel widget
            slider_name: Name to give the new slider
            value: Initial slider value (0.0 to 1.0)
            position: [X, Y] position (if parent is CanvasPanel)
            size: [Width, Height] of the slider

        Returns:
            Dict containing success status and widget properties
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("add_analog_slider", {
                "widget_name": widget_name,
                "slider_name": slider_name,
                "value": value,
                "position": position,
                "size": size
            })
            return response if response else {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def add_common_hierarchical_scroll_box(
        ctx: Context,
        widget_name: str,
        parent_widget: str,
        scroll_name: str,
        orientation: str = "Vertical",
        position: List[float] = [0.0, 0.0],
        size: List[float] = [300.0, 400.0]
    ) -> Dict[str, Any]:
        """
        Add a Common Hierarchical Scroll Box widget (from Common UI plugin).

        Scroll box with support for hierarchical/nested content.

        Args:
            widget_name: Name of the target Widget Blueprint
            parent_widget: Name of the parent panel widget
            scroll_name: Name to give the new scroll box
            orientation: "Vertical" or "Horizontal"
            position: [X, Y] position (if parent is CanvasPanel)
            size: [Width, Height] of the scroll box

        Returns:
            Dict containing success status and widget properties
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("add_common_hierarchical_scroll_box", {
                "widget_name": widget_name,
                "scroll_box_name": scroll_name,
                "orientation": orientation,
                "position": position,
                "size": size
            })
            return response if response else {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def add_common_visibility_switcher(
        ctx: Context,
        widget_name: str,
        parent_widget: str,
        switcher_name: str,
        active_index: int = 0,
        position: List[float] = [0.0, 0.0],
        size: List[float] = [300.0, 200.0]
    ) -> Dict[str, Any]:
        """
        Add a Common Visibility Switcher widget (from Common UI plugin).

        Widget that switches visibility of children based on active index.

        Args:
            widget_name: Name of the target Widget Blueprint
            parent_widget: Name of the parent panel widget
            switcher_name: Name to give the new switcher
            active_index: Initially visible child index
            position: [X, Y] position (if parent is CanvasPanel)
            size: [Width, Height] of the switcher

        Returns:
            Dict containing success status and widget properties
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("add_common_visibility_switcher", {
                "widget_name": widget_name,
                "switcher_name": switcher_name,
                "active_index": active_index,
                "position": position,
                "size": size
            })
            return response if response else {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": str(e)}

    logger.info("Widget Common UI extended tools registered")


def register_widget_readonly_tools(mcp: FastMCP):
    """Register read-only widget inspection tools only (no mutations)."""

    @mcp.tool()
    def get_widget_hierarchy(
        ctx: Context,
        widget_name: str
    ) -> Dict[str, Any]:
        """Get the full widget hierarchy tree."""
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect"}
            response = unreal.send_command("get_widget_hierarchy", {
                "blueprint_name": widget_name
            })
            return response if response else {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def get_widget_properties(
        ctx: Context,
        widget_name: str,
        target_widget: str
    ) -> Dict[str, Any]:
        """Get all properties of a widget."""
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect"}
            response = unreal.send_command("get_widget_properties", {
                "blueprint_name": widget_name,
                "widget_name": target_widget
            })
            return response if response else {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def get_common_ui_config(ctx: Context) -> Dict[str, Any]:
        """Get the current Common UI configuration."""
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("get_common_ui_config", {})
            return response if response else {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def find_widgets(
        ctx: Context,
        widget_name: str,
        widget_type: str = None,
        name_pattern: str = None
    ) -> Dict[str, Any]:
        """Find all widgets matching a type or name pattern."""
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect"}
            params = {"blueprint_name": widget_name}
            if widget_type:
                params["widget_type"] = widget_type
            if name_pattern:
                params["name_pattern"] = name_pattern
            response = unreal.send_command("find_widgets", params)
            return response if response else {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def get_all_widgets(
        ctx: Context,
        widget_name: str
    ) -> Dict[str, Any]:
        """Get all widgets in the blueprint."""
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect"}
            response = unreal.send_command("get_all_widgets", {
                "blueprint_name": widget_name
            })
            return response if response else {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def get_widget_bounds(
        ctx: Context,
        widget_name: str,
        target_widget: str
    ) -> Dict[str, Any]:
        """Get widget position and size bounds."""
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect"}
            response = unreal.send_command("get_widget_bounds", {
                "blueprint_name": widget_name,
                "widget_name": target_widget
            })
            return response if response else {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def get_parent(
        ctx: Context,
        widget_name: str,
        target_widget: str
    ) -> Dict[str, Any]:
        """Get parent widget."""
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect"}
            response = unreal.send_command("get_parent", {
                "blueprint_name": widget_name,
                "widget_name": target_widget
            })
            return response if response else {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def get_children(
        ctx: Context,
        widget_name: str,
        target_widget: str
    ) -> Dict[str, Any]:
        """Get children widgets."""
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect"}
            response = unreal.send_command("get_children", {
                "blueprint_name": widget_name,
                "widget_name": target_widget
            })
            return response if response else {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def list_presets(
        ctx: Context,
        widget_type: str = None
    ) -> Dict[str, Any]:
        """List all available style presets."""
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect"}
            params = {}
            if widget_type:
                params["widget_type"] = widget_type
            response = unreal.send_command("list_presets", params)
            return response if response else {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def get_widget_animations(
        ctx: Context,
        widget_name: str
    ) -> Dict[str, Any]:
        """Get all animations in a UMG Widget Blueprint."""
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("get_widget_animations", {
                "blueprint_name": widget_name
            })
            return response if response else {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def get_animation_state(
        ctx: Context,
        widget_name: str,
        animation_name: str
    ) -> Dict[str, Any]:
        """Get the current state of an animation."""
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect"}
            response = unreal.send_command("get_animation_state", {
                "blueprint_name": widget_name,
                "animation_name": animation_name
            })
            return response if response else {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": str(e)}

    logger.info("Widget read-only tools registered")