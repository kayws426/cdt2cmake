/*
 * sourcediscovery.cpp
 *
 *  Created on: 16/04/2013
 *      Author: nicholas
 */

#include "sourcediscovery.h"
#include <algorithm>
#include <dirent.h>

using std::string;

bool is_source_filename(const string& filename)
{
	static const auto c_types = {".c", ".C", "c++", ".cc", ".cpp", ".cxx"};

	string::size_type pos = filename.rfind('.');
	if(pos == string::npos)
		return false;

	auto file_type = filename.substr(pos);
	return std::find(begin(c_types), end(c_types), file_type) != end(c_types);
}

bool is_c_source_filename(const string& filename)
{
	static const auto c_types = {".c"};

	string::size_type pos = filename.rfind('.');
	if(pos == string::npos)
		return false;

	auto file_type = filename.substr(pos);
	return std::find(begin(c_types), end(c_types), file_type) != end(c_types);
}

bool is_cxx_source_filename(const string& filename)
{
	static const auto c_types = {".C", "c++", ".cc", ".cpp", ".cxx"};

	string::size_type pos = filename.rfind('.');
	if(pos == string::npos)
		return false;

	auto file_type = filename.substr(pos);
	return std::find(begin(c_types), end(c_types), file_type) != end(c_types);
}

void find_sources(const string& base_path, const string& path, const std::function<bool(string)>& predicate, std::vector<source_file>& sources)
{
	string abs_path = base_path;

	if(!path.empty())
		abs_path += "/" + path;

	DIR* directory = opendir(abs_path.c_str());
	if(directory)
	{
		while (dirent* dir = readdir(directory))
		{
			string name = dir->d_name;
			if(dir->d_type == DT_DIR)
			{
				if(name == "." || name == ".." || name == "CMakeFiles")
					continue;

				string rel_path = name;
				if(!path.empty())
					rel_path = path + "/" + name;

				find_sources(base_path, rel_path, predicate, sources);
			}
			else if(dir->d_type == DT_REG)
			{
				if(predicate(name))
				{
					string rel_source = name;
					if(!path.empty())
						rel_source = path + "/" + name;

					sources.push_back({name, path});
				}
			}
		}
		closedir(directory);
	}
}

using std::vector;
vector<source_file> find_sources(const string& base_path, const std::function<bool(string)>& predicate)
{
	vector<source_file> sources;
	find_sources(base_path, {}, predicate, sources);

	return std::move(sources);
}
