#include <iostream>
#include "../SWC2Obj/SWCReader.h"
#include "../SWC2Obj/SWCReader.cpp"
#include "../SWC2Obj/cmdline.h"

std::vector< Path > searchPath(std::vector<Vertex>& point_vector)
{
	std::vector<int> leaf_nodes;
	std::map<int, int> vertex_hash;
	for (auto i = 0; i < point_vector.size(); i++)
	{
		vertex_hash[point_vector[i].current_id] = i;
	}
	for (auto& i : point_vector)
	{
		if (i.previous_id != -1)
		{
			point_vector[vertex_hash[i.previous_id]].degree++;
		}
	}
	for (auto& i : point_vector)
	{
		if (i.degree == 0 && i.previous_id != -1)
		{
			leaf_nodes.push_back(i.current_id);
		}
	}
	std::vector< Path > paths;
	auto path_index = 0;
	for (auto& left_node : leaf_nodes)
	{
		Path path;
		std::vector<Vertex> vertices;

		auto cur_idx = left_node;
		while (cur_idx != -1 && !point_vector[vertex_hash[cur_idx]].is_visited)
		{
			auto& buf_vertex = point_vector[vertex_hash[cur_idx]];
			buf_vertex.is_visited = true;
			vertices.push_back(buf_vertex);
			cur_idx = buf_vertex.previous_id;
		}

		if (cur_idx != -1)
		{
			vertices.push_back(point_vector[vertex_hash[cur_idx]]);
		}

		path.path = vertices;
		path.path_type = path_index;
		path_index++;
		paths.push_back(path);
	}
	return paths;
}

void boundingBox(std::vector<Vertex>& point_vector, double x_space, double y_space, double z_space, Vec3D<double>& start_position, Vec3D<int>& dimension)
{
	
	double max_x = -999999, min_x = 999999;
	double max_y = -999999, min_y = 999999;
	double max_z = -999999, min_z = 999999;

	for (auto& vertex : point_vector)
	{

		//write_file << "v " << (vertex.x) / x_space << " " << (vertex.y) / y_space << " " << (vertex.z) / z_space << std::endl;
		//vertex.x /= x_space;
		//vertex.y /= y_space;
		//vertex.z /= z_space;
		vertex.x /= 0.5;
		vertex.y /= 0.5;
		vertex.z /= 0.5;

		max_x = std::max(max_x, vertex.x);
		max_y = std::max(max_y, vertex.y);
		max_z = std::max(max_z, vertex.z);
		min_x = std::min(min_x, vertex.x);
		min_y = std::min(min_y, vertex.y);
		min_z = std::min(min_z, vertex.z);
	}

	std::cout << "Bounding box : [" << min_x << ", " << min_y << ", " << min_z << "] --- [" << max_x << ", " << max_y << ", " << max_z << "]" << std::endl;

	std::cout << "Dimension x : " << (max_x - min_x) << std::endl;
	std::cout << "Dimension y : " << (max_y - min_y) << std::endl;
	std::cout << "Dimension z : " << (max_z - min_z) << std::endl;

	start_position = { min_x, min_y, min_z};
	dimension = {int(max_x - min_x + 1), int(max_y - min_y + 1), int(max_z - min_z + 1) };
}
#include <math.h>
void getSphereBoundingBox(Vec3D<double>& center_point, double radius, Vec3D<int> &low_bounding_int, Vec3D<int> &high_bounding_int)
{
	auto low_bounding = center_point - radius;
	low_bounding_int = { int(center_point.x - radius), int(center_point.y - radius), int(center_point.z - radius) };
	high_bounding_int = { int(ceil(center_point.x + radius)+0.1), int(ceil(center_point.y + radius)+0.1), int (ceil(center_point.z + radius)+0.1) };
}

void average(std::vector<float>& mask_vector, Vec3D<int>& dimension)
{
	int offset[8][3] = { {0, 0, 0}, {0, 0, 1}, {0 ,1, 0}, {0, 1, 1},
							{1, 0, 0}, {1, 0, 1}, {1 ,1, 0}, {1, 1, 1}};
	for(auto k = 0;k< dimension.z-1; k++)
	{
		for(auto j=0;j<dimension.y-1; j++)
		{
			for(auto i=0; i<dimension.x-1; i++)
			{
				auto sum = 0.0;
				for (auto& p : offset)
				{
					sum += mask_vector[(p[0] + k) * dimension.x * dimension.y + (p[1] + j) * dimension.x + (p[2] + i)];
				}
				mask_vector[k * dimension.x * dimension.y + j * dimension.x + i] = sum/8;
			}
		}
		std::cout << "Process for averaging mask vector : " << k << " in " << dimension.z - 1 << std::endl;
	}
}
void writeMHD(std::string file_name, int NDims, Vec3D<int> DimSize, Vec3D<int> ElementSize, Vec3D<double> ElementSpacing, Vec3D<double> Position, bool ElementByteOrderMSB, std::string ElementDataFile)
{
	std::ofstream writer(file_name);
	writer << "NDims = " << NDims << std::endl;
	writer << "DimSize = " << DimSize.x << " " << DimSize.y << " " << DimSize.z << std::endl;
	writer << "ElementSize = " << ElementSize.x << " " << ElementSize.y << " " << ElementSize.z << std::endl;
	writer << "ElementSpacing = " << ElementSpacing.x << " " << ElementSpacing.y << " " << ElementSpacing.z << std::endl;
	writer << "ElementType = " << "MET_FLOAT" << std::endl;
	writer << "Position = " << Position.x << " " << Position.y << " " << Position.z << std::endl;
	if (ElementByteOrderMSB)
		writer << "ElementByteOrderMSB = " << "True" << std::endl;
	else
		writer << "ElementByteOrderMSB = " << "False" << std::endl;
	writer << "ElementDataFile = " << ElementDataFile << std::endl;
	std::cout << "File " << ElementDataFile << " has been saved." << std::endl;
}


int main()
{
	std::string based_path = "E:/14193_30neurons/";
	std::string swc_path = "N021.swc";
	std::string swc_file = based_path + swc_path;

	
	std::string raw_file_name = swc_path.substr(0, swc_path.size() - 3) + "raw";
	std::string mhd_file_name = swc_path.substr(0, swc_path.size() - 3) + "mhd";
	int x_dimension = 28452;
	int y_dimension = 21866;
	int z_dimension = 4834;

	double x_space = 0.32;
	double y_space = 0.32;
	double z_space = 2;

	//int x_n = 889;
	//int y_n = 683;
	//int z_n = 151;

	SWCReader swc_reader;
	swc_reader.readSWC(swc_file);

	auto point_vector = swc_reader.getPointVector();

	std::cout << "swc file " + swc_file + " has been loaded." << std::endl;
	std::cout << point_vector.size() << std::endl;

	
	//boundingBox(point_vector, x_space, y_space, z_space);
	Vec3D<double> start_point{};
	Vec3D<int> dimension{};
	boundingBox(point_vector, x_space, y_space, z_space, start_point, dimension);

	Vec3D<double> space{x_space, y_space, z_space};

	auto paths = searchPath(point_vector);
	std::cout << paths.size() << std::endl;

	
	auto len = dimension.x * dimension.y * dimension.z;
	std::vector<float> mask_vector(len, 0);

	for (auto& path : paths)
	{
		for (int i = 0; i < path.path.size(); i++)
		{
			path.path[i].x -= start_point.x;
			path.path[i].y -= start_point.y;
			path.path[i].z -= start_point.z;
			if (path.path[i].radius < 0.01) path.path[i].radius = 0.1;
		}
	}


	start_point *= space;

	std::cout << "Start Points : " << start_point << std::endl;
	std::cout << "Dimension: " << dimension << std::endl;
	
	double step = 0.01;
	int window_offset = 0;
	for(int j = 0;j< paths.size();j++)
	{
		const auto path = paths[j];
		for(int i=1; i < path.path.size(); i++)
		{
			Vec3D<double> start_point = { path.path[i - 1].x, path.path[i - 1].y, path.path[i - 1].z };
			Vec3D<double> end_point = {path.path[i].x, path.path[i].y, path.path[i].z };
			auto direction = (end_point - start_point).normalized();
			auto vec_length = (end_point - start_point).length();
			std::vector<Vec3D<double>> points;
			std::vector<double> radius;
			double r = path.path[i].radius - path.path[i - 1].radius;

			points.push_back(start_point);
			radius.push_back(path.path[i - 1].radius);
			
			for(int t=1;t<vec_length/step;t++)
			{
				//if (i * step >= vec_length - step) break;
				auto cur_point = direction*(t*step) + start_point;
				points.push_back(cur_point);
				radius.push_back(r * (t * step) / vec_length + path.path[i - 1].radius);
				//std::cout << cur_point;
			}
			points.push_back(end_point);
			radius.push_back(path.path[i].radius);

			for(auto p=0;p<points.size();p++) 
			{
				Vec3D<int> low_bounding_int{};
				Vec3D<int> high_bounding_int{};
				getSphereBoundingBox(points[p], radius[p], low_bounding_int, high_bounding_int);
				//if (j==2)
				//{
				//	std::cout << "==================================" << std::endl;
				//	std::cout << points[p] << radius[p] <<"\t" << vec_length << std::endl;;
				//	std::cout << "Low bounding: " << low_bounding_int;
				//	std::cout << "High bounding: " << high_bounding_int;
				//}
				//
				int buf_z = static_cast<int>(points[p].z + 0.5) * dimension.x * dimension.y,
				buf_y = static_cast<int>(points[p].y + 0.5) * dimension.x, buf_x = static_cast<int>(points[p].x + 0.5);
				if (buf_z >= z_dimension) buf_z = z_dimension - 1;
				if (buf_y >= y_dimension) buf_y = y_dimension - 1;
				if (buf_x >= x_dimension) buf_x = x_dimension - 1;
				mask_vector[buf_z + 
					buf_y +
					buf_x] = 1;
				for(auto z = low_bounding_int.z - window_offset; z <= high_bounding_int.z + window_offset; z++)
				{
					for (auto y = low_bounding_int.y - window_offset; y <= high_bounding_int.y + window_offset; y++)
					{
						for (auto x = low_bounding_int.x - window_offset; x <= high_bounding_int.x + window_offset; x++)
						{
							Vec3D<double> buf = { x, y, z };
							
							//std::cout << points[p] << buf << points[p] - buf << (points[p] - buf).length() << std::endl;
							if ((points[p] - buf).length() <= radius[p]*1)
							{
								if (x < 0 || y < 0 || z < 0 || x >= dimension.x || y >= dimension.y || z >= dimension.z) continue;
								mask_vector[z * dimension.x * dimension.y + y * dimension.x + x] = 1;
							}
						}
					}
				}
				
				//std::cout << points[p];
				//std::cout << radius[p] << std::endl;
			}

			std::cout << "Process : " << j << " in " << paths.size() << ", " << i << " in " << paths[j].path.size() << std::endl;
		}
		int count = 0;
		for (auto i = 0; i < mask_vector.size(); i++)
		{
			if (mask_vector[i] > 0.5) count++;
		}
		std::cout << "=============================" << count << std::endl;
	}

	average(mask_vector, dimension);
	average(mask_vector, dimension);
	//average(mask_vector, dimension);
	//average(mask_vector, dimension);
	
	std::ofstream outFile(raw_file_name, std::ios::out | std::ios::binary);
	for (int i = 0; i < mask_vector.size(); i++) {
		
		outFile.write(reinterpret_cast<char*>(&mask_vector[i]), sizeof(float));
	}
	outFile.close();
	std::cout << "File has been saved." << std::endl;


	writeMHD(mhd_file_name, 3, dimension, { 1, 1, 1 }, { 0.5, 0.5, 0.5 }, start_point, false, raw_file_name);

	
}