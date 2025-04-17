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
        
        // Parse this command into tokens
        char* token = strtok(currentCmd, " ");
        
        while (token != nullptr && tokenCounts[currentCmdIndex] < MAX_TOKENS) {
            // Allocate memory for token and copy it
            commands[currentCmdIndex][tokenCounts[currentCmdIndex]] = strdup(token);
            tokenCounts[currentCmdIndex]++;
            token = strtok(nullptr, " ");
        }
        
        if (tokenCounts[currentCmdIndex] >= MAX_TOKENS) {
            std::cerr << "Error: Too many tokens in command" << std::endl;
            // Clean up and return
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

            char* args[MAX_TOKENS + 1];
            

            for (int i = 0; i < tokenCounts[0]; i++) {
                args[i] = commands[0][i];
            }
            args[tokenCounts[0]] = nullptr;  
            
            execvp(args[0], args);
            perror("execvp failed");
            exit(1);
        } else {
            int status;
            wait(&status);
            if (WIFEXITED(status)) {
                std::cout << "Process " << pid << " exited with status " << WEXITSTATUS(status) << std::endl;
            }
        }
    } else {
        int** pipes = new int*[cmdCount - 1];
        for (int i = 0; i < cmdCount - 1; i++) {
            pipes[i] = new int[2];
            if (pipe(pipes[i]) == -1) {
                perror("pipe creation failed");
                return;
            }
        }
        
        pid_t pids[MAX_COMMANDS];  
        
        for (int i = 0; i < cmdCount; i++) {
            pid_t pid = fork();
            pids[i] = pid; 
            
            if (pid < 0) {
                perror("fork failed");
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
        
        if (strchr(commandLine, '|') != nullptr) {
            parse(commandLine, cmdCount);
            if (cmdCount > 0) {
                execute(cmdCount);
            }
        } else {
            // Handle single command case
            char* token = strtok(commandLine, " ");
            tokenCounts[0] = 0;
            
            while (token != nullptr && tokenCounts[0] < MAX_TOKENS) {
                commands[0][tokenCounts[0]] = strdup(token);
                tokenCounts[0]++;
                token = strtok(nullptr, " ");
            }
            
            execute(1);
        }
        
        // Free allocated memory
        for (int i = 0; i < cmdCount; i++) {
            for (int j = 0; j < tokenCounts[i]; j++) {
                free(commands[i][j]);
            }
        }
    }
    
    return 0;
}
