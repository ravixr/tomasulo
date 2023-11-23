#include <fstream>
#include <iostream>
#include <vector>
#include <map>
#include <algorithm>
#include "tomasulo.h"

static int inv_regs_refs[INVISIBLE_REGISTERS] = { 0 };
static int reg_rename[VISIBLE_REGISTERS] = { noreg };
static bool dest_in_use[REGISTERS_MAX] = { 0 };
static bool src_in_use[REGISTERS_MAX] = { 0 };

reserve_station add_stations[ADD_STATIONS] = { 0 };
reserve_station mult_stations[MUL_STATIONS] = { 0 };
reserve_station load_stations[LOAD_STATIONS] = { 0 };

int station_sizes[STATION_TYPES] = { ADD_STATIONS, MUL_STATIONS, LOAD_STATIONS };
reserve_station *all_stations[STATION_TYPES] = { add_stations, mult_stations, load_stations };

// TODO - check dependencies
// if true dependecies put on wait queue
// if false rename
// if reserve station is full put on station queue


void read_instructions(std::deque<inst>& insts, std::string filename)
{
    static std::map<std::string, op_t> op_map({
        {"add", add}, {"sub", sub}, {"mul", mul}, {"div", divd}, {"lw ", lw}, {"sw ", sw}
    });
    static std::map<std::string, reg_t> reg_map({
        {"r0", r0}, {"r1", r1}, {"r2", r2}, {"r3", r3}, {"r4", r4}, 
        {"r5", r5}, {"r6", r6}, {"r7", r7}, {"r8", r8}, {"r9", r9}, 
        {"ra", ra}, {"rb", rb}, {"rc", rc}, {"rd", rd}, {"re", re}, 
        {"rf", rf}, {"rg", rg}, {"rh", rh}, {"ri", ri}, {"rj", rj}
    });
    static int op_time[6] = { add_time, sub_time, mul_time, div_time, lw_time, sw_time };
    std::string raw_inst;
    std::ifstream file(filename);
    if (file.is_open()) {
        while (std::getline(file, raw_inst)) {
            // convert string to inst
            inst i;
            i.op = op_map[raw_inst.substr(0, 3)];
            if (i.op == sw || i.op == lw) {
                if (i.op == sw) {
                    i.dest = noreg;
                    i.src1 = reg_map[raw_inst.substr(3, 2)];
                } else {
                    i.dest = reg_map[raw_inst.substr(3, 2)];
                    i.src1 = noreg;
                }
                size_t pos = raw_inst.find('(');
                i.imm = std::stoi(raw_inst.substr(7, pos - 7));
                i.src2 = reg_map[raw_inst.substr(pos + 1, 2)];
                i.time = op_time[i.op];             
            } else {
                i.dest = reg_map[raw_inst.substr(4, 2)];
                i.src1 = reg_map[raw_inst.substr(8, 2)];
                i.src2 = reg_map[raw_inst.substr(12, 2)];
                i.time = op_time[i.op];
            }
            insts.push_back(i);
        }
        file.close();
    }
}

void print_instructions(std::deque<inst>& insts)
{
    const std::string op[6] = { "add", "sub", "mul", "div", "lw", "sw" };
    const std::string reg[20] = { "r0", "r1", "r2", "r3", "r4", "r5", "r6","r7", "r8", "r9", 
    "ra", "rb", "rc", "rd", "re", "rf", "rg", "rh", "ri", "rj" };
    // print with time and position
    for (auto &i : insts) {
        if (i.op == sw) {
            std::cout << op[i.op] << " " << reg[i.src1] << ", " << i.imm << "(" << reg[i.src2] << ") - time: " << i.time << std::endl;
        } else if (i.op == lw) {
            std::cout << op[i.op] << " " << reg[i.dest] << ", " << i.imm << "(" << reg[i.src2] << ") - time: " << i.time << std::endl; 
        } else {
            std::cout << op[i.op] << " " << reg[i.dest] << ", " << reg[i.src1] << ", " << reg[i.src2] << " - time: " << i.time << std::endl;
        }
    }
}

int main()
{
    std::deque<inst> insts;
    read_instructions(insts, "input.txt");
    print_instructions(insts);
    return 0;
}