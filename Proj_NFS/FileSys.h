// CPSC 3500: File System
// Implements the file system commands that are available to the shell.

#ifndef FILESYS_H
#define FILESYS_H

#include <string>       // For std::string
#include <sys/types.h>  // For socket types (might not be strictly needed here, but doesn't hurt)
#include "BasicFileSys.h" // <--- CRITICAL FIX: Include the full definition here!
#include "Blocks.h"     // Also needed for block definitions

class FileSys {
private:
    BasicFileSys bfs;   // basic file system
    short curr_dir;     // current directory
    int fs_sock;        // file server socket

    // Private helper function to determine if a block is a directory
    bool is_directory(short block_num);

public:
    // Constructor
    FileSys(); // Added constructor for proper initialization

    // mounts the file system
    void mount(int sock);

    // unmounts the file system
    void unmount();

    // make a directory
    std::string mkdir(const char *name); // Return string for RPC status

    // list the contents of current directory
    std::string ls(); // Return string for RPC status

    // switch to a directory
    std::string cd(const char *name); // Return string for RPC status

    // switch to home directory
    std::string home(); // Return string for RPC status

    // remove a directory
    std::string rmdir(const char *name); // Return string for RPC status

    // create an empty data file
    std::string create(const char *name); // Return string for RPC status

    // append data to a data file
    std::string append(const char *name, const char *data); // Return string for RPC status

    // display the contents of a data file
    std::string cat(const char *name); // Return string for RPC status

    // display the first N bytes of the file
    std::string head(const char *name, unsigned int n); // Return string for RPC status

    // delete a data file
    std::string rm(const char *name); // Return string for RPC status

    // display stats about file or directory
    std::string stat(const char *name); // Return string for RPC status

    // Note: ls_rpc() should be on the Shell class, not FileSys.
    // The FileSys class has the *local* file system operations (like ls()),
    // while the Shell class has the *remote* procedure call (RPC) functions (like ls_rpc()).
    // I've removed `std::string ls_rpc();` and `std::string get_ls_listing();` from here.
    // The `ls()` method itself is already returning a string as needed.
};

#endif