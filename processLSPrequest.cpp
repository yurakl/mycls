#include "processLSPrequest.hpp"


Project * project = NULL;

std::string compiler_path;


std::string include_path;
std::vector <Symbol> libSymbols;

/**
 * @brief A vector containing information about built-in symbols and constructs.
 *
 * This vector stores predefined symbols and language constructs 
 * that are specified in a JSON file and loaded at runtime.
 */
std::vector<Symbol> defaultSymbols;

/**
 * @brief A list of token patterns used for symbol extraction.
 *
 * This unordered map associates different symbol types with their corresponding 
 * regex-based search patterns. Each entry consists of:
 * - A symbol type (`SymbolKind`).
 * - The index of the capture group containing the main identifier (e.g., variable or function name).
 * - The index of the capture group containing the full match with additional details.
 * - A `std::regex` pattern used for matching the symbol in the text.
 *
 * The map is used to identify various C++ constructs such as variables, methods, 
 * functions, structs, and classes.
 */
 
std::unordered_map <SymbolKind, SymbolOpt> SymbolOptions =
{ 
    {SymbolKind::Variable, {3, 0, std::regex(R"(\b([*&]*([a-zA-Z_]+)\s*[*&]*\s+)+[*&]*([a-zA-Z_]+)\s*[;|=])")}},
    {SymbolKind::Method,   {4, 0, std::regex(R"((\w[\w\s*&]+)\s+(\w+)(\s*::\s*)(\w+)\s*\(([^)]*))")}},
    {SymbolKind::Function, {2, 0, std::regex(R"((\w[\w\s*&]+)\s+(\w+)\s*\(([^)]*)\))")}},
    {SymbolKind::Struct,   {1, 0, std::regex(R"(\bstruct\s+(\w+)\s*)")}},
    {SymbolKind::Class,    {1, 0, std::regex(R"(\bclass\s+(\w+)\s*)")}},
    {SymbolKind::File,     {2, 0, std::regex(R"(#include\s+("|<)(.+)("|>))")}}
};

/**
 * @brief A vector containing user-defined data types and functions.
 *
 * This vector stores custom data types and functions that are defined by the user
 * and are processed during the program's execution.
 */
std::vector <Symbol> symbolList;


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

    std::ifstream file("D:\\NULP\\Code\\mycls\\ccpp.json");
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

<<<<<<< HEAD
    findLibSymbols(text);
=======
    ignoreComment(text);
>>>>>>> ignore_comment
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

void symbolSearch(std::string& text,
                    std::string::const_iterator begin,
                        std::string::const_iterator end,
                            std::regex& regex,
                                SymbolKind kind,
                                    std::vector <struct Symbol>& symbolList)
{
    std::smatch match;
    std::string::const_iterator start = text.begin();
     
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
                struct Symbol temprorary = {
                                        .label  = static_cast<std::string>(match[SymbolOptions[kind].name]),
                                        .detail = static_cast<std::string>(match[0]),
                                        .kind   = kind,
                                        .startLine  = sline,
                                        .endLine    = eline};
                
                
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

                        //~ std::cerr << std::string(match[0].first, match[0].second) << std::endl;
                        //~ std::cerr << "-------Block---------------" << std::endl;
                        //~ std::cerr << std::string(begin, endBlock) << std::endl;
                        //~ std::cerr << "-------Block---------------" << std::endl;
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
                if (kind == SymbolKind::Variable)
                {
                    //~ std::cerr << "Type search: " << std::string(match[0].first, match[0].second) << std::endl;
                    std::string word = R"(\b((?!const|static|extern|mutable|volatile|thread_local|constexpr|explicit)\w+)+\b)";
                    //~ std::cerr << "Regex search: " << word << std::endl;
                    std::regex_search(match[0].first, match[0].second, match, std::regex{word});
                    temprorary.type = match[1];
                    
                }
                //~ std::cerr << "----------Text------------" << std::endl;
                //~ std::cerr << text << std::endl;
                //~ std::cerr << "----------Text------------" << std::endl;
                symbolList.push_back(temprorary); 
    } 
}

/**
 * @brief Extracts a block of code enclosed in curly braces ({}) from the given text.
 *
 * This function takes iterators pointing to the beginning and the end of the text,
 * searches for the matching pair of curly braces, and returns an iterator to the
 * end of the block (i.e., the closing brace). It throws an exception if no valid
 * pair of curly braces is found.
 *
 * @param begin Iterator pointing to the start of the block (opening brace).
 * @param end Iterator pointing to the end of the text where the search stops.
 * @return std::string::const_iterator Iterator pointing to the closing brace of the block.
 * @throws std::invalid_argument If no matching pair of curly braces is found.
 */
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
        //~ std::cerr << "Namespace detected" << std::endl;
        std::string word(match[1].first, match[1].second);
        auto symbol_it = std::find_if(symbolList.begin(), symbolList.end(), [&word](auto& iterator) { return iterator.label == word;});

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

<<<<<<< HEAD
void findLibSymbols(std::string& text)
{
    std::smatch match;
    std::string::const_iterator begin(text.begin());
    std::string::const_iterator end(text.end());
    while(std::regex_search(begin, end, match, std::regex(R"(#include\s+(<)(.+)(>))")))
    {
        
        std::string path = include_path + std::string(match[2].first, match[2].second);
        std::cerr << "Searching: " << path << std::endl;
        std::ifstream lib(path, std::ios::in);
        if (!lib)
        {
            std::cerr << "File " << path << "is not found" << std::endl;
            break;
        }

        std::ostringstream buffer;
        buffer << lib.rdbuf();
        std::string  libtext(buffer.str());
        
        for (auto sym_it = SymbolOptions.begin(); sym_it != SymbolOptions.end(); sym_it++)
        {
            //~ std::cerr << "Searching: " << static_cast<int>(sym_it->first) << std::endl;
            symbolSearch(libtext, libtext.begin(), libtext.end(), sym_it->second.regex, sym_it->first, libSymbols);
        }
        begin = match[0].second;
        
    }

    std::cerr <<"LibSymbols" << std::endl;
    std::for_each(libSymbols.begin(), libSymbols.end(), [](auto& iterator){ std::cerr << iterator.label << "-" << iterator.type << std::endl; });
        
=======
void ignoreComment(std::string& text)
{

    std::cerr << "ignoreComment" << std::endl;
    std::smatch match;
    std::string::const_iterator begin   = text.begin();
    std::string::const_iterator end     = text.end();
    size_t start = 0;
    while(std::regex_search(begin, end, match, std::regex(R"(\/\*((.*?)(\n*))*\*\/)")))
    {
        std::cerr << "Match: "      << std::string(match[0].first, match[0].second) << std::endl; 
        std::cerr << "Position: "   << match.position(0) << std::endl; 
        std::cerr << "Length: "     << match.length(0) << std::endl;
        std::cerr << "Text: "       << std::string(begin + match.position(0), begin + match.position(0) + match.length(0)) << std::endl;
        std::string::iterator startBlock = text.begin() + start + match.position(0);
        std::string::iterator   endBlock = text.begin() + start + match.position(0) + match.length(0);
        
        for_each(   startBlock,
                    endBlock,
                    [](char& iterator){iterator = ' ';} );
        begin = match[0].second;
        start = begin - text.begin();
    }
    
    begin = text.begin();
    start = 0;
    while(std::regex_search(begin, end, match, std::regex(R"(\/\/(.+)\n)")))
    {
        std::cerr << "Match: "      << std::string(match[0].first, match[0].second) << std::endl; 
        std::cerr << "Position: "   << match.position(0) << std::endl; 
        std::cerr << "Length: "     << match.length(0) << std::endl;
        std::cerr << "Text: "       << std::string(begin + match.position(0), begin + match.position(0) + match.length(0) - 1) << std::endl; 
        std::string::iterator startBlock = text.begin() + start + match.position(0);
        std::string::iterator   endBlock = text.begin() + start + match.position(0) + match.length(0) - 1;
        
        for_each(   startBlock,
                    endBlock,
                    [](char& iterator){iterator = ' ';} );
        begin = match[0].second;
        start = begin - text.begin();
    }
    
    std::cerr << "----------Text------------" << std::endl;
    std::cerr << text << std::endl;
    std::cerr << "----------Text------------" << std::endl;
    
>>>>>>> ignore_comment
}
