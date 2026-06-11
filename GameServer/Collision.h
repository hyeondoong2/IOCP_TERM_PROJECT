#pragma once

extern std::array<std::array<bool, WORLD_WIDTH>, WORLD_HEIGHT> GCollision;

void InitCollisionTile();
bool isCollision(short x_pos, short y_pos);