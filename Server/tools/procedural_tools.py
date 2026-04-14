"""
Procedural World Building Tools for Unreal MCP.

High-level tools that compose spawn_actor + set_actor_transform calls
to generate walls, towers, staircases, arches, pyramids, and mazes.
"""

import logging
import math
import random
from typing import Dict, List, Any, Optional
from mcp.server.fastmcp import FastMCP, Context

logger = logging.getLogger("UnrealMCP")

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def _get_connection():
    from unreal_mcp_server import get_unreal_connection
    unreal = get_unreal_connection()
    if not unreal:
        raise RuntimeError("Failed to connect to Unreal Engine")
    return unreal


def _spawn_block(unreal, name: str, location: List[float],
                 rotation: List[float] = None,
                 scale: List[float] = None) -> Optional[Dict]:
    """Spawn a StaticMeshActor cube and optionally scale it."""
    rotation = rotation or [0.0, 0.0, 0.0]
    resp = unreal.send_command("spawn_actor", {
        "name": name,
        "type": "STATICMESHACTOR",
        "location": [float(v) for v in location],
        "rotation": [float(v) for v in rotation],
    })
    if resp and resp.get("status") != "error" and scale and scale != [1.0, 1.0, 1.0]:
        unreal.send_command("set_actor_transform", {
            "name": name,
            "location": [float(v) for v in location],
            "rotation": [float(v) for v in rotation],
            "scale": [float(v) for v in scale],
        })
    return resp


# ---------------------------------------------------------------------------
# Registration
# ---------------------------------------------------------------------------

def register_procedural_tools(mcp: FastMCP):
    """Register procedural world-building tools with the MCP server."""

    @mcp.tool()
    def create_wall(
        ctx: Context,
        name_prefix: str = "Wall",
        location: List[float] = [0.0, 0.0, 0.0],
        length: float = 1000.0,
        height: float = 300.0,
        thickness: float = 20.0,
        segment_size: float = 100.0,
        rotation_yaw: float = 0.0,
    ) -> Dict[str, Any]:
        """Build a wall from tiled cube segments.

        Args:
            name_prefix: Prefix for spawned actor names.
            location: World-space origin [X, Y, Z] of the wall start.
            length: Total wall length in cm.
            height: Wall height in cm.
            thickness: Wall thickness in cm.
            segment_size: Size of each cube segment in cm (default 100 = 1m UE cube).
            rotation_yaw: Yaw rotation of the entire wall in degrees.
        """
        try:
            unreal = _get_connection()
            yaw_rad = math.radians(rotation_yaw)
            dir_x = math.cos(yaw_rad)
            dir_y = math.sin(yaw_rad)

            cols = max(1, int(length / segment_size))
            rows = max(1, int(height / segment_size))
            scale_x = (length / cols) / 100.0
            scale_y = thickness / 100.0
            scale_z = (height / rows) / 100.0

            spawned = 0
            for r in range(rows):
                for c in range(cols):
                    offset = (c * scale_x * 100.0) + (scale_x * 50.0)
                    z = location[2] + (r * scale_z * 100.0) + (scale_z * 50.0)
                    x = location[0] + dir_x * offset
                    y = location[1] + dir_y * offset
                    actor_name = f"{name_prefix}_{r}_{c}"
                    _spawn_block(unreal, actor_name,
                                 [x, y, z],
                                 [0.0, rotation_yaw, 0.0],
                                 [scale_x, scale_y, scale_z])
                    spawned += 1

            return {"success": True, "actors_spawned": spawned,
                    "dimensions": {"cols": cols, "rows": rows}}
        except Exception as e:
            logger.error(f"create_wall error: {e}")
            return {"success": False, "error": str(e)}

    @mcp.tool()
    def create_tower(
        ctx: Context,
        name_prefix: str = "Tower",
        location: List[float] = [0.0, 0.0, 0.0],
        base_size: float = 300.0,
        height: float = 1000.0,
        floors: int = 5,
        style: str = "square",
        taper: float = 0.0,
    ) -> Dict[str, Any]:
        """Build a tower by stacking scaled cubes.

        Args:
            name_prefix: Prefix for spawned actor names.
            location: World-space origin [X, Y, Z] of the tower base centre.
            base_size: Width/depth of the base floor in cm.
            height: Total tower height in cm.
            floors: Number of stacked floor slabs.
            style: 'square' or 'tapered'. Tapered reduces size per floor.
            taper: Fraction to shrink per floor (0.0-0.15). Only used if style='tapered'.
        """
        try:
            unreal = _get_connection()
            floor_h = height / floors
            scale_z = floor_h / 100.0
            spawned = 0

            for f in range(floors):
                shrink = 1.0 - (taper * f) if style == "tapered" else 1.0
                shrink = max(0.3, shrink)
                sz = base_size * shrink
                scale_xy = sz / 100.0
                z = location[2] + (f * floor_h) + (floor_h * 0.5)
                actor_name = f"{name_prefix}_F{f}"
                _spawn_block(unreal, actor_name,
                             [location[0], location[1], z],
                             [0.0, 0.0, 0.0],
                             [scale_xy, scale_xy, scale_z])
                spawned += 1

            return {"success": True, "actors_spawned": spawned, "floors": floors}
        except Exception as e:
            logger.error(f"create_tower error: {e}")
            return {"success": False, "error": str(e)}

    @mcp.tool()
    def create_staircase(
        ctx: Context,
        name_prefix: str = "Stair",
        location: List[float] = [0.0, 0.0, 0.0],
        steps: int = 10,
        step_width: float = 150.0,
        step_depth: float = 40.0,
        step_height: float = 20.0,
        rotation_yaw: float = 0.0,
    ) -> Dict[str, Any]:
        """Build a staircase from stepped cubes.

        Args:
            name_prefix: Prefix for spawned actor names.
            location: World-space origin [X, Y, Z] at the bottom-front of the stairs.
            steps: Number of steps.
            step_width: Width of each step in cm.
            step_depth: Depth (tread) of each step in cm.
            step_height: Height (rise) of each step in cm.
            rotation_yaw: Yaw rotation of the staircase in degrees.
        """
        try:
            unreal = _get_connection()
            yaw_rad = math.radians(rotation_yaw)
            dir_x = math.cos(yaw_rad)
            dir_y = math.sin(yaw_rad)

            scale_x = step_depth / 100.0
            scale_y = step_width / 100.0
            scale_z = step_height / 100.0
            spawned = 0

            for i in range(steps):
                offset = i * step_depth + step_depth * 0.5
                x = location[0] + dir_x * offset
                y = location[1] + dir_y * offset
                z = location[2] + (i * step_height) + (step_height * 0.5)
                actor_name = f"{name_prefix}_{i}"
                _spawn_block(unreal, actor_name,
                             [x, y, z],
                             [0.0, rotation_yaw, 0.0],
                             [scale_x, scale_y, scale_z])
                spawned += 1

            return {"success": True, "actors_spawned": spawned, "steps": steps}
        except Exception as e:
            logger.error(f"create_staircase error: {e}")
            return {"success": False, "error": str(e)}

    @mcp.tool()
    def create_arch(
        ctx: Context,
        name_prefix: str = "Arch",
        location: List[float] = [0.0, 0.0, 0.0],
        radius: float = 300.0,
        thickness: float = 50.0,
        depth: float = 100.0,
        segments: int = 12,
        rotation_yaw: float = 0.0,
    ) -> Dict[str, Any]:
        """Build a semicircular arch from rotated cube segments.

        Args:
            name_prefix: Prefix for spawned actor names.
            location: World-space origin [X, Y, Z] at the arch base centre.
            radius: Arch inner radius in cm.
            thickness: Block thickness in cm.
            depth: Arch depth (into screen) in cm.
            segments: Number of segments in the semicircle.
            rotation_yaw: Yaw rotation of the entire arch in degrees.
        """
        try:
            unreal = _get_connection()
            spawned = 0
            arc_len_per_seg = (math.pi * radius) / segments
            scale_x = arc_len_per_seg / 100.0
            scale_y = depth / 100.0
            scale_z = thickness / 100.0
            centre_r = radius + thickness * 0.5

            for i in range(segments):
                angle = math.pi * (i + 0.5) / segments
                cx = location[0] + math.cos(angle) * centre_r
                cz = location[2] + math.sin(angle) * centre_r
                roll = math.degrees(angle) - 90.0
                actor_name = f"{name_prefix}_{i}"
                _spawn_block(unreal, actor_name,
                             [cx, location[1], cz],
                             [0.0, rotation_yaw, roll],
                             [scale_x, scale_y, scale_z])
                spawned += 1

            return {"success": True, "actors_spawned": spawned, "segments": segments}
        except Exception as e:
            logger.error(f"create_arch error: {e}")
            return {"success": False, "error": str(e)}

    @mcp.tool()
    def create_pyramid(
        ctx: Context,
        name_prefix: str = "Pyramid",
        location: List[float] = [0.0, 0.0, 0.0],
        base_size: float = 1000.0,
        layers: int = 10,
        layer_height: float = 50.0,
    ) -> Dict[str, Any]:
        """Build a stepped pyramid from stacked, shrinking square layers.

        Args:
            name_prefix: Prefix for spawned actor names.
            location: World-space origin [X, Y, Z] at the pyramid base centre.
            base_size: Width/depth of the bottom layer in cm.
            layers: Number of stacked layers.
            layer_height: Height of each layer in cm.
        """
        try:
            unreal = _get_connection()
            spawned = 0
            scale_z = layer_height / 100.0

            for i in range(layers):
                t = i / max(1, layers - 1)
                sz = base_size * (1.0 - t * 0.9)
                scale_xy = sz / 100.0
                z = location[2] + (i * layer_height) + (layer_height * 0.5)
                actor_name = f"{name_prefix}_L{i}"
                _spawn_block(unreal, actor_name,
                             [location[0], location[1], z],
                             [0.0, 0.0, 0.0],
                             [scale_xy, scale_xy, scale_z])
                spawned += 1

            return {"success": True, "actors_spawned": spawned, "layers": layers}
        except Exception as e:
            logger.error(f"create_pyramid error: {e}")
            return {"success": False, "error": str(e)}

    @mcp.tool()
    def create_maze(
        ctx: Context,
        name_prefix: str = "Maze",
        location: List[float] = [0.0, 0.0, 0.0],
        rows: int = 8,
        cols: int = 8,
        cell_size: float = 200.0,
        wall_height: float = 300.0,
        wall_thickness: float = 20.0,
        seed: int = 0,
    ) -> Dict[str, Any]:
        """Generate a solvable maze via recursive backtracking.

        The entrance is at (0,0) and the exit at (rows-1, cols-1).

        Args:
            name_prefix: Prefix for spawned actor names.
            location: World-space origin [X, Y, Z] of the maze corner.
            rows: Number of cell rows.
            cols: Number of cell columns.
            cell_size: Size of each cell in cm.
            wall_height: Height of maze walls in cm.
            wall_thickness: Thickness of maze walls in cm.
            seed: Random seed (0 = random).
        """
        try:
            unreal = _get_connection()
            rng = random.Random(seed if seed != 0 else None)

            # --- Generate maze grid using recursive backtracking ---
            # Each cell stores which walls are open: set of ('N','S','E','W')
            grid = [[set() for _ in range(cols)] for _ in range(rows)]
            visited = [[False] * cols for _ in range(rows)]
            opposites = {'N': 'S', 'S': 'N', 'E': 'W', 'W': 'E'}
            deltas = {'N': (-1, 0), 'S': (1, 0), 'E': (0, 1), 'W': (0, -1)}

            stack = [(0, 0)]
            visited[0][0] = True
            while stack:
                r, c = stack[-1]
                neighbors = []
                for d, (dr, dc) in deltas.items():
                    nr, nc = r + dr, c + dc
                    if 0 <= nr < rows and 0 <= nc < cols and not visited[nr][nc]:
                        neighbors.append((d, nr, nc))
                if neighbors:
                    d, nr, nc = rng.choice(neighbors)
                    grid[r][c].add(d)
                    grid[nr][nc].add(opposites[d])
                    visited[nr][nc] = True
                    stack.append((nr, nc))
                else:
                    stack.pop()

            # --- Spawn walls ---
            scale_h = wall_height / 100.0
            scale_t = wall_thickness / 100.0
            scale_cell = cell_size / 100.0
            spawned = 0

            for r in range(rows):
                for c in range(cols):
                    cx = location[0] + c * cell_size + cell_size * 0.5
                    cy = location[1] + r * cell_size + cell_size * 0.5
                    cz = location[2] + wall_height * 0.5

                    # North wall (top of cell, along X axis)
                    if 'N' not in grid[r][c]:
                        wx = cx
                        wy = cy - cell_size * 0.5
                        actor_name = f"{name_prefix}_N_{r}_{c}"
                        _spawn_block(unreal, actor_name,
                                     [wx, wy, cz],
                                     [0.0, 0.0, 0.0],
                                     [scale_cell, scale_t, scale_h])
                        spawned += 1

                    # West wall (left of cell, along Y axis)
                    if 'W' not in grid[r][c]:
                        wx = cx - cell_size * 0.5
                        wy = cy
                        actor_name = f"{name_prefix}_W_{r}_{c}"
                        _spawn_block(unreal, actor_name,
                                     [wx, wy, cz],
                                     [0.0, 0.0, 0.0],
                                     [scale_t, scale_cell, scale_h])
                        spawned += 1

            # Bottom and right border walls
            for c in range(cols):
                bx = location[0] + c * cell_size + cell_size * 0.5
                by = location[1] + rows * cell_size
                bz = location[2] + wall_height * 0.5
                actor_name = f"{name_prefix}_BS_{c}"
                _spawn_block(unreal, actor_name,
                             [bx, by, bz],
                             [0.0, 0.0, 0.0],
                             [scale_cell, scale_t, scale_h])
                spawned += 1

            for r in range(rows):
                bx = location[0] + cols * cell_size
                by = location[1] + r * cell_size + cell_size * 0.5
                bz = location[2] + wall_height * 0.5
                actor_name = f"{name_prefix}_RE_{r}"
                _spawn_block(unreal, actor_name,
                             [bx, by, bz],
                             [0.0, 0.0, 0.0],
                             [scale_t, scale_cell, scale_h])
                spawned += 1

            return {"success": True, "actors_spawned": spawned,
                    "grid_size": {"rows": rows, "cols": cols}}
        except Exception as e:
            logger.error(f"create_maze error: {e}")
            return {"success": False, "error": str(e)}
