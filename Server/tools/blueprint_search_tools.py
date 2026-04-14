"""
Blueprint search and discovery tools for Unreal MCP.

Provides action search, class function listing, node type info,
category browsing, and placing searched actions into Blueprint graphs.
"""

import logging
from mcp.server.fastmcp import FastMCP, Context
from typing import Dict, Any, Optional, List

# Get logger
logger = logging.getLogger("UnrealMCP")


def register_blueprint_search_tools(mcp: FastMCP):
    """Register all blueprint search and discovery MCP tools."""

    @mcp.tool()
    def search_blueprint_actions(
        ctx: Context,
        keyword: str,
        blueprint_name: str = "",
        limit: int = 50,
        offset: int = 0,
        class_filter: str = ""
    ) -> Dict[str, Any]:
        """
        Search the Blueprint action database by keyword.

        This is the primary discovery tool - searches all available Blueprint
        actions (functions, events, macros, etc.) by matching against action
        names, categories, and keywords. Results are paginated.

        Use this to find what nodes are available before placing them. The
        results include owner_path and spawner_index which can be passed to
        place_searched_action to create the node in a graph.

        Args:
            keyword: Search term to match against action names, categories, and keywords.
                     Case-insensitive partial match. Examples: "Print", "SetActorLocation",
                     "Delay", "ForEachLoop"
            blueprint_name: Optional Blueprint name for context-specific actions
            limit: Maximum number of results to return (1-200, default 50)
            offset: Number of matching results to skip for pagination (default 0)
            class_filter: Optional class name to restrict results to actions from
                          a specific class (e.g., "Actor", "GameplayStatics")

        Returns:
            Dict with results array (action_name, category, keywords, node_class,
            owner_path, spawner_index), total_matches, offset, limit, returned count
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            params = {
                "keyword": keyword,
                "limit": limit,
                "offset": offset,
            }
            if blueprint_name:
                params["blueprint_name"] = blueprint_name
            if class_filter:
                params["class_filter"] = class_filter

            response = unreal.send_command("search_blueprint_actions", params)

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return response

        except Exception as e:
            logger.error(f"Error searching blueprint actions: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def get_class_functions(
        ctx: Context,
        class_name: str,
        include_inherited: bool = True
    ) -> Dict[str, Any]:
        """
        List all Blueprint-callable functions on a UClass.

        Returns all functions marked as BlueprintCallable or BlueprintPure,
        with parameter info, return types, and metadata.

        Args:
            class_name: Name of the class to inspect. Supports multiple formats:
                       - Simple name: "Actor", "GameplayStatics", "PrimitiveComponent"
                       - With prefix: "AActor", "UGameplayStatics"
                       - Full path: "/Script/Engine.Actor"
            include_inherited: If true (default), includes functions from parent classes.
                             Set to false to see only functions defined directly on this class.

        Returns:
            Dict with class_name, class_path, functions array (name, is_static, is_pure,
            category, owning_class, parameters with name/type/is_return/is_output)
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("get_class_functions", {
                "class_name": class_name,
                "include_inherited": include_inherited
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return response

        except Exception as e:
            logger.error(f"Error getting class functions: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def get_node_type_info(
        ctx: Context,
        node_class_name: str
    ) -> Dict[str, Any]:
        """
        Get information about a specific Blueprint node type.

        Returns class details, description, and default pin layout for a node type.
        Useful for understanding what a node does before placing it.

        Args:
            node_class_name: Name of the node class. Examples:
                           - "K2Node_IfThenElse" (Branch node)
                           - "K2Node_CallFunction" (Function call)
                           - "K2Node_Event" (Event node)
                           - "K2Node_Timeline" (Timeline)
                           - "K2Node_ForEachElementInArray" (For Each Loop)

        Returns:
            Dict with class_name, parent_class, description, node_title,
            and default_pins array
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("get_node_type_info", {
                "node_class_name": node_class_name
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return response

        except Exception as e:
            logger.error(f"Error getting node type info: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def search_by_category(
        ctx: Context,
        category: str = "",
        blueprint_name: str = "",
        limit: int = 100
    ) -> Dict[str, Any]:
        """
        Browse Blueprint actions by category.

        When called without a category, returns all top-level categories.
        When called with a category, returns all actions in that category.

        Args:
            category: Category to filter by. Leave empty to get top-level categories.
                     Examples: "Math", "Utilities", "Flow Control", "Variables"
                     Use "|" separator for subcategories: "Math|Integer"
            blueprint_name: Optional Blueprint name for context
            limit: Maximum actions to return when filtering by category (1-500, default 100)

        Returns:
            Without category: Dict with categories array and category_count
            With category: Dict with results array (action_name, category, keywords,
                          node_class, owner_path, spawner_index) and result_count
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            params = {"limit": limit}
            if category:
                params["category"] = category
            if blueprint_name:
                params["blueprint_name"] = blueprint_name

            response = unreal.send_command("search_by_category", params)

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return response

        except Exception as e:
            logger.error(f"Error searching by category: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def place_searched_action(
        ctx: Context,
        blueprint_name: str,
        owner_path: str,
        spawner_index: int,
        position: Optional[List[float]] = None,
        graph_name: str = ""
    ) -> Dict[str, Any]:
        """
        Place a node from search results into a Blueprint graph.

        Uses the owner_path and spawner_index from search_blueprint_actions or
        search_by_category results to place the exact action as a node.

        Args:
            blueprint_name: Name of the target Blueprint
            owner_path: Owner path from search results (identifies the action source)
            spawner_index: Spawner index from search results (identifies the specific action)
            position: Optional [X, Y] position in the graph. Defaults to [0, 0]
            graph_name: Optional graph name. Defaults to "EventGraph"

        Returns:
            Dict with the created node's GUID, title, class, position, and pin details
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            if position is None:
                position = [0, 0]

            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            params = {
                "blueprint_name": blueprint_name,
                "owner_path": owner_path,
                "spawner_index": spawner_index,
                "position": position,
            }
            if graph_name:
                params["graph_name"] = graph_name

            response = unreal.send_command("place_searched_action", params)

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return response

        except Exception as e:
            logger.error(f"Error placing searched action: {e}")
            return {"status": "error", "error": str(e)}
