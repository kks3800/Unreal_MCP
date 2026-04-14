"""
Behavior Tree creation and editing tools for Unreal MCP.

Provides comprehensive Behavior Tree management via MCP commands:
- Asset Management: Create, delete, list, save, open BT assets
- Composite Nodes: Selector, Sequence, Simple Parallel
- Task Nodes: Wait, MoveTo, PlaySound, PlayAnimation, RunBehavior, etc.
- Decorator Nodes: Blackboard, Cooldown, Loop, TimeLimit, ForceSuccess, etc.
- Service Nodes: DefaultFocus, RunEQS
- Structure Commands: Connect, disconnect, reorder, delete, move, arrange nodes
- Runtime Commands: Run, stop, pause, resume BTs; blackboard value manipulation
"""

import logging
from mcp.server.fastmcp import FastMCP, Context
from typing import List, Optional, Dict, Any

# Get logger
logger = logging.getLogger("UnrealMCP")


def register_behavior_tree_tools(mcp: FastMCP):
    """Register all Behavior Tree-related MCP tools."""

    #=========================================================================
    # BT Asset Management (7 tools)
    #=========================================================================

    @mcp.tool()
    def create_behavior_tree(
        ctx: Context,
        tree_name: str,
        path: str = "/Game/AI",
        blackboard_asset: str = ""
    ) -> Dict[str, Any]:
        """
        Create a new Behavior Tree asset.

        Args:
            tree_name: Name for the new Behavior Tree
            path: Content path (default: /Game/AI)
            blackboard_asset: Optional Blackboard asset to associate

        Returns:
            Dict with success status and tree info
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.warning("Failed to connect to Unreal Engine")
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("create_behavior_tree", {
                "tree_name": tree_name,
                "path": path,
                "blackboard_asset": blackboard_asset
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error creating Behavior Tree: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def delete_behavior_tree(
        ctx: Context,
        tree_name: str
    ) -> Dict[str, Any]:
        """
        Delete a Behavior Tree asset.

        Args:
            tree_name: Name or path of the Behavior Tree to delete

        Returns:
            Dict with success status
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("delete_behavior_tree", {
                "tree_name": tree_name
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error deleting Behavior Tree: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def list_behavior_trees(
        ctx: Context,
        path: str = "/Game",
        recursive: bool = True
    ) -> Dict[str, Any]:
        """
        List Behavior Tree assets in a path.

        Args:
            path: Content path to search (default: /Game)
            recursive: Search recursively (default: True)

        Returns:
            Dict with list of Behavior Tree assets
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("list_behavior_trees", {
                "path": path,
                "recursive": recursive
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error listing Behavior Trees: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def get_behavior_tree_info(
        ctx: Context,
        tree_name: str
    ) -> Dict[str, Any]:
        """
        Get information about a Behavior Tree.

        Args:
            tree_name: Name or path of the Behavior Tree to query

        Returns:
            Dict with tree properties, node count, and structure overview
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("get_behavior_tree_info", {
                "tree_name": tree_name
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error getting Behavior Tree info: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def save_behavior_tree(
        ctx: Context,
        tree_name: str
    ) -> Dict[str, Any]:
        """
        Save a Behavior Tree asset to disk.

        Args:
            tree_name: Name or path of the Behavior Tree to save

        Returns:
            Dict with success status
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("save_behavior_tree", {
                "tree_name": tree_name
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error saving Behavior Tree: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def open_behavior_tree(
        ctx: Context,
        tree_name: str
    ) -> Dict[str, Any]:
        """
        Open a Behavior Tree in the editor.

        Args:
            tree_name: Name or path of the Behavior Tree to open

        Returns:
            Dict with success status and tree info
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("open_behavior_tree", {
                "tree_name": tree_name
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error opening Behavior Tree: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def set_behavior_tree_blackboard(
        ctx: Context,
        tree_name: str,
        blackboard_name: str
    ) -> Dict[str, Any]:
        """
        Associate a Blackboard asset with a Behavior Tree.

        Args:
            tree_name: Name or path of the Behavior Tree
            blackboard_name: Name or path of the Blackboard asset to associate

        Returns:
            Dict with success status
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("set_behavior_tree_blackboard", {
                "tree_name": tree_name,
                "blackboard_name": blackboard_name
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error setting Behavior Tree blackboard: {e}")
            return {"status": "error", "error": str(e)}

    #=========================================================================
    # Composite Nodes (3 tools)
    #=========================================================================

    @mcp.tool()
    def add_bt_selector(
        ctx: Context,
        tree_name: str,
        parent_node_index: int = -1
    ) -> Dict[str, Any]:
        """
        Add a Selector composite node. Executes children left-to-right, succeeds on first success.

        Args:
            tree_name: Name of the Behavior Tree
            parent_node_index: Index of parent node (-1 for root)

        Returns:
            Dict with success status and new node index
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("add_bt_selector", {
                "tree_name": tree_name,
                "parent_node_index": parent_node_index
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error adding BT Selector: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def add_bt_sequence(
        ctx: Context,
        tree_name: str,
        parent_node_index: int = -1
    ) -> Dict[str, Any]:
        """
        Add a Sequence composite node. Executes children left-to-right, fails on first failure.

        Args:
            tree_name: Name of the Behavior Tree
            parent_node_index: Index of parent node (-1 for root)

        Returns:
            Dict with success status and new node index
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("add_bt_sequence", {
                "tree_name": tree_name,
                "parent_node_index": parent_node_index
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error adding BT Sequence: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def add_bt_simple_parallel(
        ctx: Context,
        tree_name: str,
        parent_node_index: int = -1,
        finish_mode: str = "AbortBackground"
    ) -> Dict[str, Any]:
        """
        Add a Simple Parallel composite. Runs main task and background tree simultaneously.

        Args:
            tree_name: Name of the Behavior Tree
            parent_node_index: Index of parent node (-1 for root)
            finish_mode: Finish mode - AbortBackground or WaitForBackground (default: AbortBackground)

        Returns:
            Dict with success status and new node index
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("add_bt_simple_parallel", {
                "tree_name": tree_name,
                "parent_node_index": parent_node_index,
                "finish_mode": finish_mode
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error adding BT Simple Parallel: {e}")
            return {"status": "error", "error": str(e)}

    #=========================================================================
    # Task Nodes (10 tools)
    #=========================================================================

    @mcp.tool()
    def add_bt_task_wait(
        ctx: Context,
        tree_name: str,
        parent_node_index: int,
        wait_time: float = 5.0,
        random_deviation: float = 0.0
    ) -> Dict[str, Any]:
        """
        Add a Wait task node.

        Args:
            tree_name: Name of the Behavior Tree
            parent_node_index: Index of parent composite node
            wait_time: Time to wait in seconds (default: 5.0)
            random_deviation: Random deviation added to wait time (default: 0.0)

        Returns:
            Dict with success status and new node index
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("add_bt_task_wait", {
                "tree_name": tree_name,
                "parent_node_index": parent_node_index,
                "wait_time": wait_time,
                "random_deviation": random_deviation
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error adding BT Wait task: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def add_bt_task_move_to(
        ctx: Context,
        tree_name: str,
        parent_node_index: int,
        bb_key: str = "",
        acceptable_radius: float = 5.0
    ) -> Dict[str, Any]:
        """
        Add a MoveTo task node. Navigates AI to a blackboard key location.

        Args:
            tree_name: Name of the Behavior Tree
            parent_node_index: Index of parent composite node
            bb_key: Blackboard key containing target location or actor
            acceptable_radius: Distance threshold to consider arrival (default: 5.0)

        Returns:
            Dict with success status and new node index
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("add_bt_task_move_to", {
                "tree_name": tree_name,
                "parent_node_index": parent_node_index,
                "bb_key": bb_key,
                "acceptable_radius": acceptable_radius
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error adding BT MoveTo task: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def add_bt_task_play_sound(
        ctx: Context,
        tree_name: str,
        parent_node_index: int,
        sound_path: str = ""
    ) -> Dict[str, Any]:
        """
        Add a PlaySound task node.

        Args:
            tree_name: Name of the Behavior Tree
            parent_node_index: Index of parent composite node
            sound_path: Asset path to the sound to play

        Returns:
            Dict with success status and new node index
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("add_bt_task_play_sound", {
                "tree_name": tree_name,
                "parent_node_index": parent_node_index,
                "sound_path": sound_path
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error adding BT PlaySound task: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def add_bt_task_play_animation(
        ctx: Context,
        tree_name: str,
        parent_node_index: int,
        anim_path: str = "",
        looping: bool = False,
        non_blocking: bool = False
    ) -> Dict[str, Any]:
        """
        Add a PlayAnimation task node.

        Args:
            tree_name: Name of the Behavior Tree
            parent_node_index: Index of parent composite node
            anim_path: Asset path to the animation to play
            looping: Whether the animation should loop (default: False)
            non_blocking: Whether the task completes immediately (default: False)

        Returns:
            Dict with success status and new node index
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("add_bt_task_play_animation", {
                "tree_name": tree_name,
                "parent_node_index": parent_node_index,
                "anim_path": anim_path,
                "looping": looping,
                "non_blocking": non_blocking
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error adding BT PlayAnimation task: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def add_bt_task_run_behavior(
        ctx: Context,
        tree_name: str,
        parent_node_index: int,
        sub_tree_name: str = ""
    ) -> Dict[str, Any]:
        """
        Add a RunBehavior task that pushes a subtree.

        Args:
            tree_name: Name of the Behavior Tree
            parent_node_index: Index of parent composite node
            sub_tree_name: Name or path of the sub-tree to run

        Returns:
            Dict with success status and new node index
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("add_bt_task_run_behavior", {
                "tree_name": tree_name,
                "parent_node_index": parent_node_index,
                "sub_tree_name": sub_tree_name
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error adding BT RunBehavior task: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def add_bt_task_run_eqs(
        ctx: Context,
        tree_name: str,
        parent_node_index: int,
        query_template: str = "",
        bb_key: str = ""
    ) -> Dict[str, Any]:
        """
        Add a RunEQSQuery task node.

        Args:
            tree_name: Name of the Behavior Tree
            parent_node_index: Index of parent composite node
            query_template: Name or path of the EQS query template
            bb_key: Blackboard key to store the query result

        Returns:
            Dict with success status and new node index
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("add_bt_task_run_eqs", {
                "tree_name": tree_name,
                "parent_node_index": parent_node_index,
                "query_template": query_template,
                "bb_key": bb_key
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error adding BT RunEQS task: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def add_bt_task_finish_with_result(
        ctx: Context,
        tree_name: str,
        parent_node_index: int,
        result: str = "Succeeded"
    ) -> Dict[str, Any]:
        """
        Add a FinishWithResult task that instantly returns a specified result.

        Args:
            tree_name: Name of the Behavior Tree
            parent_node_index: Index of parent composite node
            result: Result to return - Succeeded, Failed, or Aborted (default: Succeeded)

        Returns:
            Dict with success status and new node index
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("add_bt_task_finish_with_result", {
                "tree_name": tree_name,
                "parent_node_index": parent_node_index,
                "result": result
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error adding BT FinishWithResult task: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def add_bt_task_make_noise(
        ctx: Context,
        tree_name: str,
        parent_node_index: int,
        loudness: float = 1.0
    ) -> Dict[str, Any]:
        """
        Add a MakeNoise task node.

        Args:
            tree_name: Name of the Behavior Tree
            parent_node_index: Index of parent composite node
            loudness: Loudness of the noise (default: 1.0)

        Returns:
            Dict with success status and new node index
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("add_bt_task_make_noise", {
                "tree_name": tree_name,
                "parent_node_index": parent_node_index,
                "loudness": loudness
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error adding BT MakeNoise task: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def add_bt_task_rotate_to_face(
        ctx: Context,
        tree_name: str,
        parent_node_index: int,
        bb_key: str = ""
    ) -> Dict[str, Any]:
        """
        Add a RotateToFaceBBEntry task node.

        Args:
            tree_name: Name of the Behavior Tree
            parent_node_index: Index of parent composite node
            bb_key: Blackboard key containing the target to face

        Returns:
            Dict with success status and new node index
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("add_bt_task_rotate_to_face", {
                "tree_name": tree_name,
                "parent_node_index": parent_node_index,
                "bb_key": bb_key
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error adding BT RotateToFace task: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def add_bt_task_set_key_value(
        ctx: Context,
        tree_name: str,
        parent_node_index: int,
        bb_key: str = "",
        value: str = ""
    ) -> Dict[str, Any]:
        """
        Add a task that sets a blackboard key value.

        Args:
            tree_name: Name of the Behavior Tree
            parent_node_index: Index of parent composite node
            bb_key: Blackboard key to set
            value: Value to assign (as string, parsed by C++ side)

        Returns:
            Dict with success status and new node index
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("add_bt_task_set_key_value", {
                "tree_name": tree_name,
                "parent_node_index": parent_node_index,
                "bb_key": bb_key,
                "value": value
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error adding BT SetKeyValue task: {e}")
            return {"status": "error", "error": str(e)}

    #=========================================================================
    # Decorator Nodes (10 tools)
    #=========================================================================

    @mcp.tool()
    def add_bt_decorator_blackboard(
        ctx: Context,
        tree_name: str,
        target_node_index: int,
        bb_key: str = "",
        notify_observer: str = "OnValueChange",
        flow_abort_mode: str = "None"
    ) -> Dict[str, Any]:
        """
        Add a Blackboard decorator. Gates execution based on BB key value.

        Args:
            tree_name: Name of the Behavior Tree
            target_node_index: Index of the node to attach the decorator to
            bb_key: Blackboard key to observe
            notify_observer: When to re-evaluate - OnValueChange or OnResultChange (default: OnValueChange)
            flow_abort_mode: Abort mode - None, Self, LowerPriority, or Both (default: None)

        Returns:
            Dict with success status and decorator info
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("add_bt_decorator_blackboard", {
                "tree_name": tree_name,
                "target_node_index": target_node_index,
                "bb_key": bb_key,
                "notify_observer": notify_observer,
                "flow_abort_mode": flow_abort_mode
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error adding BT Blackboard decorator: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def add_bt_decorator_cooldown(
        ctx: Context,
        tree_name: str,
        target_node_index: int,
        cooldown_time: float = 5.0
    ) -> Dict[str, Any]:
        """
        Add a Cooldown decorator.

        Args:
            tree_name: Name of the Behavior Tree
            target_node_index: Index of the node to attach the decorator to
            cooldown_time: Cooldown duration in seconds (default: 5.0)

        Returns:
            Dict with success status and decorator info
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("add_bt_decorator_cooldown", {
                "tree_name": tree_name,
                "target_node_index": target_node_index,
                "cooldown_time": cooldown_time
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error adding BT Cooldown decorator: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def add_bt_decorator_loop(
        ctx: Context,
        tree_name: str,
        target_node_index: int,
        num_loops: int = 3,
        infinite_loop: bool = False
    ) -> Dict[str, Any]:
        """
        Add a Loop decorator.

        Args:
            tree_name: Name of the Behavior Tree
            target_node_index: Index of the node to attach the decorator to
            num_loops: Number of loops (default: 3, ignored if infinite_loop is True)
            infinite_loop: Whether to loop indefinitely (default: False)

        Returns:
            Dict with success status and decorator info
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("add_bt_decorator_loop", {
                "tree_name": tree_name,
                "target_node_index": target_node_index,
                "num_loops": num_loops,
                "infinite_loop": infinite_loop
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error adding BT Loop decorator: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def add_bt_decorator_time_limit(
        ctx: Context,
        tree_name: str,
        target_node_index: int,
        time_limit: float = 10.0
    ) -> Dict[str, Any]:
        """
        Add a TimeLimit decorator. Fails subtree after time limit.

        Args:
            tree_name: Name of the Behavior Tree
            target_node_index: Index of the node to attach the decorator to
            time_limit: Maximum execution time in seconds (default: 10.0)

        Returns:
            Dict with success status and decorator info
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("add_bt_decorator_time_limit", {
                "tree_name": tree_name,
                "target_node_index": target_node_index,
                "time_limit": time_limit
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error adding BT TimeLimit decorator: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def add_bt_decorator_force_success(
        ctx: Context,
        tree_name: str,
        target_node_index: int
    ) -> Dict[str, Any]:
        """
        Add a ForceSuccess decorator. Changes child result to Success.

        Args:
            tree_name: Name of the Behavior Tree
            target_node_index: Index of the node to attach the decorator to

        Returns:
            Dict with success status and decorator info
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("add_bt_decorator_force_success", {
                "tree_name": tree_name,
                "target_node_index": target_node_index
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error adding BT ForceSuccess decorator: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def add_bt_decorator_compare_bb(
        ctx: Context,
        tree_name: str,
        target_node_index: int,
        key_a: str = "",
        key_b: str = "",
        operator: str = "IsEqualTo"
    ) -> Dict[str, Any]:
        """
        Add a CompareBBEntries decorator.

        Args:
            tree_name: Name of the Behavior Tree
            target_node_index: Index of the node to attach the decorator to
            key_a: First blackboard key to compare
            key_b: Second blackboard key to compare
            operator: Comparison operator - IsEqualTo or IsNotEqualTo (default: IsEqualTo)

        Returns:
            Dict with success status and decorator info
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("add_bt_decorator_compare_bb", {
                "tree_name": tree_name,
                "target_node_index": target_node_index,
                "key_a": key_a,
                "key_b": key_b,
                "operator": operator
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error adding BT CompareBBEntries decorator: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def add_bt_decorator_cone_check(
        ctx: Context,
        tree_name: str,
        target_node_index: int,
        cone_half_angle: float = 45.0,
        cone_origin_key: str = "",
        observed_key: str = ""
    ) -> Dict[str, Any]:
        """
        Add a ConeCheck decorator.

        Args:
            tree_name: Name of the Behavior Tree
            target_node_index: Index of the node to attach the decorator to
            cone_half_angle: Half-angle of the cone in degrees (default: 45.0)
            cone_origin_key: Blackboard key for the cone origin
            observed_key: Blackboard key for the observed target

        Returns:
            Dict with success status and decorator info
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("add_bt_decorator_cone_check", {
                "tree_name": tree_name,
                "target_node_index": target_node_index,
                "cone_half_angle": cone_half_angle,
                "cone_origin_key": cone_origin_key,
                "observed_key": observed_key
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error adding BT ConeCheck decorator: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def add_bt_decorator_does_path_exist(
        ctx: Context,
        tree_name: str,
        target_node_index: int,
        bb_key_a: str = "",
        bb_key_b: str = ""
    ) -> Dict[str, Any]:
        """
        Add a DoesPathExist decorator.

        Args:
            tree_name: Name of the Behavior Tree
            target_node_index: Index of the node to attach the decorator to
            bb_key_a: Blackboard key for the start location
            bb_key_b: Blackboard key for the end location

        Returns:
            Dict with success status and decorator info
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("add_bt_decorator_does_path_exist", {
                "tree_name": tree_name,
                "target_node_index": target_node_index,
                "bb_key_a": bb_key_a,
                "bb_key_b": bb_key_b
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error adding BT DoesPathExist decorator: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def add_bt_decorator_is_at_location(
        ctx: Context,
        tree_name: str,
        target_node_index: int,
        bb_key: str = "",
        acceptable_radius: float = 5.0
    ) -> Dict[str, Any]:
        """
        Add an IsAtLocation decorator.

        Args:
            tree_name: Name of the Behavior Tree
            target_node_index: Index of the node to attach the decorator to
            bb_key: Blackboard key containing the target location
            acceptable_radius: Distance threshold (default: 5.0)

        Returns:
            Dict with success status and decorator info
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("add_bt_decorator_is_at_location", {
                "tree_name": tree_name,
                "target_node_index": target_node_index,
                "bb_key": bb_key,
                "acceptable_radius": acceptable_radius
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error adding BT IsAtLocation decorator: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def add_bt_decorator_tag_cooldown(
        ctx: Context,
        tree_name: str,
        target_node_index: int,
        cooldown_tag: str = "",
        cooldown_duration: float = 5.0,
        add_to_existing: bool = True
    ) -> Dict[str, Any]:
        """
        Add a TagCooldown decorator.

        Args:
            tree_name: Name of the Behavior Tree
            target_node_index: Index of the node to attach the decorator to
            cooldown_tag: Gameplay tag for the cooldown
            cooldown_duration: Cooldown duration in seconds (default: 5.0)
            add_to_existing: Whether to add to existing cooldown or reset (default: True)

        Returns:
            Dict with success status and decorator info
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("add_bt_decorator_tag_cooldown", {
                "tree_name": tree_name,
                "target_node_index": target_node_index,
                "cooldown_tag": cooldown_tag,
                "cooldown_duration": cooldown_duration,
                "add_to_existing": add_to_existing
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error adding BT TagCooldown decorator: {e}")
            return {"status": "error", "error": str(e)}

    #=========================================================================
    # Service Nodes (2 tools)
    #=========================================================================

    @mcp.tool()
    def add_bt_service_default_focus(
        ctx: Context,
        tree_name: str,
        target_node_index: int,
        bb_key: str = "",
        interval: float = 0.5,
        random_deviation: float = 0.1
    ) -> Dict[str, Any]:
        """
        Add a DefaultFocus service. Auto-sets AI focus to BB key actor/location.

        Args:
            tree_name: Name of the Behavior Tree
            target_node_index: Index of the node to attach the service to
            bb_key: Blackboard key containing the focus target
            interval: Tick interval in seconds (default: 0.5)
            random_deviation: Random deviation added to interval (default: 0.1)

        Returns:
            Dict with success status and service info
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("add_bt_service_default_focus", {
                "tree_name": tree_name,
                "target_node_index": target_node_index,
                "bb_key": bb_key,
                "interval": interval,
                "random_deviation": random_deviation
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error adding BT DefaultFocus service: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def add_bt_service_run_eqs(
        ctx: Context,
        tree_name: str,
        target_node_index: int,
        query_template: str = "",
        bb_key: str = "",
        interval: float = 0.5,
        random_deviation: float = 0.1
    ) -> Dict[str, Any]:
        """
        Add a RunEQS service. Periodically runs EQS query.

        Args:
            tree_name: Name of the Behavior Tree
            target_node_index: Index of the node to attach the service to
            query_template: Name or path of the EQS query template
            bb_key: Blackboard key to store query results
            interval: Tick interval in seconds (default: 0.5)
            random_deviation: Random deviation added to interval (default: 0.1)

        Returns:
            Dict with success status and service info
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("add_bt_service_run_eqs", {
                "tree_name": tree_name,
                "target_node_index": target_node_index,
                "query_template": query_template,
                "bb_key": bb_key,
                "interval": interval,
                "random_deviation": random_deviation
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error adding BT RunEQS service: {e}")
            return {"status": "error", "error": str(e)}

    #=========================================================================
    # Structure Commands (9 tools)
    #=========================================================================

    @mcp.tool()
    def connect_bt_nodes(
        ctx: Context,
        tree_name: str,
        parent_index: int,
        child_index: int
    ) -> Dict[str, Any]:
        """
        Connect two BT nodes (parent output to child input).

        Args:
            tree_name: Name of the Behavior Tree
            parent_index: Index of the parent node
            child_index: Index of the child node

        Returns:
            Dict with success status
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("connect_bt_nodes", {
                "tree_name": tree_name,
                "parent_index": parent_index,
                "child_index": child_index
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error connecting BT nodes: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def disconnect_bt_nodes(
        ctx: Context,
        tree_name: str,
        parent_index: int,
        child_index: int
    ) -> Dict[str, Any]:
        """
        Disconnect two BT nodes.

        Args:
            tree_name: Name of the Behavior Tree
            parent_index: Index of the parent node
            child_index: Index of the child node

        Returns:
            Dict with success status
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("disconnect_bt_nodes", {
                "tree_name": tree_name,
                "parent_index": parent_index,
                "child_index": child_index
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error disconnecting BT nodes: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def reorder_bt_children(
        ctx: Context,
        tree_name: str,
        parent_index: int,
        child_order: List[int] = []
    ) -> Dict[str, Any]:
        """
        Reorder children of a composite node.

        Args:
            tree_name: Name of the Behavior Tree
            parent_index: Index of the parent composite node
            child_order: List of child indices in the desired order

        Returns:
            Dict with success status
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("reorder_bt_children", {
                "tree_name": tree_name,
                "parent_index": parent_index,
                "child_order": child_order
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error reordering BT children: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def delete_bt_node(
        ctx: Context,
        tree_name: str,
        node_index: int
    ) -> Dict[str, Any]:
        """
        Delete a node from the Behavior Tree.

        Args:
            tree_name: Name of the Behavior Tree
            node_index: Index of the node to delete

        Returns:
            Dict with success status
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("delete_bt_node", {
                "tree_name": tree_name,
                "node_index": node_index
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error deleting BT node: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def move_bt_node(
        ctx: Context,
        tree_name: str,
        node_index: int,
        x: int = 0,
        y: int = 0
    ) -> Dict[str, Any]:
        """
        Move a BT node to a new position in the graph.

        Args:
            tree_name: Name of the Behavior Tree
            node_index: Index of the node to move
            x: New X position in the graph (default: 0)
            y: New Y position in the graph (default: 0)

        Returns:
            Dict with success status
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("move_bt_node", {
                "tree_name": tree_name,
                "node_index": node_index,
                "x": x,
                "y": y
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error moving BT node: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def auto_arrange_bt(
        ctx: Context,
        tree_name: str
    ) -> Dict[str, Any]:
        """
        Auto-arrange all nodes in the Behavior Tree graph.

        Args:
            tree_name: Name of the Behavior Tree

        Returns:
            Dict with success status
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("auto_arrange_bt", {
                "tree_name": tree_name
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error auto-arranging BT: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def get_bt_tree_structure(
        ctx: Context,
        tree_name: str
    ) -> Dict[str, Any]:
        """
        Get the full tree structure as a JSON hierarchy.

        Args:
            tree_name: Name of the Behavior Tree

        Returns:
            Dict with tree structure including all nodes, connections, and hierarchy
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("get_bt_tree_structure", {
                "tree_name": tree_name
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error getting BT tree structure: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def get_bt_node_info(
        ctx: Context,
        tree_name: str,
        node_index: int
    ) -> Dict[str, Any]:
        """
        Get detailed information about a specific BT node.

        Args:
            tree_name: Name of the Behavior Tree
            node_index: Index of the node to query

        Returns:
            Dict with node type, properties, decorators, services, and connections
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("get_bt_node_info", {
                "tree_name": tree_name,
                "node_index": node_index
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error getting BT node info: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def set_bt_node_property(
        ctx: Context,
        tree_name: str,
        node_index: int,
        property_name: str = "",
        value: str = ""
    ) -> Dict[str, Any]:
        """
        Set a property value on a BT node.

        Args:
            tree_name: Name of the Behavior Tree
            node_index: Index of the node to modify
            property_name: Name of the property to set
            value: Value to assign (as string, parsed by C++ side)

        Returns:
            Dict with success status
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("set_bt_node_property", {
                "tree_name": tree_name,
                "node_index": node_index,
                "property_name": property_name,
                "value": value
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error setting BT node property: {e}")
            return {"status": "error", "error": str(e)}

    #=========================================================================
    # Runtime Commands (10 tools)
    #=========================================================================

    @mcp.tool()
    def run_behavior_tree(
        ctx: Context,
        actor_name: str,
        tree_name: str
    ) -> Dict[str, Any]:
        """
        Run a Behavior Tree on an AI-controlled actor (PIE only).

        Args:
            actor_name: Name of the AI actor in the level
            tree_name: Name or path of the Behavior Tree asset to run

        Returns:
            Dict with success status
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("run_behavior_tree", {
                "actor_name": actor_name,
                "tree_name": tree_name
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error running Behavior Tree: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def stop_behavior_tree(
        ctx: Context,
        actor_name: str,
        stop_mode: str = "Safe"
    ) -> Dict[str, Any]:
        """
        Stop a running Behavior Tree.

        Args:
            actor_name: Name of the AI actor in the level
            stop_mode: Stop mode - Safe or Forced (default: Safe)

        Returns:
            Dict with success status
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("stop_behavior_tree", {
                "actor_name": actor_name,
                "stop_mode": stop_mode
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error stopping Behavior Tree: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def pause_behavior_tree(
        ctx: Context,
        actor_name: str,
        reason: str = "MCP"
    ) -> Dict[str, Any]:
        """
        Pause a running Behavior Tree.

        Args:
            actor_name: Name of the AI actor in the level
            reason: Reason string for the pause (default: MCP)

        Returns:
            Dict with success status
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("pause_behavior_tree", {
                "actor_name": actor_name,
                "reason": reason
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error pausing Behavior Tree: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def resume_behavior_tree(
        ctx: Context,
        actor_name: str,
        reason: str = "MCP"
    ) -> Dict[str, Any]:
        """
        Resume a paused Behavior Tree.

        Args:
            actor_name: Name of the AI actor in the level
            reason: Reason string that was used to pause (default: MCP)

        Returns:
            Dict with success status
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("resume_behavior_tree", {
                "actor_name": actor_name,
                "reason": reason
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error resuming Behavior Tree: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def restart_behavior_tree(
        ctx: Context,
        actor_name: str
    ) -> Dict[str, Any]:
        """
        Restart a running Behavior Tree.

        Args:
            actor_name: Name of the AI actor in the level

        Returns:
            Dict with success status
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("restart_behavior_tree", {
                "actor_name": actor_name
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error restarting Behavior Tree: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def get_bt_runtime_state(
        ctx: Context,
        actor_name: str
    ) -> Dict[str, Any]:
        """
        Get the runtime state of a Behavior Tree on an actor.

        Args:
            actor_name: Name of the AI actor in the level

        Returns:
            Dict with runtime state including active nodes, execution path, and status
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("get_bt_runtime_state", {
                "actor_name": actor_name
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error getting BT runtime state: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def set_blackboard_value(
        ctx: Context,
        actor_name: str,
        key_name: str,
        value_type: str,
        value: str
    ) -> Dict[str, Any]:
        """
        Set a blackboard value on a running AI actor.

        Args:
            actor_name: Name of the AI actor in the level
            key_name: Name of the blackboard key
            value_type: Type of value - Bool, Float, Int, Vector, String, Object, Name, Rotator, Enum
            value: Value as string (parsed by C++ side based on value_type; for Vector use "x,y,z" format)

        Returns:
            Dict with success status
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("set_blackboard_value", {
                "actor_name": actor_name,
                "key_name": key_name,
                "value_type": value_type,
                "value": value
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error setting blackboard value: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def get_blackboard_value(
        ctx: Context,
        actor_name: str,
        key_name: str
    ) -> Dict[str, Any]:
        """
        Get a blackboard value from a running AI actor.

        Args:
            actor_name: Name of the AI actor in the level
            key_name: Name of the blackboard key

        Returns:
            Dict with key value and type
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("get_blackboard_value", {
                "actor_name": actor_name,
                "key_name": key_name
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error getting blackboard value: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def get_blackboard_state(
        ctx: Context,
        actor_name: str
    ) -> Dict[str, Any]:
        """
        Get all blackboard key values from a running AI actor.

        Args:
            actor_name: Name of the AI actor in the level

        Returns:
            Dict with all blackboard keys, their types, and current values
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("get_blackboard_state", {
                "actor_name": actor_name
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error getting blackboard state: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def clear_blackboard_value(
        ctx: Context,
        actor_name: str,
        key_name: str
    ) -> Dict[str, Any]:
        """
        Clear a blackboard value on a running AI actor.

        Args:
            actor_name: Name of the AI actor in the level
            key_name: Name of the blackboard key to clear

        Returns:
            Dict with success status
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("clear_blackboard_value", {
                "actor_name": actor_name,
                "key_name": key_name
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error clearing blackboard value: {e}")
            return {"status": "error", "error": str(e)}
