#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned short u16;

static const char *opc[] = {
	"SET", "ADD", "SUB", "MUL", "DIV", "MOD",
	"NOT", "AND", "OR", "XOR", "SHL", "SHR",
	"IFE", "IFN", "IFG", "IFL", "IFGE", "IFLE", 
	"JMP", "JTR", "PUSH", "POP", "RET"
};

static const char *regs = "ABCDXYZJ";

static void dis_opr(u16 *mem, u16 b, u16 ip, char *out) {
	u16 w = 0;
	if(b >= 0x10 && b <= 0x19) {
		w = mem[ip+1];
	}

	if(b >= 0x00 && b <= 0x07) {
		sprintf(out+strlen(out), "%c", regs[b]);
	} else if(b >= 0x08 && b <= 0x0F) {
		sprintf(out+strlen(out), "[%c]", regs[b-0x08]);
	} else if(b >= 0x10 && b <= 0x17) {
		sprintf(out+strlen(out), "[%c+0x%04X]", regs[b-0x10], w);
	} else if(b == 0x18) {
		sprintf(out+strlen(out), "0x%04X", w);
	} else if(b == 0x19) {
		sprintf(out+strlen(out), "[0x%04X]", w);
	} else if(b == 0x1A) {
		sprintf(out+strlen(out), "SP");
	} else if(b == 0x1B) {
		sprintf(out+strlen(out), "IP");
	}
}

void disassemble(u16 *mem, u16 ip, char *out) {
	sprintf(out, "%04X:\t", ip);

	u16 ins = mem[ip];
	u16 op = ins&0x3F;
	u16 ins_num = sizeof(opc)/sizeof(opc[0]);

	sprintf(out+strlen(out), "%04X\t", ins);

	if(op > ins_num) {
		sprintf(out+strlen(out), "%s", "DAT");
	} else {
		sprintf(out+strlen(out), "%s ", opc[op]);
	}

	if((op == 6) || (op >= 0x12 && op <= 0x15)) {
		dis_opr(mem, ins>>11, ip, out);
	} else if(op == 0x16) {

	} else if(op > ins_num) {

	} else {
		dis_opr(mem, ins>>11, ip, out);
		sprintf(out+strlen(out), ",");
		if(ins>>11 >= 0x10 && ins>>11<=0x19) dis_opr(mem, (ins>>6)&0x1F, ip+1, out);
		else dis_opr(mem, (ins>>6)&0x1F, ip, out);
	}	

	sprintf(out+strlen(out), "\n");
}