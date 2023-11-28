#ifndef TOMASULO_H
#define TOMASULO_H

#include <deque>
#include <string>

#define VISIBLE_REGISTERS 12
#define INVISIBLE_REGISTERS 24
#define REGISTERS_MAX (VISIBLE_REGISTERS + INVISIBLE_REGISTERS)

enum reg_t { noreg, r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, ra, rb, rc, rd, re, rf, rg, rh, ri, rj, rk, rl, rm, rn, ro, rp, rq, rr, rs, rt, ru, rv, rw, rx };

// valid operations
enum op_t { add, mul, lw, sub, divd, sw };
enum funum_t { add1, add2, mul1, mul2, load1, load2, store1, store2 };
enum used_as_t { free_reg, dest, src };

struct regstat_t {
    std::string value;
    used_as_t used_as;
    int dest_used_by;
    reg_t renamed_to;
    reg_t renamed_from;
    int rename_ref_count;
};

struct inst_t {
    op_t op;
    reg_t dest;
    reg_t src1;
    reg_t src2;
    int imm;
    int time;
    int issue;
    int exec;
    int write;
    int commit;
};

#define add_time 2
#define sub_time 2
#define mul_time 10
#define div_time 40
#define lw_time 5
#define sw_time 5


// Reserve station sizes
#define ALL_STATIONS 6
#define STATION_TYPES 3
#define ADD_STATIONS 2
#define MUL_STATIONS 2
#define LOAD_STATIONS 2

struct fu_t {
    int id;
    bool busy;
    inst_t *inst;
    int vj;
    int vk;
    int qj;
    int qk;
    int time_left;
    bool locks1;
    bool locks2;
};

#endif // TOMASULO_H