#include <iostream>

#include "VMUtils/include/VMUtils/cmdline.hpp"
#include "VMat/include/VMat/geometry.h"
#include "VMat/include/VMat/numeric.h"


#include <string>
#include <fstream>
#include <set>
#include "json_struct.h"

struct BlockID
{
	int x, y, z;
	bool operator==(const BlockID& other) const
	{
		return x == other.x && y == other.y && z == other.z;
	}

	bool operator<(const BlockID& other) const
	{
		return x < other.x ? true : (y == other.y ? z < other.z : y < other.y);
	}
	std::vector<int> toArray() const
	{
		return { x,y,z };
	}
};

bool readObj(const std::string& filename, std::vector<ysl::Point3f>& point_set)
{
	std::ifstream in(filename.c_str());
	if (!in.good())
	{
		std::cout << "ERROR: loading obj:(" << filename << ") file is not good" << "\n";
		exit(0);
	}

	char buffer[256], str[255];
	float f1, f2, f3;
	while (!in.getline(buffer, 255).eof())
	{
		buffer[255] = '\0';

		sscanf_s(buffer, "%s", str, 255);

		// reading a vertex  
		if (buffer[0] == 'v' && (buffer[1] == ' ' || buffer[1] == 32))
		{
			if (sscanf(buffer, "v %f %f %f", &f1, &f2, &f3) == 3)
			{
				point_set.push_back({ f1, f2, f3 });
				// mesh.verts.push_back(float3(f1, f2, f3));
			}
			else
			{
				std::cout << "ERROR: vertex not in wanted format in OBJLoader" << "\n";
				exit(-1);
			}
		} 
		else
		{
			
		}
	}
	return false;
}

void calcBlockArray(const std::string & filename, const int block_based, const int padding, ysl::Vec3i volume_size, std::set<BlockID>& block_set)
{
	//open obj files	
	std::vector<ysl::Point3f> point_set;
	readObj(filename, point_set);

	const ysl::Vec3i volumeDataSizeNoRepeat = volume_size;
	int b = (int)(std::pow(2, block_based) - 2 * padding);
	const ysl::Vec3i blockDataSizeNoRepeat = {b,b,b};
	const int px = ysl::RoundUpDivide(volume_size.x, b);
	const int py = ysl::RoundUpDivide(volume_size.y, b);
	const int pz = ysl::RoundUpDivide(volume_size.z, b);
	ysl::Vec3i pageTableSize = {px,py,pz};


	for(auto & point: point_set)
	{
		// address translation
		//const auto x = point.x * volumeDataSizeNoRepeat.x / (blockDataSizeNoRepeat.x * pageTableSize.x)
		const auto x = (point.x / volume_size.x*volumeDataSizeNoRepeat.x / (blockDataSizeNoRepeat.x*pageTableSize.x)*pageTableSize.x);
		const auto y = (point.y / volume_size.y*volumeDataSizeNoRepeat.y / (blockDataSizeNoRepeat.y*pageTableSize.y)*pageTableSize.y);
		const auto z = (point.z / volume_size.z*volumeDataSizeNoRepeat.z / (blockDataSizeNoRepeat.z*pageTableSize.z)*pageTableSize.z);
		//ivec3 entry3DIndex = ivec3(samplePos*pageTableSize);
		//unsigned int entryFlatIndex = entry3DIndex.z * pageTableSize.x*pageTableSize.y + entry3DIndex.y * pageTableSize.x + entry3DIndex.x;
		//auto blockIndex = ysl::Linear({ x,y,z }, {pageTableSize.x,pageTableSize.y});

		block_set.insert({ (int)x,(int)y,(int)z });
	}
	std::cout << "Block number for obj file : " << filename << " has been calculated" << std::endl;
}

int main(int argc, char* argv[])
{

	struct OutputJsonStruct BlocksJson;
	struct InputJsonStruct ObjFilesJson;

	
	std::string obj_json_path = "";

	int block_based = 7;	//pow
	int padding = 2;			//padding
	int x_dimension = 0;
	int y_dimension = 0;
	int z_dimension = 0;

	if(argc<2)
	{
		std::cout << "Error: please use command line parameter." << std::endl;
		//return 0;
	}

	cmdline::parser a;
	a.add<std::string>("objfiles", 'f', "input obj json files", false, R"(D:\project\science_project\SWC2Obj\x64\Release\workspace\obj_files.json)");
	a.add<std::string>("output", 'o', "output json files", false, R"(D:\project\science_project\SWC2Obj\x64\Release\workspace\block_array.json)");
	a.add<int>("block_based", 'b', "block length base number", false, 7);
	a.add<int>("padding", 'p', "block padding", false, 2);
	a.add<int>("x", 'x', "x dimension of raw volume", false, 28452);
	a.add<int>("y", 'y', "y dimension of raw volume", false, 21866);
	a.add<int>("z", 'z', "z dimension of raw volume", false, 4834);

	a.parse_check(argc, argv);

	obj_json_path =				a.get<std::string>("objfiles");
	std::string output_file =	a.get<std::string>("output");
	block_based =				a.get<int>("block_based");
	padding =					a.get<int>("padding");
	x_dimension =				a.get<int>("x");
	y_dimension =				a.get<int>("y");
	z_dimension =				a.get<int>("z");


	try
	{
		std::ifstream input_stream(obj_json_path);
		input_stream >> ObjFilesJson;

		std::string file_prefix = ObjFilesJson.file_prefix;
		auto obj_files = ObjFilesJson.obj_files;

		std::vector<BlockNumberJsonStruct> buf_blocks;

		
		
		for (const auto& obj_file : obj_files)
		{
			std::cout << "--------------------------Calculation for " << obj_file << "--------------------------" << std::endl;

			
			std::set<BlockID> block_set;

			std::vector<int> block_max_bounding(3, -0xfffffff);
			std::vector<int> block_min_bounding(3, 0xfffffff);
			
			calcBlockArray(file_prefix + obj_file, block_based, padding, {x_dimension, y_dimension, z_dimension}, block_set);

			struct BlockNumberJsonStruct buf_block_number_json;
			buf_block_number_json.set_obj_file(file_prefix + obj_file);
			std::vector<std::vector<int>> block_array;
			for(auto & block: block_set)
			{
				block_array.push_back(block.toArray());
				block_max_bounding[0] = std::max(block_max_bounding[0], block.x);
				block_max_bounding[1] = std::max(block_max_bounding[1], block.y);
				block_max_bounding[2] = std::max(block_max_bounding[2], block.z);

				block_min_bounding[0] = std::min(block_min_bounding[0], block.x);
				block_min_bounding[1] = std::min(block_min_bounding[1], block.y);
				block_min_bounding[2] = std::min(block_min_bounding[2], block.z);
			}
			

			buf_block_number_json.set_block_number_array(block_array);

			buf_block_number_json.set_block_max_bounding(block_max_bounding);
			buf_block_number_json.set_block_min_bounding(block_min_bounding);

			buf_block_number_json.set_padding(padding);
			buf_block_number_json.set_block_based_number(block_based);

			buf_blocks.push_back(buf_block_number_json);

			double size = block_set.size() * pow(2, block_based * 3)/1024/1024;

			unsigned long long bounding_number = (block_max_bounding[0] - block_min_bounding[0] + 1) *
				(block_max_bounding[1] - block_min_bounding[1] + 1) *
				(block_max_bounding[2] - block_min_bounding[2] + 1);
			
			std::cout << "Block number :\t\t" << block_set.size() << std::endl;
			std::cout << "Bounding block number :\t" << bounding_number << std::endl;
			
			std::cout << "Size of the block data for " << obj_file << " :\t" << size << "MB" << std::endl;

			size = (block_max_bounding[0] - block_min_bounding[0] + 1)*
				(block_max_bounding[1] - block_min_bounding[1] + 1)*
				(block_max_bounding[2] - block_min_bounding[2] + 1)
			* pow(2, block_based * 3) / 1024 / 1024 / 1024;

			std::cout << "Size of the bounding box data for " << obj_file << " : " << size << "GB" << std::endl;
			
		}

		BlocksJson.set_blocks(buf_blocks);

		std::ofstream output_stream(output_file);
		vm::json::Writer writer;
		writer.write(output_stream, BlocksJson);

		std::cout << "Block file has been saved." << std::endl;

	}
	catch (std::exception & e)
	{
		vm::println("{}", e.what());
	}

}
