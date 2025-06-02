// server.cpp
#include <iostream>
#include <string>
#include <cstdlib>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>     // For close()
#include <cstring>      // For memset, strerror
#include <sstream>      // For stringstream parsing

#include "FileSys.h"
using namespace std;

// Helper function to send all data in a buffer (handles partial sends)
ssize_t send_all(int sockfd, const char *buf, size_t len) {
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

// Helper function to receive data until \r\n (for client commands)
// This implementation is more robust for commands.
string receive_client_command(int sockfd) {
    string received_line;
    char c;
    while (true) {
        ssize_t bytes_read = recv(sockfd, &c, 1, 0);
        if (bytes_read <= 0) { // Connection closed or error
            if (bytes_read == 0) {
                cout << "Client disconnected." << endl;
            } else {
                cerr << "Error receiving data from client: " << strerror(errno) << endl;
            }
            return ""; // Signal end of connection or error
        }
        received_line += c;
        // Check for end of line marker
        if (received_line.length() >= 2 &&
            received_line[received_line.length() - 2] == '\r' &&
            received_line[received_line.length() - 1] == '\n') {
            break; // Found the delimiter
        }
    }
    // Remove the \r\n from the end of the command
    return received_line.substr(0, received_line.length() - 2);
}


int main(int argc, char* argv[]) {
    if (argc < 2) {
        cout << "Usage: ./nfsserver port#\n";
        return -1;
    }
    int port = atoi(argv[1]);

    int listen_sock; // Socket for listening for new connections
    int comm_sock;   // Socket for communication with an accepted client

    listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_sock < 0) {
        cerr << "Error creating socket" << endl;
        return -1;
    }

    int optval = 1;
    setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval , sizeof(int));

    struct sockaddr_in server_address;
    memset((char *) &server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(port);

    if (bind(listen_sock, (struct sockaddr *) &server_address, sizeof(server_address)) < 0) {
        cerr << "Error binding socket" << endl;
        close(listen_sock);
        return -1;
    }

    if (listen(listen_sock, 5) < 0) {
        cerr << "Error listening on socket" << endl;
        close(listen_sock);
        return -1;
    }

    cout << "NFS Server listening on port " << port << "..." << endl;

    struct sockaddr_in client_address;
    socklen_t client_len = sizeof(client_address);
    comm_sock = accept(listen_sock, (struct sockaddr *) &client_address, &client_len);
    if (comm_sock < 0) {
        cerr << "Error accepting client connection" << endl;
        close(listen_sock);
        return -1;
    }

    cout << "Client connected. New communication socket: " << comm_sock << endl;

    FileSys fs;
    fs.mount(comm_sock);

    cout << "File system mounted. Server waiting for commands." << endl;

    string client_request_line;
    while (true) {
        client_request_line = receive_client_command(comm_sock);

        if (client_request_line.empty()) { // Client disconnected or error
            break;
        }

        cout << "Received command: [" << client_request_line << "]" << endl;

        // Parse command and its arguments
        stringstream ss(client_request_line);
        string command_name;
        string arg1, arg2;

        ss >> command_name;
        ss >> arg1;
        ss >> arg2;

        string fs_raw_response; // This will hold the "Status_code Status_message\nbody_content" from FileSys

        // --- Command Processing and FileSys Invocation ---
        if (command_name == "ls") {
            fs_raw_response = fs.ls();
        } else if (command_name == "mkdir") {
            fs_raw_response = fs.mkdir(arg1.c_str());
        } else if (command_name == "cd") {
            fs_raw_response = fs.cd(arg1.c_str());
        } else if (command_name == "home") {
            fs_raw_response = fs.home();
        } else if (command_name == "rmdir") { // <--- ADDED rmdir HANDLING
            fs_raw_response = fs.rmdir(arg1.c_str());
        } else if (command_name == "create") {
            fs_raw_response = fs.create(arg1.c_str());
        } else if (command_name == "append") {
            // Reconstruct data as it might contain spaces
            string data_to_append = arg2;
            string temp_arg;
            while (ss >> temp_arg) {
                data_to_append += " " + temp_arg;
            }
            fs_raw_response = fs.append(arg1.c_str(), data_to_append.c_str());
        } else if (command_name == "cat") {
            fs_raw_response = fs.cat(arg1.c_str());
        } else if (command_name == "head") {
            try {
                unsigned int n = stoul(arg2);
                fs_raw_response = fs.head(arg1.c_str(), n);
            } catch (const std::exception& e) {
                fs_raw_response = "400 Bad Request\nInvalid number for head N";
            }
        } else if (command_name == "rm") {
            fs_raw_response = fs.rm(arg1.c_str());
        } else if (command_name == "stat") {
            fs_raw_response = fs.stat(arg1.c_str());
        }
        else {
            fs_raw_response = "400 Bad Request\nUnknown command";
        }

        // --- Format Server Response ---
        // The FileSys methods return "Status_code Status_message\nbody_content"
        // We need to reformat this into the final message format:
        // "Status_code Status_message\r\nLength:size_in_bytes\r\n\r\n<message_body>"

        stringstream fs_response_ss(fs_raw_response);
        string status_line_from_fs;
        string body_from_fs;

        getline(fs_response_ss, status_line_from_fs); // Read the first line (e.g., "200 OK")
        // Read the rest as body (getline with no delimiter reads until end, or you can use .str() directly on remaining ss)
        body_from_fs = fs_response_ss.str();

        // Remove any trailing newlines/carriage returns from status_line_from_fs
        // (getline might include the newline character if not careful, depends on implementation)
        if (!status_line_from_fs.empty() && status_line_from_fs.back() == '\r') {
             status_line_from_fs.pop_back();
        }
        if (!status_line_from_fs.empty() && status_line_from_fs.back() == '\n') {
             status_line_from_fs.pop_back();
        }

        // Build the final response string
        stringstream full_response_ss;
        full_response_ss << status_line_from_fs << "\r\n"; // First header line
        full_response_ss << "Length:" << body_from_fs.length() << "\r\n"; // Second header line
        full_response_ss << "\r\n"; // Blank line
        full_response_ss << body_from_fs; // Message body

        string full_response = full_response_ss.str();

        cout << "Sending response (Total Bytes: " << full_response.length() << "):\n" << full_response << "END_RESPONSE_DELIMITER\n"; // For debugging

        // Send the full response back to the client
        if (send_all(comm_sock, full_response.c_str(), full_response.length()) == -1) {
            cerr << "Error sending response: " << strerror(errno) << endl;
            break; // Break loop on send error
        }
    }

    // Client disconnected or error occurred, close sockets and unmount
    cout << "Server shutting down. Closing sockets and unmounting file system." << endl;
    close(comm_sock); // Close communication socket
    close(listen_sock); // Close listening socket
    fs.unmount(); // This will also close fs_sock

    return 0;
}