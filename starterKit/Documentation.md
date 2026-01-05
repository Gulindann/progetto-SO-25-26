# PandOSsh Phase 1 - Implementation Notes

## 1. PCB Manager (pcb.c)
* **List Management:** The module utilizes the Linux-like list macros provided in `listx.h` (e.g., `list_for_each_entry`) for all queue manipulations, ensuring code consistency and type safety.
* **Safe Initialization:** The `allocPcb` function performs a complete reset of all PCB fields (pointers, processor state, statistics) to `0` or `NULL`. This guarantees that reused PCBs do not carry residual data from previous processes.

## 2. ASL Manager (asl.c)
* **Ordered List Strategy:** The Active Semaphore List (`semd_h`) is maintained **sorted by key** (`s_key`). This design choice satisfies the requirement for ordered insertion and enables early termination during search operations when the iterator exceeds the target key.
* **Internal Helper:** A `static` helper function, `getSemd`, centralizes the search logic. This prevents code duplication across `insertBlocked`, `removeBlocked`, and `headBlocked`.

## 3. General Notes
* **Visibility:** All internal data structures (`pcbFree_h`, `semd_h`) and helper functions are declared as `static` to restrict their scope to the definition file, preventing namespace pollution.
