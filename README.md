# Operating Systems Projects

## Overview
This repository contains three projects completed for the Operating Systems course, demonstrating key concepts in system programming, concurrency, and process management.

## Projects

### 1. Custom Shell (CustomShell.c)
**Description:** A custom shell implementation that mimics basic shell functionality.

**Key Features:**
- Built-in command support:
  - `exit`: Terminate the shell
  - `cd`: Change current directory
  - `path`: Modify executable search paths
- Parallel command execution using `&`
- Input/output redirection support
- Dynamic path management
- Error handling for various command scenarios

**Technical Highlights:**
- Uses `fork()` and `execv()` for command execution
- Implements custom path resolution mechanism
- Handles memory allocation and deallocation
- Supports multiple command parsing and execution strategies

### 2. Multithreaded File Compression (MutexLocks.c)
**Description:** A multithreaded file compression program designed to efficiently compress image frames.

**Key Features:**
- Parallel processing of image files using pthreads
- Uses zlib for compression
- Supports processing up to 8 files simultaneously
- Generates compressed video file
- Performance tracking with time and compression rate calculations

**Technical Highlights:**
- Uses mutex locks for thread-safe operations
- Dynamic thread creation based on file count
- Implements a worker thread model for file compression
- Sorts input files before processing
- Provides runtime and compression rate statistics

### 3. Producer-Consumer Threading Model (Multithreaded.c)
**Description:** A multithreaded program demonstrating the producer-consumer pattern using a circular buffer.

**Key Features:**
- Circular buffer implementation with fixed size (15 positions)
- User-interactive input processing
- Synchronization using mutex locks and condition variables
- Character-by-character buffer manipulation

**Technical Highlights:**
- Uses POSIX threads (pthreads)
- Implements thread synchronization primitives
- Demonstrates safe shared resource access
- Handles dynamic input processing
- Supports input truncation and exit mechanism

## Requirements
- GCC Compiler
- POSIX Threading Support
- zlib (for MutexLocks.c)

## Compilation Notes
- CustomShell: `gcc -o shell CustomShell.c`
- MutexLocks: `gcc -o compress MutexLocks.c -lz -lpthread`
- Multithreaded: `gcc -o producer_consumer Multithreaded.c -lpthread`

## Author
James Ocampo
Operating Systems Course Project
