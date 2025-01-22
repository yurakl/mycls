
#include <iostream>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <utility>
#include "json.hpp"
#include "client.hpp"  
#include <regex>  
using json = nlohmann::json;

void processAllRequests(std::string& request, std::vector<std::string>& answer_queque);
int  processLSPRequest(const std::string& request, std::string& answer);
void handleInitialize(const json& j, std::string& answer);
void onDidChange(const json& j);
void onDidOpen(const json& j, std::string& answer);
void onDidClose(const json& j, std::string& answer);
void onDocumentSymbol(const json& j, std::string& answer);


struct SymbolOpt { 
    int name;
    int detail;
    std::regex regex;
};

enum class SymbolKind {
    File = 1,
    Module = 2,
    Namespace = 3,
    Package = 4,
    Class = 5,
    Method = 6,
    Property = 7,
    Field = 8,
    Constructor = 9,
    Enum = 10,
    Interface = 11,
    Function = 12,
    Variable = 13,
    Constant = 14,
    String = 15,
    Number = 16,
    Boolean = 17,
    Array = 18,
    Object = 19,
    Key = 20,
    Null = 21,
    EnumMember = 22,
    Struct = 23,
    Event = 24,
    Operator = 25,
    TypeParameter = 26
};


//~ Symbol descriptor
struct Symbol {
    std::string name;
    std::string detail;
    SymbolKind  kind;
    int         startLine;
    int         endLine;

    //~ List of nested symbols
    std::vector<Symbol> children; 
};

void symbolSearch(std::string& text,
                    std::string::const_iterator begin,
                        std::string::const_iterator end,
                            std::regex& regex,
                                SymbolKind kind,
                                    std::vector <struct Symbol>& symbolList);

std::string::const_iterator extractBlock(const std::string::const_iterator begin, const std::string::const_iterator end);
