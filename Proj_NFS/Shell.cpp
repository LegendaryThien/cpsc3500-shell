// Shell.cpp
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm> // For std::min
#include <cstring>   // For memset, bcopy, strerror
#include <cstdlib>   // For stoi (string to int), strtoul
#include <stdexcept> // For std::invalid_argument, std::out_of_range
#include <errno.h>   // For errno
#include <vector>    // Added for std::vector

// Include necessary networking headers explicitly
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>  // For close()

using namespace std;

#include "Shell.h"

static const string PROMPT_STRING = "NFS> ";  // shell prompt

// Helper to send all data in a buffer (handles partial sends)
ssize_t shell_send_all(int sockfd, const char *buf, size_t len) {
    size_t total = 0; // how many bytes we've sent
    size_t bytesleft = len; // how many we have left to send
    ssize_t n;

    while(total < len) {
        n = send(sockfd, buf + total, bytesleft, 0);
        if (n == -1) { break; }
        total += n;
        bytesleft -= n;
    }
    return n == -1 ? -1 : total; // return -1 on failure, total bytes sent on success
}

// Helper to receive a response and parse it
// Returns true on success, false on error or disconnection.
bool receive_and_parse_response(int sockfd, int &status_code, string &status_message, string &body_content) {
    // Clear previous content
    status_code = -1;
    status_message.clear();
    body_content.clear();

    string received_data_buffer;
    char temp_buffer[1024]; // Temporary buffer for reading
    ssize_t bytes_read;
    size_t header_end_pos = string::npos;

    // Phase 1: Read data until we find "\r\n\r\n" which marks end of headers
    while (header_end_pos == string::npos) {
        bytes_read = recv(sockfd, temp_buffer, sizeof(temp_buffer) - 1, 0);
        if (bytes_read <= 0) { // 0 means connection closed, <0 means error
            if (bytes_read == 0) {
                cerr << "Server disconnected unexpectedly." << endl;
            } else {
                cerr << "Error receiving data from server: " << strerror(errno) << endl;
            }
            return false;
        }
        temp_buffer[bytes_read] = '\0';
        received_data_buffer += temp_buffer;

        header_end_pos = received_data_buffer.find("\r\n\r\n");
    }

    // Extract headers string and initial part of body
    string headers_str = received_data_buffer.substr(0, header_end_pos);
    body_content = received_data_buffer.substr(header_end_pos + 4); // +4 to skip "\r\n\r\n"

    // Parse headers
    stringstream header_ss(headers_str);
    string line;

    // Line 1: Status_code Status_message
    if (getline(header_ss, line, '\r')) { // Read until \r
        header_ss.ignore(1); // Ignore the \n
        stringstream status_line_ss(line);
        status_line_ss >> status_code; // Read status code
        // Read rest of line for message, removing leading space
        getline(status_line_ss, status_message);
        if (!status_message.empty() && status_message[0] == ' ') {
            status_message = status_message.substr(1);
        }
    } else {
        cerr << "Error parsing status line." << endl;
        return false;
    }

    // Line 2: Length:size_in_bytes
    int expected_body_length = 0;
    if (getline(header_ss, line, '\r')) { // Read until \r
        header_ss.ignore(1); // Ignore the \n
        size_t length_prefix_pos = line.find("Length:");
        if (length_prefix_pos != string::npos) {
            try {
                expected_body_length = stoi(line.substr(length_prefix_pos + 7));
            } catch (const std::exception& e) {
                cerr << "Error parsing body length: " << e.what() << endl;
                return false;
            }
        } else {
            cerr << "Error: 'Length:' header not found." << endl;
            return false;
        }
    } else {
        cerr << "Error parsing length line." << endl;
        return false;
    }

    // Phase 2: Read remaining body content if necessary
    int current_body_read_len = body_content.length();
    int remaining_body_to_read = expected_body_length - current_body_read_len;

    vector<char> dynamic_body_buffer(remaining_body_to_read + 1); // For remaining body
    while (remaining_body_to_read > 0) {
        bytes_read = recv(sockfd, dynamic_body_buffer.data(), remaining_body_to_read, 0);
        if (bytes_read <= 0) {
            cerr << "Error or connection closed while receiving remaining body." << endl;
            return false;
        }
        body_content.append(dynamic_body_buffer.data(), bytes_read); // Append bytes to string
        remaining_body_to_read -= bytes_read;
    }

    return true; // Successfully received and parsed response
}


// Shell constructor, do not change it!!
Shell::Shell() : cs_sock(-1), is_mounted(false) {
}

// Mount the network file system with server name and port number in the format of server:port
void Shell::mountNFS(string fs_loc) {
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

    // If all operations are completed successfully, set is_mounted to true
    is_mounted = true;
    cout << "NFS mounted successfully to " << fs_loc << endl;
}

// Unmount the network file system if it was mounted
void Shell::unmountNFS() {
    // close the socket if it was mounted
    if (is_mounted) {
        close(cs_sock); // Close the client socket
        cs_sock = -1;   // Invalidate the socket descriptor
        is_mounted = false; // Set mounted flag to false
        cout << "NFS unmounted successfully." << endl;
    } else {
        cout << "NFS is not mounted." << endl;
    }
}

// Helper to display generic success/error messages based on assignment examples
// This centralizes the logic for "OK" vs "success" and error codes.
void display_rpc_result(int status_code, const string& status_message, const string& body_content) {
    if (status_code == 200) {
        // For simple success commands (mkdir, cd, home, rmdir, create, append, rm)
        // Assignment examples show "success" for these.
        // Server sends "OK" for these, so map "OK" to "success".
        if (status_message == "OK") {
            cout << "success" << endl;
        } else {
            // This else block is for commands like ls, cat, head, stat where body_content
            // contains the actual output and needs to be printed directly.
            // These cases are handled individually in their _rpc functions to print body_content.
            // This block should ideally not be reached for 200 OK from ls, cat, head, stat.
            cout << status_message << endl; // Fallback, but typically not used for 200 OK
        }
    } else { // Error status
        // Error messages are printed directly as "Code Message"
        cout << status_code << " " << status_message << endl;
    }
}

// Remote procedure call on mkdir
void Shell::mkdir_rpc(string dname) {
  if (!is_mounted) { cout << "Error: NFS not mounted." << endl; return; }
  // Construct and send the command
  string command = "mkdir " + dname + "\r\n";
  if (shell_send_all(cs_sock, command.c_str(), command.length()) == -1) {
    cerr << "Error sending mkdir command to server.\n";
    return;
  }

  // Receive and parse the server's response
  int status_code;
  string status_message;
  string body_content; // mkdir typically has no body

  if (!receive_and_parse_response(cs_sock, status_code, status_message, body_content)) {
      return; // Error message already printed by helper
  }
  display_rpc_result(status_code, status_message, body_content);
}

// Remote procedure call on cd
void Shell::cd_rpc(string dname) {
    if (!is_mounted) {
        cout << "Error: NFS not mounted." << endl;
        return;
    }

    // 1. Construct and send the command
    string command = "cd " + dname + "\r\n"; // Client to Server Request Message Format
    if (shell_send_all(cs_sock, command.c_str(), command.length()) == -1) {
        cerr << "Error sending cd command to server.\n";
        return;
    }

    // 2. Receive and parse the server's response
    int status_code;
    string status_message;
    string body_content; // cd usually has no body, but parse for consistency

    if (!receive_and_parse_response(cs_sock, status_code, status_message, body_content)) {
        return; // Error message already printed by helper
    }
    display_rpc_result(status_code, status_message, body_content);
}


// Remote procedure call on home
void Shell::home_rpc() {
  if (!is_mounted) { cout << "Error: NFS not mounted." << endl; return; }

  // 1. Construct and send the command
  string command = "home\r\n"; // "home" command has no arguments
  if (shell_send_all(cs_sock, command.c_str(), command.length()) == -1) {
    cerr << "Error sending home command to server.\n";
    return;
  }

  // 2. Receive and parse the server's response
  int status_code;
  string status_message;
  string body_content; // home command usually has no body

  if (!receive_and_parse_response(cs_sock, status_code, status_message, body_content)) {
      return; // Error message already printed by helper
  }
  display_rpc_result(status_code, status_message, body_content);
}

// Remote procedure call on rmdir
void Shell::rmdir_rpc(string dname) {
    if (!is_mounted) {
        cout << "Error: NFS not mounted." << endl;
        return;
    }

    // 1. Construct and send the command: "rmdir <dname>\r\n"
    string command = "rmdir " + dname + "\r\n";
    if (shell_send_all(cs_sock, command.c_str(), command.length()) == -1) {
        cerr << "Error sending rmdir command to server.\n";
        return;
    }

    // 2. Receive and parse the server's response
    int status_code;
    string status_message;
    string body_content; // rmdir typically has no body, but parse for consistency

    if (!receive_and_parse_response(cs_sock, status_code, status_message, body_content)) {
        return; // Error message already printed by helper
    }
    display_rpc_result(status_code, status_message, body_content);
}

// Remote procedure call on ls
void Shell::ls_rpc() {
    if (!is_mounted) {
        cout << "Error: NFS not mounted." << endl;
        return;
    }

    // 1. Construct and send the command
    string command = "ls\r\n"; // Client to Server Request Message Format
    if (shell_send_all(cs_sock, command.c_str(), command.length()) == -1) {
        cerr << "Error sending ls command to server.\n";
        return;
    }

    // 2. Receive and parse the server's response
    int status_code;
    string status_message;
    string body_content;

    if (!receive_and_parse_response(cs_sock, status_code, status_message, body_content)) {
        // Error message already printed by receive_and_parse_response
        return;
    }

    // 3. Display the message - Special handling for ls: print body_content directly on success
    if (status_code == 200) { // OK status
        cout << body_content << endl; // Print the message body (directory contents)
    } else { // Error status
        cout << status_code << " " << status_message << endl;
    }
}

// Remote procedure call on create
void Shell::create_rpc(string fname) {
  if (!is_mounted) { cout << "Error: NFS not mounted." << endl; return; }

  // 1. Construct and send the command: "create <fname>\r\n"
  string command = "create " + fname + "\r\n";
  if (shell_send_all(cs_sock, command.c_str(), command.length()) == -1) {
    cerr << "Error sending create command to server.\n";
    return;
  }

  // 2. Receive and parse the server's response
  int status_code;
  string status_message;
  string body_content; // create typically has no body

  if (!receive_and_parse_response(cs_sock, status_code, status_message, body_content)) {
      return; // Error message already printed by helper
  }
  display_rpc_result(status_code, status_message, body_content);
}

// Remote procedure call on append
void Shell::append_rpc(string fname, string data) {
  if (!is_mounted) { cout << "Error: NFS not mounted." << endl; return; }
  // Construct and send the command: "append <filename> <data>\r\n"
  string command = "append " + fname + " " + data + "\r\n";
  if (shell_send_all(cs_sock, command.c_str(), command.length()) == -1) {
    cerr << "Error sending append command to server.\n";
    return;
  }

  // Receive and parse the server's response
  int status_code;
  string status_message;
  string body_content; // append typically has no body

  if (!receive_and_parse_response(cs_sock, status_code, status_message, body_content)) {
      return; // Error message already printed by helper
  }
  display_rpc_result(status_code, status_message, body_content);
}

// Remote procesure call on cat
void Shell::cat_rpc(string fname) {
  if (!is_mounted) { cout << "Error: NFS not mounted." << endl; return; }
  // Construct and send the command: "cat <filename>\r\n"
  string command = "cat " + fname + "\r\n";
  if (shell_send_all(cs_sock, command.c_str(), command.length()) == -1) {
    cerr << "Error sending cat command to server.\n";
    return;
  }

  // Receive and parse the server's response
  int status_code;
  string status_message;
  string body_content;

  if (!receive_and_parse_response(cs_sock, status_code, status_message, body_content)) {
      return; // Error message already printed by helper
  }

  // Display the message - Special handling for cat: print body_content directly on success
  if (status_code == 200) {
      cout << body_content; // cat body includes trailing newline, so no endl here
  } else {
      cout << status_code << " " << status_message << endl;
  }
}

// Remote procedure call on head
void Shell::head_rpc(string fname, int n) { // Note: 'n' is int here, but unsigned int in requirements
  if (!is_mounted) { cout << "Error: NFS not mounted." << endl; return; }
  // Construct and send the command: "head <filename> <n>\r\n"
  string command = "head " + fname + " " + to_string(n) + "\r\n";
  if (shell_send_all(cs_sock, command.c_str(), command.length()) == -1) {
    cerr << "Error sending head command to server.\n";
    return;
  }

  // Receive and parse the server's response
  int status_code;
  string status_message;
  string body_content;

  if (!receive_and_parse_response(cs_sock, status_code, status_message, body_content)) {
      return; // Error message already printed by helper
  }

  // Display the message - Special handling for head: print body_content directly on success
  if (status_code == 200) {
      cout << body_content; // head body includes trailing newline, so no endl here
  } else {
      cout << status_code << " " << status_message << endl;
  }
}

// Remote procedure call on rm
void Shell::rm_rpc(string fname) {
    if (!is_mounted) {
        cout << "Error: NFS not mounted." << endl;
        return;
    }

    // 1. Construct and send the command: "rm <filename>\r\n"
    string command = "rm " + fname + "\r\n";
    if (shell_send_all(cs_sock, command.c_str(), command.length()) == -1) {
        cerr << "Error sending rm command to server.\n";
        return;
    }

    // 2. Receive and parse the server's response
    int status_code;
    string status_message;
    string body_content; // rm typically has no body

    if (!receive_and_parse_response(cs_sock, status_code, status_message, body_content)) {
        return; // Error message already printed by helper
    }
    display_rpc_result(status_code, status_message, body_content);
}

// Remote procedure call on stat
void Shell::stat_rpc(string fname) {
    if (!is_mounted) {
        cout << "Error: NFS not mounted." << endl;
        return;
    }

    // 1. Construct and send the command: "stat <name>\r\n"
    string command = "stat " + fname + "\r\n";
    if (shell_send_all(cs_sock, command.c_str(), command.length()) == -1) {
        cerr << "Error sending stat command to server.\n";
        return;
    }

    // 2. Receive and parse the server's response
    int status_code;
    string status_message;
    string body_content;

    if (!receive_and_parse_response(cs_sock, status_code, status_message, body_content)) {
        return; // Error message already printed by helper
    }

    // 3. Display the message - Special handling for stat: print body_content directly on success
    if (status_code == 200) {
        cout << body_content << endl; // stat output is the body, needs a final endl
    } else {
        cout << status_code << " " << status_message << endl;
    }
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
    unsigned long n = strtoul(command.append_data.c_str(), NULL, 0);
    if (0 == errno) {
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
        if (ss >> junk) { // Check for extra tokens
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