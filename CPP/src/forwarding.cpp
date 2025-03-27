#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <vector>
#include <string>
#include <unordered_map>
#include <algorithm>
#include <bitset>

using namespace std;

// Global variables for register busy times, opcode list, cycle management, and pipeline table.
unordered_map<string, int> register_busy;
vector<string> opcodes;
int number_of_opcodes = 0;
int current_cycle = 1;
int cycle_of_prev_IF = 0;
vector<vector<string>> output_table;

// A structure to hold decoded instruction information.
struct InstructionInfo {
    string type = "UNKNOWN";
    vector<string> input_registers;
    string output_register = "";
};

// Helper: convert hex string to 32-bit binary string.
string hexToBin32(const string &hexStr) {
    unsigned int num = stoul(hexStr, nullptr, 16);
    bitset<32> bs(num);
    return bs.to_string(); // already padded to 32 bits
}

// Pipeline stage functions.
void IF(int i) {
    cycle_of_prev_IF = current_cycle;
    output_table[i].push_back("IF");
    current_cycle++;
}

InstructionInfo ID(const string &opcode_hex, int i) {
    InstructionInfo result;
    string opcode_bin = hexToBin32(opcode_hex);
    // Extract the last 7 bits (opcode field)
    string opcode_field = opcode_bin.substr(25, 7);

    if (opcode_field == "0110011") { // R-type
        int rd  = stoi(opcode_bin.substr(20, 5), nullptr, 2);
        int rs1 = stoi(opcode_bin.substr(12, 5), nullptr, 2);
        int rs2 = stoi(opcode_bin.substr(7, 5), nullptr, 2);
        result.type = "R";
        result.output_register = "x" + to_string(rd);
        result.input_registers.push_back("x" + to_string(rs1));
        result.input_registers.push_back("x" + to_string(rs2));
    }
    else if (opcode_field == "0010011") { // I-type (ALU immediate)
        int rd  = stoi(opcode_bin.substr(20, 5), nullptr, 2);
        int rs1 = stoi(opcode_bin.substr(12, 5), nullptr, 2);
        result.type = "I";
        result.output_register = "x" + to_string(rd);
        result.input_registers.push_back("x" + to_string(rs1));
    }
    else if (opcode_field == "0000011") { // Load
        int rd  = stoi(opcode_bin.substr(20, 5), nullptr, 2);
        int rs1 = stoi(opcode_bin.substr(12, 5), nullptr, 2);
        result.type = "LOAD";
        result.output_register = "x" + to_string(rd);
        result.input_registers.push_back("x" + to_string(rs1));
    }
    else if (opcode_field == "0100011") { // Store
        int rs1 = stoi(opcode_bin.substr(12, 5), nullptr, 2);
        int rs2 = stoi(opcode_bin.substr(7, 5), nullptr, 2);
        result.type = "STORE";
        result.input_registers.push_back("x" + to_string(rs1));
        result.input_registers.push_back("x" + to_string(rs2));
    }
    else if (opcode_field == "1100011") { // Branch
        int rs1 = stoi(opcode_bin.substr(12, 5), nullptr, 2);
        int rs2 = stoi(opcode_bin.substr(7, 5), nullptr, 2);
        result.type = "BRANCH";
        result.input_registers.push_back("x" + to_string(rs1));
        result.input_registers.push_back("x" + to_string(rs2));
    }

    output_table[i].push_back("ID");
    current_cycle++;
    return result;
}

void EXE(int i) {
    output_table[i].push_back("EXE");
    current_cycle++;
}

void MEM(int i) {
    output_table[i].push_back("MEM");
    current_cycle++;
}

void WB(const string &inst_op_reg, int i) {
    output_table[i].push_back("WB");
    // Mark the register busy until the current cycle
    register_busy[inst_op_reg] = current_cycle;
}

// The pipeline function schedules the stages for each instruction.
void pipeline() {
    for (int i = 0; i < number_of_opcodes; i++) {
        // Align this instruction with previous IF cycle count
        for (int j = 0; j < cycle_of_prev_IF; j++) {
            output_table[i].push_back(" ");
        }
        current_cycle = cycle_of_prev_IF + 1;

        // Wait until previous instruction's cycle cell is not "-" (for i>0)
        while (i > 0 && current_cycle <= output_table[i - 1].size() &&
               output_table[i - 1][current_cycle - 1] == "-") {
            output_table[i].push_back(" ");
            current_cycle++;
        }
        IF(i);

        while (i > 0 && current_cycle <= output_table[i - 1].size() &&
               output_table[i - 1][current_cycle - 1] == "-") {
            output_table[i].push_back("-");
            current_cycle++;
        }
        InstructionInfo inst = ID(opcodes[i], i);

        if (inst.type == "R") {
            // Check both input registers are free.
            while (max(register_busy[inst.input_registers[0]],
                       register_busy[inst.input_registers[1]]) >= current_cycle) {
                output_table[i].push_back("-");
                current_cycle++;
            }
            while (i > 0 && current_cycle <= output_table[i - 1].size() &&
                   output_table[i - 1][current_cycle - 1] == "-") {
                output_table[i].push_back("-");
                current_cycle++;
            }
            EXE(i);
            while (i > 0 && current_cycle <= output_table[i - 1].size() &&
                   output_table[i - 1][current_cycle - 1] == "-") {
                output_table[i].push_back("-");
                current_cycle++;
            }
            MEM(i);
        }
        else if (inst.type == "I") {
            while (register_busy[inst.input_registers[0]] >= current_cycle) {
                output_table[i].push_back("-");
                current_cycle++;
            }
            while (i > 0 && current_cycle <= output_table[i - 1].size() &&
                   output_table[i - 1][current_cycle - 1] == "-") {
                output_table[i].push_back("-");
                current_cycle++;
            }
            EXE(i);
            while (i > 0 && current_cycle <= output_table[i - 1].size() &&
                   output_table[i - 1][current_cycle - 1] == "-") {
                output_table[i].push_back("-");
                current_cycle++;
            }
            MEM(i);
        }
        else if (inst.type == "LOAD") {
            while (register_busy[inst.input_registers[0]] >= current_cycle) {
                output_table[i].push_back("-");
                current_cycle++;
            }
            while (i > 0 && current_cycle <= output_table[i - 1].size() &&
                   output_table[i - 1][current_cycle - 1] == "-") {
                output_table[i].push_back("-");
                current_cycle++;
            }
            EXE(i);
            while (i > 0 && current_cycle <= output_table[i - 1].size() &&
                   output_table[i - 1][current_cycle - 1] == "-") {
                output_table[i].push_back("-");
                current_cycle++;
            }
            MEM(i);
        }
        else if (inst.type == "STORE") {
            while (register_busy[inst.input_registers[1]] >= current_cycle) {
                output_table[i].push_back("-");
                current_cycle++;
            }
            while (i > 0 && current_cycle <= output_table[i - 1].size() &&
                   output_table[i - 1][current_cycle - 1] == "-") {
                output_table[i].push_back("-");
                current_cycle++;
            }
            EXE(i);
            while (register_busy[inst.input_registers[0]] >= current_cycle) {
                output_table[i].push_back("-");
                current_cycle++;
            }
            while (i > 0 && current_cycle <= output_table[i - 1].size() &&
                   output_table[i - 1][current_cycle - 1] == "-") {
                output_table[i].push_back("-");
                current_cycle++;
            }
            MEM(i);
        }

        // Add WB stage for all instructions (if applicable)
        while (i > 0 && current_cycle <= output_table[i - 1].size() &&
               output_table[i - 1][current_cycle - 1] == "-") {
            output_table[i].push_back("-");
            current_cycle++;
        }
        WB(inst.output_register, i);
    }
}

// Function to print the pipeline table.
void print_table() {
    // Determine the maximum number of cycles (columns) across all instructions.
    size_t max_cols = 0;
    for (const auto &row : output_table) {
        if (row.size() > max_cols)
            max_cols = row.size();
    }
    
    // Pad rows with empty strings so that all have the same length.
    for (auto &row : output_table) {
        while (row.size() < max_cols)
            row.push_back("");
    }
    
    // Create header with cycle numbers starting at 1.
    vector<string> header;
    header.push_back("Instruction");
    for (size_t c = 0; c < max_cols; c++)
        header.push_back(to_string(c + 1));
    
    // Determine width for the instruction column.
    size_t instr_col_width = 11; // at least "Instruction"
    for (const auto &instr : opcodes)
        instr_col_width = max(instr_col_width, instr.size());
    
    // Print header.
    cout << left << setw(instr_col_width) << "Instruction" << " | ";
    for (size_t c = 1; c < header.size(); c++) {
        cout << setw(3) << header[c];
        if (c < header.size() - 1)
            cout << " | ";
    }
    cout << "\n" << string(instr_col_width + 3 + (max_cols * 6 - 3), '-') << "\n";
    
    // Print each instruction row.
    for (size_t i = 0; i < output_table.size(); i++) {
        cout << left << setw(instr_col_width) << opcodes[i] << " | ";
        for (size_t j = 0; j < output_table[i].size(); j++) {
            cout << setw(3) << output_table[i][j];
            if (j < output_table[i].size() - 1)
                cout << " | ";
        }
        cout << "\n";
    }
}

int main() {
    // Initialize register_busy with registers x0 to x31, all set to 0.
    for (int i = 0; i < 32; i++) {
        string reg = "x" + to_string(i);
        register_busy[reg] = 0;
    }
    
    // Read opcodes from "input.txt"
    ifstream infile("input.txt");
    if (!infile) {
        cerr << "Error: input.txt not found." << endl;
        return 1;
    }
    string line;
    while (getline(infile, line)) {
        if (!line.empty()) {
            // Trim whitespace from both ends.
            istringstream iss(line);
            string opcode;
            iss >> opcode;
            if (!opcode.empty()) {
                opcodes.push_back(opcode);
            }
        }
    }
    infile.close();
    
    number_of_opcodes = opcodes.size();
    output_table.resize(number_of_opcodes);
    
    // Run the pipeline simulation and print the table.
    pipeline();
    print_table();
    
    return 0;
}
