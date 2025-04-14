#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <unistd.h>
#include <sys/wait.h>

using namespace std;

// Function to parse the command line into arguments
vector<string> parseCommand(const string& commandLine) {
    vector<string> args;
    string arg;
    bool inQuotes = false;
    
    for (size_t i = 0; i < commandLine.length(); i++) {
        char c = commandLine[i];
        
        if (c == '"') {
            inQuotes = !inQuotes;
        } else if (c == ' ' && !inQuotes) {
            if (!arg.empty()) {
                args.push_back(arg);
                arg.clear();
            }
        } else {
            arg += c;
        }
    }
    
    if (!arg.empty()) {
        args.push_back(arg);
    }
    
    return args;
}

// Function to execute the command
void executeCommand(const vector<string>& args) {
    if (args.empty()) {
        return;
    }
    
    // Handle built-in commands
    if (args[0] == "echo") {
        // Echo command - print all arguments except the command itself
        for (size_t i = 1; i < args.size(); i++) {
            cout << args[i];
            if (i < args.size() - 1) {
                cout << " ";
            }
        }
        cout << endl;
        return;
    }
    
    // For external commands, we would need to fork and exec
    // This is a simplified version that only handles echo
    cout << "Command not implemented: " << args[0] << endl;
}

int main() {
    string commandLine;
    
    cout << "myshell$";
    
    while (getline(cin, commandLine)) {
        if (commandLine.empty()) {
            cout << "myshell$";
            continue;
        }
        
        vector<string> args = parseCommand(commandLine);
        
        if (!args.empty()) {
            if (args[0] == "exit") {
                break;
            }
            
            executeCommand(args);
        }
        
        cout << "myshell$";
    }
    
    return 0;
}
