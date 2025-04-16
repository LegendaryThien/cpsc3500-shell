#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/wait.h>
#include <vector>
#include <string>

// Maximum number of tokens (arguments) a command can have
const int MAX_TOKENS = 20;
const int MAX_COMMANDS = 10;

// Structure to hold a command and its arguments
struct Command {
    // Array of pointers to command arguments (including the command itself)
    // The last element must be NULL for execvp to work properly
    char* cmds[MAX_TOKENS + 1];
    // Number of arguments in this command
    int cmdCount;
};

// Function to parse a command line with pipes
void parseWithPipes(char* commandLine, Command commands[], int& cmdCount) {
    // Make a copy of the command line to avoid modifying the original
    char cmdLineCopy[1024];
    strcpy(cmdLineCopy, commandLine);
    
    // Count the number of pipe characters to determine how many commands we have
    int pipeCount = 0;
    for (int i = 0; cmdLineCopy[i] != '\0'; i++) {
        if (cmdLineCopy[i] == '|') {
            pipeCount++;
        }
    }
    
    // The number of commands is the number of pipes plus 1
    cmdCount = pipeCount + 1;
    
    // Split the command line by pipe character
    char* saveptr;
    char* cmd = strtok_r(cmdLineCopy, "|", &saveptr);
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
            // Allocate memory for the token and copy it
            commands[currentCmd].cmds[commands[currentCmd].cmdCount] = strdup(token);
            commands[currentCmd].cmdCount++;
            token = strtok_r(nullptr, " ", &saveptr2);
        }
        
        // Set the last argument to NULL (required by execvp)
        commands[currentCmd].cmds[commands[currentCmd].cmdCount] = nullptr;
        
        // Get the next command
        cmd = strtok_r(nullptr, "|", &saveptr);
        currentCmd++;
    }
}

// Function to execute piped commands
void executePipes(Command commands[], int cmdCount) {
    if (cmdCount == 0) return;
    
    // If there's only one command, execute it normally
    if (cmdCount == 1) {
        // Create a new process to execute the command
        pid_t pid = fork();
        
        if (pid < 0) {
            return;
        } else if (pid == 0) { // child process
            // Replace the child process with the new program
            execvp(commands[0].cmds[0], commands[0].cmds);
            exit(1);
        } else { // parent process
            // Wait for the child process to complete
            int status;
            wait(&status);
        }
    } else {
        // Create pipes for communication between processes
        int** pipes = new int*[cmdCount - 1];
        for (int i = 0; i < cmdCount - 1; i++) {
            pipes[i] = new int[2];
            if (pipe(pipes[i]) == -1) {
                return;
            }
        }
        
        // Create child processes for each command
        for (int i = 0; i < cmdCount; i++) {
            pid_t pid = fork();
            
            if (pid < 0) {
                return;
            } else if (pid == 0) { // Child process
                // Set up pipes for this child
                if (i > 0) { // Not the first command
                    // Read from the previous pipe
                    close(0);
                    dup(pipes[i-1][0]);
                    close(pipes[i-1][1]);
                }
                
                if (i < cmdCount - 1) { // Not the last command
                    // Write to the next pipe
                    close(1);
                    dup(pipes[i][1]);
                    close(pipes[i][0]);
                }
                
                // Execute the command
                execvp(commands[i].cmds[0], commands[i].cmds);
                exit(1);
            } else { // Parent process
                // Close the pipes that the parent doesn't need
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
        
        // Clean up the pipes
        for (int i = 0; i < cmdCount - 1; i++) {
            delete[] pipes[i];
        }
        delete[] pipes;
    }
}

int main() {
    // Buffer to store the command line input
    char commandLine[1024];
    // Array of commands for piped commands
    Command commands[MAX_COMMANDS];
    int cmdCount;
    
    // Main shell loop
    while (true) {
        // Display the shell prompt
        std::cout << "myshell$";
        std::cout.flush();
        
        // Read command from stdin
        if (fgets(commandLine, sizeof(commandLine), stdin) == nullptr) {
            // Handle EOF (Ctrl+D)
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
        
        // Check if the command contains a pipe
        if (strchr(commandLine, '|') != nullptr) {
            // Parse and execute piped commands
            parseWithPipes(commandLine, commands, cmdCount);
            if (cmdCount > 0) {
                executePipes(commands, cmdCount);
            }
        } else {
            // Parse and execute a single command
            Command command;
            command.cmdCount = 0;
            
            // Split the command by spaces to get individual arguments
            char* token = strtok(commandLine, " ");
            
            // Store each argument in the command structure
            while (token != nullptr && command.cmdCount < MAX_TOKENS) {
                command.cmds[command.cmdCount++] = token;
                token = strtok(nullptr, " "); // Get the next token
            }
            
            // Set the last argument to NULL (required by execvp)
            command.cmds[command.cmdCount] = nullptr;
            
            if (command.cmdCount > 0) {
                // Create a new process to execute the command
                pid_t pid = fork();
                
                if (pid < 0) {
                    continue;
                } else if (pid == 0) { // child process
                    // Replace the child process with the new program
                    execvp(command.cmds[0], command.cmds);
                    exit(1);
                } else { // parent process
                    // Wait for the child process to complete
                    int status;
                    wait(&status);
                }
            }
        }
    }
    
    return 0;
}
