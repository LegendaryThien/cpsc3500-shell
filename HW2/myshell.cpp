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
const int MAX_COMMANDS = 10;

// Structure to hold a command and its arguments
struct Command {
    // Array of pointers to command arguments (including the command itself)
    // The last element must be NULL for execvp to work properly
    char* cmds[MAX_TOKENS + 1];
    // Number of arguments in this command
    int cmdCount;
};

// Function to parse the command line into a command
void parse(char* commandLine, Command& command) {
    // Remove trailing newline if present
    size_t len = strlen(commandLine);
    if (len > 0 && commandLine[len-1] == '\n') {
        commandLine[len-1] = '\0';
    }
    
    // Parse this command into arguments
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
}

// Function to parse a command line with pipes
void parseWithPipes(char* commandLine, Command commands[], int& cmdCount) {
    std::cout << "DEBUG: Parsing command line: " << commandLine << std::endl;
    
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
    std::cout << "DEBUG: Found " << pipeCount << " pipes, so " << cmdCount << " commands" << std::endl;
    
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
        
        std::cout << "DEBUG: Processing command " << currentCmd << ": " << cmd << std::endl;
        
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
            std::cout << "DEBUG: Added argument: " << token << std::endl;
            commands[currentCmd].cmdCount++;
            token = strtok_r(nullptr, " ", &saveptr2);
        }
        
        // Set the last argument to NULL (required by execvp)
        commands[currentCmd].cmds[commands[currentCmd].cmdCount] = nullptr;
        
        // Get the next command
        cmd = strtok_r(nullptr, "|", &saveptr);
        currentCmd++;
    }
    
    std::cout << "DEBUG: Total commands parsed: " << cmdCount << std::endl;
    
    // Debug output to verify all commands were parsed correctly
    for (int i = 0; i < cmdCount; i++) {
        std::cout << "DEBUG: Command " << i << " has " << commands[i].cmdCount << " arguments: ";
        for (int j = 0; j < commands[i].cmdCount; j++) {
            std::cout << commands[i].cmds[j] << " ";
        }
        std::cout << std::endl;
    }
}

// Function to execute a command
void execute(Command& command) {
    if (command.cmdCount == 0) return;
    
    // Create a new process to execute the command
    pid_t pid = fork();
    
    if (pid < 0) {
        // Fork failed
        std::cerr << "Fork failed" << std::endl;
        return;
    } if (pid == 0) { // child process
        // Replace the child process with the new program
        execvp(command.cmds[0], command.cmds);
        // If execvp returns, it means the command was not found
        std::cerr << "Command not found: " << command.cmds[0] << std::endl;
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

// Function to execute piped commands
void executePipes(Command commands[], int cmdCount) {
    if (cmdCount == 0) return;
    
    std::cout << "DEBUG: Executing " << cmdCount << " piped commands" << std::endl;
    
    // Debug output to verify commands before execution
    for (int i = 0; i < cmdCount; i++) {
        std::cout << "DEBUG: About to execute command " << i << ": ";
        for (int j = 0; j < commands[i].cmdCount; j++) {
            std::cout << commands[i].cmds[j] << " ";
        }
        std::cout << std::endl;
    }
    
    // If there's only one command, execute it normally
    if (cmdCount == 1) {
        std::cout << "DEBUG: Single command execution" << std::endl;
        execute(commands[0]);
        return;
    }
    
    // Create pipes for communication between processes
    int** pipes = new int*[cmdCount - 1];
    for (int i = 0; i < cmdCount - 1; i++) {
        pipes[i] = new int[2];
        if (pipe(pipes[i]) == -1) {
            std::cerr << "Pipe creation failed" << std::endl;
            return;
        }
        std::cout << "DEBUG: Created pipe " << i << std::endl;
    }
    
    // Create child processes for each command
    for (int i = 0; i < cmdCount; i++) {
        std::cout << "DEBUG: Creating process for command " << i << ": " << commands[i].cmds[0] << std::endl;
        
        pid_t pid = fork();
        
        if (pid < 0) {
            std::cerr << "Fork failed" << std::endl;
            return;
        } else if (pid == 0) { // Child process
            std::cout << "DEBUG: Child process " << i << " (PID: " << getpid() << ") starting" << std::endl;
            
            // Set up pipes for this child
            if (i > 0) { // Not the first command
                // Read from the previous pipe
                dup2(pipes[i-1][0], STDIN_FILENO);
                close(pipes[i-1][1]);
                std::cout << "DEBUG: Child " << i << " reading from pipe " << (i-1) << std::endl;
            }
            
            if (i < cmdCount - 1) { // Not the last command
                // Write to the next pipe
                dup2(pipes[i][1], STDOUT_FILENO);
                close(pipes[i][0]);
                std::cout << "DEBUG: Child " << i << " writing to pipe " << i << std::endl;
            }
            
            // Close all other pipes
            for (int j = 0; j < cmdCount - 1; j++) {
                if (j != i) close(pipes[j][0]);
                if (j != i - 1) close(pipes[j][1]);
            }
            
            // Execute the command
            std::cout << "DEBUG: Child " << i << " executing: " << commands[i].cmds[0] << std::endl;
            execvp(commands[i].cmds[0], commands[i].cmds);
            std::cerr << "Command not found: " << commands[i].cmds[0] << std::endl;
            exit(1);
        } else { // Parent process
            std::cout << "DEBUG: Parent created child " << i << " with PID: " << pid << std::endl;
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
        std::cout << "DEBUG: Child process " << i << " completed with status: " << status << std::endl;
    }
    
    // Clean up the pipes
    for (int i = 0; i < cmdCount - 1; i++) {
        delete[] pipes[i];
    }
    delete[] pipes;
}

int main() {
    // Buffer to store the command line input
    char commandLine[1024];
    // Command structure to store parsed command
    Command command;
    // Array of commands for piped commands
    Command commands[MAX_COMMANDS];
    int cmdCount;
    
    // Print the shell's PID
    std::cout << "Shell (Parent) PID: " << getpid() << std::endl;
    
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
            std::cout << "DEBUG: Detected pipe in command: " << commandLine << std::endl;
            // Parse and execute piped commands
            parseWithPipes(commandLine, commands, cmdCount);
            if (cmdCount > 0) {
                executePipes(commands, cmdCount);
            }
        } else {
            // Parse and execute a single command
            parse(commandLine, command);
            if (command.cmdCount > 0) {
                execute(command);
            }
        }
    }
    
    return 0;
}
