#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <fstream>
#include <random>
#include <chrono>
#include <atomic>
#include <vector>
#include <stdexcept>
#include <semaphore.h>
#include <stdarg.h>

#define DEBUG 0

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
    
    timetoexpire.tv_sec = (unsigned int)time(NULL) + seconds;
    timetoexpire.tv_nsec = 0;
    
    return pthread_cond_timedwait(&conditionvar, &mutex, &timetoexpire);
}

struct CarInfo {
    int carID;
    char direction;
    time_t arrivalTime;
    time_t startTime;
    time_t endTime;
};

sem_t constructionZone;
sem_t northQueue;
sem_t southQueue;
pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t counter_mutex = PTHREAD_MUTEX_INITIALIZER;

bool program_running = true;
char current_direction = 'N';
int cars_waiting_north = 0;
int cars_waiting_south = 0;
std::ofstream car_log("car.log");
std::ofstream flagger_log("flagger.log");
std::mt19937 rng(std::chrono::steady_clock::now().time_since_epoch().count());
std::uniform_real_distribution<double> dist(0.0, 1.0);
std::atomic<int> next_car_id(1);
std::atomic<int> total_cars_passed(0);

void initialize_logs() {
    pthread_mutex_lock(&log_mutex);
    if (!car_log.is_open() || !flagger_log.is_open()) {
        throw std::runtime_error("Failed to open log files");
    }
    car_log << "carID direction arrival-time start-time end-time" << std::endl;
    flagger_log << "Time State" << std::endl;
    car_log.flush();
    flagger_log.flush();
    pthread_mutex_unlock(&log_mutex);
}

std::string getCurrentTime() {
    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);
    char buffer[9];
    strftime(buffer, sizeof(buffer), "%H:%M:%S", timeinfo);
    return std::string(buffer);
}

void check_pthread(int rc, const char* msg) {
    if (rc != 0) {
        fprintf(stderr, "Error: %s (error code: %d)\n", msg, rc);
        throw std::runtime_error(msg);
    }
}

void* north_car_thread(void* arg) {
    CarInfo* car = static_cast<CarInfo*>(arg);
    if (!car) {
        throw std::runtime_error("Invalid car info pointer");
    }
    
    car->arrivalTime = time(nullptr);
    std::string arrival_time = getCurrentTime();
    
    sem_wait(&northQueue);
    cars_waiting_north++;
    sem_post(&northQueue);
    
    sem_wait(&constructionZone);
    
    car->startTime = time(nullptr);
    std::string start_time = getCurrentTime();
    
    pthread_sleep(1);
    
    car->endTime = time(nullptr);
    std::string end_time = getCurrentTime();
    
    pthread_mutex_lock(&log_mutex);
    if (!car_log.is_open()) {
        pthread_mutex_unlock(&log_mutex);
        throw std::runtime_error("Car log file is not open");
    }
    car_log << car->carID << " N "
            << arrival_time << " "
            << start_time << " "
            << end_time << std::endl;
    car_log.flush();
    pthread_mutex_unlock(&log_mutex);
    
    sem_wait(&northQueue);
    cars_waiting_north--;
    sem_post(&northQueue);
    
    pthread_mutex_lock(&counter_mutex);
    total_cars_passed++;
    pthread_mutex_unlock(&counter_mutex);
    
    sem_post(&constructionZone);
    
    pthread_exit(NULL);
}

void* south_car_thread(void* arg) {
    CarInfo* car = static_cast<CarInfo*>(arg);
    if (!car) {
        throw std::runtime_error("Invalid car info pointer");
    }
    
    car->arrivalTime = time(nullptr);
    std::string arrival_time = getCurrentTime();
    
    sem_wait(&southQueue);
    cars_waiting_south++;
    sem_post(&southQueue);
    
    sem_wait(&constructionZone);
    
    car->startTime = time(nullptr);
    std::string start_time = getCurrentTime();
    
    pthread_sleep(1);
    
    car->endTime = time(nullptr);
    std::string end_time = getCurrentTime();
    
    pthread_mutex_lock(&log_mutex);
    if (!car_log.is_open()) {
        pthread_mutex_unlock(&log_mutex);
        throw std::runtime_error("Car log file is not open");
    }
    car_log << car->carID << " S "
            << arrival_time << " "
            << start_time << " "
            << end_time << std::endl;
    car_log.flush();
    pthread_mutex_unlock(&log_mutex);
    
    sem_wait(&southQueue);
    cars_waiting_south--;
    sem_post(&southQueue);
    
    pthread_mutex_lock(&counter_mutex);
    total_cars_passed++;
    pthread_mutex_unlock(&counter_mutex);
    
    sem_post(&constructionZone);
    
    pthread_exit(NULL);
}

void* flagger_thread(void* arg) {
    while (program_running) {
        sem_wait(&northQueue);
        sem_wait(&southQueue);
        bool has_cars = (cars_waiting_north > 0 || cars_waiting_south > 0);
        sem_post(&southQueue);
        sem_post(&northQueue);
        
        if (!has_cars) {
            pthread_mutex_lock(&log_mutex);
            if (!flagger_log.is_open()) {
                pthread_mutex_unlock(&log_mutex);
                throw std::runtime_error("Flagger log file is not open");
            }
            flagger_log << getCurrentTime() << " sleep" << std::endl;
            flagger_log.flush();
            pthread_mutex_unlock(&log_mutex);
            
            pthread_sleep(5);
            continue;
        }
        
        pthread_mutex_lock(&log_mutex);
        if (!flagger_log.is_open()) {
            pthread_mutex_unlock(&log_mutex);
            throw std::runtime_error("Flagger log file is not open");
        }
        flagger_log << getCurrentTime() << " woken-up" << std::endl;
        flagger_log.flush();
        pthread_mutex_unlock(&log_mutex);
        
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
        
        if (current_direction == 'N') {
            sem_post(&constructionZone);
            pthread_sleep(2);
            sem_wait(&constructionZone);
        } else {
            sem_post(&constructionZone);
            pthread_sleep(2);
            sem_wait(&constructionZone);
        }

        pthread_mutex_lock(&log_mutex);
        flagger_log << getCurrentTime() << " sleep" << std::endl;
        flagger_log.flush();
        pthread_mutex_unlock(&log_mutex);
        
        pthread_sleep(5);
    }
    
    pthread_exit(NULL);
}

int main(int argc, char* argv[]) {
    try {
        if (argc != 2) {
            fprintf(stderr, "Usage: %s <number_of_cars>\n", argv[0]);
            return 1;
        }
        
        int target_cars = atoi(argv[1]);
        if (target_cars <= 0) {
            fprintf(stderr, "Number of cars must be positive\n");
            return 1;
        }
        
        pthread_t flagger;
        std::vector<pthread_t> car_threads;
        std::vector<CarInfo*> car_infos;
        
        initialize_logs();
        
        sem_init(&constructionZone, 0, 1);
        sem_init(&northQueue, 0, 1);
        sem_init(&southQueue, 0, 1);
        
        check_pthread(pthread_create(&flagger, NULL, flagger_thread, NULL),
                     "Failed to create flagger thread");
        
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
        
        bool in_burst = true;
        
        while (total_cars_passed < target_cars) {
            if (in_burst) {
                if (dist(rng) < 0.8) {
                    CarInfo* new_car = new (std::nothrow) CarInfo();
                    if (!new_car) {
                        throw std::runtime_error("Failed to allocate memory for new car");
                    }
                    
                    new_car->carID = next_car_id++;
                    new_car->direction = (dist(rng) < 0.5) ? 'N' : 'S';
                    
                    pthread_t new_car_thread;
                    check_pthread(pthread_create(&new_car_thread, NULL, 
                        (new_car->direction == 'N') ? north_car_thread : south_car_thread, 
                        new_car), "Failed to create car thread");
                    
                    car_threads.push_back(new_car_thread);
                    car_infos.push_back(new_car);
                } else {
                    in_burst = false;
                    pthread_sleep(5);
                    in_burst = true;
                }
            } else {
                pthread_sleep(5);
                in_burst = true;
            }
        }
        
        for (size_t i = 0; i < car_threads.size(); i++) {
            check_pthread(pthread_join(car_threads[i], NULL), "Failed to join car thread");
            delete car_infos[i];
        }
        
        program_running = false;
        
        check_pthread(pthread_join(flagger, NULL), "Failed to join flagger thread");
        
        sem_destroy(&constructionZone);
        sem_destroy(&northQueue);
        sem_destroy(&southQueue);
        
        if (car_log.is_open()) {
            car_log.flush();
            car_log.close();
        }
        if (flagger_log.is_open()) {
            flagger_log.flush();
            flagger_log.close();
        }
        
    } catch (const std::exception& e) {
        fprintf(stderr, "Fatal error: %s\n", e.what());
        return 1;
    }
    
    return 0;
}

