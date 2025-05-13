#ifndef HW4_H
#define HW4_H

#include <pthread.h>
#include <semaphore.h>
#include <queue>
#include <string>
#include <fstream>
#include <mutex>
#include <atomic>

// Constants
const int MAX_CARS = 10;  
const int CONSTRUCTION_TIME = 1;  
const int GAP_TIME = 5;  

struct CarInfo {
    int carID;
    char direction;  // 'N' or 'S'
    time_t arrivalTime;
    time_t startTime;
    time_t endTime;
};

class RoadSystem {
private:
    // Synchronization primitives
    sem_t constructionZone;  
    sem_t flaggerSignal;    
    pthread_mutex_t queueMutex;  
    pthread_mutex_t logMutex;    
    pthread_mutex_t counterMutex;  

    // Threads
    pthread_t flaggerThread;
    pthread_t northProducerThread;
    pthread_t southProducerThread;

    // Data structures
    std::queue<CarInfo> northQueue;
    std::queue<CarInfo> southQueue;
    std::atomic<int> totalCarsPassed;
    int targetCars;
    bool isNorthActive;
    bool isSouthActive;

    // Log files
    std::ofstream carLog;
    std::ofstream flaggerLog;

    // Helper functions
    std::string getCurrentTime();
    void logCarEvent(const CarInfo& car);
    void logFlaggerEvent(const std::string& state);

    // Thread functions
    static void* car_thread(void* arg);
    static void* flagger_thread(void* arg);
    static void* north_producer(void* arg);
    static void* south_producer(void* arg);

public:
    RoadSystem(int targetCars);
    ~RoadSystem();

    void start();
    void waitForCompletion();
};

// pthread_sleep function
int pthread_sleep(int seconds);

#endif // HW4_H 