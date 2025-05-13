#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> // for sleep
#include <time.h>   // for time functions
#include <fstream>  // for file operations
#include <random>
#include <chrono>
#include <atomic>

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
pthread_cond_t north_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t south_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t flagger_cond = PTHREAD_COND_INITIALIZER;

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

// Car thread function
void* car_thread(void* arg) {
    CarInfo* car = (CarInfo*)arg;
    
    // Record arrival time
    car->arrivalTime = time(nullptr);
    
    // Car arrives
    printf("Car %d arrived from %c\n", car->carID, car->direction);
    
    pthread_mutex_lock(&road_mutex);
    
    // Increment waiting cars count
    if (car->direction == 'N') {
        cars_waiting_north++;
    } else {
        cars_waiting_south++;
    }
    
    // Wake up flagger if sleeping
    if (!flagger_awake) {
        flagger_awake = true;
        pthread_cond_signal(&flagger_cond);
    }
    
    // Wait for permission to proceed
    while (current_direction != car->direction) {
        if (car->direction == 'N') {
            pthread_cond_wait(&north_cond, &road_mutex);
        } else {
            pthread_cond_wait(&south_cond, &road_mutex);
        }
    }
    
    // Record start time
    car->startTime = time(nullptr);
    
    // Car passes
    printf("Car %d passing from %c\n", car->carID, car->direction);
    sleep(1); // Simulate time to pass
    
    // Record end time
    car->endTime = time(nullptr);
    
    // Log car event
    pthread_mutex_lock(&log_mutex);
    car_log << car->carID << " " 
            << car->direction << " "
            << getCurrentTime() << " "
            << getCurrentTime() << " "
            << getCurrentTime() << std::endl;
    pthread_mutex_unlock(&log_mutex);
    
    // Decrement waiting cars count
    if (car->direction == 'N') {
        cars_waiting_north--;
    } else {
        cars_waiting_south--;
    }
    
    // Increment total cars passed
    total_cars_passed++;
    
    pthread_mutex_unlock(&road_mutex);
    pthread_exit(NULL);
}

// Flagger thread function
void* flagger_thread(void* arg) {
    
    while (program_running) {  // Changed condition to check program_running
        pthread_mutex_lock(&road_mutex);
        
        // Sleep if no cars waiting
        while (cars_waiting_north == 0 && cars_waiting_south == 0 && program_running) {  // Added program_running check
            flagger_awake = false;
            printf("Flagger is sleeping\n");
            
            // Log sleep event
            pthread_mutex_lock(&log_mutex);
            flagger_log << getCurrentTime() << " sleep" << std::endl;
            pthread_mutex_unlock(&log_mutex);
            
            pthread_cond_wait(&flagger_cond, &road_mutex);
            if (!program_running) {  // Check if we should exit
                pthread_mutex_unlock(&road_mutex);
                pthread_exit(NULL);
            }
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
        
        // Signal waiting cars in the current direction
        if (current_direction == 'N') {
            pthread_cond_broadcast(&north_cond);
        } else {
            pthread_cond_broadcast(&south_cond);
        }
        
        pthread_mutex_unlock(&road_mutex);
        sleep(2); // Give time for cars to pass
    }
    
    pthread_exit(NULL);
}

bool shouldCreateNextCar() {
    return dist(rng) < 0.8;  // 80% chance
}

// Add this function before main()
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

int main(int argc, char* argv[]) {
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
    std::vector<pthread_t> car_threads;  // Dynamic array for car threads
    std::vector<CarInfo*> car_infos;     // Dynamic array for car info
    
    // Initialize log files with headers
    initialize_logs();
    
    // Create flagger thread
    if (pthread_create(&flagger, NULL, flagger_thread, NULL) != 0) {
        perror("Failed to create flagger thread");
        return 1;
    }
    
    // Create first car
    CarInfo* first_car = new CarInfo();
    first_car->carID = next_car_id++;
    first_car->direction = 'N';  // Start with North direction
    
    pthread_t first_car_thread;
    if (pthread_create(&first_car_thread, NULL, car_thread, first_car) != 0) {
        perror("Failed to create first car thread");
        return 1;
    }
    
    car_threads.push_back(first_car_thread);
    car_infos.push_back(first_car);
    
    // Keep creating cars with 80% probability until we reach target
    while (total_cars_passed < target_cars) {
        if (shouldCreateNextCar()) {
            CarInfo* new_car = new CarInfo();
            new_car->carID = next_car_id++;
            new_car->direction = (car_infos.back()->direction == 'N') ? 'S' : 'N';  // Alternate directions
            
            pthread_t new_car_thread;
            if (pthread_create(&new_car_thread, NULL, car_thread, new_car) != 0) {
                perror("Failed to create car thread");
                break;
            }
            
            car_threads.push_back(new_car_thread);
            car_infos.push_back(new_car);
        }
        
        pthread_sleep(1);  // Wait a second before deciding on next car
    }
    
    // Wait for all car threads to complete
    for (size_t i = 0; i < car_threads.size(); i++) {
        pthread_join(car_threads[i], NULL);
        delete car_infos[i];  // Clean up car info
    }
    
    // Signal flagger to exit
    pthread_mutex_lock(&road_mutex);
    program_running = false;
    pthread_cond_signal(&flagger_cond);  // Wake up flagger to check program_running
    pthread_mutex_unlock(&road_mutex);
    
    // Wait for flagger thread to complete
    pthread_join(flagger, NULL);
    
    // Close and flush log files
    car_log.flush();
    flagger_log.flush();
    car_log.close();
    flagger_log.close();
    
    return 0;
}
