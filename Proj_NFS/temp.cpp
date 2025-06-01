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
    
    // Unmount the file system
    fs.unmount();
    
    return 0;
}
