#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/wait.h>
#include <vector>
#include <string>

// Maximum number of tokens (arguments) a command can have
#define MAX_TOKENS 20

// Structure to hold a command and its arguments
struct Command {
    // Array of pointers to command arguments (including the command itself)
    // The last element must be NULL for execvp to work properly
    char* cmds[MAX_TOKENS + 1];
    // Number of arguments in this command
    int cmdCount;
    
    // Constructor to initialize the structure
    Command() : cmdCount(0) {
        // Initialize all pointers to NULL
        for (int i = 0; i < MAX_TOKENS + 1; i++) {
            cmds[i] = nullptr;
        }
    }
    
    // Destructor to free allocated memory
    ~Command() {
        for (int i = 0; i < cmdCount; i++) {
            if (cmds[i] != nullptr) {
                free(cmds[i]);
                cmds[i] = nullptr;
            }
        }
    }
};

// Function to parse the command line into multiple commands separated by pipes
void parse(char* commandLine, std::vector<Command>& commands) {
    // Remove trailing newline if present
    size_t len = strlen(commandLine);
    if (len > 0 && commandLine[len-1] == '\n') {
        commandLine[len-1] = '\0';
    }
    
    std::cout << "Parsing command line: '" << commandLine << "'" << std::endl;
    
    // Create a copy of the command line to avoid modifying the original
    char* cmdLineCopy = strdup(commandLine);
    if (cmdLineCopy == nullptr) {
        std::cerr << "Memory allocation failed" << std::endl;
        return;
    }
    
    // Split the command line by pipe character
    char* cmdPart = strtok(cmdLineCopy, "|");
    
    while (cmdPart != nullptr) {
        Command command;
        
        std::cout << "Processing command part: '" << cmdPart << "'" << std::endl;
        
        // Create a copy of this command part to avoid modifying the original
        char* cmdPartCopy = strdup(cmdPart);
        if (cmdPartCopy == nullptr) {
            std::cerr << "Memory allocation failed" << std::endl;
            free(cmdLineCopy);
            return;
        }
        
        // Split this command part by spaces to get individual arguments
        char* token = strtok(cmdPartCopy, " ");
        
        // Skip leading spaces
        while (token != nullptr && *token == '\0') {
            token = strtok(nullptr, " ");
        }
        
        // Store each argument in the command structure
        while (token != nullptr && command.cmdCount < MAX_TOKENS) {
            // Allocate memory for the token and copy it
            command.cmds[command.cmdCount] = strdup(token);
            if (command.cmds[command.cmdCount] == nullptr) {
                std::cerr << "Memory allocation failed" << std::endl;
                // Free already allocated memory
                for (int i = 0; i < command.cmdCount; i++) {
                    free(command.cmds[i]);
                }
                free(cmdPartCopy);
                free(cmdLineCopy);
                return;
            }
            
            std::cout << "  Added token: '" << token << "'" << std::endl;
            command.cmdCount++;
            token = strtok(nullptr, " ");
        }
        
        // Set the last argument to NULL (required by execvp)
        command.cmds[command.cmdCount] = nullptr;
        
        // Add this command to the vector if it has arguments
        if (command.cmdCount > 0) {
            // Create a deep copy of the command to store in the vector
            Command cmdCopy;
            cmdCopy.cmdCount = command.cmdCount;
            
            for (int i = 0; i < command.cmdCount; i++) {
                cmdCopy.cmds[i] = strdup(command.cmds[i]);
                if (cmdCopy.cmds[i] == nullptr) {
                    std::cerr << "Memory allocation failed" << std::endl;
                    // Free already allocated memory
                    for (int j = 0; j < i; j++) {
                        free(cmdCopy.cmds[j]);
                    }
                    free(cmdPartCopy);
                    free(cmdLineCopy);
                    return;
                }
            }
            cmdCopy.cmds[cmdCopy.cmdCount] = nullptr;
            
            commands.push_back(cmdCopy);
            std::cout << "Added command with " << cmdCopy.cmdCount << " arguments" << std::endl;
        }
        
        // Free the command part copy
        free(cmdPartCopy);
        
        // Get the next command part
        cmdPart = strtok(nullptr, "|");
    }
    
    // Free the command line copy
    free(cmdLineCopy);
    
    std::cout << "Total commands parsed: " << commands.size() << std::endl;
}

// Function to execute multiple commands with pipes
void execute(std::vector<Command>& commands) {
    if (commands.empty()) return;
    
    std::cout << "Executing " << commands.size() << " commands" << std::endl;
    
    // If there's only one command, execute it directly
    if (commands.size() == 1) {
        std::cout << "Single command execution: " << commands[0].cmds[0] << std::endl;
        pid_t pid = fork();
        
        if (pid < 0) {
            std::cerr << "Fork failed" << std::endl;
            return;
        } else if (pid == 0) { // child process
            execvp(commands[0].cmds[0], commands[0].cmds);
            exit(1);
        } else { // parent process
            int status;
            wait(&status);
        }
        return;
    }
    
    // For multiple commands with pipes
    int cmdCount = commands.size();
    std::cout << "Setting up " << (cmdCount - 1) << " pipes for " << cmdCount << " commands" << std::endl;
    
    int** fd = new int*[cmdCount - 1];
    
    // Create pipes
    for (int i = 0; i < cmdCount - 1; i++) {
        fd[i] = new int[2];
        pipe(fd[i]);
        std::cout << "Created pipe " << i << ": [" << fd[i][0] << ", " << fd[i][1] << "]" << std::endl;
    }
    
    // Execute each command
    for (int i = 0; i < cmdCount; i++) {
        std::cout << "Forking for command " << i << ": " << commands[i].cmds[0] << std::endl;
        pid_t pid = fork();
        
        if (pid < 0) {
            std::cerr << "Fork failed" << std::endl;
            return;
        } 
        
        else if (pid == 0) { // child process
            std::cout << "Child process " << i << " (PID: " << getpid() << ") executing: " << commands[i].cmds[0] << std::endl;
            
            // Set up pipes for this command
            if (i > 0) {
                // Not the first command, read from previous pipe
                std::cout << "  Child " << i << " reading from pipe " << (i-1) << std::endl;
                close(0);
                dup(fd[i-1][0]);
                close(fd[i-1][0]);
                close(fd[i-1][1]);
            }
            
            if (i < cmdCount - 1) {
                // Not the last command, write to next pipe
                std::cout << "  Child " << i << " writing to pipe " << i << std::endl;
                close(1);
                dup(fd[i][1]);
                close(fd[i][0]);
                close(fd[i][1]);
            }
            
            // Close all other pipes
            for (int j = 0; j < cmdCount - 1; j++) {
                if (j != i && j != i-1) {
                    close(fd[j][0]);
                    close(fd[j][1]);
                }
            }
            
            // Execute the command
            std::cout << "  Child " << i << " executing: " << commands[i].cmds[0] << std::endl;
            execvp(commands[i].cmds[0], commands[i].cmds);
            std::cerr << "  Child " << i << " execvp failed for: " << commands[i].cmds[0] << std::endl;
            exit(1);
        } else {
            std::cout << "Parent created child " << i << " with PID: " << pid << std::endl;
        }
    }
    
    // Parent process: close all pipe file descriptors
    std::cout << "Parent closing all pipe file descriptors" << std::endl;
    for (int i = 0; i < cmdCount - 1; i++) {
        close(fd[i][0]);
        close(fd[i][1]);
    }
    
    // Wait for all child processes to complete
    std::cout << "Parent waiting for all children to complete" << std::endl;
    for (int i = 0; i < cmdCount; i++) {
        int status;
        wait(&status);
        std::cout << "Child " << i << " completed with status: " << status << std::endl;
    }
    
    // Clean up pipe arrays
    for (int i = 0; i < cmdCount - 1; i++) {
        delete[] fd[i];
    }
    delete[] fd;
}

int main() {
    // Buffer to store the command line input
    char commandLine[1024];
    // Vector to store multiple commands
    std::vector<Command> commands;
    
    // Main shell loop
    while (true) {
        // Display the shell prompt
        std::cout << "myshell$ ";
        
        // Read command from stdin
        fgets(commandLine, sizeof(commandLine), stdin);
        
        // Clear the commands vector for the new input
        commands.clear();
        
        // Parse the command line
        parse(commandLine, commands);
        
        // Execute the commands if there are any
        if (!commands.empty()) {
            execute(commands);
        }
    }
    
    return 0;
}
