register_busy = {f'x{i}': 0 for i in range(32)}

with open("input.txt", 'r') as f:
    instructions = [line.strip() for line in f if line.strip()]

number_of_instructions = len(instructions)

current_cycle = 1
cycle_of_prev_IF = 0
output_table = [[] for _ in range(number_of_instructions)]


def IF(i):
    global current_cycle, cycle_of_prev_IF
    cycle_of_prev_IF = current_cycle
    while i > 0 and current_cycle <= len(output_table[i-1]) and output_table[i-1][current_cycle-1] == "-":
        output_table[i].append(" ")
    output_table[i].append("IF")
    current_cycle += 1


def ID(instruction_hex, i):
    global current_cycle

    opcode_bin = bin(int(instruction_hex, 16))[2:].zfill(32)

    opcode_field = opcode_bin[-7:]
    
    result = {
        "type": "UNKNOWN",
        "input_registers": [],
        "output_register": None
    }
    
    if opcode_field == "0110011":
        rd  = int(opcode_bin[-12:-7], 2)
        rs1 = int(opcode_bin[-20:-15], 2)
        rs2 = int(opcode_bin[-25:-20], 2)
        result["type"] = "R"
        result["output_register"] = f"x{rd}"
        result["input_registers"] = [f"x{rs1}", f"x{rs2}"]

    elif opcode_field == "0010011":
        rd  = int(opcode_bin[-12:-7], 2)
        rs1 = int(opcode_bin[-20:-15], 2)
        result["type"] = "I"
        result["output_register"] = f"x{rd}"
        result["input_registers"] = [f"x{rs1}"]

    elif opcode_field == "0000011":
        rd  = int(opcode_bin[-12:-7], 2)
        rs1 = int(opcode_bin[-20:-15], 2)
        result["type"] = "LOAD"
        result["output_register"] = f"x{rd}"
        result["input_registers"] = [f"x{rs1}"]

    elif opcode_field == "0100011":
        rs1 = int(opcode_bin[-20:-15], 2)
        rs2 = int(opcode_bin[-25:-20], 2)
        result["type"] = "STORE"
        result["input_registers"] = [f"x{rs1}", f"x{rs2}"]

    elif opcode_field == "1100011":
        rs1 = int(opcode_bin[-20:-15], 2)
        rs2 = int(opcode_bin[-25:-20], 2)
        result["type"] = "BRANCH"
        result["input_registers"] = [f"x{rs1}", f"x{rs2}"]

    while i > 0 and current_cycle <= len(output_table[i-1]) and output_table[i-1][current_cycle-1] == "-":
        output_table[i].append("-")
    output_table[i].append("ID")
    current_cycle += 1
    return result


def EX(i):
    global current_cycle
    while i>0 and current_cycle <= len(output_table[i-1]) and output_table[i-1][current_cycle-1] == "-":
        output_table[i].append("-")
    output_table[i].append("EXE")
    current_cycle += 1


def MEM(i):
    global current_cycle
    while i>0 and current_cycle <= len(output_table[i-1]) and output_table[i-1][current_cycle-1] == "-":
        output_table[i].append("-")
    output_table[i].append("MEM")
    current_cycle += 1


def WB(i, inst_out_register):
    global register_busy, current_cycle
    output_table[i].append("WB")
    register_busy[inst_out_register] = current_cycle
    current_cycle += 1


def pipeline():
    global current_cycle, cycle_of_prev_IF
    for i in range(number_of_instructions):
        instruction = instructions[i]
        output_table[i].extend([" "] * cycle_of_prev_IF)
        current_cycle = cycle_of_prev_IF + 1
        
        IF(i)
        inst = ID(instruction, i)
        
        if inst["type"] == "R":
            while max(register_busy[inst["input_registers"][0]], register_busy[inst["input_registers"][1]]) >= current_cycle:
                output_table[i].append("-")
                current_cycle += 1        
            EX(i)
            MEM(i)
        
        elif inst["type"] == "I":
            while register_busy[inst["input_registers"][0]] >= current_cycle:
                output_table[i].append("-")
                current_cycle += 1
            EX(i)
            MEM(i)
        
        elif inst["type"] in ["LOAD", "STORE"]:
            EX(i)
            if len(inst["input_registers"]) == 2:
                while max(register_busy[inst["input_registers"][0]], register_busy[inst["input_registers"][1]]) >= current_cycle:
                    output_table[i].append("-")
                    current_cycle += 1
            else:
                while register_busy[inst["input_registers"][0]] >= current_cycle:
                    output_table[i].append("-")
                    current_cycle += 1        
            MEM(i)
        
        WB(i, inst["output_register"])        
        print(output_table[i])
    

pipeline()
