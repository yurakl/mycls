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

        // Додаємо пакет до черги пакетів
        request_queque.push_back(std::move(a_request));

        // Видаляємо початок - знайдений пакет
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
        // Парсимо JSON запит
        json j = json::parse(request); 
        std::cerr << "processLSPRequest: " << j["method"] << std::endl;;
        // Перевіряємо, чи є необхідні поля
        if (j.contains("jsonrpc") && j.contains("method")) {
            std::string method = j["method"];

            // Виводимо повідомлення на основі методу запиту
            
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
                //onDidChange(j);
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
    
    std::string fname = j["params"]["textDocument"]["uri"];
    fname = fname.substr(8);
    std::cerr << "About to map file: " << fname << std::endl;

    HANDLE  file_handle     = CreateFileA(fname.c_str(), GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    HANDLE  mapping_handle  = CreateFileMappingA(file_handle, NULL, PAGE_READONLY | SEC_COMMIT, 0, 0, NULL);
    char *  mapping         = (char *) MapViewOfFile(mapping_handle, FILE_MAP_READ,  0, 0, 0);

    if (mapping == NULL)
    {
        std::cerr << "Cant map file" << fname << std::endl;
        return;
    }
 
    project->files.emplace(fname, ProjectFile{fname, "text"});
  
    auto it = project->files.find(fname);

    it->second.map_info.file_handle      = file_handle;
    it->second.map_info.mapping_handle   = mapping_handle;
    it->second.map_info.mapping          = mapping;

    std::cerr << "File is added to project: " << fname << std::endl;
}

void onDidClose(const json& j, std::string& answer)
{

    std::string fname = j["params"]["textDocument"]["uri"];

    auto it = project->files.find(fname);

    if (it->second.included <= 0)
    {
        UnmapViewOfFile(it->second.map_info.mapping);

        CloseHandle(it->second.map_info.mapping_handle);

        CloseHandle(it->second.map_info.file_handle);

        project->files.erase(it);
    }
}

void onDocumentSymbol(const json& j, std::string& answer) {

    std::cerr << "onDocumentSymbol Handler" << std::endl;
    std::string fname = j["params"]["textDocument"]["uri"];

    auto it = project->files.find(fname.substr(8));
        
    std::cout << *it.first << std::endl;
    //std::string_view view(it->second.map_info.mapping);
    /*
    while (view.find("class") != std::string::npos)
    {
        
    }
    */

}



