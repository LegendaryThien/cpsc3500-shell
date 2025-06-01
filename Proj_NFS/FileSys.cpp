// CPSC 3500: File System
// Implements the file system commands that are available to the shell.

#include <cstring>
#include <iostream>
#include <unistd.h>
using namespace std;

#include "FileSys.h"
#include "BasicFileSys.h"
#include "Blocks.h"

// mounts the file system
void FileSys::mount(int sock) {
  bfs.mount();
  curr_dir = 1; //by default current directory is home directory, in disk block #1
  fs_sock = sock; //use this socket to receive file system operations from the client and send back response messages
}

// unmounts the file system
void FileSys::unmount() {
  bfs.unmount();
  close(fs_sock);
}

// make a directory
void FileSys::mkdir(const char *name)
{
  // Check if name is too long
  if (strlen(name) > MAX_FNAME_SIZE) {
    cout << "Error: Directory name too long" << endl;
    return;
  }

  // Read current directory block
  struct dirblock_t dir_block;
  bfs.read_block(curr_dir, (void *) &dir_block);

  // Check if directory is full
  if (dir_block.num_entries >= MAX_DIR_ENTRIES) {
    cout << "Error: Directory is full" << endl;
    return;
  }

  // Check if name already exists
  for (int i = 0; i < dir_block.num_entries; i++) {
    if (strcmp(dir_block.dir_entries[i].name, name) == 0) {
      cout << "Error: Directory already exists" << endl;
      return;
    }
  }

  // Get a free block for the new directory
  short new_block = bfs.get_free_block();
  if (new_block == 0) {
    cout << "Error: Disk is full" << endl;
    return;
  }

  // Initialize new directory block
  struct dirblock_t new_dir;
  new_dir.magic = DIR_MAGIC_NUM;
  new_dir.num_entries = 0;
  for (int i = 0; i < MAX_DIR_ENTRIES; i++) {
    new_dir.dir_entries[i].block_num = 0;
  }
  bfs.write_block(new_block, (void *) &new_dir);

  // Add entry to current directory
  strcpy(dir_block.dir_entries[dir_block.num_entries].name, name);
  dir_block.dir_entries[dir_block.num_entries].block_num = new_block;
  dir_block.num_entries++;
  bfs.write_block(curr_dir, (void *) &dir_block);

  cout << "Directory created successfully" << endl;
}

// switch to a directory
void FileSys::cd(const char *name)
{
}

// switch to home directory
void FileSys::home() {
}

// remove a directory
void FileSys::rmdir(const char *name)
{
}

// list the contents of current directory
void FileSys::ls()
{
}

// create an empty data file
void FileSys::create(const char *name)
{
}

// append data to a data file
void FileSys::append(const char *name, const char *data)
{
}

// display the contents of a data file
void FileSys::cat(const char *name)
{
}

// display the first N bytes of the file
void FileSys::head(const char *name, unsigned int n)
{
}

// delete a data file
void FileSys::rm(const char *name)
{
}

// display stats about file or directory
void FileSys::stat(const char *name)
{
}

// HELPER FUNCTIONS (optional)

