# Traffic Management Simulation

## Team Members and Contributions
- Anh-Thien Nguyen - Implemented the core simulation logic, thread management, and synchronization

## Threads and Their Functions
1. Main Thread
   - Function: `main()`
   - Purpose: Initializes the simulation, creates the flagger thread, and manages car creation

2. Car Threads
   - Function: `north_car_thread()` and `south_car_thread()`
   - Purpose: Simulates individual cars passing through the construction zone
   - Each car thread handles its own passage through the construction zone

3. Flagger Thread
   - Function: `flagger_thread()`
   - Purpose: Manages traffic flow and coordinates car passage
   - Makes decisions about which direction to allow based on queue lengths
   - Sleeps when no cars are present

## Synchronization Primitives

### Semaphores
1. `constructionZone` (initial value: 1)
   - Purpose: Controls access to the construction zone
   - Binary semaphore ensuring only one car can be in the construction zone at a time

2. `northQueue` (initial value: 1)
   - Purpose: Protects access to the north queue counter
   - Ensures thread-safe access to cars_waiting_north

3. `southQueue` (initial value: 1)
   - Purpose: Protects access to the south queue counter
   - Ensures thread-safe access to cars_waiting_south

### Mutex Locks
1. `log_mutex`
   - Purpose: Protects access to log files (car.log and flagger.log)
   - Ensures thread-safe logging of car and flagger events

2. `counter_mutex`
   - Purpose: Protects access to the total_cars_passed counter
   - Ensures accurate counting of cars that have passed through

## Strengths
- Efficient use of semaphores to prevent deadlocks
- Clear separation of concerns between car threads and flagger thread
- Proper resource management with mutex locks
- Detailed logging system for debugging and analysis
- Thread-safe implementation of all shared resources
- Proper cleanup of resources in the main thread

## Weaknesses
- Limited scalability for more complex traffic patterns
- Fixed probability for car arrival (80%)
- No dynamic adjustment of traffic flow based on queue lengths
- Could benefit from more sophisticated traffic management algorithms
- No error handling for file I/O operations 