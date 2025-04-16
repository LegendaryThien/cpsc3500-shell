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

int** fid = new int*[2];

for (int i = 0; i < cmdCount; i++) {
    fid[i] = new int[2];
    pipe(fid[i]);
}

for (int i = 0; i < cmdCount; i++) {
    pid_t pid = fork();
    if (pid == 0) {
        // Child process
        if (i == 0) {
            // First command
            dup2(fid[i][1], 1);
            close(fid[i][0]);
            close(fid[i][1]);
        }
        else if (i == cmdCount - 1) {
            dup2(fid[i][0], 0);
            close(fid[i][0]);
            close(fid[i][1]);
        }
        else {
            dup2(fid[i][0], 0);
            dup2(fid[i][1], 1);
            close(fid[i][0]);
            close(fid[i][1]);
        }
        execvp(cmds[i], cmds);
    }
    else {
        // Parent process
        close(fid[i][0]);   
        close(fid[i][1]);
    }
}

for (int i = 0; i < cmdCount; i++) {
    wait(NULL);
}