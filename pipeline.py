import sys

# Initialize register busy times
register_busy = {f'x{i}': 0 for i in range(32)}

with open("input.txt", 'r') as f:
    instructions = [line.strip() for line in f if line.strip()]

number_of_instructions = len(instructions)

# Global variables for cycle management
current_cycle = 1
cycle_of_prev_IF = 0
output_table = [[] for _ in range(number_of_instructions)]


def IF(i):
    global current_cycle, cycle_of_prev_IF
    cycle_of_prev_IF = current_cycle
    output_table[i].append("IF")
    current_cycle += 1


def ID(instruction_hex, i):
    global current_cycle
    # Convert hex to 32-bit binary string
    opcode_bin = bin(int(instruction_hex, 16))[2:].zfill(32)
    # Extract the 7-bit opcode field (usually the last 7 bits)
    opcode_field = opcode_bin[-7:]
    
    result = {
        "type": "UNKNOWN",
        "input_registers": [],
        "output_register": None
    }
    
    # R-type instructions (opcode field "0110011")
    if opcode_field == "0110011":
        rd  = int(opcode_bin[-12:-7], 2)
        rs1 = int(opcode_bin[-20:-15], 2)
        rs2 = int(opcode_bin[-25:-20], 2)
        result["type"] = "R"
        result["output_register"] = f"x{rd}"
        result["input_registers"] = [f"x{rs1}", f"x{rs2}"]
    
    # I-type instructions (ALU immediate) (opcode field "0010011")
    elif opcode_field == "0010011":
        rd  = int(opcode_bin[-12:-7], 2)
        rs1 = int(opcode_bin[-20:-15], 2)
        result["type"] = "I"
        result["output_register"] = f"x{rd}"
        result["input_registers"] = [f"x{rs1}"]
    
    # Load instructions (opcode field "0000011")
    elif opcode_field == "0000011":
        rd  = int(opcode_bin[-12:-7], 2)
        rs1 = int(opcode_bin[-20:-15], 2)
        result["type"] = "LOAD"
        result["output_register"] = f"x{rd}"
        result["input_registers"] = [f"x{rs1}"]
    
    # Store instructions (opcode field "0100011")
    elif opcode_field == "0100011":
        rs1 = int(opcode_bin[-20:-15], 2)
        rs2 = int(opcode_bin[-25:-20], 2)
        result["type"] = "STORE"
        result["input_registers"] = [f"x{rs1}", f"x{rs2}"]
    
    # Branch instructions (opcode field "1100011")
    elif opcode_field == "1100011":
        rs1 = int(opcode_bin[-20:-15], 2)
        rs2 = int(opcode_bin[-25:-20], 2)
        result["type"] = "BRANCH"
        result["input_registers"] = [f"x{rs1}", f"x{rs2}"]
    
    output_table[i].append("ID")
    current_cycle += 1
    return result


def EX(i):
    global current_cycle
    output_table[i].append("EXE")
    current_cycle += 1


def MEM(i):
    global current_cycle
    output_table[i].append("MEM")
    current_cycle += 1


def WB(instruction, i):
    global current_cycle
    output_table[i].append("WB")
    if instruction.get("output_register"):
        register_busy[instruction["output_register"]] = current_cycle
    current_cycle += 1


def pipeline():
    global current_cycle, cycle_of_prev_IF
    for i in range(number_of_instructions):
        instruction = instructions[i]
        # Align this instruction with previous IF cycle count
        output_table[i].extend([" "] * cycle_of_prev_IF)
        # Set current cycle to one more than the previous IF cycle (i.e. start scheduling this instruction)
        current_cycle = cycle_of_prev_IF + 1
        
        # Decode once and store result
        IF(i)
        inst = ID(instruction, i)
        
        # Hazard check and scheduling based on type
        if inst["type"] == "R":
            # Ensure both input registers are free
            while max(register_busy[inst["input_registers"][0]], register_busy[inst["input_registers"][1]]) > current_cycle:
                output_table[i].append("-")
                current_cycle += 1        
            EX(i)
            MEM(i)
        
        elif inst["type"] == "I":
            while register_busy[inst["input_registers"][0]] > current_cycle:
                output_table[i].append("-")
                current_cycle += 1
            EX(i)
            MEM(i)
        
        elif inst["type"] in ["LOAD", "STORE"]:
            EX(i)
            # For memory, if there are two registers, check both
            if len(inst["input_registers"]) == 2:
                while max(register_busy[inst["input_registers"][0]], register_busy[inst["input_registers"][1]]) > current_cycle:
                    output_table[i].append("-")
                    current_cycle += 1
            else:
                while register_busy[inst["input_registers"][0]] > current_cycle:
                    output_table[i].append("-")
                    current_cycle += 1        
            MEM(i)
        
        # Add WB stage for types that require it
        WB(inst, i)
        print(output_table[i])
        
        # Update cycle_of_prev_IF for the next instruction (example: IF was scheduled at the very beginning)
        cycle_of_prev_IF += 1  # For a simple pipeline, each instruction's IF starts one cycle later

pipeline()
