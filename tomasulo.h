#ifndef TOMASULO_H
#define TOMASULO_H

#include <deque>

#define VISIBLE_REGISTERS 10
#define INVISIBLE_REGISTERS 10
#define REGISTERS_MAX (VISIBLE_REGISTERS + INVISIBLE_REGISTERS)

enum reg_t { noreg = -1, r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, ra, rb, rc, rd, re, rf, rg, rh, ri, rj };
enum op_t { add, mul, lw, sub, divd, sw };
enum reservation_stations { add1, add2, mul1, mul2, load1, load2, store1, store2 };

struct inst {
    op_t op;
    reg_t dest;
    reg_t src1;
    reg_t src2;
    int imm;
    int time;
};

#define add_time 2
#define sub_time 2
#define mul_time 10
#define div_time 40
#define lw_time 5
#define sw_time 5


// Reserve station sizes
#define STATION_TYPES 3

#define ADD_STATIONS 2
#define MUL_STATIONS 2
#define LOAD_STATIONS 2

struct reserve_station {
    bool busy;
    inst curr_inst;
    int vj;
    int vk;
    int qj;
    int qk;
    std::deque<inst> next_insts;
};

#endif // TOMASULO_H