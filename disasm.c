#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned short u16;

static const char *opc[] = {
	"SET", "ADD", "SUB", "MUL", "DIV", "MOD",
	"NOT", "AND", "OR", "XOR", "SHL", "SHR",
	"IFE", "IFN", "IFG", "IFL", "JMP", "JTR",
	"PUSH", "POP", "RET", "END"
};

static const char *regs = "ABCDXYZ";

static void dis_opr(u16 *mem, u16 b, u16 ip, char *out) {
	u16 w = 0;
	if(b >= 0x15 && b <= 0x1D) {
		w = mem[ip+1];
	}

	if(b >= 0x00 && b <= 0x06) {
		sprintf(out+strlen(out), "%c", regs[b]);
	} else if(b >= 0x07 && b <= 0x0D) {
		sprintf(out+strlen(out), "[%c]", regs[b-0x07]);
	} else if(b >= 0x0E && b <= 0x14) {
		sprintf(out+strlen(out), "[%c+A]", regs[b-0x0E]);
	} else if(b >= 0x15 && b <= 0x1B) {
		sprintf(out+strlen(out), "[%c+0x%04X]", regs[b-0x15], w);
	} else if(b == 0x1C) {
		sprintf(out+strlen(out), "0x%04X", w);
	} else if(b == 0x1D) {
		sprintf(out+strlen(out), "[0x%04X]", w);
	} else if(b == 0x1E) {
		sprintf(out+strlen(out), "SP");
	} else if(b == 0x1F) {
		sprintf(out+strlen(out), "IP");
	}
}

void disassemble(u16 *mem, u16 ip, char *out) {
	sprintf(out, "%04X:\t", ip);

	u16 ins = mem[ip];
	u16 op = ins&0x3F;

	sprintf(out+strlen(out), "%04X\t", ins);

	if(op > 0x15) {
		sprintf(out+strlen(out), "%s", "DAT");
	} else {
		sprintf(out+strlen(out), "%s ", opc[op]);
	}

	if((op == 6) || (op >= 0x10 && op <= 0x13)) {
		dis_opr(mem, ins>>11, ip, out);
	} else if(op == 0x14) {

	} else if(op == 0x15) {

	} else if(op > 0x1F) {

	} else {
		dis_opr(mem, ins>>11, ip, out);
		sprintf(out+strlen(out), ",");
		dis_opr(mem, (ins>>6)&0x1F, ip, out);
	}	

	sprintf(out+strlen(out), "\n");
}