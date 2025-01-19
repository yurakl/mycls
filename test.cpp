#include <iostream>
#include <cstdio>
#include <string>
#include <thread>
#include <chrono>

#define win32

const int buf_size = 4096;

#ifdef win32
    #include <windows.h>
    HANDLE hStdInRead,  hStdInWrite; 
    HANDLE hStdOutRead, hStdOutWrite; 
#endif

void connectPipe(const std::string& request, std::string& answer)
{
    long unsigned written;
    char from_pipe[buf_size];
    memset(from_pipe, 0, buf_size);

    WriteFile(hStdInWrite, request.c_str(), request.size(), &written, NULL); 
    
    ReadFile(hStdOutRead, from_pipe, buf_size, &written, NULL);
    
    answer = from_pipe;
    

}

int main() {

    std::string  request, answer;
    

    SECURITY_ATTRIBUTES saAttrIn, saAttrOut; 
    saAttrIn.nLength = sizeof(SECURITY_ATTRIBUTES); 
    saAttrOut.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttrIn.bInheritHandle = TRUE; 
    saAttrOut.bInheritHandle = TRUE; 
    saAttrIn.lpSecurityDescriptor = NULL; 
    saAttrOut.lpSecurityDescriptor = NULL;

    if (!CreatePipe(&hStdInRead, &hStdInWrite, &saAttrIn, 0)) 
    { 
        std::cerr << "CreatePipe failed" << std::endl; 
        return 1; 
    }  
    if (!CreatePipe(&hStdOutRead, &hStdOutWrite, &saAttrOut, 0)) 
    { 
        std::cerr << "CreatePipe failed" << std::endl; 
        return 1; 
    }   


    #ifdef win32
        std::cout << "Process Creating" << std::endl;
        STARTUPINFO si;
        PROCESS_INFORMATION pi;

        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        si.dwFlags = STARTF_USESTDHANDLES;
        si.hStdOutput = hStdOutWrite; 
        si.hStdInput  = hStdInRead;
        ZeroMemory(&pi, sizeof(pi));

        // Створюємо процес
        if (!
            CreateProcess(
            NULL, // Ім'я програми
            (char *)  "mycls.exe ap", // Програма, яку ми хочемо запустити
            NULL, 
            NULL, 
            TRUE, 
            0, 
            NULL, 
            NULL, 
            &si, 
            &pi)) {
            std::cerr << "CreateProcess failed!" << std::endl;
            return 1;
        
    }
    std::cout << "Process is created" << std::endl;

    #endif

    std::this_thread::sleep_for(std::chrono::seconds(1));

    request = R"({
        "jsonrpc": "2.0",
        "id": 1,
        "method": "initialize",
        "params": {
            "rootUri": "file:///c:/projects/my_project",
            "workspaceFolders": [
                {
                    "uri": "file:///c:/projects/my_project",
                    "name": "MyProject"
                }
            ]
        }
    })";
      
   
    std::this_thread::sleep_for(std::chrono::seconds(1)); // Паузи на 3 секунди
    connectPipe(request, answer);   
    request = "exit";
    std::cout << "Answer: " << answer;
    std::this_thread::sleep_for(std::chrono::seconds(1)); // Паузи на 3 секунди

    // Очікуємо завершення дочірнього процесу

    TerminateProcess(pi.hProcess, 0);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    std::cout << "Quit" << std::endl;
     
    return 0;
}