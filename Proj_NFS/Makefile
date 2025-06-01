# Compiler and flags
CXX = g++
CXXFLAGS = -g -O0 -std=c++11

# Object files for the server
SERVER_OBJS = BasicFileSys.o Disk.o FileSys.o server.o

# Target for the NFS Server executable
nfsserver: $(SERVER_OBJS)
	$(CXX) -o $@ $(SERVER_OBJS)

# Generic rule to compile .cpp files into .o files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

# Clean rule to remove generated files
clean:
	rm -f $(SERVER_OBJS) nfsserver DISK # Also remove the DISK file for a fresh start