/*
Basic commands:
ls - List files and directories
pwd - Print working directory
cd - Change directory
echo - Print text
cat - Display file contents
grep - Search for patterns in files
wc - Count lines, words, and characters
ps - Display process information
date - Show current date and time
whoami - Show current user
exit - Exit the shell
*/

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
    char* args[MAX_TOKENS + 1];
    // Number of arguments in this command
    int argCount;
};

// Function to parse the command line into a command
void parseCommand(char* commandLine, Command& command) {
    // Trim leading/trailing whitespace from the command
    while (*commandLine == ' ') commandLine++;
    char* end = commandLine + strlen(commandLine) - 1;
    while (end > commandLine && *end == ' ') {
        *end = '\0';
        end--;
    }
    
    // Parse this command into arguments
    command.argCount = 0;
    // Split the command by spaces to get individual arguments
    char* token = strtok(commandLine, " ");
    
    // Store each argument in the command structure
    while (token != nullptr && command.argCount < MAX_TOKENS) {
        command.args[command.argCount++] = token;
        token = strtok(nullptr, " ");
    }
    // Set the last argument to NULL (required by execvp)
    command.args[command.argCount] = nullptr;
}

// Function to execute a command
void executeCommand(Command& command) {
    if (command.argCount == 0) return;
    
    // Create a new process to execute the command
    pid_t pid = fork();
    
    if (pid < 0) {
        // Fork failed
        std::cerr << "Fork failed" << std::endl;
        return;
    } else if (pid == 0) {
        // Child process
        // Replace the child process with the new program
        execvp(command.args[0], command.args);
        // If execvp returns, it means the command was not found
        std::cerr << "Command not found: " << command.args[0] << std::endl;
        exit(1);
    } else {
        // Parent process
        // Wait for the child process to complete
        int status;
        wait(&status);
        
        // Check if the child process exited normally
        if (WIFEXITED(status)) {
            std::cout << "process " << pid << " exits with exit status value " 
                      << WEXITSTATUS(status) << std::endl;
        }
    }
}

int main() {
    // Buffer to store the command line input
    char commandLine[1024];
    // Command structure to store parsed command
    Command command;
    
    // Main shell loop
    while (true) {
        // Display the shell prompt
        std::cout << "myshell$";
        std::cout.flush();
        
        // Read command from stdin
        if (fgets(commandLine, sizeof(commandLine), stdin) != nullptr) {
            // Remove trailing newline if present
            size_t len = strlen(commandLine);
            if (len > 0 && commandLine[len-1] == '\n') {
                commandLine[len-1] = '\0';
            }
            
            // Parse the command line
            parseCommand(commandLine, command);
            
            if (command.argCount > 0) {
                // Check for exit command
                if (strcmp(command.args[0], "exit") == 0) {
                    break;
                }
                
                // Execute the command
                executeCommand(command);
            }
        }
    }
    
    return 0;
}
