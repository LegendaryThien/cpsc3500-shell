#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> // for sleep
#include <time.h>   // for time functions
#include <fstream>  // for file operations
#include <random>
#include <chrono>
#include <atomic>
#include <vector>
#include <stdexcept>
#include <semaphore.h>

// Helper function for thread sleep
int pthread_sleep(int seconds) {
    pthread_mutex_t mutex;
    pthread_cond_t conditionvar;
    struct timespec timetoexpire;
    
    if(pthread_mutex_init(&mutex,NULL)) {
        return -1;
    }
    if(pthread_cond_init(&conditionvar,NULL)) {
        return -1;
    }
    
    //When to expire is an absolute time, so get the current time and add
    //it to our delay time
    timetoexpire.tv_sec = (unsigned int)time(NULL) + seconds;
    timetoexpire.tv_nsec = 0;
    
    return pthread_cond_timedwait(&conditionvar, &mutex, &timetoexpire);
}

// Example struct for car info
struct CarInfo {
    int carID;
    char direction; // 'N' or 'S'
    time_t arrivalTime;
    time_t startTime;
    time_t endTime;
};

// Synchronization primitives
sem_t constructionZone;  // Controls access to construction zone
sem_t northQueue;       // Controls access to north queue
sem_t southQueue;       // Controls access to south queue
pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t counter_mutex = PTHREAD_MUTEX_INITIALIZER;

// Global variables
bool program_running = true;  // New flag to control program execution
char current_direction = 'N';  // Default direction
int cars_waiting_north = 0;
int cars_waiting_south = 0;
std::ofstream car_log("car.log");
std::ofstream flagger_log("flagger.log");
std::mt19937 rng(std::chrono::steady_clock::now().time_since_epoch().count());
std::uniform_real_distribution<double> dist(0.0, 1.0);
std::atomic<int> next_car_id(1);  // For generating unique car IDs
std::atomic<int> total_cars_passed(0);  // Track total cars that have passed through

// Initialize log headers
void initialize_logs() {
    pthread_mutex_lock(&log_mutex);
    car_log << "carID direction arrival-time start-time end-time" << std::endl;
    flagger_log << "Time State" << std::endl;
    pthread_mutex_unlock(&log_mutex);
}

// Helper function to get current time as string
std::string getCurrentTime() {
    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);
    char buffer[9];
    strftime(buffer, sizeof(buffer), "%H:%M:%S", timeinfo);
    return std::string(buffer);
}

// Helper function to check pthread operations
void check_pthread(int rc, const char* msg) {
    if (rc != 0) {
        fprintf(stderr, "Error: %s (error code: %d)\n", msg, rc);
        throw std::runtime_error(msg);
    }
}

// North-bound car thread function
void* north_car_thread(void* arg) {
    CarInfo* car = static_cast<CarInfo*>(arg);
    if (!car) {
        throw std::runtime_error("Invalid car info pointer");
    }
    
    // Record arrival time
    car->arrivalTime = time(nullptr);
    printf("Car %d arrived from N\n", car->carID);
    
    // Increment north queue counter
    sem_wait(&northQueue);
    cars_waiting_north++;
    sem_post(&northQueue);
    
    // Wait for permission to enter construction zone
    sem_wait(&constructionZone);
    
    // Record start time
    car->startTime = time(nullptr);
    printf("Car %d passing from N\n", car->carID);
    
    // Simulate passage through construction zone
    pthread_sleep(1);
    
    // Record end time
    car->endTime = time(nullptr);
    
    // Log car event
    pthread_mutex_lock(&log_mutex);
    car_log << car->carID << " N "
            << getCurrentTime() << " "
            << getCurrentTime() << " "
            << getCurrentTime() << std::endl;
    car_log.flush();
    pthread_mutex_unlock(&log_mutex);
    
    // Decrement counters
    sem_wait(&northQueue);
    cars_waiting_north--;
    sem_post(&northQueue);
    
    pthread_mutex_lock(&counter_mutex);
    total_cars_passed++;
    pthread_mutex_unlock(&counter_mutex);
    
    // Release construction zone
    sem_post(&constructionZone);
    
    pthread_exit(NULL);
}

// South-bound car thread function
void* south_car_thread(void* arg) {
    CarInfo* car = static_cast<CarInfo*>(arg);
    if (!car) {
        throw std::runtime_error("Invalid car info pointer");
    }
    
    // Record arrival time
    car->arrivalTime = time(nullptr);
    printf("Car %d arrived from S\n", car->carID);
    
    // Increment south queue counter
    sem_wait(&southQueue);
    cars_waiting_south++;
    sem_post(&southQueue);
    
    // Wait for permission to enter construction zone
    sem_wait(&constructionZone);
    
    // Record start time
    car->startTime = time(nullptr);
    printf("Car %d passing from S\n", car->carID);
    
    // Simulate passage through construction zone
    pthread_sleep(1);
    
    // Record end time
    car->endTime = time(nullptr);
    
    // Log car event
    pthread_mutex_lock(&log_mutex);
    car_log << car->carID << " S "
            << getCurrentTime() << " "
            << getCurrentTime() << " "
            << getCurrentTime() << std::endl;
    car_log.flush();
    pthread_mutex_unlock(&log_mutex);
    
    // Decrement counters
    sem_wait(&southQueue);
    cars_waiting_south--;
    sem_post(&southQueue);
    
    pthread_mutex_lock(&counter_mutex);
    total_cars_passed++;
    pthread_mutex_unlock(&counter_mutex);
    
    // Release construction zone
    sem_post(&constructionZone);
    
    pthread_exit(NULL);
}

// Flagger thread function
void* flagger_thread(void* arg) {
    while (program_running) {
        // Check if there are cars waiting
        sem_wait(&northQueue);
        sem_wait(&southQueue);
        bool has_cars = (cars_waiting_north > 0 || cars_waiting_south > 0);
        sem_post(&southQueue);
        sem_post(&northQueue);
        
        if (!has_cars) {
            // Log sleep event
            pthread_mutex_lock(&log_mutex);
            flagger_log << getCurrentTime() << " sleep" << std::endl;
            flagger_log.flush();
            pthread_mutex_unlock(&log_mutex);
            
            printf("Flagger is sleeping\n");
            pthread_sleep(5);
            continue;
        }
        
        // Log wake up event
        pthread_mutex_lock(&log_mutex);
        flagger_log << getCurrentTime() << " woken-up" << std::endl;
        flagger_log.flush();
        pthread_mutex_unlock(&log_mutex);
        
        printf("Flagger woke up\n");
        
        // Determine which direction to allow
        sem_wait(&northQueue);
        sem_wait(&southQueue);
        
        if (cars_waiting_north >= 10) {
            current_direction = 'N';
        } else if (cars_waiting_south >= 10) {
            current_direction = 'S';
        } else if (cars_waiting_north > 0 && cars_waiting_south > 0) {
            current_direction = (current_direction == 'N') ? 'S' : 'N';
        } else if (cars_waiting_north > 0) {
            current_direction = 'N';
        } else {
            current_direction = 'S';
        }
        
        sem_post(&southQueue);
        sem_post(&northQueue);
        
        printf("Flagger allowing traffic from %c\n", current_direction);
        
        // Signal cars in the current direction to proceed
        if (current_direction == 'N') {
            sem_post(&constructionZone);
            pthread_sleep(2);  // Allow time for cars to pass
            sem_wait(&constructionZone);
        } else {
            sem_post(&constructionZone);
            pthread_sleep(2);  // Allow time for cars to pass
            sem_wait(&constructionZone);
        }

        // Log going back to sleep
        pthread_mutex_lock(&log_mutex);
        flagger_log << getCurrentTime() << " sleep" << std::endl;
        flagger_log.flush();
        pthread_mutex_unlock(&log_mutex);
        
        printf("Flagger going back to sleep\n");
        pthread_sleep(5);
    }
    
    pthread_exit(NULL);
}

bool shouldCreateNextCar() {
    return dist(rng) < 0.8;  // 80% chance
}

int main(int argc, char* argv[]) {
    try {
        // Check for command line argument
        if (argc != 2) {
            printf("Usage: %s <number_of_cars>\n", argv[0]);
            return 1;
        }
        
        int target_cars = atoi(argv[1]);
        if (target_cars <= 0) {
            printf("Number of cars must be positive\n");
            return 1;
        }
        
        pthread_t flagger;
        std::vector<pthread_t> car_threads;
        std::vector<CarInfo*> car_infos;
        
        // Initialize log files with headers
        initialize_logs();
        
        // Initialize semaphores
        sem_init(&constructionZone, 0, 1);  // Binary semaphore for construction zone
        sem_init(&northQueue, 0, 1);        // Binary semaphore for north queue
        sem_init(&southQueue, 0, 1);        // Binary semaphore for south queue
        
        // Create flagger thread
        check_pthread(pthread_create(&flagger, NULL, flagger_thread, NULL),
                     "Failed to create flagger thread");
        
        // Create first car
        CarInfo* first_car = new (std::nothrow) CarInfo();
        if (!first_car) {
            throw std::runtime_error("Failed to allocate memory for first car");
        }
        
        first_car->carID = next_car_id++;
        first_car->direction = 'N';
        
        pthread_t first_car_thread;
        check_pthread(pthread_create(&first_car_thread, NULL, 
            (first_car->direction == 'N') ? north_car_thread : south_car_thread, 
            first_car), "Failed to create first car thread");
        
        car_threads.push_back(first_car_thread);
        car_infos.push_back(first_car);
        
        // Keep creating cars with 80% probability until we reach target
        while (total_cars_passed < target_cars) {
            if (shouldCreateNextCar()) {
                CarInfo* new_car = new (std::nothrow) CarInfo();
                if (!new_car) {
                    throw std::runtime_error("Failed to allocate memory for new car");
                }
                
                new_car->carID = next_car_id++;
                // Randomly assign direction with 50% probability for each
                new_car->direction = (dist(rng) < 0.5) ? 'N' : 'S';
                
                pthread_t new_car_thread;
                check_pthread(pthread_create(&new_car_thread, NULL, 
                    (new_car->direction == 'N') ? north_car_thread : south_car_thread, 
                    new_car), "Failed to create car thread");
                
                car_threads.push_back(new_car_thread);
                car_infos.push_back(new_car);
            }
            
            pthread_sleep(1);
        }
        
        // Wait for all car threads to complete
        for (size_t i = 0; i < car_threads.size(); i++) {
            check_pthread(pthread_join(car_threads[i], NULL), "Failed to join car thread");
            delete car_infos[i];
        }
        
        // Signal flagger to exit
        program_running = false;
        
        // Wait for flagger thread to complete
        check_pthread(pthread_join(flagger, NULL), "Failed to join flagger thread");
        
        // Clean up semaphores
        sem_destroy(&constructionZone);
        sem_destroy(&northQueue);
        sem_destroy(&southQueue);
        
        // Close and flush log files
        car_log.flush();
        flagger_log.flush();
        car_log.close();
        flagger_log.close();
        
    } catch (const std::exception& e) {
        fprintf(stderr, "Fatal error: %s\n", e.what());
        return 1;
    }
    
    return 0;
}

