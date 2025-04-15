/*
First pipe (fd1):
Child 1 writes to fd1[1] (output from cat)
Child 2 reads from fd1[0] (input to tr A-Z a-z)
Parent closes both ends

Second pipe (fd2):
Child 2 writes to fd2[1] (output from tr A-Z a-z)
Child 3 reads from fd2[0] (input to tr -C a-z '\n')
Parent closes both ends

Third pipe (fd3):
Child 3 writes to fd3[1] (output from tr -C a-z '\n')
Child 4 reads from fd3[0] (input to sed)
Parent closes both ends

Fourth pipe (fd4):
Child 4 writes to fd4[1] (output from sed)
Child 5 reads from fd4[0] (input to sort)
Parent closes both ends
*/

#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <cstring>
#include <cstdlib>

int main() {
    int fd1[2]; // first pipe
    int fd2[2]; // second pipe
    int fd3[2]; // third pipe
    int fd4[2]; // fourth pipe

    pipe(fd1); // creates first pipe
    pipe(fd2); // creates second pipe
    pipe(fd3); // creates third pipe
    pipe(fd4); // creates fourth pipe

    pid_t pid1 = fork();

    if (pid1 == 0) {
        // Child 1: run `cat hw1test2.cpp`, output goes to first pipe
        close(1); // Close stdout

        dup(fd1[1]); // duplicate first pipe write end

        close(fd1[0]); // close unused read end
        close(fd1[1]); // close after duplicating
        close(fd2[0]); // close unused second pipe
        close(fd2[1]); // close unused second pipe
        close(fd3[0]); // close unused third pipe
        close(fd3[1]); // close unused third pipe
        close(fd4[0]); // close unused fourth pipe
        close(fd4[1]); // close unused fourth pipe

        char* args[] = { (char*)"cat", (char*)"4pipe.cpp", nullptr };
        execvp(args[0], args);
    }

    pid_t pid2 = fork();

    if (pid2 == 0) {
        // Child 2: run `tr A-Z a-z`, input comes from first pipe, output goes to second pipe
        close(0); // Close stdin
        close(1); // Close stdout

        dup(fd1[0]); // duplicate first pipe read end to stdin
        dup(fd2[1]); // duplicate second pipe write end to stdout

        close(fd1[1]); // close unused write end
        close(fd1[0]); // close after duplicating
        close(fd2[0]); // close unused read end
        close(fd2[1]); // close after duplicating
        close(fd3[0]); // close unused third pipe
        close(fd3[1]); // close unused third pipe
        close(fd4[0]); // close unused fourth pipe
        close(fd4[1]); // close unused fourth pipe

        char* args[] = { (char*)"tr", (char*)"A-Z", (char*)"a-z", nullptr };
        execvp(args[0], args);
    }

    pid_t pid3 = fork();

    if (pid3 == 0) {
        // Child 3: run `tr -C a-z '\n'`, input comes from second pipe, output goes to third pipe
        close(0); // Close stdin
        close(1); // Close stdout

        dup(fd2[0]); // duplicate second pipe read end to stdin
        dup(fd3[1]); // duplicate third pipe write end to stdout
        
        close(fd1[0]); // close unused first pipe
        close(fd1[1]); // close unused first pipe
        close(fd2[1]); // close unused write end
        close(fd2[0]); // close after duplicating
        close(fd3[0]); // close unused read end
        close(fd3[1]); // close after duplicating
        close(fd4[0]); // close unused fourth pipe
        close(fd4[1]); // close unused fourth pipe

        char* args[] = { (char*)"tr", (char*)"-C", (char*)"a-z", (char*)"\n", nullptr };
        execvp(args[0], args);
    }

    pid_t pid4 = fork();

    if (pid4 == 0) {
        // Child 4: run `sed '/^$/d'`, input comes from third pipe, output goes to fourth pipe
        close(0); // Close stdin
        close(1); // Close stdout

        dup(fd3[0]); // duplicate third pipe read end to stdin
        dup(fd4[1]); // duplicate fourth pipe write end to stdout

        close(fd1[0]); // close unused first pipe
        close(fd1[1]); // close unused first pipe
        close(fd2[0]); // close unused second pipe
        close(fd2[1]); // close unused second pipe
        close(fd3[1]); // close unused write end
        close(fd3[0]); // close after duplicating
        close(fd4[0]); // close unused read end
        close(fd4[1]); // close after duplicating

        char* args[] = { (char*)"sed", (char*)"/^$/d", nullptr };
        execvp(args[0], args);
    }

    pid_t pid5 = fork();

    if (pid5 == 0) {
        // Child 5: run `sort`, input comes from fourth pipe
        close(0); // Close stdin

        dup(fd4[0]); // duplicate fourth pipe read end to stdin

        close(fd1[0]); // close unused first pipe
        close(fd1[1]); // close unused first pipe
        close(fd2[0]); // close unused second pipe
        close(fd2[1]); // close unused second pipe
        close(fd3[0]); // close unused third pipe
        close(fd3[1]); // close unused third pipe
        close(fd4[1]); // close unused write end
        close(fd4[0]); // close after duplicating

        char* args[] = { (char*)"sort", nullptr };
        execvp(args[0], args);
    }

    // Parent process
    close(fd1[0]);
    close(fd1[1]);
    close(fd2[0]);
    close(fd2[1]);
    close(fd3[0]);
    close(fd3[1]);
    close(fd4[0]);
    close(fd4[1]);

    int status;
    waitpid(pid1, &status, 0);
    waitpid(pid2, &status, 0);
    waitpid(pid3, &status, 0);
    waitpid(pid4, &status, 0);
    waitpid(pid5, &status, 0);

    return 0;
}