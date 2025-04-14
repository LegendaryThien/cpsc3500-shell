#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_TOKENS 20
#define MAX_TOKEN_LENGTH 20

// Function to parse the command line into arguments
void parseCommand(char* commandLine, char** args, int& argCount) {
    argCount = 0;
    char* token = strtok(commandLine, " ");
    
    while (token != nullptr && argCount < MAX_TOKENS) {
        args[argCount++] = token;
        token = strtok(nullptr, " ");
    }
    args[argCount] = nullptr;  // execvp requires null-terminated array
}

int main() {
    char commandLine[1024];
    char* args[MAX_TOKENS + 1];
    int argCount;
    
    while (true) {
        // Display prompt
        std::cout << "myshell$";
        std::cout.flush();
        
        // Read command from stdin
        if (fgets(commandLine, sizeof(commandLine), stdin) != nullptr) {
            // Remove trailing newline if present
            size_t len = strlen(commandLine);
            if (len > 0 && commandLine[len-1] == '\n') {
                commandLine[len-1] = '\0';
            }
            
            // Parse the command
            parseCommand(commandLine, args, argCount);
            
            if (argCount > 0) {
                // Check for exit command
                if (strcmp(args[0], "exit") == 0) {
                    break;
                }
                
                // Fork a child process
                pid_t pid = fork();
                
                if (pid < 0) {
                    // Error occurred
                    std::cerr << "Fork failed" << std::endl;
                    return 1;
                } else if (pid == 0) {
                    // Child process
                    execvp(args[0], args);
                    // If execvp returns, there was an error
                    std::cerr << "Command not found: " << args[0] << std::endl;
                    exit(1);
                } else {
                    // Parent process
                    int status;
                    wait(&status);
                    
                    if (WIFEXITED(status)) {
                        std::cout << "process " << pid << " exits with exit status value " 
                                  << WEXITSTATUS(status) << std::endl;
                    }
                }
            }
        }
    }
    
    return 0;
}
