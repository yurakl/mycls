#include <iostream>
#include <Windows.h>
#include <windows.h>
#include <winbase.h>
#include "processLSPrequest.hpp"

const int buf_size = 4096;

 
void viaPipe() {
 
    long unsigned read = 0;
    char          from_pipe[buf_size];
    HANDLE        handle_pipe_read  = GetStdHandle(STD_INPUT_HANDLE);
    HANDLE        handle_pipe_write = GetStdHandle(STD_OUTPUT_HANDLE);

    std::string request;
    std::vector<std::string> answer_queque;
    
    while (ReadFile(handle_pipe_read, from_pipe, buf_size, &read, NULL)) 
    {
        from_pipe[read] = '\0';
        request += from_pipe; 

        if (request.ends_with('}') || request.ends_with("}\r\n") || request.ends_with("}\n"))
        {
            processAllRequests(request, answer_queque);

            if (!answer_queque.empty())
            {
                for (const auto& answer : answer_queque)
                {
                        WriteFile(handle_pipe_write, answer.c_str(), answer.size(), &read, NULL); 
                }

                memset(from_pipe, 0, buf_size);
                request.clear();
                answer_queque.clear();
            }
        }
    }
    std::cerr << "End session" << std::endl;
}

 

int main(int argc, char* argv[]) {  

    DWORD pid = GetCurrentProcessId();
    std::cerr << "PID: " << pid << std::endl;

    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <mode> [additional_args]\n";
        std::cerr << "Modes: check=<filename>, named_pipe=<pipe_name>, anonymous_pipe\n";
        return -1;
    }

    std::string mode = argv[1];

    if(mode.find("-", 0) == 0)
    {
        std::cerr  << "Do not use '-' or '--' before an option" << std::endl;
        return -1;
    }

    if (mode.find("check", 0) != std::string::npos) {
        std::string filename;
        filename = mode.substr(6); 
        
    }   else if (mode == "ap") {  
        std::cerr << "Anonymous pipe mode" << std::endl; 
        viaPipe();

    } else {
    
        std::cerr << "Error: Unknown mode " << mode << "\n";
        return 1;
    
    }

    return 0;
}
