#include <iostream>
#include "FileSys.h"
#include "BasicFileSys.h"

using namespace std;

int main() {
    // Create file system instance
    FileSys fs;
    
    // Mount the file system (using -1 as socket since we're testing locally)
    fs.mount(-1);
    
    // Test creating some directories
    cout << "Testing mkdir command..." << endl;
    
    // Test 1: Create a valid directory
    cout << "\nTest 1: Creating directory 'testdir'" << endl;
    fs.mkdir("testdir");
    
    // Test 2: Create another directory
    cout << "\nTest 2: Creating directory 'docs'" << endl;
    fs.mkdir("docs");
    
    // Test 3: Try to create duplicate directory
    cout << "\nTest 3: Attempting to create duplicate directory 'testdir'" << endl;
    fs.mkdir("testdir");
    
    // Test 4: Try to create directory with long name
    cout << "\nTest 4: Attempting to create directory with long name" << endl;
    fs.mkdir("this_is_a_very_long_directory_name_that_should_fail");
    
    // Unmount the file system
    fs.unmount();
    
    return 0;
}
