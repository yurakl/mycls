#include "processLSPrequest.hpp"


Project * project = NULL;
struct LanguageData LanguageData;

//~ std::string var_prefix(R"(\b(?:)");
//~ std::string var_suffix(R"()\s+(\w+)\s*))");

std::map <SymbolKind, SymbolOpt> SymbolOptions =
{
    {SymbolKind::Class,    {1, 0, std::regex(R"(\bclass\s+(\w+)\s*)")}},
    {SymbolKind::Struct,   {1, 0, std::regex(R"(\bstruct\s+(\w+)\s*)")}}, 
    {SymbolKind::Function, {2, 0, std::regex(R"((\w[\w\s*&]+)\s+(\w+)\s*\(([^)]*)\))")}},
    {SymbolKind::Method,   {4, 0, std::regex(R"((\w[\w\s*&]+)\s+(\w+)(\s*::\s*)(\w+)\s*\(([^)]*))")}},
    //~ {SymbolKind::Variable, {2, 0, std::regex(R"((\w[\w\s*&]+)\s+(\w+)\s*(;|=))")}}
    {SymbolKind::Variable, {3, 0, std::regex(R"(\b([*&]*([a-zA-Z_]+)\s*[*&]*\s+)+[*&]*([a-zA-Z_]+)\s*[;|=])")}}
};

void processAllRequests(std::string& request, std::vector<std::string>& answer_queque)
{
    std::vector<std::string> request_queque;

    size_t start = 0;
    while((start = request.find("Content-Length:")) != std::string::npos)
    {

        size_t end = request.find("\r\n", 15);
        if (end == std::string::npos) {
            std::cerr << "Error: Headers not terminated correctly.\n";
            break;
        }

        size_t body_length = std::stoi(request.substr(15, end));
        std::cerr << "Defined Content-Length: " << body_length << std::endl;
        size_t header_end = request.find("\r\n\r\n", start);

        if (header_end == std::string::npos) {
            std::cerr << "Error: Headers not terminated correctly.\n";
            break;
        }

        size_t body_start = header_end + 4;

        if (request.size() < body_start + body_length) {
            std::cerr << "Error: Incomplete body.\n";
            break;
        }
        // Вичленяємо пакет
        std::string a_request = request.substr(body_start, body_length);
        std::cerr << "Single request:\n" << a_request << "\n!!!!!" << std::endl;

        //~ Додаємо пакет до черги пакетів
        request_queque.push_back(std::move(a_request));

        //~ Видаляємо початок - знайдений пакет
        request = request.substr(body_start + body_length);
    }

    std::string answer;
    for (const auto& r : request_queque)
    {
            processLSPRequest(r, answer);
        if (!answer.empty())
        {
            answer_queque.emplace_back("Content-Length: "
                                                + std::to_string(answer.size())
                                                + std::string("\r\n\r\n")
                                                + answer);
            answer.clear();
        }
    }
    return;
}

int processLSPRequest(const std::string& request, std::string& answer)
{ 
    try {
        //~ Парсимо JSON запит
        json j = json::parse(request); 
        std::cerr << "processLSPRequest: " << j["method"] << std::endl;;
         
        if (j.contains("jsonrpc") && j.contains("method")) {
            std::string method = j["method"];
 
            if (method == "textDocument/didOpen") {
                std::cerr << "didOpen" << std::endl;
                onDidOpen(j, answer);
            }
            else if (method == "textDocument/didCLose") {
                std::cerr << "didClose" << std::endl;
                onDidClose(j, answer);
            }
            else if (method == "textDocument/didChange") {
                std::cerr << "didChange" << std::endl;
                onDidChange(j);
            }
            else if (method == "textDocument/documentSymbol") {
                std::cerr << "documentSymbol" << std::endl;
                onDocumentSymbol(j, answer);
            }
            else if (method == "initialize") {
                handleInitialize(j, answer);
            }
            else if (method == "textDocument/completion") {
                answer = "completion";
            }
            else if (method == "textDocument/definition") {
                answer = "definition";
            }         
            else {
                answer = "Uknown method";
            }
        }
        else {
            answer = "Invalid request format.";
            return -1;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        answer = "Error processing request: " + static_cast<std::string>(e.what());
    }
    return 0;
}
 
void handleInitialize(const json& j,  std::string& answer) {
    std::cerr << "handleInitialize" << std::endl;
    std::string rootUri     = j["params"]["rootUri"]; 
    std::string projectId   = std::to_string(std::hash<std::string>{}(rootUri));

    project = new Project(rootUri, projectId);

    std::ifstream file("D:\\NULP\\Code\\mycls\\ccpp.json");
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open keywords.json" << std::endl;
        return;
    }

    std::cerr << "Read Language Data" << std::endl;

    json data = json::parse(file);
    // if (rootUri.end_with(".c"))
    LanguageData.keywords   = data.at("languages").at("C++").at("keywords").get<std::vector<std::string>>();
    LanguageData.constructs = data.at("languages").at("C++").at("constructs").get<std::vector<std::string>>();
    LanguageData.types      = data.at("languages").at("C++").at("built-in types").get<std::vector<std::string>>();
    std::cerr << "Read Language Data Done" << std::endl;

    json response;
    response["jsonrpc"] = "2.0";
    response["id"] = j["id"];
    response["result"] = {
        {
            {"capabilities", {
                {"textDocumentSync", 2},
                {"completionProvider", {{"resolveProvider", false}}},
                {"codeActionProvider", {"quickfix", "refactor"}},
                {"documentFormattingProvider", 1}
            }},
            {"serverInfo", {
                {"name", "mycls"},
                {"version", "0.0"}
            }}
        }
    };
    answer = std::move(response.dump());
}

void onDidOpen(const json& j, std::string& answer)
{
    
    std::string path    = j["params"]["textDocument"]["uri"];
    std::string fname   = path.substr(8);
 
    project->files.emplace(fname, ProjectFile{fname, "text"});
  
    auto it = project->files.find(fname);
    it->second.path = path;
    
    it->second.text = j["params"]["textDocument"]["text"];
 
    std::cerr << "File is added to project: " << fname << std::endl;
}

void onDidClose(const json& j, std::string& answer)
{

    std::string fname = j["params"]["textDocument"]["uri"];

    auto it = project->files.find(fname);

    if (it->second.included <= 0)
    {
        project->files.erase(it);
    }
}

void onDocumentSymbol(const json& j, std::string& answer) {

    std::cerr << "onDocumentSymbol Handler" << std::endl;
    std::string fname = j["params"]["textDocument"]["uri"];

    auto it = project->files.find(fname.substr(8));
    
    std::vector <struct Symbol> symbolList;

    std::string text = it->second.text;
    std::string::const_iterator begin {text.begin()};
    std::string::const_iterator end   {text.end()};
 
    //~ std::string temprorary;
    //~ for (const auto& word : LanguageData.types)
    //~ {
        //~ temprorary += word + '|';
    //~ }
    //~ for (const auto& word : LanguageData.custom)
    //~ {
        //~ temprorary += word + '|';
    //~ }
    //~ temprorary.pop_back();
    //~ std::string var_regex = var_prefix + temprorary + var_suffix;
    
    for (auto sym_it = SymbolOptions.begin(); sym_it != SymbolOptions.end(); sym_it++)
    {
        //~ std::cerr << "onDocumentSymbol Handler: " << static_cast<int>(sym_it->first) << std::endl;
        symbolSearch(text, begin, end, sym_it->second.regex, sym_it->first, symbolList);
    }
 
    
    
    
    json response;
    response["jsonrpc"] = "2.0";
    response["id"] = j["id"];  
    json symbs;

    for (const auto& sym : symbolList)
    {
        
        json the_symb  =    {
                            {"name",   sym.name},
                            {"detail", sym.detail},
                            {"kind",   sym.kind},
                            {"range",
                                    {
                                        {"start",
                                            {   {"line", sym.startLine},  {"character", 0}    }
                                        },
                                        {"end",
                                            {   {"line", sym.endLine},  {"character", 0}    }
                                        }
                                    }
                            },
                            {"children", ""}
                            };
                            
        if (!sym.children.empty())
        { 
            the_symb["children"] = json::array(); 
            for (const auto& child : sym.children)
            {
                the_symb["children"] +=

                    {
                            {"name",    child.name},
                            {"detail",  child.detail},
                            {"kind",    child.kind},
                            {"range",
                                    {
                                        {"start",
                                            {   {"line", child.startLine},  {"character", 0}    }
                                        },
                                        {"end",
                                            {   {"line", child.endLine},  {"character", 0}    }
                                        }
                                    }
                            },
                            {"children", ""}
                            };
            } 
        }
        symbs += the_symb;
    }
    response["result"] = std::move(symbs);
   
    std::cerr << "onDocumentSymbol Handler End" << std::endl;
    std::cerr << response << std::endl;
    std::cerr << "onDocumentSymbol Handler End" << std::endl;
    answer = response.dump(); 
    return ;

}


void onDidChange(const json& j)
{
    std::string fname = j["params"]["textDocument"]["uri"];
    auto it = project->files.find(fname.substr(8));
    
    it->second.text = j["params"]["contentChanges"][0]["text"];

    return;
}

void symbolSearch(std::string& text,
                    std::string::const_iterator begin,
                        std::string::const_iterator end,
                            std::regex& regex,
                                SymbolKind kind,
                                    std::vector <struct Symbol>& symbolList)
{
    std::smatch match;
    std::string::const_iterator start = text.begin();

    //~ std::cerr << "Searching:****************" << std::endl;
    //~ std::cerr << std::string(begin, end) << std::endl;
    //~ std::cerr << "Searching:****************" << std::endl;
    
    while (std::regex_search(begin, end, match, regex))
    {
                begin = match[0].second;
                
                //~ std::cerr << "Founded: " << match[0] << ", " << match[1] << ", " << match[2] << std::endl;
                int sline = 0, eline = 0;
                for (auto it = start; it != match[0].first; ++it)
                 {
                     if (*it == '\n')
                     {
                        sline++;
                     }
                 }
                 
                 eline = sline;
                 for (auto it = match[0].first; it != match[0].second; ++it)
                 {
                     if (*it == '\n')
                     {
                        eline++;
                     }
                 }
                struct Symbol temprorary = {static_cast<std::string>(match[SymbolOptions[kind].name]),
                                            static_cast<std::string>(match[0]), kind, sline, eline};
                
                
                if(kind == SymbolKind::Class ||  kind == SymbolKind::Function || kind == SymbolKind::Struct)
                {
                    try
                    {
                        auto startBlock = match[0].first; 
                        auto endBlock   = extractBlock(begin, end);
                        
                        size_t startPosition = startBlock - start;
                        size_t endPosition   = endBlock - start + 1;
                        
                        //~ std::cerr << "Block start: " <<  std::string(begin, endBlock)  << "End Block\n"  << std::endl;
                        
                        for (auto sym_it = SymbolOptions.begin(); sym_it != SymbolOptions.end(); sym_it++)
                        {
                            symbolSearch(text, begin, endBlock, sym_it->second.regex, sym_it->first, temprorary.children);
                        }
                        
                        for (std::string::iterator  str_it = text.begin() + startPosition;
                                                    str_it != text.begin() + endPosition;  str_it++)
                        {
                            if (*str_it != '\n')
                            {
                                *str_it = ' ';
                            }
                        }
                        
                        begin = endBlock;

                    }
                    catch (const std::exception& e)
                    {
                        //~ std::cerr << "Exception caught: " << e.what() << ". Continuing...\n";
                        continue;
                    }
                }

                symbolList.push_back(temprorary); 
    } 
}

std::string::const_iterator extractBlock(const std::string::const_iterator begin, const std::string::const_iterator end)
{
    
    auto openPos = std::find(begin, end, '{'); 
    if (openPos == end) {
        throw std::runtime_error("Відкриваючу дужку не знайдено!");
    }
    auto current = openPos;
    int bracketCount = 1;

    while (bracketCount > 0 && current != end) {
        ++current;
        if (current == end) break;  
        if (*current == '{') {
            ++bracketCount;
        } else if (*current == '}') {
            --bracketCount;
        }
    }

    if (bracketCount != 0) {
        throw std::runtime_error("Відповідну закриваючу дужку не знайдено!");
    }
 
    return current + 1;
}                        
