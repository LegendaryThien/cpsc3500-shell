#include <iostream>
#include <string>
#include <cstdlib>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h> // For close()
#include <cstring> // For memset (already included, but good to be explicit for bzero replacement)
#include "FileSys.h"
using namespace std;

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cout << "Usage: ./nfsserver port#\n";
        return -1;
    }
    int port = atoi(argv[1]);

    // Networking part: create the socket and accept the client connection
    int listen_sock; // Socket for listening for new connections
    int comm_sock;   // Socket for communication with an accepted client

    // 1. Create a TCP socket
    // AF_INET for IPv4, SOCK_STREAM for TCP, 0 for default protocol
    listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_sock < 0) {
        cerr << "Error creating socket\n";
        return -1;
    }

    // Optional: Allow reuse of local addresses (helps with rapid restarts)
    int optval = 1;
    setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval , sizeof(int));

    // 2. Bind the socket to the specified port
    struct sockaddr_in server_address;
    // bzero((char *) &server_address, sizeof(server_address)); // Original line
    memset((char *) &server_address, 0, sizeof(server_address)); // Replaced bzero with memset

    server_address.sin_family = AF_INET; // IPv4
    server_address.sin_addr.s_addr = INADDR_ANY; // Listen on any available network interface
    server_address.sin_port = htons(port); // Convert port number to network byte order

    if (bind(listen_sock, (struct sockaddr *) &server_address, sizeof(server_address)) < 0) {
        cerr << "Error binding socket\n";
        close(listen_sock); // Close the listening socket if bind fails
        return -1;
    }

    // 3. Listen for incoming connections
    // 5 is the backlog queue size (number of pending connections)
    if (listen(listen_sock, 5) < 0) {
        cerr << "Error listening on socket\n";
        close(listen_sock);
        return -1;
    }

    cout << "NFS Server listening on port " << port << "...\n";

    // 4. Accept a client connection
    // The server is designed to handle a single client at a time
    struct sockaddr_in client_address;
    socklen_t client_len = sizeof(client_address);
    comm_sock = accept(listen_sock, (struct sockaddr *) &client_address, &client_len);
    if (comm_sock < 0) {
        cerr << "Error accepting client connection\n";
        close(listen_sock);
        return -1;
    }

    cout << "Client connected. New communication socket: " << comm_sock << "\n";

    // Mount the file system using the new communication socket
    FileSys fs;
    fs.mount(comm_sock); // 'sock' from the original template is now 'comm_sock'
                         // which is the new socket for communication.

    // Loop: get the command from the client and invoke the file
    // system operation which returns the results or error messages back to the client
    // until the client closes the TCP connection.
    // (This part will be implemented in a later task, currently just a placeholder)
    cout << "File system mounted. Server waiting for commands (loop not yet implemented).\n";

    // For now, after accepting, we'll just wait for a key press to simulate ending
    // In the full implementation, this will be a loop handling client requests.
    cout << "Press Enter to unmount and exit...\n";
    cin.ignore(); // Wait for user input

    // Close the communication socket after the client disconnects or server shuts down
    close(comm_sock);

    // Close the listening socket
    close(listen_sock);

    // Unmount the file system
    fs.unmount(); // Note: fs.unmount() also closes fs_sock, which is comm_sock in this context.

    return 0;
}