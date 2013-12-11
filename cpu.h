#ifndef __CPU_H__
#define __CPU_H__

#include <stdint.h>

#define zero(a)		(memset((a), 0, sizeof((a))))
#define count(a)	(sizeof((a))/sizeof((a)[0]))

#define OPO(op)         (op&0x3f)
#define OPD(op)         (op>>11)
#define OPS(op)         ((op>>6)&0x1f)
#define USINGNW(w)      ((w >= 0x10 && w <= 0x19))

typedef uint8_t u8;
typedef uint16_t u16;
typedef int16_t s16;
typedef uint32_t u32;

enum _registers {
        rA, rB, rC, rD, rX, rY, rZ, rJ
};

enum _instructions {
        STO,
        ADD, SUB, MUL, DIV, MOD,
        NOT, AND, OR, XOR, SHL, SHR,
        MULS, DIVS, MODS,
        IFE, IFN, IFG, IFL, IFA, IFB,
        JMP, JTR, PUSH, POP, RET, RETI,
        IAR, INT, HWI, HWQ, HWN
};

struct cpu {
        u16 r[8];
        u16 sp, ip, ia, o;
        u16 mem[0x8000];
        u32 cycles;

        // Linked list of devices
        struct device *devices;

        u32 interrupt_enabled;
};

// union is not needed, we don't 
// need to access the mid and hid separately
struct device {
        struct device *next;

        u16 index; // hardware index
        u16 mid; // manufacter id
        u16 hid; // hardware id

        u32 (*_init)(struct cpu *);
        void (*_handle_interrupt)(u16);
};

// We want to export these to the devices
void error(const char *format, ...);
void *scalloc(size_t size);

#endif