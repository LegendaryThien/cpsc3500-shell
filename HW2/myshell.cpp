#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/wait.h>
#include <vector>
#include <string>

#define MAX_TOKENS 20
#define MAX_COMMANDS 10
#define MAX_TOKEN_LENGTH 20

struct Command {
    char* cmds[MAX_TOKENS + 1];
    int cmdCount;
};

void parseWithPipes(char* commandLine, Command commands[], int& cmdCount) {
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
        
        // Parse this command into arguments
        commands[currentCmd].cmdCount = 0;
        
        // Make a copy of the command to avoid modifying the original
        char cmdCopy[1024];
        strcpy(cmdCopy, cmd);
        
        char* saveptr2;
        char* token = strtok_r(cmdCopy, " ", &saveptr2);
        
        while (token != nullptr && commands[currentCmd].cmdCount < MAX_TOKENS) {
            // Check token length
            if (strlen(token) > MAX_TOKEN_LENGTH) {
                std::cerr << "Error: Token length exceeds " << MAX_TOKEN_LENGTH << " characters: " << token << std::endl;
                return;
            }
            
            // Allocate memory for the token and copy it
            commands[currentCmd].cmds[commands[currentCmd].cmdCount] = strdup(token);
            commands[currentCmd].cmdCount++;
            token = strtok_r(nullptr, " ", &saveptr2);
        }
        
        if (commands[currentCmd].cmdCount >= MAX_TOKENS && token != nullptr) {
            std::cerr << "Error: Too many tokens in command (max " << MAX_TOKENS << ")" << std::endl;
            return;
        }
        
        // Set the last argument to NULL (required by execvp)
        commands[currentCmd].cmds[commands[currentCmd].cmdCount] = nullptr;
        
        // Get the next command
        cmd = strtok_r(nullptr, "|", &saveptr);
        currentCmd++;
    }
}

void executePipes(Command commands[], int cmdCount) {
    if (cmdCount == 0) return;
    
    if (cmdCount == 1) {
        pid_t pid = fork();
        
        if (pid < 0) {
            return;
        } else if (pid == 0) {
            execvp(commands[0].cmds[0], commands[0].cmds);
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
                return;
            }
        }
        
        for (int i = 0; i < cmdCount; i++) {
            pid_t pid = fork();
            
            if (pid < 0) {
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
                
                execvp(commands[i].cmds[0], commands[i].cmds);
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
        
        for (int i = 0; i < cmdCount; i++) {
            int status;
            wait(&status);
        }
        
        for (int i = 0; i < cmdCount - 1; i++) {
            delete[] pipes[i];
        }
        delete[] pipes;
    }
}

void parseSingle(char* commandLine, Command& command) {
    command.cmdCount = 0;
    
    // Skip leading whitespace
    while (*commandLine == ' ') commandLine++;
    
    // Split the command by spaces to get individual arguments
    char* token = strtok(commandLine, " ");
    
    // Store each argument in the command structure
    while (token != nullptr && command.cmdCount < MAX_TOKENS) {
        // Check token length
        if (strlen(token) > MAX_TOKEN_LENGTH) {
            std::cerr << "Error: Token length exceeds " << MAX_TOKEN_LENGTH << " characters: " << token << std::endl;
            return;
        }
        
        command.cmds[command.cmdCount++] = token;
        token = strtok(nullptr, " "); // Get the next token
    }
    
    if (command.cmdCount >= MAX_TOKENS && token != nullptr) {
        std::cerr << "Error: Too many tokens in command (max " << MAX_TOKENS << ")" << std::endl;
        return;
    }
    
    // Set the last argument to NULL (required by execvp)
    command.cmds[command.cmdCount] = nullptr;
}

void executeSingle(Command& command) {
    if (command.cmdCount > 0) {
        pid_t pid = fork();
        
        if (pid < 0) {
            return;
        } else if (pid == 0) {
            execvp(command.cmds[0], command.cmds);
            exit(1);
        } else {
            int status;
            wait(&status);
        }
    }
}

int main() {
    char commandLine[1024];
    Command commands[MAX_COMMANDS];
    int cmdCount;
    
    while (true) {
        std::cout << "myshell$";
        
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
            parseWithPipes(commandLine, commands, cmdCount);
            if (cmdCount > 0) {
                executePipes(commands, cmdCount);
            }
        } else {
            Command command;
            parseSingle(commandLine, command);
            executeSingle(command);
        }
    }
    
    return 0;
}
