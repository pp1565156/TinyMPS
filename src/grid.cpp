#include "grid.h"

#include <algorithm>
#include <cmath>
#include <iostream>

namespace tiny_mps
{

Grid::Grid(double grid_width, const MatrixXd& coordinates, const Matrix<bool, Dynamic, 1> & valid_coordinates, int dimension) : coordinates(coordinates), valid_coordinates(valid_coordinates)
{
	this->grid_width = grid_width;
	this->dimension = dimension;
	size = coordinates.cols();

	resetHash();
}

Grid::~Grid()
{
	begin_hash.clear();
}

void
Grid::getNeighbors(int hash, int& begin, int& end)
{
	if(begin_hash.empty())
	{
		begin = -1; end = -1; return;
	}

	if(begin_hash.find(hash) == begin_hash.end())
	{
		begin = -1; end = -1; return;
	}

	begin = begin_hash[hash].first;
	end = begin_hash[hash].second;
}

void
Grid::sumNeighborScalars(VectorXd& output, std::function<double(int, int)> interaction)
{
	output = VectorXd::Zero(size);
	for(int i_particle = 0; i_particle < size; i_particle++)
	{
		if(isValidCoordinates(i_particle) == 0) continue;

		int ix, iy, iz;
		toIndex(coordinates.col(i_particle), ix, iy, iz);
		int x_begin, x_end, y_begin, y_end, z_begin, z_end;
		x_begin = std::max(ix - 1, 0);
		y_begin = std::max(iy - 1, 0);
		z_begin = std::max(iz - 1, 0);
		x_end = std::min(ix + 1, getGridNumberX() - 1);
		y_end = std::min(iy + 1, getGridNumberY() - 1);
		z_end = std::min(iz + 1, getGridNumberZ() - 1);
		if(dimension == 2)
		{
			z_begin = 0;
			z_end = 0;
		}

		double ans = 0;
		for(int i = z_begin; i <= z_end; i++)
		{
			for(int j = y_begin; j <= y_end; j++)
			{
				for(int k = x_begin; k <= x_end; k++)
				{
					int begin, end;
					getNeighbors(toHash(k, j, i), begin, end);
					if(begin == -1 || end == -1) continue;

					Vector3d r_i = coordinates.col(i_particle);
					for(int n = begin; n <= end; n++)
					{
						int j_particle = grid_hash[n].second;
						if(i_particle == j_particle) continue;
						Vector3d r_j = coordinates.col(j_particle);
						Vector3d r_ji = r_j - r_i;
						if(r_ji.norm() > grid_width) continue;

						ans += interaction(i_particle, j_particle);						
					}
				}
			}
		}
		output(i_particle) = ans;
	}
}

void 
Grid::sumNeighborVectors(MatrixXd& output, std::function<void(int, int, const Vector3d&)> interaction)
{
	output = MatrixXd::Zero(3, size);
	for(int i_particle = 0; i_particle < size; i_particle++)
	{
		if(isValidCoordinates(i_particle) == 0) continue;

		int ix, iy, iz;
		toIndex(coordinates.col(i_particle), ix, iy, iz);
		int x_begin, x_end, y_begin, y_end, z_begin, z_end;
		x_begin = std::max(ix - 1, 0);
		y_begin = std::max(iy - 1, 0);
		z_begin = std::max(iz - 1, 0);
		x_end = std::min(ix + 1, getGridNumberX() - 1);
		y_end = std::min(iy + 1, getGridNumberY() - 1);
		z_end = std::min(iz + 1, getGridNumberZ() - 1);
		if(dimension == 2)
		{
			z_begin = 0;
			z_end = 0;
		}

		Vector3d ans;
		for(int i = z_begin; i <= z_end; i++)
		{
			for(int j = y_begin; j <= y_end; j++)
			{
				for(int k = x_begin; k <= x_end; k++)
				{
					int begin, end;
					getNeighbors(toHash(k, j, i), begin, end);
					if(begin == -1 || end == -1) continue;

					Vector3d r_i = coordinates.col(i_particle);
					for(int n = begin; n <= end; n++)
					{
						int j_particle = grid_hash[n].second;
						if(i_particle == j_particle) continue;						
						Vector3d r_j = coordinates.col(j_particle);
						Vector3d r_ji = r_j - r_i;
						if(r_ji.norm() > grid_width) continue;

						Vector3d tmp;
						interaction(i_particle, j_particle, tmp);
						ans += tmp;
					}
				}
			}
		}
		output.col(i_particle) = ans;
	}
}

void
Grid::resetHash()
{
	if(!begin_hash.empty()) begin_hash.clear();
	if(!grid_hash.empty()) grid_hash.clear();

	int pt_num = coordinates.cols();
	if(pt_num == 0) return;

	grid_hash.resize(pt_num);

	getMaxCoordinates(higher_bounds);
	getMinCoordinates(lower_bounds);
	Vector3d diff = higher_bounds - lower_bounds;
	if(getDimension() == 2) diff(2) = 0;
	for(int i = 0; i < 3; i++)
	{
		grid_number[i] = std::ceil(diff(i) / grid_width) + 1;
	}

	for(int i = 0; i < pt_num; i++)
	{
		grid_hash[i] = std::make_pair(toHash(coordinates.col(i)), i);
	}

	std::sort(grid_hash.begin(), grid_hash.end());

	int start_i = 0;
	int start_value = grid_hash[0].first;
	for(int i = 1; i < pt_num; i++)
	{
		if(start_value != grid_hash[i].first)
		{
			begin_hash[start_value] = std::make_pair(start_i, i - 1);
			start_i = i;
			start_value = grid_hash[i].first;
		}

		if(i == pt_num - 1)
		{
			begin_hash[start_value] = std::make_pair(start_i, i);
		}
	}
}

}