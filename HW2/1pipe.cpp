#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <cstring>
#include <cstdlib>

int main() {
    int fd[2]; // file descriptor for pipe

    pipe(fd); // creates a pipe

    pid_t pid1 = fork();


    if (pid1 == 0) {
        // Child 1: run `ls`, output goes to pipe

        close(1); // Close stdout

        dup(fd[1]); // duplicate pipe write end

        close(fd[0]); // close unused read end
        close(fd[1]); // close after duplicating


        /*
        when ls runs and writes its output to stdout (which it does by default), 
        that output actually goes to the pipe because stdout has been redirected to the pipe's write end.
        */

        char* args[] = { (char*)"ls", nullptr };
        execvp(args[0], args);
    }

    pid_t pid2 = fork();
    

    if (pid2 == 0) {
        // Child 2: run `wc -l`, input comes from pipe

        close(0); // Close stdin

        dup(fd[0]); // duplicate pipe read end to stdin
        
        close(fd[1]); // close unused write end
        close(fd[0]); // close after duplicating

        /*
        when wc runs and reads its input from stdin (which it does by default), 
        that input actually comes from the pipe because stdin has been redirected to the pipe's read end.
        */

        char* args[] = { (char*)"wc", (char*)"-l", nullptr };
        execvp(args[0], args);
    }

    // Parent process
    close(fd[0]);
    close(fd[1]);

    int status;
    waitpid(pid1, &status, 0);
    waitpid(pid2, &status, 0);

    return 0;
}