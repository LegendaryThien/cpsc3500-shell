Thien Nguyen  CPSC3500 HW2 - My Shell

This project implements a simple shell in C++ that supports command execution and pipe operations. The shell can handle single commands, as well as multiple commands connected by pipes.

Strength:
- Clean implementation of pipe operations
- Proper handling of file descriptors in pipe operations
- Good error handling for token length and count
- Efficient command parsing using strtok_r()

Weakness:
- Limited built-in command support
- No support for input/output redirection
- No support for background processes
- Memory management could be improved

