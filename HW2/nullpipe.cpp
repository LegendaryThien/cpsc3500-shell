#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/wait.h>
#include <cstdlib>

#define MAX_TOKENS 20

int main() {
    // Example command: ls | wc -l
    char* cmds1[MAX_TOKENS + 1] = {(char*)"ls", nullptr};
    char* cmds2[MAX_TOKENS + 1] = {(char*)"wc", (char*)"-l", nullptr};
    int cmdCount = 2;
    
    // Create pipe
    int fd[2];
    if (pipe(fd) == -1) {
        std::cerr << "Pipe creation failed" << std::endl;
        return 1;
    }
    
    // Fork first child process
    pid_t pid1 = fork();
    if (pid1 < 0) {
        std::cerr << "Fork failed" << std::endl;
        return 1;
    } else if (pid1 == 0) {
        // Child 1: run first command, output goes to pipe
        close(1);  // Close stdout
        dup(fd[1]);  // Duplicate pipe write end to stdout
        close(fd[0]);  // Close unused read end
        close(fd[1]);  // Close after duplicating
        
        // Execute the command
        execvp(cmds1[0], cmds1);
        std::cerr << "Command execution failed: " << cmds1[0] << std::endl;
        exit(1);
    }
    
    // Fork second child process
    pid_t pid2 = fork();
    if (pid2 < 0) {
        std::cerr << "Fork failed" << std::endl;
        return 1;
    } else if (pid2 == 0) {
        // Child 2: run second command, input comes from pipe
        close(0);  // Close stdin
        dup(fd[0]);  // Duplicate pipe read end to stdin
        close(fd[1]);  // Close unused write end
        close(fd[0]);  // Close after duplicating
        
        // Execute the command
        execvp(cmds2[0], cmds2);
        std::cerr << "Command execution failed: " << cmds2[0] << std::endl;
        exit(1);
    }
    
    // Parent process
    close(fd[0]);  // Close both ends of the pipe
    close(fd[1]);
    
    // Wait for both child processes to complete
    int status;
    waitpid(pid1, &status, 0);
    waitpid(pid2, &status, 0);
    
    return 0;
}


