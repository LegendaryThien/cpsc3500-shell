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
const int MAX_TOKENS = 20;

// Structure to hold a command and its arguments
struct Command {
    // Array of pointers to command arguments (including the command itself)
    // The last element must be NULL for execvp to work properly
    char* args[MAX_TOKENS + 1];
    // Number of arguments in this command
    int argCount;
};

// Function to parse the command line into a command
void parse(char* commandLine, Command& command) {
    // Remove trailing newline if present
    size_t len = strlen(commandLine);
    if (len > 0 && commandLine[len-1] == '\n') {
        commandLine[len-1] = '\0';
    }
    
    // Parse this command into arguments
    command.argCount = 0;
    
    // Split the command by spaces to get individual arguments
    char* token = strtok(commandLine, " ");
    
    // Store each argument in the command structure
    while (command.argCount < MAX_TOKENS) {
        command.args[command.argCount++] = token;
        token = strtok(nullptr, " "); // Get the next token
    }
    
    // Set the last argument to NULL (required by execvp)
    command.args[command.argCount] = nullptr;
}

// Function to execute a command
void execute(Command& command) {
    if (command.argCount == 0) return;
    
    // Create a new process to execute the command
    pid_t pid = fork();
    
    if (pid < 0) {
        // Fork failed
        std::cerr << "Fork failed" << std::endl;
        return;
    } if (pid == 0) { // child process
        // Replace the child process with the new program
        execvp(command.args[0], command.args);
        // If execvp returns, it means the command was not found
        std::cerr << "Command not found: " << command.args[0] << std::endl;
        exit(1);
    } else { // parent process
    
        // Wait for the child process to complete
        int status;
        wait(&status);
        
        // Check if the child process exited normally
        if (WIFEXITED(status)) {
            std::cout << "Child process " << pid << " exits with exit status value " 
                      << WEXITSTATUS(status) << std::endl;
        }
    }
}

int main() {
    // Buffer to store the command line input
    char commandLine[1024];
    // Command structure to store parsed command
    Command command;
    
    // Print the shell's PID
    std::cout << "Shell (Parent) PID: " << getpid() << std::endl;
    
    // Main shell loop
    while (true) {
        // Display the shell prompt
        std::cout << "myshell$";
        std::cout.flush();
        
        // Read command from stdin
        fgets(commandLine, sizeof(commandLine), stdin); // removing causes segfault
        
        // Parse the command line
        parse(commandLine, command);

        // Execute the command if there are arguments
        if (command.argCount > 0) {
            execute(command);
        }
        
    }
    
    return 0;
}
