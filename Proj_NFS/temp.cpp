#include <iostream>
#include "FileSys.h"
#include "BasicFileSys.h"

using namespace std;

int main() {
    // Create instance of FileSys
    FileSys fs;
    
    // Mount the file system (using -1 as socket since we're testing locally)
    fs.mount(-1);
    
    // Test 1: Create a valid directory
    cout << "\nTest 1: Creating a valid directory 'testdir'" << endl;
    fs.mkdir("testdir");
    
    // Test 2: Try to create directory with same name (should fail)
    cout << "\nTest 2: Creating directory with same name (should fail)" << endl;
    fs.mkdir("testdir");
    
    // Test 3: Create directory with very long name (should fail)
    cout << "\nTest 3: Creating directory with very long name (should fail)" << endl;
    char long_name[256];
    for(int i = 0; i < 255; i++) {
        long_name[i] = 'a';
    }
    long_name[255] = '\0';
    fs.mkdir(long_name);
    
    // Test 4: Create multiple directories
    cout << "\nTest 4: Creating multiple directories" << endl;
    fs.mkdir("dir1");
    fs.mkdir("dir2");
    fs.mkdir("dir3");
    
    // Test 5: Create directory with special characters
    cout << "\nTest 5: Creating directory with special characters" << endl;
    fs.mkdir("test-dir");
    fs.mkdir("test_dir");
    
    // Test 6: Test cd functionality
    cout << "\nTest 6: Testing cd functionality" << endl;
    
    // Test 6.1: Change to existing directory
    cout << "\nTest 6.1: Changing to existing directory 'testdir'" << endl;
    fs.cd("testdir");
    
    // Test 6.2: Try to change to non-existent directory
    cout << "\nTest 6.2: Attempting to change to non-existent directory 'nonexistent'" << endl;
    fs.cd("nonexistent");
    
    // Test 6.3: Create and change to nested directory
    cout << "\nTest 6.3: Creating and changing to nested directory" << endl;
    fs.mkdir("nested");
    fs.cd("nested");
    
    // Test 6.4: Create another directory in nested location
    cout << "\nTest 6.4: Creating directory in nested location" << endl;
    fs.mkdir("deep");
    
    // Test 7: Test home functionality
    cout << "\nTest 7: Testing home functionality" << endl;
    
    // Test 7.1: Change to home from nested directory
    cout << "\nTest 7.1: Changing to home from nested directory" << endl;
    fs.home();
    
    // Test 7.2: Verify we can still access directories from home
    cout << "\nTest 7.2: Verifying access to directories from home" << endl;
    fs.cd("testdir");
    fs.cd("nested");
    
    // Test 7.3: Go home again
    cout << "\nTest 7.3: Going home again" << endl;
    fs.home();

    // Test 8: rmdir functionality
    cout << "\nTest 8: Testing rmdir functionality" << endl;

    // Test 8.1: Remove an empty directory (should succeed)
    cout << "\nTest 8.1: Removing empty directory 'dir1' (should succeed)" << endl;
    fs.rmdir("dir1");

    // Test 8.2: Try to remove a non-existent directory (should fail)
    cout << "\nTest 8.2: Removing non-existent directory 'nope' (should fail)" << endl;
    fs.rmdir("nope");

    // Test 8.3: Try to remove a non-empty directory (should fail)
    cout << "\nTest 8.3: Removing non-empty directory 'testdir' (should fail)" << endl;
    fs.rmdir("testdir");

    // Test 8.4: Remove a nested empty directory (should succeed)
    cout << "\nTest 8.4: Removing nested empty directory 'deep' (should succeed)" << endl;
    fs.cd("testdir");
    fs.cd("nested");
    fs.rmdir("deep");
    fs.home();

    // Test 9: create and ls functionality
    cout << "\nTest 9: Testing create and ls functionality" << endl;

    // Test 9.1: Create files in home directory
    cout << "\nTest 9.1: Creating files 'file1', 'file2', 'file3' in home directory" << endl;
    fs.create("file1");
    fs.create("file2");
    fs.create("file3");

    // Test 9.2: List contents of home directory
    cout << "\nTest 9.2: Listing contents of home directory" << endl;
    fs.ls();

    // Test 9.3: Attempt to create a file with duplicate name (should fail)" << endl;
    fs.create("file1");

    // Test 9.4: Attempt to create a file with a name that's too long (should fail)" << endl;
    fs.create("thisfilenameistoolong");

    // Test 9.5: Create and list in a nested directory
    cout << "\nTest 9.5: Creating and listing in nested directory 'testdir/nested'" << endl;
    fs.cd("testdir");
    fs.cd("nested");
    fs.create("nestedfile");
    fs.ls();
    fs.home();

    // Test 10: rm functionality
    cout << "\nTest 10: Testing rm functionality" << endl;

    // Test 10.1: Remove an existing file (should succeed)
    cout << "\nTest 10.1: Removing file 'file1' (should succeed)" << endl;
    fs.rm("file1");

    // Test 10.2: Attempt to remove a non-existent file (should fail)
    cout << "\nTest 10.2: Removing non-existent file 'nopefile' (should fail)" << endl;
    fs.rm("nopefile");

    // Test 10.3: Attempt to remove a directory using rm (should fail)
    cout << "\nTest 10.3: Removing directory 'testdir' using rm (should fail)" << endl;
    fs.rm("testdir");

    // Test 10.4: List contents after removals
    cout << "\nTest 10.4: Listing contents after removals" << endl;
    fs.ls();

    // Test 11: append functionality
    cout << "\nTest 11: Testing append functionality" << endl;

    // Test 11.1: Append data to an existing file
    cout << "\nTest 11.1: Appending data to 'file2'" << endl;
    fs.append("file2", "Hello, World!");

    // Test 11.2: Attempt to append to a non-existent file
    cout << "\nTest 11.2: Appending to non-existent file 'nopefile'" << endl;
    fs.append("nopefile", "This should fail");

    // Test 11.3: Attempt to append to a directory
    cout << "\nTest 11.3: Appending to directory 'testdir'" << endl;
    fs.append("testdir", "This should fail");

    // Test 11.4: Append data that spans multiple blocks
    cout << "\nTest 11.4: Appending large data to 'file3'" << endl;
    char large_data[256];
    for (int i = 0; i < 255; i++) {
        large_data[i] = 'A';
    }
    large_data[255] = '\0';
    fs.append("file3", large_data);

    // Test 11.5: List contents after appending
    cout << "\nTest 11.5: Listing contents after appending" << endl;
    fs.ls();

    // Test 12: stat, cat, and head functionality
    cout << "\nTest 12: Testing stat, cat, and head functionality" << endl;

    // Test 12.1: Get stats for a file
    cout << "\nTest 12.1: Getting stats for file 'file2'" << endl;
    fs.stat("file2");

    // Test 12.2: Get stats for a directory
    cout << "\nTest 12.2: Getting stats for directory 'testdir'" << endl;
    fs.stat("testdir");

    // Test 12.3: Get stats for non-existent entry
    cout << "\nTest 12.3: Getting stats for non-existent entry 'nope'" << endl;
    fs.stat("nope");

    // Test 12.4: Display contents of a file
    cout << "\nTest 12.4: Displaying contents of file 'file2'" << endl;
    fs.cat("file2");

    // Test 12.5: Attempt to cat a non-existent file
    cout << "\nTest 12.5: Attempting to cat non-existent file 'nope'" << endl;
    fs.cat("nope");

    // Test 12.6: Attempt to cat a directory
    cout << "\nTest 12.6: Attempting to cat directory 'testdir'" << endl;
    fs.cat("testdir");

    // Test 12.7: Display first 5 bytes of a file
    cout << "\nTest 12.7: Displaying first 5 bytes of file 'file2'" << endl;
    fs.head("file2", 5);

    // Test 12.8: Display first 100 bytes of a file (more than file size)
    cout << "\nTest 12.8: Displaying first 100 bytes of file 'file2'" << endl;
    fs.head("file2", 100);

    // Test 12.9: Attempt to head a non-existent file
    cout << "\nTest 12.9: Attempting to head non-existent file 'nope'" << endl;
    fs.head("nope", 10);

    // Unmount the file system
    fs.unmount();
    
    return 0;
}
