#include "cpu.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>

/*
        TODO:

        1. Make devices functional :)
*/

// 1 Mhz in nanoseconds
#define CPU_TICK                1000           
#define STACK_START             0x8000

extern void disassemble(uint16_t *mem, uint16_t ip, char *out);

// Load init functions for devices
// Init function always returns the device id
extern uint32_t keyboard_init(struct cpu *proc);
extern void keyboard_handle_interrupt();

extern uint32_t floppy_init(struct cpu *proc);
extern void floppy_handle_interrupt();

extern uint32_t monitor_init(struct cpu *proc);
extern void monitor_handle_interrupt();

// id contains mid and hid
// _init is the init function for the device
// _handle_interrupt is the routine that handles all hardware interrupts trigged
// on the device
static void add_device(struct cpu *p, uint32_t (*_init)(struct cpu *), void (*_handle_interrupt)()){
        struct device *dev = NULL;
        uint32_t id = 0;

        dev = NEW_NODE(struct device);

        if(!p->devices) 
                dev->index = 0;
        else 
                dev->index = p->devices->index+1;

        dev->_init = _init;
        dev->_handle_interrupt = _handle_interrupt;

        // Init the device
        // _init function returns the device id
        id = dev->_init(p);
        dev->mid = (id>>16)&0xffff;
        dev->hid = id&0xffff;

        dev->next = p->devices;
        p->devices = dev;
}

static struct device *find_device(struct cpu *p, uint16_t index) {
        struct device *dev = NULL;

        ENUM_LIST(dev, p->devices) {
                if(index == dev->index) {
                        return dev;
                }
        }

        return NULL;
}

static uint16_t count_devices(struct cpu *p) {
        struct device *dev = p->devices;
        uint16_t ret = 0;
        
        while(dev) {
                ret++;
                dev = dev->next;
        }

        return ret;
}

static void free_devices(struct cpu *p) {
        if(!p->devices) return;
        
        struct device *head = p->devices, *tmp = NULL;
        
        while(head) {
                tmp = head;
                head = head->next;
                free(tmp);
        }
        
        free(head);
        p->devices = NULL;
}

static void memory_dmp(struct cpu *p, const char *fn) {
        FILE *fp = NULL;
        size_t i = 0;

        if((fp = fopen(fn, "wb")) == NULL) {
                free_devices(p);
                die("error: couldn't open/write to \"%s\"", fn);
        }

        while(i < 0x8000) {
                putc(p->mem[i]>>8, fp);
                putc(p->mem[i++]&0xff, fp);
        }

        fclose(fp);
}

#define REG_DBG(r, a)              printf("%2s: 0x%04x (%d)\n", r, a, (int16_t)a)
#define SPC_DBG(s, a)              printf("%2s: 0x%04x\n", s, a)
static void debug(struct cpu *p) {
        size_t i;

        REG_DBG( "A",   p->r[rA] );
        REG_DBG( "B",   p->r[rB] );
        REG_DBG( "C",   p->r[rC] );
        REG_DBG( "D",   p->r[rD] );
        REG_DBG( "J",   p->r[rJ] );
        REG_DBG( "X",   p->r[rX] );
        REG_DBG( "Y",   p->r[rY] );
        REG_DBG( "Z",   p->r[rZ] );
        SPC_DBG( "SP",  p->sp );
        SPC_DBG( "IP",  p->ip );
        SPC_DBG( "IA",  p->ia );
        REG_DBG( "O",   p->o );
        REG_DBG( "CPU CYCLES", p->cycles );
        printf("\n");

        printf("Stack:\n");
        for(i = 0; i < 0x40; i++) {
                printf("%04X ", p->mem[(STACK_START-0x40)+i]);
                if((i+1)%16==0) printf("\n");
        }
}
#undef SPC_DBG
#undef REG_DBG

static void load(struct cpu *p, const char *fn) {
        FILE *fp = NULL;
        uint8_t b1 = 0, b2 = 0;
        uint16_t i = 0;

        if((fp = fopen(fn, "rb")) == NULL) {
                free_devices(p);
                die("error: file \"%s\" not found or not readable", fn);
        }

        while(!feof(fp) && fscanf(fp, "%c%c", &b1, &b2) != EOF) {
                p->mem[i++] = ((b1<<8)|b2)&0xffff;
        }

        fclose(fp);

        p->sp = STACK_START;
}

#define ADD_CYCLES(p, n)    (p)->cycles += n

static uint16_t *getopr(struct cpu *p, uint8_t b, uint16_t *w) {
        uint16_t *o = NULL;

        if(USINGNW(b)) {
                w = p->mem+(p->ip++);
                ADD_CYCLES(p, 1);
        }

        switch(b) {
                // register
                case rA: case rB: case rC: case rD:
                case rX: case rY: case rZ: case rJ:
                        o = p->r+b;
                        break;
                // [register]
                case 0x08: case 0x09: case 0x0a: case 0x0b:
                case 0x0c: case 0x0d: case 0x0e: case 0x0f:
                        o = p->mem+p->r[b-0x08];
                        break;
                // [register+nextw]
                case 0x10: case 0x11: case 0x12: case 0x13:
                case 0x14: case 0x15: case 0x16: case 0x17:
                        o = p->mem+(p->r[b-0x10]+*w);
                        break;
                // nextw
                case 0x18:
                        o = w;
                        break;
                // [nextw]
                case 0x19:
                        o = p->mem+*w;
                        break;
                // SP
                case 0x1a:
                        o = &p->sp;
                        break;
                // IP
                case 0x1b:
                        o = &p->ip;
                        break;
                // O
                case 0x1c:
                        o = &p->o;
                        break;

                default: break;
        }
        return o;
}

static int jump(struct cpu *p) {
        uint8_t wc = 0;
        uint16_t o, oldo;
        uint8_t d, s;
        uint16_t op = *(p->mem+p->ip);

        do {
                ADD_CYCLES(p, 1);

                wc++;
                o = OPO(op);
                d = OPD(op);
                s = OPS(op);

                oldo = o;

                if(USINGNW(d)) wc++;
                if(USINGNW(s)) wc++;
                op = *(p->mem+p->ip+wc);
        } while(oldo >= IFE && oldo <= IFB);

        return wc;
}

void halt_cpu(struct cpu *p) {
        p->halt = 1;
}

static void step(struct cpu *p) {
        char out[128] = {0};
        uint16_t op = 0;
        uint8_t o = 0;
        uint16_t *d = NULL, *s = NULL, w = 0;
        uint16_t wc = 0;
        uint32_t start_cyc = p->cycles;

        disassemble(p->mem, p->ip, out);
        puts(out);

        op = p->mem[p->ip++];
        o = OPO(op);
        d = getopr(p, OPD(op), &w);
        s = getopr(p, OPS(op), &w);
        wc = jump(p);

        switch(o) {
                // STO
                case STO: *d = *s; break;
                // ADD
                case ADD: {
                        p->o = ((*s + *d) > 0xffff ? 1 : 0);
                        *d = (*s+*d); 
                } break;
                // SUB
                case SUB: {
                        if(*d < 0) *d = ~(*d);
                        if(*s < 0) *s = ~(*s);

                        p->o = ((*d - *s) < 0 ? 0xffff : 0);
                        *d -= *s;
                } break;
                // MUL
                case MUL: *d = *s*(*d); break;
                // DIV
                case DIV: {
                        p->o = *d  % *s;
                        *d = *d/(*s);
                } break;
                // MOD
                case MOD: {
                        *d = (*d)%(*s);
                } break;
                // NOT
                case NOT: {
                        *d = ~(*d);
                } break;
                // AND
                case AND: {
                        *d = (*d)&(*s);
                } break;
                // OR
                case OR: {
                        *d = (*d)|(*s);
                } break;
                // XOR
                case XOR: {
                        *d = (*d)^(*s);
                } break;
                // SHL
                case SHL: {
                        *d = (*d)<<(*s);
                } break;
                // SHR
                case SHR: {
                        *d = (*d)>>(*s);
                } break;
                // MULS
                case MULS: {
                        *d = (int16_t)*d * (int16_t)*s;
                } break;
                // DIVS
                case DIVS: {
                        p->o = (int16_t)*d % (int16_t)*s;
                        *d = (int16_t)*d / (int16_t)*s;
                } break;
                // MODS
                case MODS: {
                        *d = (int16_t)*d % (int16_t)*s;
                } break;
                // IFE
                case IFE: {
                        if(*d != *s) p->ip += wc;
                } break;
                // IFN
                case IFN: {
                        if(*d == *s) p->ip += wc;
                } break;
                // IFG
                case IFG: {
                        if(*d <= *s) p->ip += wc;
                } break;
                // IFL
                case IFL: {
                        if(*d >= *s) p->ip += wc;
                } break;
                // IFA
                case IFA: {
                        if((int16_t)*d <= (int16_t)*s) p->ip += wc; 
                } break;
                // IFB
                case IFB: {
                        if((int16_t)*d >= (int16_t)*s) p->ip += wc;
                } break;
                // JMP
                case JMP: {
                        p->ip = *d;
                } break;
                // JTR
                case JTR: {
                        // push IP on stack
                        PUSH(p->ip);
                        // jump
                        p->ip = *d; 
                } break;
                // PUSH
                case PUSH: {
                        PUSH(*d);
                } break;
                // POP
                case POP: {
                        *d = POP();
                } break;
                // RET
                case RET: {
                        // pop IP from stack
                        p->ip = POP();
                } break;
                // RETI
                case RETI: {
                        // pop IP from stack
                        p->ip = POP();
                } break;
                // IAR 
                case IAR: {
                        // set IA to a value
                        p->ia = *d;

                } break;
                // INT
                case INT: {
                        // set A to the message
                        p->r[ rA ] = *d;
                        // push return IP to stack
                        PUSH(p->ip);
                        // jump to IA
                        p->ip = p->ia;
                } break;
                // HWI
                case HWI: {
                        // Find the device by its hardware index
                        // If it doesn't exist, we are done
                        struct device *dev = find_device(p, *d);
                        if(!dev) break;

                        // Call the handle interrupt method
                        // of the device
                        dev->_handle_interrupt();
                } break;
                // HWQ
                case HWQ: {
                        // If the device doesn't exist
                        // set registers A and B to 0
                        struct device *dev = find_device(p, *d);
                        if(dev) {
                                p->r[rA] = dev->mid;
                                p->r[rB] = dev->hid;
                        } else {
                                p->r[rA] = 0;
                                p->r[rB] = 0;
                        }
                } break;
                // HWN
                case HWN: {
                        *d = count_devices(p);
                } break;
                // HLT
                case HLT: {
                        // Halt the execution
                        halt_cpu(p);
                };
                default: break;
        }

        struct timespec slp = {
                .tv_sec = 0,
                .tv_nsec = (p->cycles-start_cyc)*CPU_TICK
        };

        nanosleep(&slp, NULL);
}

int main(int argc, char **argv) {
        char i_fn[128] = {0};
        char o_fn[128] = {0};
        static struct cpu p;

        if(argc < 3) {
                die("usage: %s <program> <memory_dump>", argv[0]);
        }
        strcpy(i_fn, argv[1]);
        strcpy(o_fn, argv[2]);

        // Init cpu and load the main program
        load(&p, i_fn);

        // Add keyboard
        // MID: 0xbeed
        // HID: 0x0011
        // _init = keyboard_init(struct cpu *)
        // handle_interrupt = keyboard_handle_interrupt()
        add_device(&p, keyboard_init, keyboard_handle_interrupt);

        // Add monitor
        // MID: 0xff21
        // HID: 0xbeba
        // _init = monitor_init(struct cpu *)
        // _handle_interrupt = monitor_handle_interrupt()
        add_device(&p, monitor_init, monitor_handle_interrupt);

        // Add floppy
        // MID: 0x32ba
        // HID: 0x236e
        // _init = floppy_init(struct cpu *)
        // _handle_interrupt = floppy_handle_interrupt()
        add_device(&p, floppy_init, floppy_handle_interrupt);

        uint32_t run = 0;
        uint8_t c = 0;
        uint32_t w1 = 0, w2 = 0;
        do {
                if(!run) {
                        c = getchar();
                } else {
                        step(&p);
                        continue;
                }

                if(c == 's') {
                        step(&p);       
                        debug(&p);
                } else if(c == 'd') {
                        scanf("%04X", &w1);
                        printf("[0x%04X] = %04X\n", w1, p.mem[w1]);
                } else if(c == 'e') {
                        scanf("%04X %04X", &w1, &w2);
                        printf("([0x%04X] = %04X) -> %04X\n", w1, p.mem[w1], w2);
                        p.mem[w1] = w2;
                } else if(c == 'r') {
                        run = 1;
                }
        } while(p.mem[p.ip] && !p.halt);

        memory_dmp(&p, o_fn);

        free_devices(&p);
        return 0;
}