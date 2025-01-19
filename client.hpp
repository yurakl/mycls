#include <string>

#include <algorithm>
#include <map>
#include <fileapi.h>
#include <fstream>
#include <vector>

class ProjectFile {
public:
    std::string path;
    std::string type;
    int included {0};
    
    std::ifstream fd;
    std::string   text;
    
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

struct LanguageData {
    std::vector<std::string> keywords;
    std::vector<std::string> constructs;
    std::vector<std::string> types;
    std::vector<std::string> custom;
};
