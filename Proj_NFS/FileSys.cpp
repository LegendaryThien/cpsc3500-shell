// CPSC 3500: File System
// Implements the file system commands that are available to the shell.

#include <cstring>
#include <iostream>
#include <unistd.h>     // For close(), write()
#include <sstream>      // For std::stringstream
#include <algorithm>    // For std::min
#include <string>       // For std::string

using namespace std;

#include "FileSys.h"
#include "BasicFileSys.h" // Included via FileSys.h now
#include "Blocks.h"       // Included via FileSys.h now

// Constructor
FileSys::FileSys() : bfs(), curr_dir(1), fs_sock(-1) {
    // BasicFileSys will be mounted/unmounted by server.cpp
}

// Helper function to check if a block is a directory
// Reads the block and checks its magic number.
bool FileSys::is_directory(short block_num) {
    // Block 0 is the superblock and not a directory or inode
    if (block_num == 0) return false;

    // Use a generic buffer to read the block and inspect its magic number
    char block_buffer[BLOCK_SIZE];
    bfs.read_block(block_num, block_buffer);

    // The magic number is the first field in both inode and directory blocks
    unsigned int magic_num = *(unsigned int *)block_buffer;

    return magic_num == DIR_MAGIC_NUM;
}

// mounts the file system
void FileSys::mount(int sock) {
  bfs.mount();
  curr_dir = 1; //by default current directory is home directory, in disk block #1
  fs_sock = sock; //use this socket to receive file system operations from the client and send back response messages
}

// unmounts the file system
void FileSys::unmount() {
  bfs.unmount();
  if (fs_sock != -1) { // Only close if it's a valid socket
      close(fs_sock);
      fs_sock = -1;
  }
}

// make a directory
string FileSys::mkdir(const char *name)
{
  // Check if name is too long
  if (strlen(name) > MAX_FNAME_SIZE) {
    return "504 File name is too long";
  }

  // Read current directory block
  struct dirblock_t dir_block;
  bfs.read_block(curr_dir, (void *) &dir_block);

  // Check if name already exists
  for (int i = 0; i < dir_block.num_entries; i++) {
    if (strcmp(dir_block.dir_entries[i].name, name) == 0) {
      return "502 File exists";
    }
  }

  // Check if directory is full
  if (dir_block.num_entries >= MAX_DIR_ENTRIES) {
    return "506 Directory is full";
  }

  // Get a free block for the new directory
  short new_block_num = bfs.get_free_block();
  if (new_block_num == 0) { // Check for disk full (block 0 is superblock, cannot be allocated)
    return "505 Disk is full";
  }

  // Initialize new directory block
  struct dirblock_t new_dir;
  new_dir.magic = DIR_MAGIC_NUM;
  new_dir.num_entries = 0;
  for (int i = 0; i < MAX_DIR_ENTRIES; i++) {
    new_dir.dir_entries[i].block_num = 0; // Unused entries indicated by block_num of 0
  }
  bfs.write_block(new_block_num, (void *) &new_dir); // Write new directory block to disk

  // Add entry to current directory
  strcpy(dir_block.dir_entries[dir_block.num_entries].name, name); // Copy name (null-terminated)
  dir_block.dir_entries[dir_block.num_entries].block_num = new_block_num; // Store new directory's block number
  dir_block.num_entries++; // Increment entry count
  bfs.write_block(curr_dir, (void *) &dir_block); // Write updated current directory back to disk

  return "200 OK"; // Success message
}

// list the contents of current directory
string FileSys::ls()
{
  struct dirblock_t dir_block;
  bfs.read_block(curr_dir, (void *)&dir_block); // Read current directory block

  stringstream ss; // Use stringstream to build the output string

  if (dir_block.num_entries == 0) { // If no entries, it's an "empty folder"
    ss << "empty folder";
  } else {
    for (int i = 0; i < dir_block.num_entries; i++) {
        ss << dir_block.dir_entries[i].name; // Print name
        // Directories should have a '/' suffix
        if (is_directory(dir_block.dir_entries[i].block_num)) {
            ss << "/";
        }
        // Add space between entries, but not after the last one
        if (i < dir_block.num_entries - 1) {
            ss << " ";
        }
    }
  }
  // The assignment example for ls success just shows the content, no "200 OK"
  // So, return "200 OK\n" followed by the content. Shell.cpp will handle printing.
  return "200 OK\n" + ss.str();
}

// switch to a directory
string FileSys::cd(const char *name)
{
  // Read current directory block
  struct dirblock_t dir_block;
  bfs.read_block(curr_dir, (void *) &dir_block);

  // Find the directory entry
  short target_block = 0;
  bool found = false;
  for (int i = 0; i < dir_block.num_entries; i++) {
    if (strcmp(dir_block.dir_entries[i].name, name) == 0) {
      found = true;
      target_block = dir_block.dir_entries[i].block_num;
      break;
    }
  }

  if (!found) {
    return "503 File does not exist";
  }

  // Verify that target is a directory
  if (!is_directory(target_block)) {
    return "500 File is not a directory";
  }

  // Change to the new directory
  curr_dir = target_block; // Update current directory tracker
  return "200 OK"; // Success message
}

// switch to home directory
string FileSys::home() {
  curr_dir = 1; // Home directory is always block 1
  return "200 OK"; // Success message
}

// remove a directory
string FileSys::rmdir(const char *name)
{
  // Read current directory block
  struct dirblock_t dir_block;
  bfs.read_block(curr_dir, (void *)&dir_block);

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
    return "503 File does not exist";
  }

  // Read the target block and check if it's a directory
  if (!is_directory(dir_block_num)) {
    return "500 File is not a directory";
  }

  // Read the target directory block to check if empty
  struct dirblock_t target_dir;
  bfs.read_block(dir_block_num, (void *)&target_dir);
  if (target_dir.num_entries > 0) {
    return "507 Directory is not empty";
  }

  // Remove the entry from the current directory
  for (int i = entry_index; i < dir_block.num_entries - 1; i++) {
    dir_block.dir_entries[i] = dir_block.dir_entries[i + 1]; // Shift subsequent entries
  }
  // Clear the last entry's block_num and name to properly mark it as unused
  dir_block.dir_entries[dir_block.num_entries - 1].block_num = 0;
  memset(dir_block.dir_entries[dir_block.num_entries - 1].name, 0, MAX_FNAME_SIZE + 1);
  dir_block.num_entries--; // Decrement entry count
  bfs.write_block(curr_dir, (void *)&dir_block); // Write updated current directory

  // Free the directory block
  bfs.reclaim_block(dir_block_num);

  return "200 OK"; // Success message
}


// create an empty data file
string FileSys::create(const char *name)
{
  // Check if name is too long
  if (strlen(name) > MAX_FNAME_SIZE) {
    return "504 File name is too long";
  }

  // Read current directory block
  struct dirblock_t dir_block;
  bfs.read_block(curr_dir, (void *)&dir_block);

  // Check if name already exists
  for (int i = 0; i < dir_block.num_entries; i++) {
    if (strcmp(dir_block.dir_entries[i].name, name) == 0) {
      return "502 File exists";
    }
  }

  // Check if directory is full
  if (dir_block.num_entries >= MAX_DIR_ENTRIES) {
    return "506 Directory is full";
  }

  // Get a free block for the new file (inode)
  short inode_block = bfs.get_free_block();
  if (inode_block == 0) { // Check for disk full (block 0 is superblock, cannot be allocated)
    return "505 Disk is full";
  }

  // Initialize inode for empty file
  struct inode_t inode;
  inode.magic = INODE_MAGIC_NUM;
  inode.size = 0;
  for (int i = 0; i < MAX_DATA_BLOCKS; i++) {
    inode.blocks[i] = 0; // No data blocks initially, 0 represents unused pointer
  }
  bfs.write_block(inode_block, (void *)&inode); // Write inode to disk

  // Add entry to current directory
  strcpy(dir_block.dir_entries[dir_block.num_entries].name, name); // Copy name (null-terminated)
  dir_block.dir_entries[dir_block.num_entries].block_num = inode_block; // Store new file's inode block number
  dir_block.num_entries++; // Increment entry count
  bfs.write_block(curr_dir, (void *)&dir_block); // Write updated current directory

  return "200 OK"; // Success message
}

// append data to a data file
string FileSys::append(const char *name, const char *data)
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
    return "503 File does not exist";
  }

  // Read the inode block and check if it's a file
  struct inode_t inode;
  bfs.read_block(inode_block_num, (void *)&inode);
  if (inode.magic != INODE_MAGIC_NUM) {
    return "501 File is a directory";
  }

  int data_len = strlen(data); // Determine data size
  if (data_len == 0) { // If no data to append, it's still a success if file exists
      return "200 OK";
  }

  // Check if append exceeds maximum file size
  if (inode.size + data_len > MAX_FILE_SIZE) {
    return "508 Append exceeds maximum file size";
  }

  int bytes_to_write_total = data_len;
  int current_append_offset = 0; // Tracks bytes from 'data' string being appended

  // Loop until all data is appended
  while (bytes_to_write_total > 0) {
      int current_file_size = inode.size;
      int last_block_index = (current_file_size == 0) ? 0 : (current_file_size - 1) / BLOCK_SIZE;
      int offset_in_last_block = current_file_size % BLOCK_SIZE;

      short current_data_block_num;
      struct datablock_t data_block;

      // Determine the block to write to
      if (offset_in_last_block == 0 && current_file_size > 0) { // Current last block is full, need a new block
          last_block_index++; // Move to next available data block pointer in inode
      }

      // Check if we need to allocate a new data block or use an existing one
      if (inode.blocks[last_block_index] == 0) { // New data block needed
          current_data_block_num = bfs.get_free_block();
          if (current_data_block_num == 0) { // Disk is full
              return "505 Disk is full";
          }
          inode.blocks[last_block_index] = current_data_block_num; // Update inode's block pointer
      } else { // Use existing block
          current_data_block_num = inode.blocks[last_block_index];
      }

      // Read the data block (necessary for partial writes to fill existing block)
      bfs.read_block(current_data_block_num, (void *)&data_block);

      // Calculate bytes to write in the current block
      int space_in_current_block = BLOCK_SIZE - offset_in_last_block; // How much space is left in this block
      int bytes_to_copy_in_this_block = min(bytes_to_write_total, space_in_current_block); // Don't write more than available space or remaining data

      // Copy data into the block
      memcpy(data_block.data + offset_in_last_block, data + current_append_offset, bytes_to_copy_in_this_block);
      bfs.write_block(current_data_block_num, (void *)&data_block); // Write updated block to disk

      // Update counters
      current_append_offset += bytes_to_copy_in_this_block; // Track how much of original data has been appended
      inode.size += bytes_to_copy_in_this_block; // Increment total file size
      bytes_to_write_total -= bytes_to_copy_in_this_block; // Decrement remaining data to write
  }

  // Write the updated inode back to disk after all data blocks are handled
  bfs.write_block(inode_block_num, (void *)&inode);

  return "200 OK"; // Success message
}

// display the contents of a data file
string FileSys::cat(const char *name)
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
    return "503 File does not exist";
  }

  // Read the inode block and check if it's a file
  struct inode_t inode;
  bfs.read_block(inode_block_num, (void *)&inode);
  if (inode.magic != INODE_MAGIC_NUM) {
    return "501 File is a directory";
  }

  stringstream ss; // Use stringstream to build the output string
  int remaining_bytes = inode.size; // Total bytes in file
  int current_block_idx = 0;
  struct datablock_t data_block;

  while (remaining_bytes > 0 && current_block_idx < MAX_DATA_BLOCKS) {
    if (inode.blocks[current_block_idx] == 0) { // Should not happen if size is correct, but defensive
      break;
    }

    bfs.read_block(inode.blocks[current_block_idx], (void *)&data_block);
    int bytes_to_read_in_block = min(remaining_bytes, BLOCK_SIZE);

    // Append data to stringstream (copy character by character or using string constructor)
    ss.write(data_block.data, bytes_to_read_in_block);

    remaining_bytes -= bytes_to_read_in_block;
    current_block_idx++;
  }
  // Print a newline when completed.
  return "200 OK\n" + ss.str() + "\n"; // Combine status and content, add trailing newline
}

// display the first N bytes of the file
string FileSys::head(const char *name, unsigned int n)
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
    return "503 File does not exist";
  }

  // Read the inode block and check if it's a file
  struct inode_t inode;
  bfs.read_block(inode_block_num, (void *)&inode);
  if (inode.magic != INODE_MAGIC_NUM) {
    return "501 File is a directory";
  }

  stringstream ss; // Use stringstream to build the output string
  // Display the first N bytes of the file. If N >= file size, print the whole file.
  unsigned int bytes_to_read_total = min(n, inode.size);
  unsigned int remaining_bytes = bytes_to_read_total;
  int current_block_idx = 0;
  struct datablock_t data_block;

  while (remaining_bytes > 0 && current_block_idx < MAX_DATA_BLOCKS) {
    if (inode.blocks[current_block_idx] == 0) { // Defensive check
      break;
    }

    bfs.read_block(inode.blocks[current_block_idx], (void *)&data_block);
    unsigned int bytes_to_read_in_block = min(remaining_bytes, (unsigned int)BLOCK_SIZE);

    // Append data to stringstream
    ss.write(data_block.data, bytes_to_read_in_block);

    remaining_bytes -= bytes_to_read_in_block;
    current_block_idx++;
  }
  // Print a newline when completed.
  return "200 OK\n" + ss.str() + "\n"; // Combine status and content, add trailing newline
}

// delete a data file
string FileSys::rm(const char *name)
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
    return "503 File does not exist";
  }

  // Read the inode block and check if it's a file
  struct inode_t inode;
  bfs.read_block(inode_block_num, (void *)&inode);
  if (inode.magic != INODE_MAGIC_NUM) {
    return "501 File is a directory";
  }

  // Free all data blocks used by the file
  for (int i = 0; i < MAX_DATA_BLOCKS; i++) {
    if (inode.blocks[i] != 0) {
      bfs.reclaim_block(inode.blocks[i]);
      inode.blocks[i] = 0; // Clear pointer in inode (though inode will be reclaimed too)
    }
  }

  // Free the inode block
  bfs.reclaim_block(inode_block_num);

  // Remove the entry from the current directory
  for (int i = entry_index; i < dir_block.num_entries - 1; i++) {
    dir_block.dir_entries[i] = dir_block.dir_entries[i + 1]; // Shift subsequent entries
  }
  // Clear the last entry's block_num and name to properly mark it as unused
  dir_block.dir_entries[dir_block.num_entries - 1].block_num = 0;
  memset(dir_block.dir_entries[dir_block.num_entries - 1].name, 0, MAX_FNAME_SIZE + 1);
  dir_block.num_entries--; // Decrement entry count
  bfs.write_block(curr_dir, (void *)&dir_block); // Write updated current directory

  return "200 OK"; // Success message
}

// display stats about file or directory
string FileSys::stat(const char *name)
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
    return "503 File does not exist";
  }

  stringstream ss; // Use stringstream to build the output string
  // Read the block and determine if it's a file or directory
  char block_buffer[BLOCK_SIZE];
  bfs.read_block(block_num, block_buffer);

  // Check magic number to determine type
  unsigned int magic = *(unsigned int *)block_buffer;
  if (magic == DIR_MAGIC_NUM) {
    // It's a directory
    // Format: Directory name: foo/Directory block: 7
    // Note: Assignment's stat example for directory name does NOT have a trailing slash
    ss << "Directory name: " << name << "\n"; // Removed the trailing slash here
    ss << "Directory block: " << block_num;
  } else if (magic == INODE_MAGIC_NUM) {
    // It's a file
    struct inode_t *inode = (struct inode_t *)block_buffer; // Cast to inode structure
    // Format: Inode block: 5\nBytes in file: 170\nNumber of blocks: 3\nFirst block: 2
    ss << "Inode block: " << block_num << "\n";
    ss << "Bytes in file: " << inode->size << "\n";

    // Calculate number of blocks (including the inode)
    int block_count = 1; // Start with 1 for the inode block itself
    for (int i = 0; i < MAX_DATA_BLOCKS; i++) {
      if (inode->blocks[i] != 0) { // If data block pointer is used
        block_count++;
      }
    }
    ss << "Number of blocks: " << block_count << "\n";

    // First block: block number of the first data block (or 0 if empty)
    short first_data_block = (inode->size == 0) ? 0 : inode->blocks[0];
    ss << "First block: " << first_data_block;
  } else {
    // Should not happen if all blocks are properly initialized
    return "500 Internal error: Invalid block type encountered";
  }
  return "200 OK\n" + ss.str(); // Combine status and content
}