#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <map>
#include <algorithm>
#include <iomanip>
#include <bitset>
#include <sstream>
#include <cctype>

using namespace std;

map<string, int> register_busy;
vector<vector<string>> output_table;
vector<string> opcodes;
int current_cycle = 1;
int cycle_of_prev_IF = 0;

struct InstructionInfo {
    string type;
    vector<string> input_registers;
    string output_register;
};

void initialize_registers() {
    for (int i = 0; i < 32; ++i) {
        register_busy["x" + to_string(i)] = 0;
    }
}

void IF(int i) {
    cycle_of_prev_IF = current_cycle;
    output_table[i].push_back("IF");
    current_cycle += 1;
}

InstructionInfo ID(const string& opcode_hex, int i) {
    InstructionInfo result;
    result.type = "UNKNOWN";
    
    // Convert hex to binary
    unsigned int opcode_int;
    stringstream ss;
    ss << hex << opcode_hex;
    ss >> opcode_int;
    bitset<32> opcode_bin(opcode_int);
    string opcode_bin_str = opcode_bin.to_string();
    
    string opcode_field = opcode_bin_str.substr(25, 7);
    
    if (opcode_field == "0110011") {
        int rd = stoi(opcode_bin_str.substr(20, 5), 0, 2);
        int rs1 = stoi(opcode_bin_str.substr(12, 5), 0, 2);
        int rs2 = stoi(opcode_bin_str.substr(7, 5), 0, 2);
        result.type = "R";
        result.output_register = "x" + to_string(rd);
        result.input_registers = {"x" + to_string(rs1), "x" + to_string(rs2)};
    } 
    else if (opcode_field == "0010011") {
        int rd = stoi(opcode_bin_str.substr(20, 5), 0, 2);
        int rs1 = stoi(opcode_bin_str.substr(12, 5), 0, 2);
        result.type = "I";
        result.output_register = "x" + to_string(rd);
        result.input_registers = {"x" + to_string(rs1)};
    } 
    else if (opcode_field == "0000011") {
        int rd = stoi(opcode_bin_str.substr(20, 5), 0, 2);
        int rs1 = stoi(opcode_bin_str.substr(12, 5), 0, 2);
        result.type = "LOAD";
        result.output_register = "x" + to_string(rd);
        result.input_registers = {"x" + to_string(rs1)};
    } 
    else if (opcode_field == "0100011") {
        int rs1 = stoi(opcode_bin_str.substr(12, 5), 0, 2);
        int rs2 = stoi(opcode_bin_str.substr(7, 5), 0, 2);
        result.type = "STORE";
        result.input_registers = {"x" + to_string(rs1), "x" + to_string(rs2)};
    } 
    else if (opcode_field == "1100011") {
        int rs1 = stoi(opcode_bin_str.substr(12, 5), 0, 2);
        int rs2 = stoi(opcode_bin_str.substr(7, 5), 0, 2);
        result.type = "BRANCH";
        result.input_registers = {"x" + to_string(rs1), "x" + to_string(rs2)};
    } 
    else if (opcode_field == "0110111") {
        int rd = stoi(opcode_bin_str.substr(20, 5), 0, 2);
        result.type = "LUI";
        result.output_register = "x" + to_string(rd);
    } 
    else if (opcode_field == "0010111") {
        int rd = stoi(opcode_bin_str.substr(20, 5), 0, 2);
        result.type = "AUIPC";
        result.output_register = "x" + to_string(rd);
    }
    
    output_table[i].push_back("ID");
    current_cycle += 1;
    return result;
}

void EXE(int i) {
    output_table[i].push_back("EXE");
    current_cycle += 1;
}

void MEM(int i) {
    output_table[i].push_back("MEM");
    current_cycle += 1;
}

void WB(const string& inst_op_reg, int i) {
    output_table[i].push_back("WB");
    if (!inst_op_reg.empty()) {
        register_busy[inst_op_reg] = current_cycle;
    }
}

void pipeline() {
    for (int i = 0; i < opcodes.size(); ++i) {
        // Fill with empty strings up to cycle_of_prev_IF
        while (output_table[i].size() < cycle_of_prev_IF) {
            output_table[i].push_back(" ");
        }
        current_cycle = cycle_of_prev_IF + 1;

        // Handle stalls from previous instruction
        while (i > 0 && current_cycle <= output_table[i-1].size() && output_table[i-1][current_cycle-1] == "-") {
            output_table[i].push_back(" ");
            current_cycle += 1;
        }
        IF(i);

        // Handle stalls from previous instruction
        while (i > 0 && current_cycle <= output_table[i-1].size() && output_table[i-1][current_cycle-1] == "-") {
            output_table[i].push_back("-");
            current_cycle += 1;
        }
        InstructionInfo inst = ID(opcodes[i], i);
        
        if (inst.type == "R") {
            int max_busy = max(register_busy[inst.input_registers[0]], register_busy[inst.input_registers[1]]);
            while (max_busy >= current_cycle) {
                output_table[i].push_back("-");
                current_cycle += 1;
            }
            
            while (i > 0 && current_cycle <= output_table[i-1].size() && output_table[i-1][current_cycle-1] == "-") {
                output_table[i].push_back("-");
                current_cycle += 1;
            }
            EXE(i);
            
            while (i > 0 && current_cycle <= output_table[i-1].size() && output_table[i-1][current_cycle-1] == "-") {
                output_table[i].push_back("-");
                current_cycle += 1;
            }
            MEM(i);
        }
        else if (inst.type == "I") {
            while (register_busy[inst.input_registers[0]] >= current_cycle) {
                output_table[i].push_back("-");
                current_cycle += 1;
            }
            
            while (i > 0 && current_cycle <= output_table[i-1].size() && output_table[i-1][current_cycle-1] == "-") {
                output_table[i].push_back("-");
                current_cycle += 1;
            }
            EXE(i);
            
            while (i > 0 && current_cycle <= output_table[i-1].size() && output_table[i-1][current_cycle-1] == "-") {
                output_table[i].push_back("-");
                current_cycle += 1;
            }
            MEM(i);
        }
        else if (inst.type == "LOAD") {
            while (register_busy[inst.input_registers[0]] >= current_cycle) {
                output_table[i].push_back("-");
                current_cycle += 1;
            }
            
            while (i > 0 && current_cycle <= output_table[i-1].size() && output_table[i-1][current_cycle-1] == "-") {
                output_table[i].push_back("-");
                current_cycle += 1;
            }
            EXE(i);
            
            while (i > 0 && current_cycle <= output_table[i-1].size() && output_table[i-1][current_cycle-1] == "-") {
                output_table[i].push_back("-");
                current_cycle += 1;
            }
            MEM(i);
        }
        else if (inst.type == "STORE") {
            while (register_busy[inst.input_registers[1]] >= current_cycle) {
                output_table[i].push_back("-");
                current_cycle += 1;
            }
            
            while (i > 0 && current_cycle <= output_table[i-1].size() && output_table[i-1][current_cycle-1] == "-") {
                output_table[i].push_back("-");
                current_cycle += 1;
            }
            EXE(i);

            while (register_busy[inst.input_registers[0]] >= current_cycle) {
                output_table[i].push_back("-");
                current_cycle += 1;
            }
            
            while (i > 0 && current_cycle <= output_table[i-1].size() && output_table[i-1][current_cycle-1] == "-") {
                output_table[i].push_back("-");
                current_cycle += 1;
            }
            MEM(i);
        }
        else if (inst.type == "BRANCH") {
            int max_busy = max(register_busy[inst.input_registers[0]], register_busy[inst.input_registers[1]]);
            while (max_busy >= current_cycle) {
                output_table[i].push_back("-");
                current_cycle += 1;
            }
            
            while (i > 0 && current_cycle <= output_table[i-1].size() && output_table[i-1][current_cycle-1] == "-") {
                output_table[i].push_back("-");
                current_cycle += 1;
            }
            EXE(i);
            
            while (i > 0 && current_cycle <= output_table[i-1].size() && output_table[i-1][current_cycle-1] == "-") {
                output_table[i].push_back("-");
                current_cycle += 1;
            }
            MEM(i);
        }
        else if (inst.type == "LUI" || inst.type == "AUIPC") {
            while (i > 0 && current_cycle <= output_table[i-1].size() && output_table[i-1][current_cycle-1] == "-") {
                output_table[i].push_back("-");
                current_cycle += 1;
            }
            EXE(i);
            
            while (i > 0 && current_cycle <= output_table[i-1].size() && output_table[i-1][current_cycle-1] == "-") {
                output_table[i].push_back("-");
                current_cycle += 1;
            }
            MEM(i);
        }
        else {
            while (i > 0 && current_cycle <= output_table[i-1].size() && output_table[i-1][current_cycle-1] == "-") {
                output_table[i].push_back("-");
                current_cycle += 1;
            }
            EXE(i);
            
            while (i > 0 && current_cycle <= output_table[i-1].size() && output_table[i-1][current_cycle-1] == "-") {
                output_table[i].push_back("-");
                current_cycle += 1;
            }
            MEM(i);
        }

        while (i > 0 && current_cycle <= output_table[i-1].size() && output_table[i-1][current_cycle-1] == "-") {
            output_table[i].push_back("-");
            current_cycle += 1;
        }
        WB(inst.output_register, i);
    }
}

void print_table() {
    // Find maximum columns needed
    size_t max_cols = 0;
    for (const auto& row : output_table) {
        if (row.size() > max_cols) {
            max_cols = row.size();
        }
    }
    
    // Calculate column widths
    size_t instr_col_width = 10; // "Instruction" length
    for (const auto& instr : opcodes) {
        if (instr.length() > instr_col_width) {
            instr_col_width = instr.length();
        }
    }
    
    // Print header
    cout << left << setw(instr_col_width) << "Instruction" << " |  ";
    for (size_t c = 0; c < max_cols; ++c) {
        cout << left << setw(3) << to_string(c+1);
        if (c != max_cols - 1) {
            cout << " | ";
        }
    }
    cout << endl;
    
    // Print separator line
    cout << string(instr_col_width, '-') << "-|-";
    for (size_t c = 0; c < max_cols; ++c) {
        cout << string(3, '-');
        if (c != max_cols - 1) {
            cout << "-|-";
        }
    }
    cout << endl;
    
    // Print each instruction row
    for (size_t i = 0; i < opcodes.size(); ++i) {
        cout << left << setw(instr_col_width) << opcodes[i] << " | ";
        for (size_t j = 0; j < max_cols; ++j) {
            if (j < output_table[i].size()) {
                cout << left << setw(3) << output_table[i][j];
            } else {
                cout << left << setw(3) << " ";
            }
            if (j != max_cols - 1) {
                cout << " | ";
            }
        }
        cout << endl;
    }
}

string clean_string(const string& str) {
    string result;
    for (char c : str) {
        if (isprint(c)) {
            result += c;
        }
    }
    return result;
}

int main() {
    initialize_registers();
    
    ifstream file("input.txt");
    if (!file.is_open()) {
        cerr << "Error opening input.txt" << endl;
        return 1;
    }
    
    string line;
    while (getline(file, line)) {
        // Clean the line (remove non-ASCII characters)
        line = clean_string(line);
        
        // Remove leading/trailing whitespace
        size_t start = line.find_first_not_of(" \t");
        if (start != string::npos) {
            size_t end = line.find_last_not_of(" \t");
            line = line.substr(start, end - start + 1);
        }
        
        if (!line.empty()) {
            opcodes.push_back(line);
        }
    }
    file.close();
    
    output_table.resize(opcodes.size());
    
    pipeline();
    print_table();
    
    return 0;
}
