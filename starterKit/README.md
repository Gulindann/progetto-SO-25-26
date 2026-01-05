# PandOSsh - Phase 1

## Team Members
* Riccardo Gadani
* Walid Birdaha
* Alexander Kapllanaj
* Lorenzo Novi

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

## Build Instructions
Prerequisites: `cmake` and the `riscv64-unknown-elf` toolchain must be installed.

2. Compile the project:
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
