## Team Members
* Riccardo Gadani
* Walid Birdaha
* Alexander Kapllanaj
* Lorenzo Novi

# PandOSsh - Phase 1

## Project Description
This project implements **Level 2 (The Queues Manager)** of the PandOSsh operating system.
It provides the core data structures required for process management and synchronization:

* **PCB Manager (`pcb.c`):** Handles process allocation, priority queues, and the process tree structure.
* **ASL Manager (`asl.c`):** Handles the Active Semaphore List, managing blocked processes on semaphores.

The implementation targets the **µRISCV** architecture.

## Files
* `phase1/pcb.c`: Source code for PCB management.
* `phase1/asl.c`: Source code for Semaphore management.
* `p1test.c`: The test suite provided to verify the implementation.
* `headers/`: Header files (`pcb.h`, `asl.h`, `types.h`, `listx.h`, `const.h`).

# PandOSsh - Phase 2

## Project Description
This project implements **Level 3 (The Nucleus)** of the PandOSsh operating system.
It builds upon the Level 2 data structures to provide:

* **Exception Handling:** Centralized management of interrupts, syscalls, TLB, and traps.
* **Process Scheduling:** A preemptive Round-Robin scheduler with time-slicing (5ms).
* **System Services (SYSCALLs):** 10 core kernel services for process creation, termination, and I/O synchronization.
* **CPU Time Accounting:** Precision tracking of CPU usage for each process.

The implementation targets the **µRISCV** architecture.

## Files 
* `initial.c`: System boot, Level 2 data structure initialization, and first process execution.
* `scheduler.c`: The preemptive Round-Robin scheduler and CPU idle state (WAIT) management.
* `syscalls.c`: Implementation of the 10 Nucleus service calls (NSYS1-NSYS10).
* `interrupts.c`: Management of device interrupts and internal timers (PLT/Interval Timer).
* `exceptions.c`: Main entry point for all hardware exceptions and trap routing.
* `traps.c`: Logic for handling program traps and the "Pass Up or Die" mechanism.
* `tlb.c`: Handler for TLB management exceptions
* `utils.c`: Support utilities 
* `headers/`: Header files.

## Build Instructions
Prerequisites: `cmake` and the `riscv64-unknown-elf` toolchain must be installed.

   Compile the project:
   ```bash
   cmake -B build
   cmake --build build
   ```

## Running the Test
To run the test suite using the µRISCV emulator:

```bash
uriscv
```

The test output will appear in **Terminal 0**.
Upon successful completion, the system will display a "System Halted" message.
