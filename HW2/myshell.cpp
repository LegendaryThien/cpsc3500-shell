#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_TOKENS 20     
#define MAX_COMMANDS 10

char* commands[MAX_COMMANDS][MAX_TOKENS];
int tokenCounts[MAX_COMMANDS];

void parse(char* commandLine, int& cmdCount) {
    // Initialize token counts
    for (int i = 0; i < MAX_COMMANDS; i++) {
        tokenCounts[i] = 0;
    }
    
    // Count the number of pipe characters to determine how many commands we have
    int pipeCount = 0;
    for (int i = 0; commandLine[i] != '\0'; i++) {
        if (commandLine[i] == '|') {
            pipeCount++;
        }
    }
    
    // The number of commands is the number of pipes plus 1
    cmdCount = pipeCount + 1;
    
    if (cmdCount > MAX_COMMANDS) {
        std::cerr << "Error: Too many commands in pipeline" << std::endl;
        cmdCount = 0;
        return;
    }
    
    // Split the command line by replacing | with null terminators
    char* currentCmd = commandLine;
    int currentCmdIndex = 0;
    
    while (currentCmdIndex < cmdCount) {
        // Trim leading whitespace
        while (*currentCmd == ' ') currentCmd++;
        
        // Find the next pipe or end of string
        char* nextPipe = strchr(currentCmd, '|');
        char* cmdEnd;
        
        if (nextPipe != nullptr) {
            *nextPipe = '\0';  
            cmdEnd = nextPipe - 1;
        } else {
            cmdEnd = currentCmd + strlen(currentCmd) - 1;
        }
        
        // Trim trailing whitespace
        while (cmdEnd > currentCmd && *cmdEnd == ' ') {
            *cmdEnd = '\0';
            cmdEnd--;
        }
        
        // Skip empty commands
        if (strlen(currentCmd) == 0) {
            std::cerr << "Error: Empty command in pipeline" << std::endl;
            // Clean up previously allocated memory
            for (int i = 0; i < currentCmdIndex; i++) {
                for (int j = 0; j < tokenCounts[i]; j++) {
                    free(commands[i][j]);
                    commands[i][j] = nullptr;
                }
                tokenCounts[i] = 0;
            }
            cmdCount = 0;
            return;
        }
        
        // Parse this command into tokens
        char* token = strtok(currentCmd, " ");
        
        while (token != nullptr && tokenCounts[currentCmdIndex] < MAX_TOKENS) {
            // Allocate memory for token and copy it
            commands[currentCmdIndex][tokenCounts[currentCmdIndex]] = strdup(token);
            if (commands[currentCmdIndex][tokenCounts[currentCmdIndex]] == nullptr) {
                std::cerr << "Error: Memory allocation failed" << std::endl;
                // Clean up previously allocated memory
                for (int i = 0; i <= currentCmdIndex; i++) {
                    for (int j = 0; j < tokenCounts[i]; j++) {
                        free(commands[i][j]);
                        commands[i][j] = nullptr;
                    }
                    tokenCounts[i] = 0;
                }
                cmdCount = 0;
                return;
            }
            tokenCounts[currentCmdIndex]++;
            token = strtok(nullptr, " ");
        }
        
        if (tokenCounts[currentCmdIndex] >= MAX_TOKENS) {
            std::cerr << "Error: Too many tokens in command" << std::endl;
            // Clean up previously allocated memory
            for (int i = 0; i <= currentCmdIndex; i++) {
                for (int j = 0; j < tokenCounts[i]; j++) {
                    free(commands[i][j]);
                    commands[i][j] = nullptr;
                }
                tokenCounts[i] = 0;
            }
            cmdCount = 0;
            return;
        }
        
        // Move to next command
        if (nextPipe != nullptr) {
            currentCmd = nextPipe + 1;
        }
        currentCmdIndex++;
    }
}

void execute(int cmdCount) {
    if (cmdCount == 0) return;
    
    if (cmdCount > MAX_COMMANDS) {
        std::cerr << "Error: Too many commands in pipeline" << std::endl;
        return;
    }
    
    if (cmdCount == 1) {
        pid_t pid = fork();
        
        if (pid < 0) {
            perror("fork failed");
            return;
        } else if (pid == 0) {
            // Child process
            char* args[MAX_TOKENS + 1];
            
            // Check if we have a valid command
            if (tokenCounts[0] == 0 || commands[0][0] == nullptr) {
                std::cerr << "Error: Empty command" << std::endl;
                exit(1);
            }
            
            // Copy arguments
            for (int i = 0; i < tokenCounts[0]; i++) {
                args[i] = commands[0][i];
            }
            args[tokenCounts[0]] = nullptr;  
            
            execvp(args[0], args);
            
            // If execvp returns, it means it failed
            std::cerr << "Error: Command '" << args[0] << "' not found or failed to execute" << std::endl;
            exit(1);
        } else {
            int status;
            wait(&status);
            if (WIFEXITED(status)) {
                if (WEXITSTATUS(status) != 0) {
                    std::cerr << "Process " << pid << " exited with status " << WEXITSTATUS(status) << std::endl;
                }
            } else if (WIFSIGNALED(status)) {
                std::cerr << "Process " << pid << " terminated by signal " << WTERMSIG(status) << std::endl;
            }
        }
    } else {
        int** pipes = new int*[cmdCount - 1];
        for (int i = 0; i < cmdCount - 1; i++) {
            pipes[i] = new int[2];
            if (pipe(pipes[i]) == -1) {
                perror("pipe creation failed");
                // Clean up pipes that were already created
                for (int j = 0; j < i; j++) {
                    delete[] pipes[j];
                }
                delete[] pipes;
                return;
            }
        }
        
        pid_t pids[MAX_COMMANDS];  
        
        for (int i = 0; i < cmdCount; i++) {
            pid_t pid = fork();
            pids[i] = pid; 
            
            if (pid < 0) {
                perror("fork failed");
                // Clean up pipes
                for (int j = 0; j < cmdCount - 1; j++) {
                    delete[] pipes[j];
                }
                delete[] pipes;
                return;
            } else if (pid == 0) {

                if (i > 0) {
                    close(0);
                    dup(pipes[i-1][0]);
                    close(pipes[i-1][1]);
                }
                
                if (i < cmdCount - 1) {
                    close(1);
                    dup(pipes[i][1]);
                    close(pipes[i][0]);
                }
                

                for (int j = 0; j < cmdCount - 1; j++) {
                    if (j != i-1 && j != i) {
                        close(pipes[j][0]);
                        close(pipes[j][1]);
                    }
                }
                

                char* args[MAX_TOKENS + 1];
                
                for (int j = 0; j < tokenCounts[i]; j++) {
                    args[j] = commands[i][j];
                }
                args[tokenCounts[i]] = nullptr;  
                
                execvp(args[0], args);
                perror("execvp failed");
                exit(1);
            } else {
                if (i > 0) {
                    close(pipes[i-1][0]);
                }
                if (i < cmdCount - 1) {
                    close(pipes[i][1]);
                }
            }
        }
        
        // Wait for all child processes to complete
        for (int i = 0; i < cmdCount; i++) {
            int status;
            waitpid(pids[i], &status, 0);
            if (WIFEXITED(status)) {
                std::cout << "Process " << pids[i] << " exited with status " << WEXITSTATUS(status) << std::endl;
            }
        }
        
        for (int i = 0; i < cmdCount - 1; i++) {
            delete[] pipes[i];
        }
        delete[] pipes;
    }
}

int main() {
    char commandLine[1024];
    int cmdCount;
    
    while (true) {
        std::cout << "myshell$ ";
        
        if (fgets(commandLine, sizeof(commandLine), stdin) == nullptr) {
            std::cout << std::endl;
            break;
        }
        
        // Remove trailing newline if present
        size_t len = strlen(commandLine);
        if (len > 0 && commandLine[len-1] == '\n') {
            commandLine[len-1] = '\0';
        }
        
        // Skip empty commands
        if (strlen(commandLine) == 0) {
            continue;
        }
        
        // Reset command count
        cmdCount = 0;
        
        if (strchr(commandLine, '|') != nullptr) {
            parse(commandLine, cmdCount);
            if (cmdCount > 0) {
                execute(cmdCount);
            }
        } else {
            // Handle single command case
            char* token = strtok(commandLine, " ");
            tokenCounts[0] = 0;
            
            // Clear any previous commands
            for (int i = 0; i < MAX_COMMANDS; i++) {
                for (int j = 0; j < tokenCounts[i]; j++) {
                    if (commands[i][j] != nullptr) {
                        free(commands[i][j]);
                        commands[i][j] = nullptr;
                    }
                }
                tokenCounts[i] = 0;
            }
            
            while (token != nullptr && tokenCounts[0] < MAX_TOKENS) {
                commands[0][tokenCounts[0]] = strdup(token);
                if (commands[0][tokenCounts[0]] == nullptr) {
                    std::cerr << "Error: Memory allocation failed" << std::endl;
                    // Clean up
                    for (int j = 0; j < tokenCounts[0]; j++) {
                        free(commands[0][j]);
                        commands[0][j] = nullptr;
                    }
                    tokenCounts[0] = 0;
                    break;
                }
                tokenCounts[0]++;
                token = strtok(nullptr, " ");
            }
            
            if (tokenCounts[0] > 0) {
                execute(1);
            }
        }
        
        // Free allocated memory
        for (int i = 0; i < cmdCount; i++) {
            for (int j = 0; j < tokenCounts[i]; j++) {
                if (commands[i][j] != nullptr) {
                    free(commands[i][j]);
                    commands[i][j] = nullptr;
                }
            }
            tokenCounts[i] = 0;
        }
    }
    
    // Final cleanup
    for (int i = 0; i < MAX_COMMANDS; i++) {
        for (int j = 0; j < tokenCounts[i]; j++) {
            if (commands[i][j] != nullptr) {
                free(commands[i][j]);
                commands[i][j] = nullptr;
            }
        }
        tokenCounts[i] = 0;
    }
    
    return 0;
}
