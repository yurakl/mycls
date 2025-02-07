#include "textProcessor.hpp"

extern std::string compiler_path;
extern std::string include_path;
extern Project * project;
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


void ignoreComment(std::string& text)
{

    std::cerr << "ignoreComment" << std::endl;
    std::smatch match;
    std::string::const_iterator begin   = text.begin();
    std::string::const_iterator end     = text.end();
    size_t start = 0;
    while(std::regex_search(begin, end, match, std::regex(R"(\/\*((.*?)(\n*))*\*\/)")))
    {
        //~ std::cerr << "Match: "      << std::string(match[0].first, match[0].second) << std::endl; 
        //~ std::cerr << "Position: "   << match.position(0) << std::endl; 
        //~ std::cerr << "Length: "     << match.length(0) << std::endl;
        //~ std::cerr << "Text: "       << std::string(begin + match.position(0), begin + match.position(0) + match.length(0)) << std::endl;
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
        //~ std::cerr << "Match: "      << std::string(match[0].first, match[0].second) << std::endl; 
        //~ std::cerr << "Position: "   << match.position(0) << std::endl; 
        //~ std::cerr << "Length: "     << match.length(0) << std::endl;
        //~ std::cerr << "Text: "       << std::string(begin + match.position(0), begin + match.position(0) + match.length(0) - 1) << std::endl; 
        std::string::iterator startBlock = text.begin() + start + match.position(0);
        std::string::iterator   endBlock = text.begin() + start + match.position(0) + match.length(0) - 1;
        
        for_each(   startBlock,
                    endBlock,
                    [](char& iterator){iterator = ' ';} );
        begin = match[0].second;
        start = begin - text.begin();
    }
    
    //~ std::cerr << "----------Text------------" << std::endl;
    //~ std::cerr << text << std::endl;
    //~ std::cerr << "----------Text------------" << std::endl;

    
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
                
                
                if(kind == SymbolKind::Namespace || kind == SymbolKind::Class ||  kind == SymbolKind::Function || kind == SymbolKind::Struct)
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


void processFile(ProjectFile& the_file)
{
    if (the_file.didChange)
    {
        the_file.symbolList.clear();
        std::string text = the_file.text;
        std::string::const_iterator begin {text.begin()};
        std::string::const_iterator end   {text.end()};
    
        ignoreComment(text);
        
        for (auto sym_it = SymbolOptions.begin(); sym_it != SymbolOptions.end(); sym_it++)
        { 
            symbolSearch(text, begin, end, sym_it->second.regex, sym_it->first, the_file.symbolList);
        }

        std::cerr << "1. " << the_file.path << ":" << std::endl;

        std::for_each(the_file.symbolList.begin(), the_file.symbolList.end(), [](auto& iterator){ std::cerr << iterator.label << "-" << static_cast<int>(iterator.kind) << std::endl; });

    }
    the_file.didChange = 0;    
}

void processIncluded(ProjectFile& the_file)
{
    for (auto& symbol : the_file.symbolList)
    {
        if(symbol.kind == SymbolKind::File || symbol.kind == SymbolKind::Package)
        {
                auto temprorary = project->files.find(symbol.label);
                if(temprorary == project->files.end())
                {
                    project->files.emplace(symbol.label, ProjectFile{symbol.label, "text"});
                    auto lib_it = project->files.find(symbol.label);

                    ProjectFile& new_item = lib_it->second;

                    if(symbol.kind == SymbolKind::Package)
                    {
                        new_item.path = include_path + symbol.label;
                    } else {
                        new_item.path = symbol.label;
                    }
                    
                    std::ifstream lib(new_item.path, std::ios::in);
                    if (!lib)
                    {
                        std::cerr << "File " << new_item.path << "is not found" << std::endl;
                        break;
                    }

                    std::ostringstream buffer;
                    buffer << lib.rdbuf();
                    new_item.text = buffer.str();
                    processFile(new_item);
                    the_file.includedFiles.push_back(&new_item);
                }
                else if (temprorary != project->files.end() && temprorary->second.didChange == 1)
                {
                    ProjectFile& new_item = temprorary->second;
                    processFile(new_item);
                }
        }
    }
}
