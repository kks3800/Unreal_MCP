"""
Environment Query System (EQS) tools for Unreal MCP.

Provides full EQS asset management:
- Asset Management: Create, delete, list, info, save, open EQS queries
- Generator Management: Add generators (SimpleGrid, OnCircle, ActorsOfClass, etc.)
- Test Management: Add tests (Distance, Dot, Trace, Pathfinding, etc.)
- Property Configuration: Set generator/test properties
"""

import logging
from mcp.server.fastmcp import FastMCP, Context
from typing import Dict, Any, Optional

logger = logging.getLogger("UnrealMCP")


def register_eqs_tools(mcp: FastMCP):
    """Register all EQS-related MCP tools."""

    #=========================================================================
    # EQS Asset Management (6 tools)
    #=========================================================================

    @mcp.tool()
    def create_eqs_query(
        ctx: Context,
        query_name: str,
        path: str = "/Game/AI/EQS"
    ) -> Dict[str, Any]:
        """
        Create a new Environment Query (EQS) asset.

        Args:
            query_name: Name for the new EQS query
            path: Content path (default: /Game/AI/EQS)
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("create_eqs_query", {
                "query_name": query_name, "path": path
            })
            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}
            return {"status": "success", "result": response}
        except Exception as e:
            logger.error(f"Error creating EQS query: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def delete_eqs_query(
        ctx: Context,
        query_name: str
    ) -> Dict[str, Any]:
        """
        Delete an EQS query asset.

        Args:
            query_name: Name or path of the EQS query to delete
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("delete_eqs_query", {"query_name": query_name})
            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}
            return {"status": "success", "result": response}
        except Exception as e:
            logger.error(f"Error deleting EQS query: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def list_eqs_queries(
        ctx: Context,
        path: str = "/Game",
        recursive: bool = True
    ) -> Dict[str, Any]:
        """
        List all EQS query assets.

        Args:
            path: Content path to search (default: /Game)
            recursive: Search recursively (default: true)
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("list_eqs_queries", {
                "path": path, "recursive": recursive
            })
            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}
            return {"status": "success", "result": response}
        except Exception as e:
            logger.error(f"Error listing EQS queries: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def get_eqs_query_info(
        ctx: Context,
        query_name: str
    ) -> Dict[str, Any]:
        """
        Get detailed information about an EQS query including all options, generators, and tests.

        Args:
            query_name: Name or path of the EQS query
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("get_eqs_query_info", {"query_name": query_name})
            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}
            return {"status": "success", "result": response}
        except Exception as e:
            logger.error(f"Error getting EQS query info: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def save_eqs_query(
        ctx: Context,
        query_name: str
    ) -> Dict[str, Any]:
        """
        Save an EQS query asset to disk.

        Args:
            query_name: Name or path of the EQS query to save
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("save_eqs_query", {"query_name": query_name})
            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}
            return {"status": "success", "result": response}
        except Exception as e:
            logger.error(f"Error saving EQS query: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def open_eqs_query(
        ctx: Context,
        query_name: str
    ) -> Dict[str, Any]:
        """
        Open an EQS query in the Unreal Editor.

        Args:
            query_name: Name or path of the EQS query to open
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("open_eqs_query", {"query_name": query_name})
            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}
            return {"status": "success", "result": response}
        except Exception as e:
            logger.error(f"Error opening EQS query: {e}")
            return {"status": "error", "error": str(e)}

    #=========================================================================
    # Generator / Option Management (4 tools)
    #=========================================================================

    @mcp.tool()
    def add_eqs_generator(
        ctx: Context,
        query_name: str,
        generator_type: str,
        option_name: str = "",
        context: str = "Querier"
    ) -> Dict[str, Any]:
        """
        Add a generator (option) to an EQS query. Each generator creates an option containing
        a generator + its tests.

        Args:
            query_name: Target EQS query
            generator_type: Generator type: SimpleGrid, PathingGrid, OnCircle, Donut, Cone,
                          ActorsOfClass, CurrentLocation, Composite
            option_name: Optional name for the option
            context: Context for generation center: Querier (default), Item
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("add_eqs_generator", {
                "query_name": query_name,
                "generator_type": generator_type,
                "option_name": option_name,
                "context": context
            })
            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}
            return {"status": "success", "result": response}
        except Exception as e:
            logger.error(f"Error adding EQS generator: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def remove_eqs_option(
        ctx: Context,
        query_name: str,
        option_index: int
    ) -> Dict[str, Any]:
        """
        Remove an option (generator + its tests) from an EQS query.

        Args:
            query_name: Target EQS query
            option_index: Index of the option to remove (0-based)
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("remove_eqs_option", {
                "query_name": query_name, "option_index": option_index
            })
            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}
            return {"status": "success", "result": response}
        except Exception as e:
            logger.error(f"Error removing EQS option: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def get_eqs_options(
        ctx: Context,
        query_name: str
    ) -> Dict[str, Any]:
        """
        Get all options (generators + tests) for an EQS query.

        Args:
            query_name: Target EQS query
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("get_eqs_options", {"query_name": query_name})
            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}
            return {"status": "success", "result": response}
        except Exception as e:
            logger.error(f"Error getting EQS options: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def set_eqs_generator_property(
        ctx: Context,
        query_name: str,
        option_index: int,
        property_name: str,
        float_value: Optional[float] = None,
        bool_value: Optional[bool] = None,
        int_value: Optional[int] = None,
        context_value: Optional[str] = None
    ) -> Dict[str, Any]:
        """
        Set a property on a generator. Supports FAIDataProviderFloatValue, bool, int, and
        context class (TSubclassOf<UEnvQueryContext>) properties.

        Args:
            query_name: Target EQS query
            option_index: Index of the option containing the generator
            property_name: Property name (e.g., GridSize, SpaceBetween, SearchRadius, GenerateAround)
            float_value: Float value for numeric properties
            bool_value: Bool value for boolean properties
            int_value: Integer value
            context_value: Context class name: Querier, Item
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}
            params = {
                "query_name": query_name,
                "option_index": option_index,
                "property_name": property_name
            }
            if float_value is not None:
                params["float_value"] = float_value
            if bool_value is not None:
                params["bool_value"] = bool_value
            if int_value is not None:
                params["int_value"] = int_value
            if context_value is not None:
                params["context_value"] = context_value
            response = unreal.send_command("set_eqs_generator_property", params)
            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}
            return {"status": "success", "result": response}
        except Exception as e:
            logger.error(f"Error setting generator property: {e}")
            return {"status": "error", "error": str(e)}

    #=========================================================================
    # Test Management (4 tools)
    #=========================================================================

    @mcp.tool()
    def add_eqs_test(
        ctx: Context,
        query_name: str,
        option_index: int,
        test_type: str,
        purpose: str = "Score",
        scoring: str = "Linear",
        scoring_factor: float = 1.0,
        comment: str = ""
    ) -> Dict[str, Any]:
        """
        Add a test to an EQS option. Tests filter or score items generated by the option's generator.

        Args:
            query_name: Target EQS query
            option_index: Index of the option to add test to
            test_type: Use the class suffix (e.g., Distance, Dot, Trace, ObjectType, PawnAlive).
                      Any UEnvQueryTest subclass is resolved dynamically at runtime.
            purpose: Test purpose: Filter, Score (default), FilterAndScore
            scoring: Scoring equation: Linear (default), Square, InverseLinear, SquareRoot, Constant
            scoring_factor: Scoring weight multiplier (default: 1.0)
            comment: Optional description of what this test does
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("add_eqs_test", {
                "query_name": query_name,
                "option_index": option_index,
                "test_type": test_type,
                "purpose": purpose,
                "scoring": scoring,
                "scoring_factor": scoring_factor,
                "comment": comment
            })
            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}
            return {"status": "success", "result": response}
        except Exception as e:
            logger.error(f"Error adding EQS test: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def remove_eqs_test(
        ctx: Context,
        query_name: str,
        option_index: int,
        test_index: int
    ) -> Dict[str, Any]:
        """
        Remove a test from an EQS option.

        Args:
            query_name: Target EQS query
            option_index: Index of the option
            test_index: Index of the test to remove (0-based)
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("remove_eqs_test", {
                "query_name": query_name,
                "option_index": option_index,
                "test_index": test_index
            })
            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}
            return {"status": "success", "result": response}
        except Exception as e:
            logger.error(f"Error removing EQS test: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def get_eqs_tests(
        ctx: Context,
        query_name: str,
        option_index: int
    ) -> Dict[str, Any]:
        """
        Get all tests for an EQS option with their configuration.

        Args:
            query_name: Target EQS query
            option_index: Index of the option
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("get_eqs_tests", {
                "query_name": query_name, "option_index": option_index
            })
            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}
            return {"status": "success", "result": response}
        except Exception as e:
            logger.error(f"Error getting EQS tests: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def set_eqs_test_property(
        ctx: Context,
        query_name: str,
        option_index: int,
        test_index: int,
        property_name: str,
        float_value: Optional[float] = None,
        bool_value: Optional[bool] = None,
        enum_value: Optional[str] = None,
        context_value: Optional[str] = None,
        array_value: Optional[list] = None
    ) -> Dict[str, Any]:
        """
        Set a property on an EQS test. Supports float (FAIDataProviderFloatValue),
        bool, enum, context class, and array enum properties.

        Args:
            query_name: Target EQS query
            option_index: Option index
            test_index: Test index
            property_name: Property name. Common properties:
                - TestPurpose: Filter, Score, FilterAndScore
                - ScoringEquation: Linear, Square, InverseLinear, SquareRoot, Constant
                - FilterType: Minimum, Maximum, Range, Match
                - ScoringFactor: float weight
                - DistanceTo: context class (Distance test)
                - TestMode: enum (Distance test: Distance3D, Distance2D, DistanceZ)
                - AllowedTypes: array of enum strings (ObjectType test)
            float_value: Float value for numeric properties
            bool_value: Bool value
            enum_value: Enum string value (e.g., "Distance3D", "Filter")
            context_value: Context class: Querier, Item
            array_value: Array of enum string values (e.g., ["Exit", "Coin"])
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}
            params = {
                "query_name": query_name,
                "option_index": option_index,
                "test_index": test_index,
                "property_name": property_name
            }
            if float_value is not None:
                params["float_value"] = float_value
            if bool_value is not None:
                params["bool_value"] = bool_value
            if enum_value is not None:
                params["enum_value"] = enum_value
            if context_value is not None:
                params["context_value"] = context_value
            if array_value is not None:
                params["array_value"] = array_value
            response = unreal.send_command("set_eqs_test_property", params)
            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}
            return {"status": "success", "result": response}
        except Exception as e:
            logger.error(f"Error setting test property: {e}")
            return {"status": "error", "error": str(e)}
