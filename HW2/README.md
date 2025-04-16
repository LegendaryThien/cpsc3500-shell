# CPSC3500 HW2 - Shell Implementation

## Overview
This project implements a simple shell in C++ that supports command execution and pipe operations. The shell can handle single commands, as well as multiple commands connected by pipes.

## Features
- Command parsing with support for arguments
- Support for single command execution
- Support for pipe operations (one or more pipes)
- Error handling for invalid commands and tokens
- Resource management for pipes and processes

## Building
To build the shell, use the following command:
```bash
make
```

## Running
To run the shell:
```bash
./myshell
```

## Implementation Details
The shell implements the following functionality:
- Command line parsing with token limits
- Single command execution using fork() and execvp()
- Pipe operations using pipe(), fork(), and dup()
- Resource management for file descriptors and memory

## Strengths
- Clean implementation of pipe operations
- Proper handling of file descriptors in pipe operations
- Good error handling for token length and count
- Efficient command parsing using strtok_r()

## Weaknesses
- Limited built-in command support
- No support for input/output redirection
- No support for background processes
- Memory management could be improved

## Author
Thien Nguyen 