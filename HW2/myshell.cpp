#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/wait.h>
#include <vector>
#include <string>

#define NUM 20      // Maximum number of tokens per command
#define SZ 500      // Size of buffer for each command
#define MAX_COMMANDS 10

// Command storage implementation:
// Instead of using a 2D array of pointers, we use a contiguous buffer approach:
// - buf[MAX_COMMANDS][SZ]: A 2D char array where each row holds tokens for one command
//   Tokens are stored contiguously with null terminators between them
// - tokenCounts[MAX_COMMANDS]: Tracks number of tokens in each command
// This approach is more memory efficient than a 2D array of pointers because:
// 1. Avoids memory fragmentation
// 2. Simpler memory management
// 3. Better cache performance due to contiguous storage
char buf[MAX_COMMANDS][SZ];
int tokenCounts[MAX_COMMANDS];

void parseWithPipes(char* commandLine, int& cmdCount) {
    // Initialize token counts and buffers
    memset(tokenCounts, 0, sizeof(tokenCounts));
    memset(buf, 0, sizeof(buf));
    
    // Count the number of pipe characters to determine how many commands we have
    int pipeCount = 0;
    for (int i = 0; commandLine[i] != '\0'; i++) {
        if (commandLine[i] == '|') {
            pipeCount++;
        }
    }
    
    // The number of commands is the number of pipes plus 1
    cmdCount = pipeCount + 1;
    
    // Split the command line by pipe character
    char* saveptr;
    char* cmd = strtok_r(commandLine, "|", &saveptr);
    int currentCmd = 0;
    
    while (cmd != nullptr && currentCmd < cmdCount) {
        // Trim leading and trailing whitespace
        while (*cmd == ' ') cmd++;
        char* end = cmd + strlen(cmd) - 1;
        while (end > cmd && *end == ' ') {
            *end = '\0';
            end--;
        }
        
        // Parse this command into tokens
        char* token = strtok(cmd, " ");
        int pos = 0;  // Position in the buffer
        
        while (token != nullptr && tokenCounts[currentCmd] < NUM && pos < SZ - 1) {
            // Copy token to buffer
            int len = strlen(token);
            if (pos + len + 1 >= SZ) break;  // Ensure we don't overflow
            
            strcpy(&buf[currentCmd][pos], token);
            pos += len + 1;  // Move position past token and null terminator
            tokenCounts[currentCmd]++;
            token = strtok(nullptr, " ");
        }
        
        // Get the next command
        cmd = strtok_r(nullptr, "|", &saveptr);
        currentCmd++;
    }
}

void executePipes(int cmdCount) {
    if (cmdCount == 0) return;
    
    if (cmdCount == 1) {
        pid_t pid = fork();
        
        if (pid < 0) {
            perror("fork failed");
            return;
        } else if (pid == 0) {
            // Create array of pointers for execvp
            char* args[NUM + 1];
            int pos = 0;
            
            // Pack tokens into argument list
            for (int i = 0; i < tokenCounts[0]; i++) {
                args[i] = &buf[0][pos];
                pos += strlen(args[i]) + 1;  // Point to next token
            }
            args[tokenCounts[0]] = nullptr;  // Null terminate the argument list
            
            execvp(args[0], args);
            perror("execvp failed");
            exit(1);
        } else {
            int status;
            wait(&status);
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
        
        for (int i = 0; i < cmdCount; i++) {
            pid_t pid = fork();
            
            if (pid < 0) {
                perror("fork failed");
                return;
            } else if (pid == 0) {
                // Set up pipes for this command
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
                
                // Close all other pipe ends
                for (int j = 0; j < cmdCount - 1; j++) {
                    if (j != i-1 && j != i) {
                        close(pipes[j][0]);
                        close(pipes[j][1]);
                    }
                }
                
                // Create array of pointers for execvp
                char* args[NUM + 1];
                int pos = 0;
                
                // Pack tokens into argument list
                for (int j = 0; j < tokenCounts[i]; j++) {
                    args[j] = &buf[i][pos];
                    pos += strlen(args[j]) + 1;  // Point to next token
                }
                args[tokenCounts[i]] = nullptr;  // Null terminate the argument list
                
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
            wait(&status);
        }
        
        // Clean up pipes
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
            parseWithPipes(commandLine, cmdCount);
            if (cmdCount > 0) {
                executePipes(cmdCount);
            }
        } else {
            // Handle single command case
            char* token = strtok(commandLine, " ");
            int pos = 0;
            tokenCounts[0] = 0;
            
            while (token != nullptr && tokenCounts[0] < NUM && pos < SZ - 1) {
                strcpy(&buf[0][pos], token);
                pos += strlen(token) + 1;
                tokenCounts[0]++;
                token = strtok(nullptr, " ");
            }
            
            executePipes(1);
        }
    }
    
    return 0;
}
