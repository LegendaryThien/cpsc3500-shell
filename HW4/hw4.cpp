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
pthread_mutex_t road_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

// Global variables
bool flagger_awake = false;
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
    CarInfo* car = nullptr;
    try {
        car = static_cast<CarInfo*>(arg);
        if (!car) {
            throw std::runtime_error("Invalid car info pointer");
        }
        
        // Record arrival time
        car->arrivalTime = time(nullptr);
        
        // Car arrives
        printf("Car %d arrived from N\n", car->carID);
        
        check_pthread(pthread_mutex_lock(&road_mutex), "Failed to lock road mutex");
        cars_waiting_north++;
        flagger_awake = true;
        check_pthread(pthread_mutex_unlock(&road_mutex), "Failed to unlock road mutex");
        
        // Busy wait for permission to proceed
        while (true) {
            check_pthread(pthread_mutex_lock(&road_mutex), "Failed to lock road mutex");
            if (current_direction == 'N') {
                break;
            }
            check_pthread(pthread_mutex_unlock(&road_mutex), "Failed to unlock road mutex");
            pthread_sleep(1);
        }
        
        // Record start time
        car->startTime = time(nullptr);
        
        // Car passes
        printf("Car %d passing from N\n", car->carID);
        sleep(1);
        
        // Record end time
        car->endTime = time(nullptr);
        
        // Log car event
        check_pthread(pthread_mutex_lock(&log_mutex), "Failed to lock log mutex");
        car_log << car->carID << " N "
                << getCurrentTime() << " "
                << getCurrentTime() << " "
                << getCurrentTime() << std::endl;
        check_pthread(pthread_mutex_unlock(&log_mutex), "Failed to unlock log mutex");
        
        // Decrement waiting cars count
        cars_waiting_north--;
        total_cars_passed++;
        
        check_pthread(pthread_mutex_unlock(&road_mutex), "Failed to unlock road mutex");
    } catch (const std::exception& e) {
        fprintf(stderr, "Error in north_car_thread: %s\n", e.what());
    }
    
    pthread_exit(NULL);
}

// South-bound car thread function
void* south_car_thread(void* arg) {
    CarInfo* car = (CarInfo*)arg;
    
    // Record arrival time
    car->arrivalTime = time(nullptr);
    
    // Car arrives
    printf("Car %d arrived from S\n", car->carID);
    
    pthread_mutex_lock(&road_mutex);
    cars_waiting_south++;
    flagger_awake = true;
    pthread_mutex_unlock(&road_mutex);
    
    // Busy wait for permission to proceed
    while (true) {
        pthread_mutex_lock(&road_mutex);
        if (current_direction == 'S') {
            break;
        }
        pthread_mutex_unlock(&road_mutex);
        pthread_sleep(1); // Sleep for 1 second between checks
    }
    
    // Record start time
    car->startTime = time(nullptr);
    
    // Car passes
    printf("Car %d passing from S\n", car->carID);
    sleep(1); // Simulate time to pass
    
    // Record end time
    car->endTime = time(nullptr);
    
    // Log car event
    pthread_mutex_lock(&log_mutex);
    car_log << car->carID << " S "
            << getCurrentTime() << " "
            << getCurrentTime() << " "
            << getCurrentTime() << std::endl;
    pthread_mutex_unlock(&log_mutex);
    
    // Decrement waiting cars count
    cars_waiting_south--;
    total_cars_passed++;
    
    pthread_mutex_unlock(&road_mutex);
    pthread_exit(NULL);
}

// Flagger thread function
void* flagger_thread(void* arg) {
    while (program_running) {
        pthread_mutex_lock(&road_mutex);
        
        // Sleep if no cars waiting
        if (cars_waiting_north == 0 && cars_waiting_south == 0) {
            flagger_awake = false;
            printf("Flagger is sleeping\n");
            
            // Log sleep event
            pthread_mutex_lock(&log_mutex);
            flagger_log << getCurrentTime() << " sleep" << std::endl;
            pthread_mutex_unlock(&log_mutex);
            
            pthread_mutex_unlock(&road_mutex);
            
            // Busy wait for cars
            while (!flagger_awake && program_running) {
                pthread_sleep(1); // Sleep for 1 second between checks
            }
            
            if (!program_running) {
                pthread_exit(NULL);
            }
            
            pthread_mutex_lock(&road_mutex);
            printf("Flagger woke up\n");
            
            // Log wake up event
            pthread_mutex_lock(&log_mutex);
            flagger_log << getCurrentTime() << " woken-up" << std::endl;
            pthread_mutex_unlock(&log_mutex);
        }
        
        // Determine which direction to allow
        if (cars_waiting_north > 0 && cars_waiting_south > 0) {
            // Switch direction if cars waiting in both directions
            current_direction = (current_direction == 'N') ? 'S' : 'N';
        } else if (cars_waiting_north > 0) {
            current_direction = 'N';
        } else {
            current_direction = 'S';
        }
        
        printf("Flagger allowing traffic from %c\n", current_direction);
        
        pthread_mutex_unlock(&road_mutex);
        pthread_sleep(2); // Give time for cars to pass
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
                new_car->direction = (car_infos.back()->direction == 'N') ? 'S' : 'N';
                
                pthread_t new_car_thread;
                check_pthread(pthread_create(&new_car_thread, NULL, 
                    (new_car->direction == 'N') ? north_car_thread : south_car_thread, 
                    new_car), "Failed to create car thread");
                
                car_threads.push_back(new_car_thread);
                car_infos.push_back(new_car);
            }
            
            if (cars_waiting_north == 0 && cars_waiting_south == 0) {
                pthread_sleep(5);
            } else {
                pthread_sleep(1);
            }
        }
        
        // Wait for all car threads to complete
        for (size_t i = 0; i < car_threads.size(); i++) {
            check_pthread(pthread_join(car_threads[i], NULL), "Failed to join car thread");
            delete car_infos[i];
        }
        
        // Signal flagger to exit
        check_pthread(pthread_mutex_lock(&road_mutex), "Failed to lock road mutex");
        program_running = false;
        flagger_awake = true;
        check_pthread(pthread_mutex_unlock(&road_mutex), "Failed to unlock road mutex");
        
        // Wait for flagger thread to complete
        check_pthread(pthread_join(flagger, NULL), "Failed to join flagger thread");
        
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

