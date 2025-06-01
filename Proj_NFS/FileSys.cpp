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
  cout << "Attempting to change to directory: " << name << endl;
  
  // Read current directory block
  struct dirblock_t dir_block;
  bfs.read_block(curr_dir, (void *) &dir_block);

  // Check if directory exists
  bool found = false;
  short target_block = 0;
  
  for (int i = 0; i < dir_block.num_entries; i++) {
    if (strcmp(dir_block.dir_entries[i].name, name) == 0) {
      found = true;
      target_block = dir_block.dir_entries[i].block_num;
      break;
    }
  }

  if (!found) {
    cout << "Error: Directory not found" << endl;
    return;
  }

  // Verify that target is a directory
  struct dirblock_t target_dir;
  bfs.read_block(target_block, (void *) &target_dir);
  
  if (target_dir.magic != DIR_MAGIC_NUM) {
    cout << "Error: Not a directory" << endl;
    return;
  }

  // Change to the new directory
  curr_dir = target_block;
  cout << "Successfully changed to directory: " << name << endl;
}

// switch to home directory
void FileSys::home() {
  cout << "Changing to home directory" << endl;
  curr_dir = 1; // Home directory is always block 1
  cout << "Successfully changed to home directory" << endl;
}

// remove a directory
void FileSys::rmdir(const char *name)
{
  // Read current directory block
  struct dirblock_t dir_block;
  bfs.read_block(curr_dir, (void *) &dir_block);

  // Find the directory entry
  int entry_index = -1;
  short dir_block_num = 0;
  for (int i = 0; i < dir_block.num_entries; i++) {
    if (strcmp(dir_block.dir_entries[i].name, name) == 0) {
      entry_index = i;
      dir_block_num = dir_block.dir_entries[i].block_num;
      break;
    }
  }

  if (entry_index == -1) {
    cout << "Error: Directory not found" << endl;
    return;
  }

  // Read the target block and check if it's a directory
  struct dirblock_t target_dir;
  bfs.read_block(dir_block_num, (void *)&target_dir);
  if (target_dir.magic != DIR_MAGIC_NUM) {
    cout << "Error: Not a directory" << endl;
    return;
  }

  // Check if the directory is empty
  if (target_dir.num_entries > 0) {
    cout << "Error: Directory not empty" << endl;
    return;
  }

  // Remove the entry from the current directory
  for (int i = entry_index; i < dir_block.num_entries - 1; i++) {
    dir_block.dir_entries[i] = dir_block.dir_entries[i + 1];
  }
  dir_block.num_entries--;
  bfs.write_block(curr_dir, (void *)&dir_block);

  // Free the directory block
  bfs.reclaim_block(dir_block_num);

  cout << "Directory removed successfully" << endl;
}

// list the contents of current directory
void FileSys::ls()
{
  struct dirblock_t dir_block;
  bfs.read_block(curr_dir, (void *)&dir_block);
  cout << "Directory contents:" << endl;
  for (int i = 0; i < dir_block.num_entries; i++) {
    cout << dir_block.dir_entries[i].name << endl;
  }
}

// create an empty data file
void FileSys::create(const char *name)
{
  // Check if name is too long
  if (strlen(name) > MAX_FNAME_SIZE) {
    cout << "Error: File name too long" << endl;
    return;
  }

  // Read current directory block
  struct dirblock_t dir_block;
  bfs.read_block(curr_dir, (void *)&dir_block);

  // Check if directory is full
  if (dir_block.num_entries >= MAX_DIR_ENTRIES) {
    cout << "Error: Directory is full" << endl;
    return;
  }

  // Check if name already exists
  for (int i = 0; i < dir_block.num_entries; i++) {
    if (strcmp(dir_block.dir_entries[i].name, name) == 0) {
      cout << "Error: File or directory already exists" << endl;
      return;
    }
  }

  // Get a free block for the new file (inode)
  short inode_block = bfs.get_free_block();
  if (inode_block == 0) {
    cout << "Error: Disk is full" << endl;
    return;
  }

  // Initialize inode for empty file
  struct inode_t inode;
  inode.magic = INODE_MAGIC_NUM;
  inode.size = 0;
  for (int i = 0; i < MAX_DATA_BLOCKS; i++) {
    inode.blocks[i] = 0;
  }
  bfs.write_block(inode_block, (void *)&inode);

  // Add entry to current directory
  strcpy(dir_block.dir_entries[dir_block.num_entries].name, name);
  dir_block.dir_entries[dir_block.num_entries].block_num = inode_block;
  dir_block.num_entries++;
  bfs.write_block(curr_dir, (void *)&dir_block);

  cout << "File created successfully" << endl;
}

// append data to a data file
void FileSys::append(const char *name, const char *data)
{
  // Read current directory block
  struct dirblock_t dir_block;
  bfs.read_block(curr_dir, (void *)&dir_block);

  // Find the file entry
  int entry_index = -1;
  short inode_block_num = 0;
  for (int i = 0; i < dir_block.num_entries; i++) {
    if (strcmp(dir_block.dir_entries[i].name, name) == 0) {
      entry_index = i;
      inode_block_num = dir_block.dir_entries[i].block_num;
      break;
    }
  }

  if (entry_index == -1) {
    cout << "Error: File not found" << endl;
    return;
  }

  // Read the inode block and check if it's a file
  struct inode_t inode;
  bfs.read_block(inode_block_num, (void *)&inode);
  if (inode.magic != INODE_MAGIC_NUM) {
    cout << "Error: Not a file" << endl;
    return;
  }

  // Calculate the size of the data to append
  int data_size = strlen(data);
  if (data_size == 0) {
    cout << "Error: No data to append" << endl;
    return;
  }

  // Check if the file is already at maximum size
  if (inode.size >= MAX_FILE_SIZE) {
    cout << "Error: File is at maximum size" << endl;
    return;
  }

  // Find the last block or allocate a new one if the file is empty
  short last_block = 0;
  if (inode.size > 0) {
    int last_block_index = (inode.size - 1) / BLOCK_SIZE;
    last_block = inode.blocks[last_block_index];
  }

  // If the file is empty or the last block is full, allocate a new block
  if (last_block == 0 || (inode.size % BLOCK_SIZE) == 0) {
    last_block = bfs.get_free_block();
    if (last_block == 0) {
      cout << "Error: Disk is full" << endl;
      return;
    }
    inode.blocks[inode.size / BLOCK_SIZE] = last_block;
  }

  // Read the last block
  struct datablock_t data_block;
  bfs.read_block(last_block, (void *)&data_block);

  // Calculate the offset in the last block
  int offset = inode.size % BLOCK_SIZE;
  int remaining_space = BLOCK_SIZE - offset;

  // Append data to the last block
  int bytes_to_write = (data_size < remaining_space) ? data_size : remaining_space;
  memcpy(data_block.data + offset, data, bytes_to_write);
  bfs.write_block(last_block, (void *)&data_block);

  // Update the inode size
  inode.size += bytes_to_write;
  bfs.write_block(inode_block_num, (void *)&inode);

  cout << "Data appended successfully" << endl;
}

// display the contents of a data file
void FileSys::cat(const char *name)
{
  // Read current directory block
  struct dirblock_t dir_block;
  bfs.read_block(curr_dir, (void *)&dir_block);

  // Find the file entry
  int entry_index = -1;
  short inode_block_num = 0;
  for (int i = 0; i < dir_block.num_entries; i++) {
    if (strcmp(dir_block.dir_entries[i].name, name) == 0) {
      entry_index = i;
      inode_block_num = dir_block.dir_entries[i].block_num;
      break;
    }
  }

  if (entry_index == -1) {
    cout << "Error: File not found" << endl;
    return;
  }

  // Read the inode block and check if it's a file
  struct inode_t inode;
  bfs.read_block(inode_block_num, (void *)&inode);
  if (inode.magic != INODE_MAGIC_NUM) {
    cout << "Error: Not a file" << endl;
    return;
  }

  // Display the contents of the file
  int remaining_bytes = inode.size;
  int current_block = 0;
  struct datablock_t data_block;

  while (remaining_bytes > 0) {
    if (inode.blocks[current_block] == 0) {
      break;
    }

    bfs.read_block(inode.blocks[current_block], (void *)&data_block);
    int bytes_to_read = (remaining_bytes < BLOCK_SIZE) ? remaining_bytes : BLOCK_SIZE;
    
    // Write the data to stdout
    write(1, data_block.data, bytes_to_read);
    
    remaining_bytes -= bytes_to_read;
    current_block++;
  }
  cout << endl;
}

// display the first N bytes of the file
void FileSys::head(const char *name, unsigned int n)
{
  // Read current directory block
  struct dirblock_t dir_block;
  bfs.read_block(curr_dir, (void *)&dir_block);

  // Find the file entry
  int entry_index = -1;
  short inode_block_num = 0;
  for (int i = 0; i < dir_block.num_entries; i++) {
    if (strcmp(dir_block.dir_entries[i].name, name) == 0) {
      entry_index = i;
      inode_block_num = dir_block.dir_entries[i].block_num;
      break;
    }
  }

  if (entry_index == -1) {
    cout << "Error: File not found" << endl;
    return;
  }

  // Read the inode block and check if it's a file
  struct inode_t inode;
  bfs.read_block(inode_block_num, (void *)&inode);
  if (inode.magic != INODE_MAGIC_NUM) {
    cout << "Error: Not a file" << endl;
    return;
  }

  // Display the first N bytes of the file
  int bytes_to_read = (n < inode.size) ? n : inode.size;
  int remaining_bytes = bytes_to_read;
  int current_block = 0;
  struct datablock_t data_block;

  while (remaining_bytes > 0) {
    if (inode.blocks[current_block] == 0) {
      break;
    }

    bfs.read_block(inode.blocks[current_block], (void *)&data_block);
    int block_bytes = (remaining_bytes < BLOCK_SIZE) ? remaining_bytes : BLOCK_SIZE;
    
    // Write the data to stdout
    write(1, data_block.data, block_bytes);
    
    remaining_bytes -= block_bytes;
    current_block++;
  }
  cout << endl;
}

// delete a data file
void FileSys::rm(const char *name)
{
  // Read current directory block
  struct dirblock_t dir_block;
  bfs.read_block(curr_dir, (void *)&dir_block);

  // Find the file entry
  int entry_index = -1;
  short inode_block_num = 0;
  for (int i = 0; i < dir_block.num_entries; i++) {
    if (strcmp(dir_block.dir_entries[i].name, name) == 0) {
      entry_index = i;
      inode_block_num = dir_block.dir_entries[i].block_num;
      break;
    }
  }

  if (entry_index == -1) {
    cout << "Error: File not found" << endl;
    return;
  }

  // Read the inode block and check if it's a file
  struct inode_t inode;
  bfs.read_block(inode_block_num, (void *)&inode);
  if (inode.magic != INODE_MAGIC_NUM) {
    cout << "Error: Not a file" << endl;
    return;
  }

  // Free all data blocks used by the file
  for (int i = 0; i < MAX_DATA_BLOCKS; i++) {
    if (inode.blocks[i] != 0) {
      bfs.reclaim_block(inode.blocks[i]);
    }
  }

  // Free the inode block
  bfs.reclaim_block(inode_block_num);

  // Remove the entry from the current directory
  for (int i = entry_index; i < dir_block.num_entries - 1; i++) {
    dir_block.dir_entries[i] = dir_block.dir_entries[i + 1];
  }
  dir_block.num_entries--;
  bfs.write_block(curr_dir, (void *)&dir_block);

  cout << "File removed successfully" << endl;
}

// display stats about file or directory
void FileSys::stat(const char *name)
{
  // Read current directory block
  struct dirblock_t dir_block;
  bfs.read_block(curr_dir, (void *)&dir_block);

  // Find the entry
  int entry_index = -1;
  short block_num = 0;
  for (int i = 0; i < dir_block.num_entries; i++) {
    if (strcmp(dir_block.dir_entries[i].name, name) == 0) {
      entry_index = i;
      block_num = dir_block.dir_entries[i].block_num;
      break;
    }
  }

  if (entry_index == -1) {
    cout << "Error: File or directory not found" << endl;
    return;
  }

  // Read the block and determine if it's a file or directory
  char block[BLOCK_SIZE];
  bfs.read_block(block_num, block);

  // Check magic number to determine type
  if (*(int *)block == DIR_MAGIC_NUM) {
    // It's a directory
    struct dirblock_t *dir = (struct dirblock_t *)block;
    cout << "Directory Name: " << name << endl;
    cout << "Block Number: " << block_num << endl;
    cout << "Number of Entries: " << dir->num_entries << endl;
  } else if (*(int *)block == INODE_MAGIC_NUM) {
    // It's a file
    struct inode_t *inode = (struct inode_t *)block;
    cout << "File Name: " << name << endl;
    cout << "Block Number: " << block_num << endl;
    cout << "File Size: " << inode->size << " bytes" << endl;
    cout << "Number of Blocks: ";
    int block_count = 0;
    for (int i = 0; i < MAX_DATA_BLOCKS; i++) {
      if (inode->blocks[i] != 0) {
        block_count++;
      }
    }
    cout << block_count << endl;
  } else {
    cout << "Error: Invalid block type" << endl;
  }
}

// HELPER FUNCTIONS (optional)

