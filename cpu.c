#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

#include "cpu.h"

// 1 Mhz in nanoseconds
#define CPU_TICK                1000           
#define STACK_START             0x8000
#define STACK_LIMIT             0x2000

extern void disassemble(u16 *mem, u16 ip, char *out);

// Load init functions for devices
// Init function always returns the device id
extern unsigned int floppy_init(struct cpu *p);
extern void floppy_handle_interrupt(u16 hwi);

extern unsigned int monitor_init(struct cpu *p);
extern void monitor_handle_interrupt(u16 hwi);

void error(const char *format, ...) {
        char buf_1[512] = {0};
        char buf_2[256] = {0};
        va_list va;

        va_start(va, format);

        strcat(buf_1, "error: ");
        vsprintf(buf_2, format, va);
        strcat(buf_1, buf_2);   

        puts(buf_1);
        exit(1);
}

void *scalloc(size_t size) {
        void *block = NULL;

        if((block = calloc(1, size)) == NULL) {
                error("calloc fail on line %d in %s", __LINE__, __FILE__);
        }

        return block;
}       

// id contains mid and hid
// _init is the init function for the device
// _handle_interrupt is the routine that handles all hardware interrupts
static void add_device(struct cpu *p, u32 (*_init)(struct cpu *), void (*_handle_interrupt)(u16)){
        struct device *dev = NULL;
        u32 id = 0;

        dev = (struct device *)scalloc(sizeof(struct device));

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

static struct device *find_device(struct cpu *p, u16 index) {
        struct device *dev = NULL;

        for(dev = p->devices; dev; dev = dev->next) {
                if(index == dev->index) {
                        return dev;
                }
        }

        return NULL;
}

static u16 count_devices(struct cpu *p) {
        struct device *dev = p->devices;
        u16 ret = 0;
        
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
                error("couldn't open/write to \"%s\"", fn);
        }

        while(i < 0x8000) {
                putc(p->mem[i]>>8, fp);
                putc(p->mem[i++]&0xff, fp);
        }

        fclose(fp);
}

static void debug(struct cpu *p) {
        size_t i;

        printf("%2s: 0x%04X\n", "A", p->r[rA]);
        printf("%2s: 0x%04X\n", "B", p->r[rB]);
        printf("%2s: 0x%04X\n", "C", p->r[rC]);
        printf("%2s: 0x%04X\n", "D", p->r[rD]);
        printf("%2s: 0x%04X\n", "X", p->r[rX]);
        printf("%2s: 0x%04X\n", "Y", p->r[rY]);
        printf("%2s: 0x%04X\n", "Z", p->r[rZ]);
        printf("%2s: 0x%04X\n", "J", p->r[rJ]);
        printf("%2s: 0x%04X\n", "IA", p->ia);
        printf("%2s: 0x%04X\n", "SP", p->sp);
        printf("%2s: 0x%04X\n", "IP", p->ip);
        printf("%2s: 0x%04X\n", "OV", p->ov);
        printf("%2s: %d\n", "CPU CYCLES", p->cycles);
        printf("\n");

        printf("Stack:\n");
        for(i = 0; i < 0x40; i++) {
                printf("%04X ", p->mem[(STACK_START-0x40)+i]);
                if((i+1)%16==0) printf("\n");
        }
}

static void load(struct cpu *p, const char *fn) {
        FILE *fp = NULL;
        u8 b1 = 0, b2 = 0;
        u16 i = 0;

        if((fp = fopen(fn, "rb")) == NULL) {
                error("file \"%s\" not found or not readable", fn);
        }

        while(!feof(fp) && fscanf(fp, "%c%c", &b1, &b2) != EOF) {
                p->mem[i++] = ((b1<<8)|b2)&0xffff;
        }

        fclose(fp);

        p->sp = STACK_START;
}

static u16 *getopr(struct cpu *p, u8 b, u16 *w) {
        u16 *o = NULL;

        if(USINGNW(b)) {
                w = p->mem+(p->ip++);
                p->cycles++;
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
                // OV
                case 0x1c:
                        o = &p->ov;
                        break;
                        
                default: break;
        }
        return o;
}

static int jump(struct cpu *p) {
        u8 wc = 0;
        u16 o, oldo;
        u8 d, s;
        u16 op = *(p->mem+p->ip);

        do {
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

static void step(struct cpu *p) {
        char out[128] = {0};
        u16 op = 0;
        u8 o = 0;
        u16 *d = NULL, *s = NULL, w = 0;
        u16 wc = 0;
        u32 start_cyc = p->cycles;
        struct timespec slp = {
                .tv_sec = 0,
                .tv_nsec = 0
        };

        disassemble(p->mem, p->ip, out);
        puts(out);

        p->cycles++;

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
                        if(*s+*d > 0xffff) 
                                p->ov = *s + *d;

                        *d = (*s+*d); 
                } break;
                // SUB
                case SUB: {
                        signed short cc = (signed short)(*d-*s);
                        if(cc < 0)
                                *d = 0xffff-*s+1;
                        else
                                *d -= *s;
                } break;
                // MUL
                case MUL: *d = *s*(*d); break;
                // DIV
                case DIV: {
                        p->ov = *d  % *s;
                        *d = *d/(*s);
                        p->cycles++;
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
                // MLS
                case MLS: {
                        *d = (s16)*d * (s16)*s;
                } break;
                // DVS
                case DVS: {
                        p->ov = (s16)*d % (s16)*s;
                        *d = (s16)*d / (s16)*s;
                        p->cycles++;
                } break;
                // MDS
                case MDS: {
                        *d = (s16)*d % (s16)*s;
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
                        if((s16)*d <= (s16)*s) p->ip += wc; 
                } break;
                // IFB
                case IFB: {
                        if((s16)*d >= (s16)*s) p->ip += wc;
                } break;
                // JMP
                case JMP: {
                        p->ip = *d;
                } break;
                // JTR
                case JTR: {
                        // push IP on stack
                        p->mem[--p->sp] = p->ip;
                        p->cycles++;
                        // jump
                        p->ip = *d; 
                } break;
                // PUSH
                case PUSH: {
                        p->mem[--p->sp] = *d;
                } break;
                // POP
                case POP: {
                        *d = p->mem[p->sp++];
                } break;
                // RET
                case RET: {
                        // pop IP from stack
                        p->ip = p->mem[p->sp++];
                } break;
                // RETI
                case RETI: {
                        // pop IP from stack
                        p->ip = p->mem[p->sp++];
                        // set IA to 0, means we are returning
                        // from interrupt routine
                        // p->ia = 0;
                } break;
                // IAR 
                case IAR: {
                        // set IA to a value
                        // means we are expecting an interrupt routine
                        // to be entered or a hardware interrupt to
                        // be triggered
                        p->ia = *d;

                } break;
                // INT
                case INT: {
                        // set A to the message
                        p->r[ rA ] = *d;
                        // push return IP to stack
                        p->mem[--p->sp] = p->ip;
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
                        dev->_handle_interrupt(p->ia);
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
                default: break;
        }

        slp.tv_nsec = (p->cycles-start_cyc)*CPU_TICK;
        nanosleep(&slp, NULL);
}

int main(int argc, char **argv) {
        char i_fn[128] = {0};
        char o_fn[128] = {0};
        static struct cpu p;

        if(argc < 3) {
                error("usage: %s <program> <memory_dump>", argv[0]);
        }
        strcpy(i_fn, argv[1]);
        strcpy(o_fn, argv[2]);

        // Init cpu and load the program
        load(&p, i_fn);
        
        // Add floppy
        // MID: 0x32ba
        // HID: 0x236e
        // _init = floppy_init()
        // _handle_interrupt = floppy_handle_interrupt(u16)
        add_device(&p, floppy_init, floppy_handle_interrupt);

        // Add monitor
        // MID: 0xff21
        // HID: 0xbeba
        // _init = monitor_init()
        // _handle_interrupt = monitor_handle_interrupt(u16)
        add_device(&p, monitor_init, monitor_handle_interrupt);

        u32 run = 0;
        u8 c = 0;
        u32 w1 = 0, w2 = 0;
        do {
                if(p.sp < STACK_START-STACK_LIMIT-1) {
                        error("STACK_LIMIT(%04X) reached", STACK_LIMIT);
                }

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
        } while(p.mem[p.ip]);

        memory_dmp(&p, o_fn);

        free_devices(&p);
        return 0;
}