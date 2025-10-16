# Concurrent Packet Filter (CPF)

## Overview

The **Concurrent Packet Filter (CPF)** is a high-performance, multithreaded simulation of a firewall application designed to process network packets concurrently. The project focuses on converting a serial packet processing program into an efficient parallel one using the **POSIX Threads (Pthreads) API** and advanced synchronization mechanisms.

This implementation utilizes a **producer-consumer model** where a single producer thread generates packets into a shared circular buffer, and multiple consumer threads (the firewall logic) concurrently dequeue, process, and log the results. A key feature of this project is the implementation of a deterministic, concurrent logging mechanism that ensures log entries are always ordered by the packet's timestamp, despite non-deterministic consumer completion times.

## Core Objectives

* **Parallel Programming:** Design and implement a parallel program using the producer-consumer model.
* **Pthreads API Mastery:** Gain practical experience with the POSIX threading API, including thread creation, synchronization primitives (mutexes, condition variables, semaphores), and thread management.
* **Concurrency Control:** Implement a thread-safe **Circular Buffer (Ring Buffer)** for inter-thread communication, ensuring proper synchronization to prevent race conditions and deadlock.
* **Deterministic Logging:** Develop a synchronization mechanism to ensure the final log file is sorted by packet timestamp *during* concurrent processing, avoiding post-processing sorting.
* **Performance:** Eliminate busy waiting in consumer threads, utilizing condition variables for efficient notification of new data.

## Architecture and Implementation Details

### 1. Packet Flow

The system operates with a multi-threaded architecture:

* **Producer Thread:** Generates simulated network packets (consisting of a made up source, destination, timestamp, and payload) and enqueues them into the shared Ring Buffer.
* **Consumer Threads (Firewall Logic):** A configurable number of threads that dequeue packets, check them against the filter function, and decide whether to **PASS** or **DROP** the packet.

### 2. Synchronization and Ring Buffer

The shared data structure is a fixed-size **Circular Buffer**. Its thread-safe access is guaranteed by using Pthreads synchronization primitives:

* **Fields:** The structure includes `write_pos`, `read_pos`, `cap` (size), and a pointer to the internal `data` buffer.
* **Synchronization:** Mutexes, semaphores, and conditional variables must be added to allow multiple threads to access the ring buffer deterministically.
* **No Busy Waiting:** Consumer threads must be notified when new packets are available and must not busy wait or sleep in a `while()` loop.

The required Ring Buffer interface to be implemented is:

* `ring_buffer_init()`: Initialize the ring buffer (allocate memory and synchronization primitives).
* `ring_buffer_enqueue()`: Add elements to the ring buffer.
* `ring_buffer_dequeue()`: Remove elements from the ring buffer.
* `ring_buffer_destroy()`: Free up the memory used by the ring\_buffer.
* `ring_buffer_stop()`: Finish up using the ring buffer for the calling thread.

### 3. Sorted Concurrent Logging

The final log file must be sorted by the packet timestamp, which is the order they came in from the producer.

* Consumer threads will process packets in a non-deterministic order, but the log output must be written in ascending order during packet processing.
* **Crucial Constraint:** Sorting the log file after the consumer threads have finished processing is strictly prohibited and points will be deducted.
* The log format is: decision (PASS or DROP), packet hash, and timestamp (details can be found in `src/serial.c`).

## Building and Running

### Building

Navigate to the `src/` directory and use the provided `Makefile` to build both the serial and parallel versions:

```bash
cd src/
make
```

This will create the serial and firewall binaries.

# Running the Parallel Firewall
* The parallel binary (firewall) requires the input file path, the output * log file path, and the number of consumer threads as arguments.
The number of consumer threads is the 3rd command-line argument.

# General Syntax
```
./firewall <input_packet_file> <output_file> <number_of_consumers>
```

# Example
```
./firewall ../tests/in/test_1000.in logs/output.log 4
```


# Technologies
* Language: C
* Concurrency: POSIX Threads (Pthreads)
* Data Structure: Circular Buffer (Ring Buffer)
* Build System: Make
