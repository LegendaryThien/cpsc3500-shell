# Compiler and flags
CXX = g++
CXXFLAGS = -g -O0 -std=c++11

# Object files common to both (or potentially used by both through FileSys)
COMMON_OBJS = BasicFileSys.o Disk.o

# Object files specific to the server
SERVER_SPECIFIC_OBJS = FileSys.o server.o

# Object files specific to the client
CLIENT_SPECIFIC_OBJS = Shell.o client.o

# All object files that can be generated (for clean rule)
ALL_OBJS = $(COMMON_OBJS) $(SERVER_SPECIFIC_OBJS) $(CLIENT_SPECIFIC_OBJS)

# --- Targets ---

# Default target (builds both server and client)
all: nfsserver nfsclient

# Target for the NFS Server executable
nfsserver: $(COMMON_OBJS) $(SERVER_SPECIFIC_OBJS)
	$(CXX) -o $@ $(COMMON_OBJS) $(SERVER_SPECIFIC_OBJS)

# Target for the NFS Client executable
# IMPORTANT: The client should NOT link server.o
# It needs Shell.o and client.o (for its main), and potentially FileSys.o for definitions.
nfsclient: $(COMMON_OBJS) $(CLIENT_SPECIFIC_OBJS) FileSys.o
	$(CXX) -o $@ $(COMMON_OBJS) $(CLIENT_SPECIFIC_OBJS) FileSys.o

# Generic rule to compile .cpp files into .o files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

# Clean rule to remove generated files
clean:
	rm -f $(ALL_OBJS) nfsserver nfsclient DISK