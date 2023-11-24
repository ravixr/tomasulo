#include <fstream>
#include <iostream>
#include <vector>
#include <map>
#include <algorithm>
#include "tomasulo.h"

std::deque<inst_t *> insts;

regstat_t registers[REGISTERS_MAX + 1] = { { 0, free_reg, -1, noreg, noreg, 0 }, };

fu_t add_stations[ADD_STATIONS] = { 0 };
fu_t mult_stations[MUL_STATIONS] = { 0 };
fu_t load_stations[LOAD_STATIONS] = { 0 };

int station_sizes[STATION_TYPES] = { ADD_STATIONS, MUL_STATIONS, LOAD_STATIONS };
fu_t *all_stations[STATION_TYPES] = { add_stations, mult_stations, load_stations };

void init_fus()
{
    int next_id = 0;
    for (int i = 0; i < STATION_TYPES; i++) {
        for (int j = 0; j < station_sizes[i]; j++) {
            all_stations[i][j].id = next_id++;
        }
    }
}

void check_rename(inst_t *i)
{
    // if false dependecies rename
    if (registers[i->dest].used_as == src) {
        reg_t rename = noreg;
        for (int j = VISIBLE_REGISTERS + 1; j <= REGISTERS_MAX; j++) {
            if (registers[j].used_as == free_reg) {
                rename = (reg_t)j;
                break;
            }
        }
        registers[i->dest].renamed_to = rename;
        registers[i->dest].renamed_from = i->dest;
        i->dest = registers[i->dest].renamed_to;
        registers[i->dest].rename_ref_count++;
    }
    if (registers[i->src1].renamed_to != noreg) {
        i->src1 = registers[i->src1].renamed_to;
        registers[i->src1].rename_ref_count++;

    }
    if (registers[i->src2].renamed_to != noreg) {
        i->src2 = registers[i->src2].renamed_to;
        registers[i->src2].rename_ref_count++;
    }
}

void issue() {
    inst_t *i  = insts.front();
    // check if renamed
    check_rename(i);
    // check if i can be issued
    fu_t *stations = all_stations[i->op % STATION_TYPES];
    int empty_station = -1;
    for (int j = 0; j < station_sizes[i->op % STATION_TYPES]; j++) {
        if (!stations[j].busy) {
            empty_station = j;
            break;
        }
    }
    if (empty_station != -1) {
        stations[empty_station].inst = i;
        i->issue = 1;
        insts.pop_front();
    }
}

void exec_fu(fu_t *fu)
{
    // TODO - make exec_fu
    if (fu->busy) {
        
    } else {

    }
}

void exec_cycle()
{
    issue();
    // exec all stations
    for (int i = 0; i < STATION_TYPES; i++) {
        for (int j = 0; j < station_sizes[i]; j++) {
            exec_fu(&all_stations[i][j]);
        }
    }
}

std::vector<inst_t> read_instructions(std::string filename)
{
    std::vector<inst_t> code;
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
            inst_t i;
            i.issue = 0;
            i.exec = 0;
            i.write = 0;
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
            code.push_back(i);
        }
        file.close();
    }
    // make insts a deque of pointers to each code inst
    for (auto &i : code) {
        insts.push_back(&i);
    }
    return code;
}

#if 0
void print_instructions()
{
    // print with time and position
    for (auto &i : insts) {
        if (i.op == sw) {
            std::cout << op[i.op] << " " << str_reg[i.src1] << ", " << i.imm << "(" << str_reg[i.src2] << ") - time: " << i.time << std::endl;
        } else if (i.op == lw) {
            std::cout << op[i.op] << " " << str_reg[i.dest] << ", " << i.imm << "(" << str_reg[i.src2] << ") - time: " << i.time << std::endl; 
        } else {
            std::cout << op[i.op] << " " << str_reg[i.dest] << ", " << str_reg[i.src1] << ", " << str_reg[i.src2] << " - time: " << i.time << std::endl;
        }
    }
}
#endif

// TODO - check dependencies
// if true dependecies put on wait queue
// if false rename
// if reserve station is full put on station queue



const std::string str_op[6] = { "add", "sub", "mul", "div", "lw", "sw" };
const std::string str_reg[21] = { "-", "r0", "r1", "r2", "r3", "r4", "r5", "r6","r7", "r8", "r9", 
"ra", "rb", "rc", "rd", "re", "rf", "rg", "rh", "ri", "rj" };
const std::string str_fus[STATION_TYPES] = { "add", "mul", "lw/sw" };

void fus()
{
    
    std::cout << "========================================= Functional Units =======================================\n";
    std::cout << "Time\tFU\tBusy\tOp\tVj\tVk\tQj\tQk\n";
    for (int i = 0; i < STATION_TYPES; i++) {
        for (int j = 0; j < station_sizes[i]; j++) {
            std::cout << all_stations[i][j].time_left << "\t"<< str_fus[i] << "\t" << all_stations[i][j].busy << "\t";
            if (all_stations[i][j].busy) {
                std::cout << str_op[all_stations[i][j].inst->op % STATION_TYPES];
            } else {
                std::cout << "-";
            }
            std::cout << "\t" << all_stations[i][j].vj << "\t" << all_stations[i][j].vk << "\t" << all_stations[i][j].qj << "\t" << all_stations[i][j].qk << "\n";
        }
    }
    std::cout << "==================================================================================================\n";
}

void show_registers()
{
    std::cout << "============================================= Registers ==========================================\n";
    // TODO - print registers
    std::cout << "==================================================================================================\n";
}

void status(std::vector<inst_t> inst_list)
{
    std::cout << "============================================= Status =============================================\n";
    std::cout << "\tinst\ti\tj\tk\tissue\tcomplete    write-back\n";
    for (int i = 0; i < inst_list.size(); i++) {
        std::cout << i << "\t" << str_op[inst_list[i].op] << "\t" << str_reg[inst_list[i].dest] << "\t" << str_reg[inst_list[i].src1] << "\t" << str_reg[inst_list[i].src2] << "\t  " << inst_list[i].issue << "\t    " << inst_list[i].exec << "\t\t" << inst_list[i].write << "\n";
    }
    std::cout << "==================================================================================================\n";
}

int main()
{
    unsigned int clock = 0;
    std::cout << "Tomasulo's Algorithm simulator\n";
    std::cout << "Reading instructions from input.txt\n";
    std::vector<inst_t> inst_list = read_instructions("input.txt");
    std::cout << "Instructions loaded\n";
    std::string input;
    init_fus();
    while (!insts.empty()) {
        clock++;
        std::cout << ">";
        std::cin >> input;
        // split input by spaces
        if (input == "help") {
            std::cout << "Commands:\n";
            std::cout << "clock - display current clock cycle\n";
            std::cout << "exit - exit simulator\n";
            std::cout << "help - display this message\n";
            std::cout << "fus - display functional units\n";
            std::cout << "next (n) - execute next clock cycle\n";
            std::cout << "registers - display registers\n";
            std::cout << "status - display status of instructions\n";
        } else if (input == "registers") {
            show_registers();
        } else if (input == "fus") {
            fus();
        } else if (input == "status") {
            status(inst_list);
        } else if (input == "next" || input == "n") {
            std::cout << "Cycle: " << clock << "\n";
            exec_cycle();
        } else if (input == "clock") {
            std::cout << "Cycle: " << clock << "\n";
        } else if (input == "exit") {
            break;
        } else {
            std::cout << "Invalid command\n";
        }
    }
    std::cout << "Simulation complete (Press enter to exit)\n";
    std::cin.get();
    return 0;
}