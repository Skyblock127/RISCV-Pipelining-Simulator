# RISC-V Pipeline Simulator

## Overview
This project implements a **RISC-V processor pipeline simulator** in C++, with two versions:
1. `noforwarding.cpp` - Implements the pipeline **without forwarding**, introducing stalls when required.
2. `forwarding.cpp` - Implements the pipeline **with forwarding**, reducing stalls by forwarding data between pipeline stages.

## Features
- **Five-stage pipeline:** Instruction Fetch (IF), Instruction Decode (ID), Execute (EXE), Memory Access (MEM), Write Back (WB).
- **Hazard Detection:** Handles data hazards by stalling (in `noforwarding.cpp`) or by forwarding (in `forwarding.cpp`).
- **Basic Instruction Support:** Supports a subset of RISC-V instructions, including arithmetic and memory operations.
- **Makefile Support:** Automates compilation using the provided `makefile`.

## Requirements
- A C++ compiler (GCC or Clang recommended)
- `make` utility (for Linux/macOS users)

## Installation & Compilation
To compile the simulator, use the provided `makefile`:
```bash
make
```
This will generate two executables:
- `noforwarding`
- `forwarding`

To clean up compiled files, use:
```bash
make clean
```

## Usage
Run the simulator with an input file containing RISC-V instructions:
```bash
./noforwarding <input_file>
```
```bash
./forwarding <input_file>
```

## Input Format
The input file should contain one RISC-V instruction per line. Example:
```
ADD x1, x2, x3
SUB x4, x1, x5
LW x6, 0(x1)
```

## Output Format
The output shows pipeline execution per cycle, for example:
```
ADD; IF; ID; EXE; MEM; WB;
SUB; ; IF; ID; EXE; MEM; WB;
```
- `;-;` represents stalls.
- `forwarding.cpp` should reduce stalls compared to `noforwarding.cpp`.

## Notes
- The current version **does not support branches or jumps**.
- Future improvements may include control hazard handling.
