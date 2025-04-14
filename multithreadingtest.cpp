#include <iostream>
#include <pthread.h>

void *thread_func(void *arg) { 
    int *p = (int *)arg; 
    int n = *p; 
    
    while (n--) { 
        std::cout << "Hello World " << *p << std::endl; 
    } 
    
    pthread_exit(NULL); 
} 

int main() { 
    pthread_t tids[10];
    int a[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10}; // Define array a
    
    for (int i = 0; i < 10; i++) { 
        pthread_create(&tids[i], NULL, thread_func, (void*)&a[i]); // Fixed syntax and passed address of array element
    } 
    
    // Removed pthread_join calls
    
    std::cout << "Main thread exiting..." << std::endl;
    return 0; 
} 

/*
 it doesn't mean that all other threads have stopped running. In your code, you've created 10 threads using pthread_create, but you've removed the pthread_join calls, which means the main thread doesn't wait for these threads to finish before exiting.
*/