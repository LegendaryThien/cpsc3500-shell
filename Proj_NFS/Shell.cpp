// CPSC 3500: Shell
// Implements a basic shell (command line interface) for the file system

#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm> // For std::find, if needed (not strictly used for parsing here, but good for string ops)
#include <cstring>   // For memset, bcopy
#include <cstdlib>   // For stoi (string to int)
#include <stdexcept> // For std::invalid_argument, std::out_of_range

// Include necessary networking headers explicitly
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>  // For close()

using namespace std;

#include "Shell.h"

static const string PROMPT_STRING = "NFS> ";	// shell prompt

// Mount the network file system with server name and port number in the format of server:port
void Shell::mountNFS(string fs_loc) {
    // fs_loc is in the format "server:port" [cite: 34]

    // 1. Parse server name and port from fs_loc
    size_t colon_pos = fs_loc.find(':');
    if (colon_pos == string::npos) {
        cerr << "Error: Invalid server:port format. Expected server:port\n";
        return;
    }
    string server_name_str = fs_loc.substr(0, colon_pos);
    string port_str = fs_loc.substr(colon_pos + 1);

    int port;
    try {
        port = stoi(port_str); // Convert port string to integer
    } catch (const std::invalid_argument& ia) {
        cerr << "Error: Invalid port number: " << port_str << "\n";
        return;
    } catch (const std::out_of_range& oor) {
        cerr << "Error: Port number out of range: " << port_str << "\n";
        return;
    }


    // 2. Create TCP socket
    // AF_INET for IPv4, SOCK_STREAM for TCP, 0 for default protocol
    cs_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (cs_sock < 0) {
        cerr << "Error creating client socket\n";
        return;
    }

    // 3. Get server host information (resolve hostname)
    struct hostent *server = gethostbyname(server_name_str.c_str());
    if (server == NULL) {
        cerr << "Error: No such host: " << server_name_str << "\n";
        close(cs_sock); // Close socket if host resolution fails
        cs_sock = -1;   // Invalidate socket descriptor
        return;
    }

    // 4. Prepare server address structure
    struct sockaddr_in server_address;
    memset(&server_address, 0, sizeof(server_address)); // Zero out the structure
    server_address.sin_family = AF_INET; // IPv4
    // bcopy is part of <strings.h> on some systems, but memcpy from <cstring> is more portable.
    // However, the assignment's context often implies BSD sockets, so bcopy might be fine on the target system.
    // For maximum portability, one might use: memcpy(&server_address.sin_addr, server->h_addr, server->h_length);
    bcopy((char *)server->h_addr,
          (char *)&server_address.sin_addr.s_addr,
          server->h_length); // Copy IP address
    server_address.sin_port = htons(port); // Set port (network byte order)

    // 5. Connect to the server
    if (connect(cs_sock, (struct sockaddr *) &server_address, sizeof(server_address)) < 0) {
        cerr << "Error connecting to server\n";
        close(cs_sock); // Close socket if connection fails
        cs_sock = -1;   // Invalidate socket descriptor
        return;
    }

    // If all operations are completed successfully, set is_mounted to true [cite: 34]
    is_mounted = true;
    cout << "NFS mounted successfully to " << fs_loc << endl;
}

// Unmount the network file system if it was mounted
void Shell::unmountNFS() {
    // close the socket if it was mounted [cite: 34]
    if (is_mounted) {
        close(cs_sock); // Close the client socket
        cs_sock = -1;   // Invalidate the socket descriptor
        is_mounted = false; // Set mounted flag to false
        cout << "NFS unmounted successfully." << endl;
    } else {
        cout << "NFS is not mounted." << endl;
    }
}

// Remote procedure call on mkdir
void Shell::mkdir_rpc(string dname) {
  if (!is_mounted) { cout << "Error: NFS not mounted." << endl; return; }
  cout << "mkdir_rpc: Not implemented yet." << endl;
}

// Remote procedure call on cd
void Shell::cd_rpc(string dname) {
  if (!is_mounted) { cout << "Error: NFS not mounted." << endl; return; }
  cout << "cd_rpc: Not implemented yet." << endl;
}

// Remote procedure call on home
void Shell::home_rpc() {
  if (!is_mounted) { cout << "Error: NFS not mounted." << endl; return; }
  cout << "home_rpc: Not implemented yet." << endl;
}

// Remote procedure call on rmdir
void Shell::rmdir_rpc(string dname) {
  if (!is_mounted) { cout << "Error: NFS not mounted." << endl; return; }
  cout << "rmdir_rpc: Not implemented yet." << endl;
}

// Remote procedure call on ls
void Shell::ls_rpc() {
  if (!is_mounted) { cout << "Error: NFS not mounted." << endl; return; }
  cout << "ls_rpc: Not implemented yet." << endl;
}

// Remote procedure call on create
void Shell::create_rpc(string fname) {
  if (!is_mounted) { cout << "Error: NFS not mounted." << endl; return; }
  cout << "create_rpc: Not implemented yet." << endl;
}

// Remote procedure call on append
void Shell::append_rpc(string fname, string data) {
  if (!is_mounted) { cout << "Error: NFS not mounted." << endl; return; }
  cout << "append_rpc: Not implemented yet." << endl;
}

// Remote procesure call on cat
void Shell::cat_rpc(string fname) {
  if (!is_mounted) { cout << "Error: NFS not mounted." << endl; return; }
  cout << "cat_rpc: Not implemented yet." << endl;
}

// Remote procedure call on head
void Shell::head_rpc(string fname, int n) { // Note: 'n' is int here, but unsigned int in requirements
  if (!is_mounted) { cout << "Error: NFS not mounted." << endl; return; }
  cout << "head_rpc: Not implemented yet." << endl;
}

// Remote procedure call on rm
void Shell::rm_rpc(string fname) {
  if (!is_mounted) { cout << "Error: NFS not mounted." << endl; return; }
  cout << "rm_rpc: Not implemented yet." << endl;
}

// Remote procedure call on stat
void Shell::stat_rpc(string fname) {
  if (!is_mounted) { cout << "Error: NFS not mounted." << endl; return; }
  cout << "stat_rpc: Not implemented yet." << endl;
}


// Executes the shell until the user quits.
void Shell::run()
{
  // make sure that the file system is mounted
  if (!is_mounted)
 	return;

  // continue until the user quits
  bool user_quit = false;
  while (!user_quit) {

    // print prompt and get command line
    string command_str;
    cout << PROMPT_STRING;
    getline(cin, command_str);

    // execute the command
    user_quit = execute_command(command_str);
  }

  // unmount the file system
  unmountNFS();
}

// Execute a script.
void Shell::run_script(char *file_name)
{
  // make sure that the file system is mounted
  if (!is_mounted)
  	return;
  // open script file
  ifstream infile;
  infile.open(file_name);
  if (infile.fail()) {
    cerr << "Could not open script file" << endl;
    return;
  }


  // execute each line in the script
  bool user_quit = false;
  string command_str;
  getline(infile, command_str, '\n');
  while (!infile.eof() && !user_quit) {
    cout << PROMPT_STRING << command_str << endl;
    user_quit = execute_command(command_str);
    getline(infile, command_str);
  }

  // clean up
  unmountNFS();
  infile.close();
}


// Executes the command. Returns true for quit and false otherwise.
bool Shell::execute_command(string command_str)
{
  // parse the command line
  struct Command command = parse_command(command_str);

  // look for the matching command
  if (command.name == "") {
    return false;
  }
  else if (command.name == "mkdir") {
    mkdir_rpc(command.file_name);
  }
  else if (command.name == "cd") {
    cd_rpc(command.file_name);
  }
  else if (command.name == "home") {
    home_rpc();
  }
  else if (command.name == "rmdir") {
    rmdir_rpc(command.file_name);
  }
  else if (command.name == "ls") {
    ls_rpc();
  }
  else if (command.name == "create") {
    create_rpc(command.file_name);
  }
  else if (command.name == "append") {
    append_rpc(command.file_name, command.append_data);
  }
  else if (command.name == "cat") {
    cat_rpc(command.file_name);
  }
  else if (command.name == "head") {
    errno = 0;
    // Changed to stoul to match head_rpc parameter type (unsigned int)
    unsigned long n = strtoul(command.append_data.c_str(), NULL, 0);
    if (0 == errno) {
      // Cast to int for head_rpc, assuming it fits.
      // If n can exceed int max, head_rpc signature should be unsigned int.
      head_rpc(command.file_name, (int)n);
    } else {
      cerr << "Invalid command line: " << command.append_data;
      cerr << " is not a valid number of bytes" << endl;
      return false;
    }
  }
  else if (command.name == "rm") {
    rm_rpc(command.file_name);
  }
  else if (command.name == "stat") {
    stat_rpc(command.file_name);
  }
  else if (command.name == "quit") {
    return true; // The run() function will call unmountNFS()
  }

  return false;
}

// Parses a command line into a command struct. Returned name is blank
// for invalid command lines.
Shell::Command Shell::parse_command(string command_str)
{
  // empty command struct returned for errors
  struct Command empty = {"", "", ""};

  // grab each of the tokens (if they exist)
  struct Command command;
  istringstream ss(command_str);
  int num_tokens = 0;
  if (ss >> command.name) {
    num_tokens++;
    if (ss >> command.file_name) {
      num_tokens++;
      if (ss >> command.append_data) {
        num_tokens++;
        string junk;
        if (ss >> junk) {
          num_tokens++;
        }
      }
    }
  }

  // Check for empty command line
  if (num_tokens == 0) {
    return empty;
  }

  // Check for invalid command lines
  if (command.name == "ls" ||
      command.name == "home" ||
      command.name == "quit")
  {
    if (num_tokens != 1) {
      cerr << "Invalid command line: " << command.name;
      cerr << " has improper number of arguments" << endl;
      return empty;
    }
  }
  else if (command.name == "mkdir" ||
      command.name == "cd"    ||
      command.name == "rmdir" ||
      command.name == "create"||
      command.name == "cat"   ||
      command.name == "rm"    ||
      command.name == "stat")
  {
    if (num_tokens != 2) {
      cerr << "Invalid command line: " << command.name;
      cerr << " has improper number of arguments" << endl;
      return empty;
    }
  }
  else if (command.name == "append" || command.name == "head")
  {
    if (num_tokens != 3) {
      cerr << "Invalid command line: " << command.name;
      cerr << " has improper number of arguments" << endl;
      return empty;
    }
  }
  else {
    cerr << "Invalid command line: " << command.name;
    cerr << " is not a command" << endl;
    return empty;
  }

  return command;
}