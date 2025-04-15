#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <cstring>
#include <cstdlib>

int main() {
    int pipefd[2]; // pipefd[0] for reading, pipefd[1] for writing

    pipe(pipefd);

    pid_t pid1 = fork();


    if (pid1 == 0) {
        // Child 1: run `ls`, output goes to pipe
        dup2(pipefd[1], STDOUT_FILENO); // replace stdout with pipe write end
        close(pipefd[0]); // close unused read end
        close(pipefd[1]); // close after duplicating

        char* args[] = { (char*)"ls", nullptr };
        execvp(args[0], args);

        // Only reaches here if exec fails
        perror("execvp ls");
        exit(EXIT_FAILURE);
    }

    pid_t pid2 = fork();
    

    if (pid2 == 0) {
        // Child 2: run `wc -l`, input comes from pipe
        dup2(pipefd[0], STDIN_FILENO); // replace stdin with pipe read end
        close(pipefd[1]); // close unused write end
        close(pipefd[0]); // close after duplicating

        char* args[] = { (char*)"wc", (char*)"-l", nullptr };
        execvp(args[0], args);

        perror("execvp wc -l");
        exit(EXIT_FAILURE);
    }

    // Parent process
    close(pipefd[0]);
    close(pipefd[1]);

    int status;
    waitpid(pid1, &status, 0);
    waitpid(pid2, &status, 0);

    return 0;
}