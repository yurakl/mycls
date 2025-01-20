#include "processLSPrequest.hpp"


Project * project = NULL;
struct LanguageData LanguageData;

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
        
     
    //~ Регулярні вирази для пошуку класів, структур і функцій
    std::regex classRegex(R"(\bclass\s+(\w+)\s*)");
    //~ std::regex structRegex(R"(\bstruct\s+(\w+)\s*{)");
    std::regex functionRegex(R"((\w[\w\s*&]+)\s+(\w+)\s*\(([^)]*)\)\s*)");
 
    std::string::const_iterator begin {it->second.text.begin()};
    std::string::const_iterator end   {it->second.text.end()};
    
    std::vector <struct Symbol> symbolList;


    for (auto sym_it = SymbolOptions.begin(); sym_it != SymbolOptions.end(); sym_it++)
    {
        symbolSearch(begin, begin, end, sym_it.second.regex, sym_it.second.first, symbolList);
    }
     
    
    json response;
    response["jsonrpc"] = "2.0";
    response["id"] = j["id"];  
    json symbs;

    for (const auto& sym : symbolList)
    {
        
        symbs +=   {
                            {"name", sym.name},
                            {"detail", sym.detail},
                            {"kind", SymbolKind::Class},
                            {"range",
                                    {
                                        {"start",
                                            {   {"line", sym.startLine},  {"character", 0}    }
                                        },
                                        {"end",
                                            {   {"line", sym.endLine},  {"character", 0}    }
                                        }
                                    }
                            }
                    };

    }
    response["result"] = std::move(symbs);
   
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

void symbolSearch(const std::string::const_iterator& start,
                    std::string::const_iterator begin,
                        std::string::const_iterator end,
                            std::regex& regex,
                                SymbolKind kind,
                                    std::vector <struct Symbol>& symbolList)
{
    std::smatch match;
    while (std::regex_search(begin, end, match, regex))
    { 
                begin = match[0].second;
                //~ int a = 0;
                //~ for (auto tt = match.begin(); tt != match.end(); tt++)
                //~ {
                    //~ std::cerr << a << ": " << *tt << std::endl;
                    //~ a++;
                //~ }
                //~ if (kind == SymbolKind::Function)
                //~ {
                    //~ std::cerr << "::::: " << match[2] << std::endl;
                //~ }
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
                 try {
                    auto endBlock =  extractBlock(begin, end);
                    begin = endBlock;
                } catch (const std::exception& e) {
                    std::cerr << "Exception caught: " << e.what() << ". Continuing...\n";
                    continue; 
                } 
                struct Symbol temprorary = {kind == SymbolKind::Function ? static_cast<std::string>(match[2]) : static_cast<std::string>(match[1]),
                                            static_cast<std::string>(match[0]), kind, sline, eline};
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
 
    return current;
}                        
