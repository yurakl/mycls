#include "processLSPrequest.hpp"

Project * project = NULL;

std::string compiler_path;

std::string include_path;

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
                //~ std::cerr << "didOpen" << std::endl;
                onDidOpen(j, answer);
            }
            else if (method == "textDocument/didCLose") {
                //~ std::cerr << "didClose" << std::endl;
                onDidClose(j, answer);
            }
            else if (method == "textDocument/didChange") {
                //~ std::cerr << "didChange" << std::endl;
                onDidChange(j);
            }
            else if (method == "textDocument/documentSymbol") {
                //~ std::cerr << "documentSymbol" << std::endl;
                onDocumentSymbol(j, answer);
            }
            else if (method == "initialize") {
                handleInitialize(j, answer);
            }
            else if (method == "textDocument/completion") {
                onComletion(j, answer);
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
    //~ std::cerr << "handleInitialize" << std::endl;
    std::string rootUri     = j["params"]["rootUri"]; 
    std::string projectId   = std::to_string(std::hash<std::string>{}(rootUri));

    project = new Project(rootUri, projectId);

    std::ifstream file("D:\\NULP\\Code\\mycls\\data\\ccpp.json");
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open keywords.json" << std::endl;
        return;
    }

    //~ std::cerr << "Read Language Data" << std::endl;

    json data = json::parse(file);
    //~ std::cerr << "Read Language Data" << std::endl;
    // if (rootUri.end_with(".c"))
    for (const auto& entry : data["languages"]["C++"])
    {
        defaultSymbols.emplace_back(Symbol{ .label      = entry["label"],
                                            .detail     = entry["detail"],
                                            .kind       = entry["kind"],
                                            .documentation  = entry["documentation"],
                                            .insertText     = entry.contains("insertText") ? entry.at("insertText") : "",
                                            .insertTextFormat = entry.contains("insertTextFormat") ? static_cast<int>(entry.at("insertTextFormat")) : 0 });
    }
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
 
    //~ std::cerr << "File is added to project: " << fname << std::endl;
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

    //~ std::cerr << "onDocumentSymbol Handler" << std::endl;
    std::string fname = j["params"]["textDocument"]["uri"];

    auto it = project->files.find(fname.substr(8));
    
    symbolList.clear();
    
    std::string text = it->second.text;
    std::string::const_iterator begin {text.begin()};
    std::string::const_iterator end   {text.end()};
 

    ignoreComment(text);
    findLibSymbols(text); 
     
    for (auto sym_it = SymbolOptions.begin(); sym_it != SymbolOptions.end(); sym_it++)
    {
        //~ std::cerr << "Searching: " << static_cast<int>(sym_it->first) << std::endl;
        symbolSearch(text, begin, end, sym_it->second.regex, sym_it->first, symbolList);
    }

    //~ std::cerr << "Symbol List start:" << std::endl;

    //~ std::for_each(symbolList.begin(), symbolList.end(), [](auto& iterator){ std::cerr << iterator.label << "-" << iterator.type << std::endl; });
    
    //~ std::cerr << "Symbol List end." << std::endl;

    
    json response;
    response["jsonrpc"] = "2.0";
    response["id"] = j["id"];  
    json symbs;

    for (const auto& sym : symbolList)
    {
        
        json the_symb  =    {
                            {"name",   sym.label},
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
                            {"name",    child.label},
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


void onComletion(const json& j, std::string& answer)
{
    //~ std::cerr << "onComletion Handler" << std::endl;
    std::string fname = j["params"]["textDocument"]["uri"];
    auto it = project->files.find(fname.substr(8));

    size_t  line         = j["params"]["position"]["line"];
    size_t  character    = j["params"]["position"]["character"];
    int     l_iterator   = 1;
    std::string::const_iterator s_iterator = it->second.text.begin(); 
    
    while(l_iterator <= line)
    {
        if (*s_iterator == '\n') [[unlikely]]
        {
            l_iterator++;
        }
        s_iterator++;
    }
    s_iterator += character - 1;
    std::string::const_iterator e_iterator = s_iterator + 1;
    
    
    while(*s_iterator != ' ' && *s_iterator != '\t' && *s_iterator != ';' && *s_iterator != '\n' && s_iterator != it->second.text.begin())
    {
        s_iterator--;
    }
    while(*e_iterator != ' ' && *e_iterator != '\t' && *e_iterator != ';' && *e_iterator != '\n' && e_iterator != it->second.text.end())
    {
        e_iterator++;
    }
    e_iterator++;
    //~ std::cerr << "1:"<< std::string(s_iterator, e_iterator)  << std::endl;
    std::smatch match;
    std::vector<Symbol> results;  
    if (std::regex_search(s_iterator,
                          e_iterator,
                          match,
                          std::regex{R"(\s+([a-zA-Z_]+)\s+)"}))
    {       
                          
        std::string word(match[1].first, match[1].second);
        
        //~ std::cerr << "2:"<< match[0] << std::endl;
        
        std::copy_if(defaultSymbols.begin(),
                     defaultSymbols.end(),
                     std::back_inserter(results),
                     [&word](auto& iterator) { return iterator.label.starts_with(word); });
        std::copy_if(symbolList.begin(),
                     symbolList.end(),
                     std::back_inserter(results),
                     [&word](auto& iterator) { return iterator.label.starts_with(word); });
 
    } else if (std::regex_search(s_iterator,
                          e_iterator,
                          match,
                          std::regex{R"(\b([a-zA-Z][a-zA-Z0-9_]*)(\.|->)([a-zA-Z]*)?)"}))
    {
            //~ std::cerr << "1..." << match[1] << std::endl;
            //~ std::cerr << "2..." << match[2] << std::endl;
            //~ std::cerr << "3..." << match[3] << std::endl;
            //~ std::cerr << "4..." << match[4] << std::endl;
            //~ std::cerr << "5..." << match[5] << std::endl;
            std::string word(match[1].first, match[1].second);
            
            auto symbol_it = std::find_if(symbolList.begin(), symbolList.end(), [&word](auto& iterator) { return iterator.label == word;});
            

            //~ std::cerr << "Name: " << symbol_it->label << " Type: " << symbol_it->type << std::endl;
            
            if(symbol_it != symbolList.end())
            {
                word = symbol_it->type;
                auto symbol_it = std::find_if(symbolList.begin(), symbolList.end(), [&word](auto& iterator) { return iterator.label == word;});
                word = std::string(match[4].first, match[4].second);
                std::copy_if(symbol_it->children.begin(),
                     symbol_it->children.end(),
                     std::back_inserter(results),
                     [&word](auto& iterator) { return iterator.label.starts_with(word); });

            }
    } else if (std::regex_search(s_iterator,
                          e_iterator,
                          match,
                          std::regex{R"(\b([a-zA-Z][a-zA-Z0-9_]*)(::)([a-zA-Z]*)?)"}))
    {
        std::cerr << "Namespace detected" << std::endl;
        std::string word(match[1].first, match[1].second);
        auto symbol_it = std::find_if(symbolList.begin(), symbolList.end(), [&word](auto& iterator) { return iterator.label == word;});

        if(symbol_it == symbolList.end())
        {
            symbol_it = std::find_if(libSymbols.begin(), libSymbols.end(), [&word](auto& iterator) { return iterator.label == word;});
        }
        
        if(symbol_it != symbolList.end())
        {
            word = std::string(match[4].first, match[4].second);
            std::copy_if(symbol_it->children.begin(),
                            symbol_it->children.end(),
                            std::back_inserter(results),
                            [&word](auto& iterator) { return iterator.label.starts_with(word);});

        }
    }
    
    //~ for(const auto& el : results)
    //~ {
        //~ std::cerr << "Result: " << el.label << std::endl;
    //~ }

    json response;
    response["jsonrpc"] = "2.0";
    response["id"] = j["id"];  
    json symbs;

    if(!results.empty())
    {
        symbs["isIncomplete"]   = "false";
        symbs["items"]   = json::array();
        int a = 0;
        for(const auto& el : results)
        {
            symbs["items"] +=   {
                                {"label",   el.label},
                                {"kind",    el.kind},
                                {"detail",  el.detail},
                                {"documentation",   el.documentation},
                                {"insertText",      el.insertText},
                                {"sortText",        std::to_string(a)}
                                };
            a++;
        }
    }
      
    response["result"] = std::move(symbs);

   
    //~ std::cerr << "onDocumentSymbol Handler End" << std::endl;
    //~ std::cerr << response << std::endl;
    //~ std::cerr << "onDocumentSymbol Handler End" << std::endl;
    answer = response.dump(); 
    return ;
}   
 


