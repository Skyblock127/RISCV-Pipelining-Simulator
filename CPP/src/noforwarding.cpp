#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <iomanip>
#include <algorithm>
#include <sstream>

using namespace std;

struct InstructionInfo {
    string type;
    vector<int> input_registers;
    int output_register; // -1 if none
};

vector<string> opcodes;
vector<vector<string>> output_table;
int current_cycle = 1;
int cycle_of_prev_IF = 0;
int register_busy[32] = {0}; // All registers start as available at cycle 0

void IF(int i) {
    cycle_of_prev_IF = current_cycle;
    output_table[i].push_back("IF");
    current_cycle++;
}

InstructionInfo ID(const string& opcode_hex, int i) {
    uint32_t opcode;
    stringstream ss;
    ss << hex << opcode_hex;
    ss >> opcode;

    uint32_t opcode_field = opcode & 0x7F; // Extract last 7 bits
    InstructionInfo info;
    info.output_register = -1;

    if (opcode_field == 0x33) { // R-type
        int rd = (opcode >> 7) & 0x1F;
        int rs1 = (opcode >> 15) & 0x1F;
        int rs2 = (opcode >> 20) & 0x1F;
        info.type = "R";
        info.output_register = rd;
        info.input_registers = {rs1, rs2};
    } else if (opcode_field == 0x13) { // I-type
        int rd = (opcode >> 7) & 0x1F;
        int rs1 = (opcode >> 15) & 0x1F;
        info.type = "I";
        info.output_register = rd;
        info.input_registers = {rs1};
    } else if (opcode_field == 0x03) { // LOAD
        int rd = (opcode >> 7) & 0x1F;
        int rs1 = (opcode >> 15) & 0x1F;
        info.type = "LOAD";
        info.output_register = rd;
        info.input_registers = {rs1};
    } else if (opcode_field == 0x23) { // STORE
        int rs1 = (opcode >> 15) & 0x1F;
        int rs2 = (opcode >> 20) & 0x1F;
        info.type = "STORE";
        info.input_registers = {rs1, rs2};
    } else if (opcode_field == 0x63) { // BRANCH
        int rs1 = (opcode >> 15) & 0x1F;
        int rs2 = (opcode >> 20) & 0x1F;
        info.type = "BRANCH";
        info.input_registers = {rs1, rs2};
    } else if (opcode_field == 0x37) { // LUI
        int rd = (opcode >> 7) & 0x1F;
        info.type = "LUI";
        info.output_register = rd;
    } else if (opcode_field == 0x17) { // AUIPC
        int rd = (opcode >> 7) & 0x1F;
        info.type = "AUIPC";
        info.output_register = rd;
    } else {
        info.type = "UNKNOWN";
    }

    output_table[i].push_back("ID");
    current_cycle++;
    return info;
}

void EXE(int i) {
    output_table[i].push_back("EXE");
    current_cycle++;
}

void MEM(int i) {
    output_table[i].push_back("MEM");
    current_cycle++;
}

void WB(int reg, int i) {
    output_table[i].push_back("WB");
    if (reg != -1) {
        register_busy[reg] = current_cycle;
    }
    current_cycle++;
}

void pipeline() {
    int num_instructions = opcodes.size();
    output_table.resize(num_instructions);

    for (int i = 0; i < num_instructions; ++i) {
        output_table[i].clear();
        // Fill with spaces for previous IF cycles
        for (int j = 0; j < cycle_of_prev_IF; ++j) {
            output_table[i].push_back(" ");
        }
        current_cycle = cycle_of_prev_IF + 1;

        // Stall if previous instruction is in MEM/WB and current cycle is blocked
        while (i > 0 && current_cycle <= output_table[i-1].size() && output_table[i-1][current_cycle - 1] == "-") {
            output_table[i].push_back(" ");
            current_cycle++;
        }
        IF(i);

        // Handle stalls due to previous instructions
        while (i > 0 && current_cycle <= output_table[i-1].size() && output_table[i-1][current_cycle - 1] == "-") {
            output_table[i].push_back("-");
            current_cycle++;
        }
        InstructionInfo inst = ID(opcodes[i], i);

        // Process based on instruction type
        if (inst.type == "R" || inst.type == "I" || inst.type == "LOAD" || inst.type == "BRANCH") {
            int max_busy = 0;
            if (inst.type == "R" || inst.type == "BRANCH") {
                max_busy = max(register_busy[inst.input_registers[0]], register_busy[inst.input_registers[1]]);
            } else if (inst.type == "I" || inst.type == "LOAD") {
                max_busy = register_busy[inst.input_registers[0]];
            }

            while (max_busy >= current_cycle) {
                output_table[i].push_back("-");
                current_cycle++;
                if (inst.type == "R" || inst.type == "BRANCH") {
                    max_busy = max(register_busy[inst.input_registers[0]], register_busy[inst.input_registers[1]]);
                } else if (inst.type == "I" || inst.type == "LOAD") {
                    max_busy = register_busy[inst.input_registers[0]];
                }
            }

            // Check for previous instruction stalls before EXE
            while (i > 0 && current_cycle <= output_table[i-1].size() && output_table[i-1][current_cycle - 1] == "-") {
                output_table[i].push_back("-");
                current_cycle++;
            }
            EXE(i);

            while (i > 0 && current_cycle <= output_table[i-1].size() && output_table[i-1][current_cycle - 1] == "-") {
                output_table[i].push_back("-");
                current_cycle++;
            }
            MEM(i);

        } else if (inst.type == "STORE") {
            // STORE has rs2 then rs1 dependencies
            while (register_busy[inst.input_registers[1]] >= current_cycle) {
                output_table[i].push_back("-");
                current_cycle++;
            }
            while (i > 0 && current_cycle <= output_table[i-1].size() && output_table[i-1][current_cycle - 1] == "-") {
                output_table[i].push_back("-");
                current_cycle++;
            }
            EXE(i);

            while (register_busy[inst.input_registers[0]] >= current_cycle) {
                output_table[i].push_back("-");
                current_cycle++;
            }
            while (i > 0 && current_cycle <= output_table[i-1].size() && output_table[i-1][current_cycle - 1] == "-") {
                output_table[i].push_back("-");
                current_cycle++;
            }
            MEM(i);
        } else if (inst.type == "LUI" || inst.type == "AUIPC") {
            // No input dependencies
            while (i > 0 && current_cycle <= output_table[i-1].size() && output_table[i-1][current_cycle - 1] == "-") {
                output_table[i].push_back("-");
                current_cycle++;
            }
            EXE(i);
            while (i > 0 && current_cycle <= output_table[i-1].size() && output_table[i-1][current_cycle - 1] == "-") {
                output_table[i].push_back("-");
                current_cycle++;
            }
            MEM(i);
        } else {
            // Unknown instruction type
            EXE(i);
            MEM(i);
        }

        // WB stage
        while (i > 0 && current_cycle <= output_table[i-1].size() && output_table[i-1][current_cycle - 1] == "-") {
            output_table[i].push_back("-");
            current_cycle++;
        }
        WB(inst.output_register, i);
    }
}

void print_table() {
    if (output_table.empty()) return;

    int max_cycles = 0;
    for (const auto& row : output_table) {
        if (row.size() > max_cycles) {
            max_cycles = row.size();
        }
    }

    // Print header
    cout << left << setw(15) << "Instruction" << " | ";
    for (int c = 0; c < max_cycles; ++c) {
        cout << setw(3) << c + 1 << " | ";
    }
    cout << endl;

    // Print separator
    cout << string(15 + (max_cycles + 1) * 5, '-') << endl;

    // Print each row
    for (size_t i = 0; i < opcodes.size(); ++i) {
        cout << left << setw(15) << opcodes[i] << " | ";
        const auto& row = output_table[i];
        for (const string& stage : row) {
            cout << setw(3) << stage << " | ";
        }
        // Fill remaining cycles with spaces
        for (int c = row.size(); c < max_cycles; ++c) {
            cout << setw(3) << " " << " | ";
        }
        cout << endl;
    }
}

int main() {
    ifstream file("input.txt");
    if (!file) {
        cerr << "Could not open input.txt" << endl;
        return 1;
    }

    string line;
    while (getline(file, line)) {
        if (!line.empty()) {
            opcodes.push_back(line);
        }
    }

    pipeline();
    print_table();

    return 0;
}