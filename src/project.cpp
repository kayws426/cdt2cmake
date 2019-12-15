/*
 * project.cpp
 *
 *  Created on: 15/04/2013
 *      Author: buildbot
 */

#include "project.h"
#include "cdtproject.h"
#include "sourcediscovery.h"
#include <map>
#include <vector>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <iterator>
#include "listfile.h"

using std::string;
using std::stringstream;
using std::vector;
using std::pair;
using std::map;

namespace cmake
{

bool has_c_sources(const map<string, vector<string> >& sources)
{
	for(const pair<string,vector<string>>& source_folder : sources)
	{
		for(const string& source : source_folder.second)
		{
			if(is_c_source_filename(source))
				return true;
		}
	}
	return false;
}

bool has_cxx_sources(const map<string, vector<string> >& sources)
{
	for(const pair<string,vector<string>>& source_folder : sources)
	{
		for(const string& source : source_folder.second)
		{
			if(is_cxx_source_filename(source))
				return true;
		}
	}
	return false;
}

void merge(const cdt::configuration_t::build_folder::compiler_t& source, cdt::configuration_t::build_folder::compiler_t& merged)
{
	for(string inc : source.includes)
	{
		if(inc.find("\"${workspace_loc:/") == 0)
		{
			inc = inc.substr(18);
			inc = inc.substr(0, inc.size() - 2);
			inc = "${CMAKE_SOURCE_DIR}/" + inc;

			if(inc[inc.size()-1] != '/')
				inc += '/';
		}
		else if(inc.find("../../") == 0)
		{
			inc = inc.substr(6);
			inc = "${CMAKE_SOURCE_DIR}/" + inc;

			if(inc[inc.size()-1] != '/')
				inc += '/';
		}
		else
		{
			if(inc[inc.size()-1] != '/')
				inc += '/';
		}

		if(std::find(merged.includes.begin(), merged.includes.end(), inc) == merged.includes.end())
		{
			merged.includes.push_back(inc);
		}
	}

	if(source.options != merged.options)
	{
		if(merged.options.empty())
			merged.options = source.options;
		else
			merged.options += " " + source.options;
	}
}

void merge(const cdt::configuration_t::build_folder::linker_t& source, cdt::configuration_t::build_folder::linker_t& merged)
{
	for(string lib : source.libs)
	{
		if(std::find(merged.libs.begin(), merged.libs.end(), lib) == merged.libs.end())
			merged.libs.push_back(lib);
	}
	for(string lib : source.lib_paths)
	{
		if(lib.empty())
			continue;
		if(lib.find("\"${workspace_loc:/") == 0)
		{
			// CMake is smarter than eclipse
			lib = "";
		}
		else if(lib.find("../../") == 0)
		{
			// CMake is smarter than eclipse
			lib = "";
		}
		else
		{
			if(lib[lib.size()-1] != '/')
				lib += '/';
		}

		if(lib.empty())
			continue;

		if(std::find(merged.lib_paths.begin(), merged.lib_paths.end(), lib) == merged.lib_paths.end())
			merged.lib_paths.push_back(lib);
	}
	if(source.flags != merged.flags)
	{
		if(merged.flags.empty())
			merged.flags = source.flags;
		else
			merged.flags += " " + source.flags;
	}
}

void merge(const cdt::configuration_t::build_folder& source, cdt::configuration_t::build_folder& merged)
{
	merge(source.c.compiler, merged.c.compiler);
	merge(source.c.linker, merged.c.linker);
	merge(source.cpp.compiler, merged.cpp.compiler);
	merge(source.cpp.linker, merged.cpp.linker);
}

void merge(const cdt::configuration_t::build_file& source, cdt::configuration_t::build_file& merged)
{
	if(source.command != merged.command)
	{
		if(merged.command.empty())
			merged.command = source.command;
		else
			merged.command += " / " + source.command;
	}
	if(source.inputs != merged.inputs)
	{
		if(merged.inputs.empty())
			merged.inputs = source.inputs;
		else
			merged.inputs += " / " + source.inputs;
	}
	if(source.outputs != merged.outputs)
	{
		if(merged.outputs.empty())
			merged.outputs = source.outputs;
		else
			merged.outputs += " / " + source.outputs;
	}
}

void generatorMerge(string& mergedEnvVar, const string& enVarToMerge, const vector<string>& prevConfigs)
{
	//wrap everything else in generators if they aren't already.
	size_t alreadyHasGenerators = mergedEnvVar.find("$<");
	if(alreadyHasGenerators == string::npos)
	{
		if(prevConfigs.size() > 2)
		{
		    /* wrap existing value around an OR generator, easy */
		    string prepender =  "$<$<OR:";
		    string pre =  "$<CONFIG:";
		    for (string prevConfig : prevConfigs)
		      prepender = prepender + pre + prevConfig + ">,";

		    mergedEnvVar = prepender.substr(0,prepender.length()-2) + ">:" + mergedEnvVar + ">";
		}
		else if (prevConfigs.size() == 2)
		{
		    /* single generator, easy */
		    mergedEnvVar =  "$<$<CONFIG:" + prevConfigs[0] + ">" + mergedEnvVar + ">";
		}
		else
		  {
			throw std::runtime_error("somethings' up");
		  }
	}

	string newConfigName = prevConfigs[prevConfigs.size()-1];
	size_t isInHereInAform = mergedEnvVar.find(enVarToMerge);
	if(isInHereInAform == string::npos)
	{
		mergedEnvVar = mergedEnvVar + " $<$<CONFIG:" + newConfigName + ">:" + enVarToMerge + ">";
	}
	else
	{
		/* Welcome to the environment merge mess! there are only 2 types of generators our existing variable may be in.
		 *  ether:
		 * 1. the variable is in a $<CONFIG:...> generator (meaning we need to encapuslate this into an $<$<OR:,:> generator)
		 * 2. the variable is already in an $<$<OR:,:>  generator and we simply need to add another option here.
		 * Deriving this is a matter of counting the '>' brackets before variable we found.
		 * Once done, we go through the obtuse substring logic that the std library has actually made fairly usable.
		 */
		size_t howManyLayer = 0,  charToSee = isInHereInAform  - 2;
		while(mergedEnvVar[charToSee--] == '>')
		  howManyLayer++;

		string preTheValue = mergedEnvVar.substr(0, isInHereInAform);
		if (howManyLayer == 1)
		{
		    size_t whereTheGeneratorStarts = preTheValue.find_last_of("$<")-1;
		    string z_beginning = mergedEnvVar.substr(0,whereTheGeneratorStarts) , z_middle = "$<OR:$<CONFIG:" + newConfigName + ">," ,
			z_end =  mergedEnvVar.substr(whereTheGeneratorStarts);

		    size_t whereToAddIt = z_end.find(enVarToMerge) -1;
		    z_end = z_end.substr(0,whereToAddIt) + ">"  + z_end.substr(whereToAddIt);
		    mergedEnvVar = z_beginning + z_middle + z_end;
		}
		else if (howManyLayer > 1)
		{
		    string z_begin = mergedEnvVar.substr(0,isInHereInAform-2), z_middle = ",$<CONFIG:" + newConfigName + ">",
			z_end = mergedEnvVar.substr(isInHereInAform-2);

		    mergedEnvVar = z_begin + z_middle + z_end;
		}
		else
		{
			throw std::runtime_error("node has an invalid depth: " + enVarToMerge );
		}
	}
}

// one step, take cdt files and write cmakelists.
void generate(cdt::project& cdtproject, bool write_files)
{
	string project_name = cdtproject.name();
	string project_path = cdtproject.path();

	map<string, vector<string> > sources;
	vector<source_file> source_files = find_sources(cdtproject.path(), is_source_filename);
	vector<string> source_filpaths;
	for(source_file& file : source_files)
	{
		sources[file.path].push_back(file.name);
		source_filpaths.push_back("${WorkspaceDirPath}/" + file.path + "/" + file.name);
	}

	bool lang_c = has_c_sources(sources);
	bool lang_cxx = has_cxx_sources(sources);

	map<string, cdt::configuration_t> artifact_configurations;
	vector<string> configuration_typeNames;

	vector<string> confs = cdtproject.cconfigurations();
	for(string& conf_name : confs)
	{
		/* create the two configurations to compair */
		cdt::configuration_t curConfig = cdtproject.configuration(conf_name);
		cdt::configuration_t& configToAdd = artifact_configurations[curConfig.artifact + to_string(curConfig.type)];

		configuration_typeNames.push_back(curConfig.name);

		/* give the candidate config it's name and artifact type */
		configToAdd.name = curConfig.artifact + to_string(curConfig.type);
		configToAdd.artifact = curConfig.artifact;

		/* give the candidate config it's prebuild settings */
		if(configToAdd.prebuild != curConfig.prebuild)
		{
			if(configToAdd.prebuild.empty())
				configToAdd.prebuild = curConfig.prebuild;
			else
				configToAdd.prebuild += " / " + curConfig.prebuild;
		}

		/* give the candidate config it's postbuild settings */
		if(configToAdd.postbuild != curConfig.postbuild)
		{
			if(configToAdd.postbuild.empty())
				configToAdd.postbuild = curConfig.postbuild;
			else
				configToAdd.postbuild += " / " + curConfig.postbuild;
		}
		configToAdd.type = curConfig.type;

		/* give the candidate config it's build folders */
		for(cdt::configuration_t::build_folder& buildFolder : curConfig.build_folders)
		{
			cdt::configuration_t::build_folder* merged_folder = nullptr;
			for(cdt::configuration_t::build_folder& abf : configToAdd.build_folders)
			{
				if(abf.path == buildFolder.path)
					merged_folder = &abf;
			}

			/* if the merge folder has not been found */
			if(!merged_folder)
			{
				cdt::configuration_t::build_folder nbf;
				nbf.path = buildFolder.path;

				configToAdd.build_folders.push_back(nbf);
				merged_folder = &configToAdd.build_folders.back();
			}
			merge(buildFolder, *merged_folder);
		}

		/* give the candidate config it's build files by looping through this configs files,
		 * Ultimately, this is a list searching algorithm done in a hard-to-read manner   */
		for(cdt::configuration_t::build_file& buildFile : curConfig.build_files)
		{
			/* create a list of build files */
			cdt::configuration_t::build_file* merged_file = nullptr;

			/* loop through the totals config and check to see if it matches? */
			for(cdt::configuration_t::build_file& abf : configToAdd.build_files)
			{
				if(abf.file == buildFile.file)
					merged_file = &abf;
			}

			/* if a merge file has been found */
			if(merged_file)
			{
				merge(buildFile, *merged_file);
			}
			else
			{
				configToAdd.build_files.push_back(buildFile);
			}
		}

		/* give the candidate config it's environment settings */
		for(cdt::configuration_t::environment_variables& envVarToMerge : curConfig.env_values)
		{
			cdt::configuration_t::environment_variables* mergedEnvVar = nullptr;

			for(cdt::configuration_t::environment_variables& envVarToCheck: configToAdd.env_values)
			{
			    /* check if the merged config has the variable already */
				if (envVarToMerge.key == envVarToCheck.key)
				  {
				    /* establish that there is indeed a key match between the configurations by assigning the pointer here */
				    mergedEnvVar = &envVarToCheck;

					/* either the envVar has the same value or it needs to have a generator if-switch */
					if (envVarToMerge.value != mergedEnvVar->value)
					{
						generatorMerge(mergedEnvVar->value, envVarToMerge.value, configuration_typeNames);
						break;
					}
				  }
			}

			/* if the merged environment variable was created (meaning we found one) */
			if (!mergedEnvVar)
			  {
			    configToAdd.env_values.push_back(envVarToMerge);
			  }
		}

		/* evaluate the candidate config's IGNORE list */
		for(cdt::configuration_t::excludes& fileToExclude : curConfig.exclude_entries)
		{
		    for (string& filepath : source_filpaths)
		    {
			size_t isInThisOne = filepath.find(fileToExclude.sourcePath);
			if(isInThisOne != string::npos)
			{
				//TODO: Figure out a decent way to store filepaths
				printf("%s does not match %s in config %s\n", filepath.c_str(), fileToExclude.sourcePath.c_str(), curConfig.name.c_str());
				filepath = "$<$<NOT:$<CONFIG:" + curConfig.name + ">>:" + filepath + ">";
//				generatorMerge(filepath, envVarToMerge.value, configuration_typeNames);
			}
		    }
		}
	}

	std::streambuf* buf;
	std::ofstream of;
	if(write_files)
	{
		of.open(project_path + "/CMakeLists.txt");
		buf = of.rdbuf();
	}
	else
	{
		buf = std::cout.rdbuf();
	}
	std::ostream master(buf);

	master << "cmake_minimum_required (VERSION 3.5)\n\n";
	master << "project (" << project_name << ")\n\n";
	master << "\n";

	for(pair<string, cdt::configuration_t> targetConfig: artifact_configurations)
	{
		cdt::configuration_t currentConf = targetConfig.second;

		for(cdt::configuration_t::environment_variables env : currentConf.env_values)
		{
		    master << "set ( " << env.key << " " << env.value << " )\n";
		}

		switch(currentConf.type)
		{
			case cdt::configuration_t::Type::Executable:
				master << "add_executable (" << currentConf.artifact;
				break;
			case cdt::configuration_t::Type::StaticLibrary:
				master << "add_library (" << currentConf.artifact << " STATIC";
				break;
			case cdt::configuration_t::Type::SharedLibrary:
				master << "add_library (" << currentConf.artifact << " SHARED";
				break;
		}

#ifdef Sources_by_idiotic_map
		for(const pair<string,vector<string>>& source_folder : sources)
		{
			for(const string& source : source_folder.second)
			{
				master
				    << "\n\t"
				    << (source_folder.first.empty() ? string{} : source_folder.first + "/")
				    << source;
			}
		}
#endif // Sources_by_idiotic_map
		for (const string&  source : source_filpaths)
		  {
			master << "\n\t" << source;
		  }
		master << ")\n\n";

		if(!currentConf.prebuild.empty() || !currentConf.postbuild.empty())
		{
			master << "\n";
			master << "# prebuild: " << currentConf.prebuild << "\n";
			master << "# postbuild: " << currentConf.postbuild << "\n";
			master << "\n";
		}

		for(cdt::configuration_t::build_folder& bf : currentConf.build_folders)
		{
			if(bf.path.empty())
			{
				// master
				{
					if(!bf.cpp.compiler.includes.empty() || !bf.c.compiler.includes.empty())
					{
						//master << "set_property (TARGET " << c.artifact << " PROPERTY INCLUDE_DIRECTORIES";
						master << "INCLUDE_DIRECTORIES(";
						if(lang_cxx)
						{
							for(string& inc : bf.cpp.compiler.includes)
								master << (bf.cpp.compiler.includes.size() > 3 ? "\n\t" : " ") << '"' << inc << '"';
						}
						if(lang_c)
						{
							for(string& inc : bf.c.compiler.includes)
								master << (bf.c.compiler.includes.size() > 3 ? "\n\t" : " ") << '"' << inc << '"';
						}
						master << ")\n\n";
					}

					vector<string> options;
					if(lang_cxx)
					{
						stringstream ss(bf.cpp.compiler.options);
						vector<string> opts(std::istream_iterator<string>{ss}, std::istream_iterator<string>{});
						for(string& opt : opts)
						{
							if(std::find(begin(options), end(options), opt) == end(options))
								options.push_back(opt);
						}
					}
					if(lang_c)
					{
						stringstream ss(bf.c.compiler.options);
						vector<string> opts(std::istream_iterator<string>{ss}, std::istream_iterator<string>{});
						for(string& opt : opts)
						{
							if(std::find(begin(options), end(options), opt) == end(options))
								options.push_back(opt);
						}
					}

					if(!options.empty())
					{
						master << "set_target_properties(" << currentConf.artifact << " PROPERTIES COMPILE_FLAGS \"";
						for(string& option : options)
							master << option << ' ';
						master << "\")\n\n";
					}
				}

				if(lang_cxx)
				{
					// use c++ linker settings.
					if(!bf.cpp.linker.flags.empty())
					{
						vector<string> flags;
						stringstream ss(bf.cpp.linker.flags);
						vector<string> opts(std::istream_iterator<string>{ss}, std::istream_iterator<string>{});

						for(string& opt : opts)
						{
							if(std::find(begin(flags), end(flags), opt) == end(flags))
								flags.push_back(opt);
						}

						master << "set_target_properties(" << currentConf.artifact << " PROPERTIES LINK_FLAGS \"";
						for(string& flag : flags)
							master << flag << ' ';
						master << "\")\n\n";
					}

					if(!bf.cpp.linker.lib_paths.empty())
					{
						master << "link_directories (";
						for(string& path : bf.cpp.linker.lib_paths)
							master
							    << (bf.cpp.linker.lib_paths.size() > 3 ? "\n\t" : " ")
							    << path;
						master << ")\n\n";
					}

					if(!bf.cpp.linker.libs.empty())
					{
						master << "target_link_libraries (" << currentConf.artifact;
						for(string& lib : bf.cpp.linker.libs)
							master << (bf.cpp.linker.libs.size() > 3 ? "\n\t" : " ") << lib;
						master << ")\n\n";
					}
				}
				else
				{
					if(!bf.c.linker.flags.empty())
					{
						vector<string> flags;
						stringstream ss(bf.c.linker.flags);
						vector<string> opts(std::istream_iterator<string>{ss}, std::istream_iterator<string>{});

						for(string& opt : opts)
						{
							if(std::find(begin(flags), end(flags), opt) == end(flags))
								flags.push_back(opt);
						}

						master << "set_target_properties(" << currentConf.artifact << " PROPERTIES LINK_FLAGS \"";
						for(string& opt : flags)
							master << opt << ' ';
						master << "\")\n\n";
					}

					if(!bf.c.linker.lib_paths.empty())
					{
						master << "link_directories (";
						for(string& path : bf.c.linker.lib_paths)
							master << (bf.c.linker.lib_paths.size() > 3 ? "\n\t" : " ") << path;
						master << ")\n\n";
					}

					if(!bf.c.linker.libs.empty())
					{
						master << "target_link_libraries (" << currentConf.artifact;
						for(string& lib : bf.c.linker.libs)
							master << (bf.c.linker.libs.size() > 3 ? "\n\t" : " ") << lib;
						master << ")\n\n";
					}
				}
			}
			else
			{
				// create subdir file.
				master << "add_subdirectory(" << bf.path << ")\n\n";
				// subdirectory
			}
		}

		for(cdt::configuration_t::build_file& bf : currentConf.build_files)
		{
			// TODO
			master << "# TODO " << bf.file << '\n';
		}
	}

	master << '\n';
}

}
