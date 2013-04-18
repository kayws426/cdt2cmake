/*
 * cdtproject.h
 *
 *  Created on: 16/04/2013
 *      Author: nicholas
 */

#ifndef CDTPROJECT_H_
#define CDTPROJECT_H_
#include <string>
#include <tinyxml.h>
#include <vector>
#include <set>

/*
 * TODO Implement interface that reads different cdt / eclipse
 * project file formats and extracts enough information to
 * create a cmake project from it.
 * Expand the Project structure to represent a cmake project
 * more completely.
 *
 * http://help.eclipse.org/indigo/index.jsp?topic=%2Forg.eclipse.platform.doc.isv%2Freference%2Fmisc%2Fproject_description_file.html
 */
class cdt_project
{
private:
	TiXmlDocument project;
	TiXmlDocument cproject;
public:
	cdt_project(const std::string& project_base);

	// .project properties
	std::string name();
	std::string comment();
	std::vector<std::string> referenced_projects();
	std::vector<std::string> natures();

	// .cproject properties
	TiXmlElement* settings();
	std::vector<std::string> cconfigurations();
	TiXmlElement* cconfiguration(const std::string& id);

	struct configuration_t
	{
		std::string name;
		std::string artifact;

		std::string prebuild;
		std::string postbuild;

		enum class Type
		{
			Executable,
			StaticLibrary,
			SharedLibrary
		} type;

		// TODO represent build commands, folders and files.
		struct build_folder
		{
			std::string path;

			std::set<std::string> cpp_includes;
			std::set<std::string> c_includes;

			std::vector<std::string> cpp_libs;
			std::vector<std::string> c_libs;

			std::set<std::string> cpp_lib_paths;
			std::set<std::string> c_lib_paths;
		};

		struct build_file
		{
			std::string file;
		};
	};

	configuration_t configuration(const std::string& cconfiguration_id);

	TiXmlElement* cdtBuildSystem_configuration(const std::string& cconfiguration_id);
};

#endif /* CDTPROJECT_H_ */
