#ifndef CLIENT
#define CLIENT

#include <string>
#include <fstream>
#include <vector>
#include "symbolOptions.hpp"

class ProjectFile {
public:
    std::string path;
    std::string type;
    int included {0};
    
    std::ifstream fd;
    std::string   text;

    int didChange {1};

    /**
     * @brief A vector containing user-defined data types and functions.
     *
     * This vector stores custom data types and functions that are defined by the user
     * and are processed during the program's execution.
     */
    std::vector <Symbol> symbolList;
    
    std::vector <ProjectFile *> includedFiles;
    
    ProjectFile(const std::string& path, const std::string& type)
        : path(path), type(type) {}
};

class Project {
public:
    std::string projectId; 
    std::string rootPath;

    std::map<std::string, ProjectFile> files;

    Project(const std::string& rootPath, std::string& projectId)
        : rootPath(rootPath), projectId(projectId)  {}

};
 
#endif 
