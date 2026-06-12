#include "pch.h"
#include "Collision.h"

std::array<std::array<bool, WORLD_WIDTH>, WORLD_HEIGHT> GCollision;

void InitCollisionTile()
{
	for (auto& row : GCollision)
	{
		row.fill(false);
	}

	std::ifstream file1("map_obstacle.csv");
	if (file1.is_open())
	{
		std::string line, cell;
		int y = 0;
		while (std::getline(file1, line) && y < WORLD_HEIGHT)
		{
			std::stringstream ss(line);
			int x = 0;
			while (std::getline(ss, cell, ',') && x < WORLD_WIDTH)
			{
				if (std::stoi(cell) > 0)
				{
					GCollision[y][x] = true; 
				}
				x++;
			}
			y++;
		}
		file1.close();
	}

	std::ifstream file2("map_obstacle2.csv");
	if (file2.is_open())
	{
		std::string line, cell;
		int y = 0;
		while (std::getline(file2, line) && y < WORLD_HEIGHT)
		{
			std::stringstream ss(line);
			int x = 0;
			while (std::getline(ss, cell, ',') && x < WORLD_WIDTH)
			{
				if (std::stoi(cell) > 0)
				{
					GCollision[y][x] = true;
				}
				x++;
			}
			y++;
		}
		file2.close();
	}
}

bool isCollision(short x_pos, short y_pos)
{
	if (x_pos < 0 || x_pos >= WORLD_WIDTH || y_pos < 0 || y_pos >= WORLD_HEIGHT)
		return true;

	return GCollision[y_pos][x_pos];
}