register_busy = {f'x{i}': 0 for i in range(32)}

with open("input.txt", 'r') as f:
    opcodes = [line.strip() for line in f if line.strip()]

number_of_opcodes = len(opcodes)
current_cycle = 1
cycle_of_prev_IF = 0
output_table = [[] for _ in range(number_of_opcodes)]

def IF(i):
    global current_cycle, cycle_of_prev_IF, output_table
    cycle_of_prev_IF = current_cycle
    output_table[i].append("IF")
    current_cycle += 1

def ID(opcode_hex, i):
    global current_cycle, output_table
    opcode_bin = bin(int(opcode_hex, 16))[2:].zfill(32)
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
    
    output_table[i].append("ID")
    current_cycle += 1
    return result

def EXE(i):
    global current_cycle, output_table
    output_table[i].append("EXE")
    current_cycle += 1

def MEM(i):
    global current_cycle, output_table
    output_table[i].append("MEM")
    current_cycle += 1

def WB(inst_op_reg, i):
    global current_cycle, register_busy, output_table
    output_table[i].append("WB")
    register_busy[inst_op_reg] = current_cycle

def pipeline():
    global current_cycle, cycle_of_prev_IF
    for i in range(number_of_opcodes):
        instruction = opcodes[i]
        output_table[i].extend([" "] * cycle_of_prev_IF)
        current_cycle = cycle_of_prev_IF + 1
        
        while i > 0 and current_cycle <= len(output_table[i-1]) and output_table[i-1][current_cycle-1] == "-":
            output_table[i].append(" ")
            current_cycle += 1    
        IF(i)
        
        while i > 0 and current_cycle <= len(output_table[i-1]) and output_table[i-1][current_cycle-1] == "-":
            output_table[i].append("-")
            current_cycle += 1   
        inst = ID(instruction, i)
        
        if inst["type"] in ["R", "I", "LOAD"]:
            while max(register_busy.get(r, 0) for r in inst["input_registers"] if r) >= current_cycle:
                output_table[i].append("-")
                current_cycle += 1
            EXE(i)
            MEM(i)
        elif inst["type"] == "STORE":
            while register_busy[inst["input_registers"][1]] >= current_cycle:
                output_table[i].append("-")
                current_cycle += 1 
            EXE(i)
            while register_busy[inst["input_registers"][0]] >= current_cycle:
                output_table[i].append("-")
                current_cycle += 1        
            MEM(i)
        while i > 0 and current_cycle <= len(output_table[i-1]) and output_table[i-1][current_cycle-1] == "-":
            output_table[i].append("-")
            current_cycle += 1  
        WB(inst["output_register"], i)

def print_table():
    max_cols = max(len(row) for row in output_table)
    padded_rows = [row + [""] * (max_cols - len(row)) for row in output_table]
    header = ["Instruction"] + [f"{c+1}" for c in range(max_cols)]
    instr_col_width = max(len(instr) for instr in opcodes)
    instr_col_width = max(instr_col_width, len("Instruction"))
    col_width = 3
    header_line = f"{'Instruction'.ljust(instr_col_width)} |  " + "|  ".join(cell.ljust(col_width) for cell in header[1:])
    print(header_line)
    print("-" * len(header_line))
    for i, row in enumerate(padded_rows):
        instr = opcodes[i].ljust(instr_col_width)
        cycle_cells = " | ".join(cell.ljust(col_width) for cell in row)
        print(f"{instr} | {cycle_cells}")

pipeline()
print_table()
