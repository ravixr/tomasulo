#include <fstream>
#include <iostream>
#include <vector>
#include <map>
#include <algorithm>
#include "tomasulo.hpp"

std::vector<inst_t> inst_list;
std::deque<inst_t *> insts;
std::vector<inst_t *> reorder_buffer;
unsigned int ticks = 0;

const std::string str_op[6] = { "add", "mul", "lw", "sub", "div", "sw" };
const std::string str_reg[REGISTERS_MAX + 1] = { "-", "r0", "r1", "r2", "r3", "r4", "r5", "r6","r7", "r8", "r9", "r10", "r11",
"ra", "rb", "rc", "rd", "re", "rf", "rg", "rh", "ri", "rj", "rk", "rl", "rm", "rn", "ro", "rp", "rq", "rr", "rs", "rt", "ru", "rv", "rw", "rx" };
const std::string str_fus[ALL_STATIONS + 1] = { "-", "add1", "add2", "mult1", "mult2", "load1", "load2" };

regstat_t registers[REGISTERS_MAX + 1] = { { "", free_reg, 0, noreg, noreg, 0 }, };

fu_t add_stations[ADD_STATIONS] = { { 0, false, nullptr, 0, 0, -1, -1, -1, 0, 0 }, };
fu_t mult_stations[MUL_STATIONS] = { { 0, false, nullptr, 0, 0, -1, -1, -1, 0, 0 }, };
fu_t load_stations[LOAD_STATIONS] = { { 0, false, nullptr, 0, 0, -1, -1, -1, 0, 0 }, };

int station_sizes[STATION_TYPES] = { ADD_STATIONS, MUL_STATIONS, LOAD_STATIONS };
fu_t *all_stations[STATION_TYPES] = { add_stations, mult_stations, load_stations };

void init_fus()
{
    int next_id = 0;
    for (int i = 0; i < STATION_TYPES; i++) {
        for (int j = 0; j < station_sizes[i]; j++) {
            all_stations[i][j].id = next_id++;
            all_stations[i][j].busy = false;
            all_stations[i][j].inst = nullptr;
            all_stations[i][j].vj = 0;
            all_stations[i][j].vk = 0;
            all_stations[i][j].qj = -1;
            all_stations[i][j].qk = -1;
            all_stations[i][j].time_left = -1;
            all_stations[i][j].locks1 = false;
            all_stations[i][j].locks2 = false;
        }
    }
}

void check_rename(inst_t *i)
{
    // if false dependecies rename
    if (registers[i->dest].renamed_to != noreg) {
        i->dest = registers[i->dest].renamed_to;
        registers[i->dest].rename_ref_count++;
    } else if (registers[i->dest].used_as != free_reg) {
        reg_t rename = noreg;
        for (int j = VISIBLE_REGISTERS + 1; j <= REGISTERS_MAX; j++) {
            if (registers[j].used_as == free_reg) {
                rename = (reg_t)j;
                registers[rename].used_as = dest;
                break;
            }
        }
        registers[i->dest].renamed_to = rename;
        registers[rename].renamed_from = i->dest;
        i->dest = rename;
        registers[i->dest].rename_ref_count++;
    }
    if (registers[i->src1].renamed_to != noreg) {
        i->src1 = registers[i->src1].renamed_to;
        registers[i->src1].rename_ref_count++;
    } else if (registers[i->src1].used_as == src) {
        reg_t rename = noreg;
        for (int j = VISIBLE_REGISTERS + 1; j <= REGISTERS_MAX; j++) {
            if (registers[j].used_as == free_reg) {
                rename = (reg_t)j;
                registers[rename].used_as = src;
                break;
            }
        }
        registers[i->src1].renamed_to = rename;
        registers[rename].renamed_from = i->src1;
        i->src1 = rename;
        registers[i->src1].rename_ref_count++;
    }
    if (registers[i->src2].renamed_to != noreg) {
        i->src2 = registers[i->src2].renamed_to;
        registers[i->src2].rename_ref_count++;
    } else if (registers[i->src2].used_as == src) {
        reg_t rename = noreg;
        for (int j = VISIBLE_REGISTERS + 1; j <= REGISTERS_MAX; j++) {
            if (registers[j].used_as == free_reg) {
                rename = (reg_t)j;
                registers[rename].used_as = src;
                break;
            }
        }
        registers[i->src2].renamed_to = rename;
        registers[rename].renamed_from = i->src2;
        i->src2 = rename;
        registers[i->src2].rename_ref_count++;
    }
}

void issue() {
    if (insts.empty()) {
        return;
    }
    inst_t *i  = insts.front();
    // check if registers must be renamed
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
    // issue instruction to station
    if (empty_station != -1) {
        stations[empty_station].inst = i;
        i->issue = ticks;
        insts.pop_front();
    }
}

void undo_rename(reg_t reg)
{
    if (registers[reg].rename_ref_count > 0) {
        registers[reg].rename_ref_count--;
    }
    if (registers[reg].rename_ref_count == 0) {
        registers[reg].used_as = free_reg;
        registers[registers[reg].renamed_from].renamed_to = noreg;
        registers[registers[reg].renamed_from].value = registers[reg].value;
        registers[reg].renamed_from = noreg;
        registers[reg].value = "";
    } else if (registers[reg].used_as == dest) {
        registers[reg].used_as = src;
        registers[reg].dest_used_by = 0;
    }
}

void exec_fu(fu_t *fu)
{
    #define USED_AS_DEST(reg) (registers[reg].used_as == dest)
    if (fu->busy) {
        // True dependencies
        if (fu->locks1 && USED_AS_DEST(fu->inst->src1)) {
            fu->qj = registers[fu->inst->src1].dest_used_by;
            return;
        } else if (fu->locks1) {
            fu->qj = -1;
            fu->vj = fu->inst->src1;
            fu->locks1 = false;
        }
        if (fu->locks2 && USED_AS_DEST(fu->inst->src2)) {
            fu->qk = registers[fu->inst->src2].dest_used_by;
            return;
        } else if (fu->locks2) {
            fu->qk = -1;
            fu->vk = fu->inst->src2;
            fu->locks2 = false;
        }
        fu->time_left--;
        // Finished executing
        if (fu->time_left == 1) {
            fu->inst->exec = ticks;
        } // Write-back
        else if (fu->time_left == 0) {
            fu->inst->write = ticks;
            registers[fu->inst->dest].value = std::string("VAL(");
            registers[fu->inst->dest].value += str_fus[fu->id + 1];
            registers[fu->inst->dest].value += std::string(")");
            // undo rename
            undo_rename(fu->inst->dest);
            undo_rename(fu->inst->src1);
            undo_rename(fu->inst->src2);
            // place inst to reorder buffer

            reorder_buffer.push_back(fu->inst);
            fu->busy = false;
            fu->inst = nullptr;
            fu->time_left = -1;
            fu->vj = 0;
            fu->vk = 0;
            fu->qj = -1;
            fu->qk = -1;
            fu->locks1 = false;
            fu->locks2 = false;
        }
    } else {
        if (fu->inst == nullptr) {
            return;
        }
        fu->busy = true;
        fu->time_left = fu->inst->time;
        registers[fu->inst->dest].used_as = dest;
        registers[fu->inst->dest].dest_used_by = fu->id;
        if (!USED_AS_DEST(fu->inst->src1))
            registers[fu->inst->src1].used_as = src;
        if (!USED_AS_DEST(fu->inst->src2))
            registers[fu->inst->src2].used_as = src;
        if (USED_AS_DEST(fu->inst->src1)) {
            fu->qj = registers[fu->inst->src1].dest_used_by;
            fu->vj = 0;
            fu->locks1 = true;
        } else {
            fu->qj = -1;
            fu->vj = fu->inst->src1;
        }
        if (USED_AS_DEST(fu->inst->src2)) {
            fu->qk = registers[fu->inst->src2].dest_used_by;
            fu->vk = 0;
            fu->locks2 = true;
        } else {
            fu->qk = -1;
            fu->vk = fu->inst->src2;
        }
    }
}

static unsigned next_reorder = 0;

void reorder()
{
    auto i = std::find(reorder_buffer.begin(), reorder_buffer.end(), &inst_list[next_reorder]);
    while (i != reorder_buffer.end() && next_reorder < inst_list.size()) {
        // commit
        (*i)->commit = ticks;
        // remove from reorder buffer
        reorder_buffer.erase(i);
        next_reorder++;
        i = std::find(reorder_buffer.begin(), reorder_buffer.end(), &inst_list[next_reorder]);
    }
}

int exec_cycle()
{
    int ret = 1;
    // check if all instructions are done
    for (int i = 0; i < STATION_TYPES; i++) {
        for (int j = 0; j < station_sizes[i]; j++) {
            if (all_stations[i][j].busy) {
                ret = 0;
            }
        }
    }
    ret = ret && insts.empty();
    if (ret)
        return ret;
    // do next clock cycle
    ticks++;
    // issue next instruction
    issue();
    // exec all stations
    for (int i = 0; i < STATION_TYPES; i++) {
        for (int j = 0; j < station_sizes[i]; j++) {
            exec_fu(&all_stations[i][j]);
        }
    }
    // reorder buffer
    reorder();

    return ret;
}

std::vector<inst_t> read_instructions(std::string filename)
{
    std::vector<inst_t> code;
    static std::map<std::string, op_t> op_map({
        {"add", add}, {"sub", sub}, {"mul", mul}, {"div", divd}, {"lw ", lw}, {"sw ", sw}
    });
    static std::map<std::string, reg_t> reg_map({
        {"r0", r0}, {"r1", r1}, {"r2", r2}, {"r3", r3}, {"r4", r4}, {"r5", r5},
        {"r6", r6}, {"r7", r7}, {"r8", r8}, {"r9", r9}, {"r10", r10}, {"r11", r11}, 
        {"ra", ra}, {"rb", rb}, {"rc", rc}, {"rd", rd}, {"re", re}, {"rf", rf}, 
        {"rg", rg}, {"rh", rh}, {"ri", ri}, {"rj", rj}, {"rk", rk}, {"rl", rl}, 
        {"rm", rm}, {"rn", rn}, {"ro", ro}, {"rp", rp}, {"rq", rq}, {"rr", rr}, 
        {"rs", rs}, {"rt", rt}, {"ru", ru}, {"rv", rv}, {"rw", rw}, {"rx", rx}
    });
    static int op_time[6] = { add_time, mul_time, lw_time, sub_time, div_time, sw_time };
    std::string raw_inst;
    std::ifstream file(filename);
    if (file.is_open()) {
        while (std::getline(file, raw_inst)) {
            // convert to lowercase
            std::transform(raw_inst.begin(), raw_inst.end(), raw_inst.begin(), [](unsigned char c) { 
                return std::tolower(c); 
            });
            // convert string to inst
            inst_t i;
            size_t comma1, comma2, paren;
            i.issue = 0;
            i.exec = 0;
            i.write = 0;
            i.commit = 0;
            i.imm = -1;
            i.op = op_map[raw_inst.substr(0, 3)];
            if (i.op == sw || i.op == lw) {
                comma1 = raw_inst.find(',');
                if (i.op == sw) {
                    i.dest = noreg;
                    i.src1 = reg_map[raw_inst.substr(3, comma1 - 3)];
                } else {
                    i.dest = reg_map[raw_inst.substr(3, comma1 - 3)];
                    i.src1 = noreg;
                }
                paren = raw_inst.find('(');
                i.imm = std::stoi(raw_inst.substr(comma1 + 2, paren - comma1 - 2));
                i.src2 = reg_map[raw_inst.substr(paren + 1, raw_inst.length() - paren - 2)];
                i.time = op_time[i.op];             
            } else {
                comma1 = raw_inst.find(',');
                i.dest = reg_map[raw_inst.substr(4, comma1 - 4)];
                comma2 = raw_inst.find(',', comma1 + 1);
                i.src1 = reg_map[raw_inst.substr(comma1 + 2, comma2 - comma1 - 2)];
                i.src2 = reg_map[raw_inst.substr(comma2 + 2, raw_inst.length() - comma2 - 2)];
                i.time = op_time[i.op];
            }
            code.push_back(i);
        }
        file.close();
    }
    // makes insts a deque of pointers to each code inst
    for (auto &i : code) {
        insts.push_back(&i);
    }
    return code;
}

void fus()
{
    std::cout << "========================================= Functional Units =======================================\n";
    std::cout << "Time\tFU\tBusy\tOp\tVi\tVj\tVk\tQj\tQk\n";
    for (int i = 0; i < STATION_TYPES; i++) {
        for (int j = 0; j < station_sizes[i]; j++) {
            if (all_stations[i][j].time_left != -1) {
                std::cout << all_stations[i][j].time_left;
            }
            std::cout << "\t" << str_fus[(2 * i) + j + 1] << "\t" << all_stations[i][j].busy << "\t";
            if (all_stations[i][j].busy) {
                std::cout << str_op[all_stations[i][j].inst->op];
            } else {
                std::cout << "-";
            }
            std::cout << "\t"; 
            if (all_stations[i][j].inst != nullptr) {
                std::cout << str_reg[all_stations[i][j].inst->dest];
            } else {
                std::cout << "-";
            }
            std::cout << "\t" << str_reg[all_stations[i][j].vj] << "\t" << str_reg[all_stations[i][j].vk] << "\t" << str_fus[all_stations[i][j].qj + 1] << "\t" << str_fus[all_stations[i][j].qk + 1] << "\n";
        }
    }
    std::cout << "==================================================================================================\n";
}

void show_registers()
{
    std::cout << "============================================= Registers ==========================================\n";
    std::cout << "Visible Registers | Invisible Registers\n";
    for (int i = 1; i <= VISIBLE_REGISTERS; i++) {
        std::cout << str_reg[i] << ": " << registers[i].value << "\t\t    " << str_reg[i + VISIBLE_REGISTERS] << ": " << registers[i + VISIBLE_REGISTERS].value << "\t" << str_reg[i + (2 * VISIBLE_REGISTERS)] << ": " << registers[i + (2 * VISIBLE_REGISTERS)].value << "\n";
    }
    std::cout << "==================================================================================================\n";
}

void status(std::vector<inst_t> inst_list)
{
    std::cout << "============================================= Status =============================================\n";
    std::cout << "\tinst\ti\tj\tk\tissue\tcomplete    write-back\tcommit\n";
    for (size_t i = 0; i < inst_list.size(); i++) {
        std::cout << i << "\t" << str_op[inst_list[i].op] << "\t" << str_reg[inst_list[i].dest] << "\t" << str_reg[inst_list[i].src1] << "\t" << str_reg[inst_list[i].src2] << "\t  ";
        if (!inst_list[i].issue) {
            std::cout << "-";
        } else {
            std::cout << inst_list[i].issue;
        }
        std::cout << "\t    "; 
        if (!inst_list[i].exec) {
            std::cout << "-";
        } else {
            std::cout << inst_list[i].exec;
        } 
        std::cout << "\t\t";
        if (!inst_list[i].write) {
            std::cout << "-\t  ";
        } else {
            std::cout << inst_list[i].write << "\t  ";
        }
        if (!inst_list[i].commit) {
            std::cout << "-\n";
        } else {
            std::cout << inst_list[i].commit << "\n";
        }
    }
    std::cout << "==================================================================================================\n";
}

void help()
{
    std::cout << "Avaiable Commands:\n";
    std::cout << "\tclock (c) - display current clock cycle\n";
    std::cout << "\texit (e) - exit simulator\n";
    std::cout << "\thelp (h) - display this message\n";
    std::cout << "\tfus (f) - display functional units\n";
    std::cout << "\tnext (n) - execute next clock cycle\n";
    std::cout << "\tregisters (r) - display registers\n";
    std::cout << "\tstatus (s) - display status of instructions\n";
}

int main(int argc, char *argv[])
{
    if (argc < 1) {
        std::cout << "Usage: ./tomasulo <input-file>\n";
        return 1;
    }
    std::cout << "Tomasulo's Algorithm simulator\n";
    inst_list = read_instructions(argv[1]);
    if (inst_list.empty()) {
        std::cout << "Invalid file name\n";
        return 1;
    }
    std::cout << "Instructions loaded\n";
    std::string input;
    init_fus();
    while (1) {
        std::cout << ">";
        std::getline(std::cin, input);
        if (input == "help" || input == "h") {
            help();
        } else if (input == "registers" || input == "r") {
            show_registers();
        } else if (input == "fus" || input == "f") {
            fus();
        } else if (input == "status" || input == "s") {
            status(inst_list);
        } else if (input == "next" || input == "n") {
            if (exec_cycle()) {
                std::cout << "All operations done\n";
            } else {
                std::cout << "Cycle: " << ticks << "\n";
            }
        } else if (input == "clock" || input == "c") {
            std::cout << "Cycle: " << ticks << "\n";
        } else if (input == "exit" || input == "e") {
            break;
        } else {
            std::cout << "Invalid command\n";
            help();
        }
    }
    std::cout << "Simulation complete (press enter to exit)" << std::endl;
    std::cin.get();
    return 0;
}