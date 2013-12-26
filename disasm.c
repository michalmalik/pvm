#include "cpu.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *opc[] = {
	"STO", "ADD", "SUB", "MUL", "DIV", "MOD",
	"NOT", "AND", "OR", "XOR", "SHL", "SHR",
	"MULS", "DIVS", "MODS",
	"IFE", "IFN", "IFG", "IFL", "IFA", "IFB", 
	"JMP", "JTR", "PUSH", "POP", "RET", "RETI",
	"IAR", "INT", "HWI", "HWQ", "HWN", "HLT"
};

static const char *regs = "ABCDXYZJ";

static void dis_opr(uint16_t *mem, uint16_t b, uint16_t ip, char *out) {
	uint16_t w = 0;

	if(USINGNW(b)) {
		w = mem[ip+1];
	}

	// register
	if(b >= 0x00 && b <= 0x07) {
		sprintf(out+strlen(out), "%c", regs[b]);
	// [register]
	} else if(b >= 0x08 && b <= 0x0F) {
		sprintf(out+strlen(out), "[%c]", regs[b-0x08]);
	// [register+nextw]
	} else if(b >= 0x10 && b <= 0x17) {
		sprintf(out+strlen(out), "[%c+0x%04X]", regs[b-0x10], w);
	// nextw
	} else if(b == 0x18) {
		sprintf(out+strlen(out), "0x%04X", w);
	// [nextw]
	} else if(b == 0x19) {
		sprintf(out+strlen(out), "[0x%04X]", w);
	// SP
	} else if(b == 0x1a) {
		sprintf(out+strlen(out), "SP");
	// IP
	} else if(b == 0x1b) {
		sprintf(out+strlen(out), "IP");
	// OV
	} else if(b == 0x1c) {
		sprintf(out+strlen(out), "O");
	}
}

void disassemble(uint16_t *mem, uint16_t ip, char *out) {
	sprintf(out, "%04X:\t", ip);

	uint16_t ins = mem[ip];
	uint16_t op = OPO(ins);
	uint16_t ins_num = COUNT(opc);

	sprintf(out+strlen(out), "%04X\t", ins);

	if(op > ins_num) {
		sprintf(out+strlen(out), "%s", "DAT");
	} else {
		sprintf(out+strlen(out), "%s ", opc[op]);
	}

	// NOT, JMP, JTR, PUSH, POP, IAR, INT, HWI, HWQ, HWN
	if((op == NOT) || (op >= JMP && op <= POP) || (op >= IAR && op <= HWN)) {
		dis_opr(mem, OPD(ins), ip, out);
	// RET, RETI, HLT
	} else if(op == RET || op == RETI || op == HLT) {

	// DAT
	} else if(op > ins_num) {

	// everything else
	} else {
		dis_opr(mem, OPD(ins), ip, out);
		sprintf(out+strlen(out), ",");
		if(OPD(ins) >= 0x10 && OPD(ins) <= 0x19) dis_opr(mem, OPS(ins), ip+1, out);
		else dis_opr(mem, OPS(ins), ip, out);
	}	
}