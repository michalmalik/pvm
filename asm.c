#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

typedef unsigned char u8;
typedef unsigned short u16;

#define zero(a)		(memset((a),0,sizeof((a))))
#define count(a)	(sizeof((a))/sizeof((a)[0]))

u16 MEM[0x8000];
u16 IP = 0;

struct file_asm {
	FILE *fp;
	char i_fn[64];
	char tokenstr[64];
	char linebuffer[256];
	
	int linenumber;

	char *lineptr;

	int token;
	u16 tokennum;
};

struct symbol {
	struct symbol *next;

	char name[128];
	u16 ip;
	u16 num_value;
	char str_value[128];

	struct file_asm f;
};

struct file_asm *cf = NULL;

struct symbol *symbols = NULL;
struct symbol *fixes = NULL;

char *o_fn = NULL;
char *sym_fn = NULL;

char *tn[] = {
	"A", "B", "C", "D", "X", "Y", "Z",
	"SP", "IP",
	"SET", "CMP", 
	"ADD", "SUB", "MUL", "DIV", "MOD",
	"NOT", "AND", "OR", "XOR", "SHL", "SHR",
	"JMP", "JE", "JNE", "JG", "JL", "JGE", "JLE", "JTR",
	"RET", "PUSH", "POP", "END", "DAT", 
	".", "#", ":", ",", "[", "]", "+",
	"(NUMBER)", "(STRING)", "(QUOTED STRING)", "(EOF)"
};

enum _tokens {
	tA, tB, tC, tD, tX, tY, tZ,
	tSP, tIP,
	tSET, tCMP, 
	tADD, tSUB, tMUL, tDIV, tMOD,
	tNOT, tAND, tOR, tXOR, tSHL, tSHR,
	tJMP, tJE, tJNE, tJG, tJL, tJGE, tJLE, tJTR,
	tRET, tPUSH, tPOP, tEND, tDAT,
	tDOT, tHASH, tCOLON, tCOMMA, tBS, tBE, tPLUS,
	tNUM, tSTR, tQSTR, tEOF
};

#define LASTTOKEN	tEOF

void error(const char *format, ...) {
	char buf_1[512];
	char buf_2[256];
	va_list va;

	zero(buf_1);
	zero(buf_2);

	va_start(va, format);

	strcat(buf_1, "error: ");
	vsprintf(buf_2, format, va);
	strcat(buf_1, buf_2);	

	puts(buf_1);
	exit(1);
}

int init_asm(struct file_asm *fs, char *i_fn) {
	fs->fp = NULL;
	zero(fs->i_fn);
	zero(fs->tokenstr);
	zero(fs->linebuffer);

	fs->linenumber = 0;

	fs->lineptr = fs->linebuffer;
	fs->token = 0;
	fs->tokennum = 0;

	strcpy(fs->i_fn, i_fn);

	if((fs->fp = fopen(i_fn, "r")) == NULL) {
		error("file \"%s\" does not exist", fs->i_fn);
	}

	return 1;
}

void output(const char *fn) {
	FILE *fp = NULL;
	size_t i;

	if((fp = fopen(fn, "wb")) == NULL) {
		error("couldn't open/write to \"%s\"", fn);
	}

	for(i = 0; i < count(MEM); i++) {
		fputc((MEM[i]>>8)&0xFF, fp);
		fputc(MEM[i]&0xFF, fp);
	}
	fclose(fp);
}

void add_symbol(const char *name, u16 ip, u16 *num_value, char *str_value) {
	struct symbol *s = NULL;
	for(s = symbols; s; s = s->next) {
		if(!strcmp(name, s->name)) {
			error("symbol \"%s\" already defined", name);
		}
	}
	s = malloc(sizeof(struct symbol));

	zero(s->name);
	zero(s->str_value);

	s->ip = ip;
	strcpy(s->name, name);
	if(num_value) s->num_value = *num_value;
	if(str_value) strcpy(s->str_value, str_value);
	s->f = *cf;
	s->next = symbols;
	symbols = s;
}

void to_fix(const char *name, u16 ip, u16 *num_value, char *str_value) {
	struct symbol *f = malloc(sizeof(struct symbol));

	zero(f->name);
	zero(f->str_value);

	f->ip = ip;
	strcpy(f->name, name);
	if(num_value) f->num_value = *num_value;
	if(str_value) strcpy(f->str_value, str_value);
	f->f = *cf;
	f->next = fixes;
	fixes = f;
}

struct symbol *get_symbol(const char *name) {
	struct symbol *s = NULL;
	for(s = symbols; s; s = s->next) {
		if(!strcmp(s->name, name)) return s;
	}
	return NULL;
}

void fix_symbols() {
	struct symbol *f = NULL, *s = NULL;
	for(f = fixes; f; f = f->next) {
		s = get_symbol(f->name);
		if(!s) error("symbol \"%s\" is not defined", f->name);
		MEM[f->ip] = s->ip+f->num_value;
	}
}

void dump_symbols(const char *fn) {
	FILE *fp = NULL;
	struct symbol *s = NULL;

	if((fp = fopen(fn, "wb")) == NULL) {
		error("couldn't open/write to \"%s\"", fn);
	}

	for(s = symbols; s; s = s->next) {
		if(s->str_value[0]) fprintf(fp, "%s %s 0x%04X \"%s\"\n", s->f.i_fn, s->name, s->ip, s->str_value);
		else fprintf(fp, "%s %s 0x%04X 0x%04X\n", s->f.i_fn, s->name, s->ip, s->num_value);
	}

	fclose(fp);
}

int next_token() {
	char c = 0, *x = 0;
	zero(cf->tokenstr);
next_line:
	if(!*cf->lineptr) {
		if(feof(cf->fp)) return tEOF;
		if(fgets(cf->linebuffer, 256, cf->fp) == 0) return tEOF;
		cf->lineptr = cf->linebuffer;
		cf->linenumber++;
	}

	while(*cf->lineptr <= ' ') {
		if(*cf->lineptr == 0) goto next_line;
		cf->lineptr++;
	}
	
	switch((c = *cf->lineptr++)) {
		case '.': return tDOT;
		case '#': return tHASH;
		case ':': return tCOLON;
		case ',': return tCOMMA;
		case '[': return tBS;
		case ']': return tBE;
		case '+': return tPLUS;
		case ';': {
			*cf->lineptr = 0;
			goto next_line;
		}
		case '"': {
			x = cf->tokenstr;
			for(;;) {
				switch((c = *cf->lineptr++)) {
					case '"': {
						return tQSTR;
						break;
					}
					default: {
						*x++ = c;
						break;
					}
				}
			}
			break;
		}
		default: {
			if(isalpha(c)) {
				cf->lineptr--;				
				x = cf->tokenstr;
				while(isalnum(*cf->lineptr) || *cf->lineptr == '_') {
					*x++ = *cf->lineptr++;
				}
				*x = 0;
				int i;
				for(i = 0; i < LASTTOKEN; i++) {
					if(!strcmp(tn[i], cf->tokenstr)) {
						return i;
					}
				}
				return tSTR;
			} else if(isdigit(c)) {
				cf->lineptr--;
				cf->tokennum = strtoul(cf->lineptr, &cf->lineptr, 0);
				return tNUM;				
			}
			break;	
		}
	}
	return -1;
}

int next() {
	return (cf->token = next_token());		
}

void expect(int t) {
	if(next() != t) {
		error("expected: \"%s\", got: \"%s\"", tn[t], tn[cf->token]);		
	}
}

void assemble_o(u16 *o, int *v) {
	next();
	switch(cf->token) {
		case tA: case tB: case tC: case tD:
		case tX: case tY: case tZ: {
			*o = cf->token&0x7;
			break;
		}

		case tSP: {
			*o = 0x1E;
			break;
		}

		case tIP: {
			*o = 0x1F;
			break;
		}

		case tNUM: {
			*o = 0x1C;
			*v = cf->tokennum;
			break;
		}

		case tSTR: {
			*o = 0x1C;
			char buf[128];
			zero(buf);
			strcpy(buf, cf->tokenstr);
			struct symbol *sym = get_symbol(buf);
			if(sym) {
				*v = sym->ip;
			} else {
				to_fix(buf, IP+1, NULL, NULL);
				*v = 0;
			}
			break;
		}

		default: {
			if(cf->token != tBS) error("expected [");
			break;
		}
	}

	if(cf->token == tBS) {
		next();

		if(cf->token <= 6) {
			*o = cf->token&7;
			next();
			if(cf->token == tPLUS) {
				next();
				if(cf->token == tNUM) {
					*o += 0x15;
					*v = cf->tokennum;
				}
				else if(cf->token == tSTR) {
					*o += 0x15;
					char buf[128];
					zero(buf);
					strcpy(buf, cf->tokenstr);
					struct symbol *sym = get_symbol(buf);
					if(sym) {
						*v = sym->ip;
					} else {
						to_fix(buf, IP+1, NULL, NULL);
						*v = 0;
					}
				} else if(cf->token <= 6) {
					*o += 0x0E;
				} else {
					error("expected [register+nextw]");
				}
				next();
			} else if(cf->token == tBE) {
				*o += 0x7;
			} 

			if(cf->token != tBE) {
				error("expected ]");
			}
		}

		if(cf->token == tNUM) {
			*v = cf->tokennum;
			*o = 0x1D;
			next();
			if(cf->token == tPLUS) {
				next();
				if(cf->token == tNUM) {
					*v += cf->tokennum;
				} else if(cf->token <= 6) {
					*o = (cf->token&7)+0x15;
				} else if(cf->token == tSTR) {
					*o = 0x1D;
					char buf[128];
					zero(buf);
					strcpy(buf, cf->tokenstr);
					struct symbol *sym = get_symbol(buf);
					if(sym) {
						*v += sym->ip;
					} else {
						to_fix(buf, IP+1, (u16 *)v, NULL);
					}
				} else {
					error("expected value");
				}
				next();
			}
		}

		if(cf->token == tSTR) {
			*o = 0x1D;
			*v = 0;
			char buf[128];
			zero(buf);
			strcpy(buf, cf->tokenstr);
			next();
			if(cf->token == tPLUS) {
				next();
				if(cf->token == tNUM) {
					*v += cf->tokennum;
				} else if(cf->token <= 6) {
					*o = (cf->token&0x7)+0x15;
				} else {
					error("expected value");
				}
				next();
			}
			struct symbol *sym = get_symbol(buf);
			if(sym) {
				*v = sym->ip;
			} else {
				to_fix(buf, IP+1, (u16 *)v, NULL);
			}

			if(cf->token != tBE) {
				error("expected ]");
			}
		}
	}
} 

void assemble_i(int inst, u16 d, u16 s, int v1, int v2) {
	u16 o = 0;
	switch(inst) {
		case tSET: o = 1; break;
		case tCMP: o = 2; break;
		case tADD: o = 3; break;
		case tSUB: o = 4; break;
		case tMUL: o = 5; break;
		case tDIV: o = 6; break;
		case tMOD: o = 7; break;
		case tNOT: o = 8; break;
		case tAND: o = 9; break;
		case tOR: o = 0xA; break;
		case tXOR: o = 0xB; break;
		case tSHL: o = 0xC; break;
		case tSHR: o = 0xD; break;
		case tJMP: o = 0xE; break;
		case tJE: o = 0xF; break;
		case tJNE: o = 0x10; break;
		case tJG: o = 0x11; break;
		case tJL: o = 0x12; break;
		case tJGE: o = 0x13; break;
		case tJLE: o = 0x14; break;
		case tJTR: o = 0x15; break;
		case tRET: o = 0x16; break;
		case tPUSH: o = 0x17; break;
		case tPOP: o = 0x18; break;
		case tEND: o = 0x19; break;
	}
	MEM[IP++] = (u16)(o|(s<<6)|(d<<11));
	if(v1 != -1) MEM[IP++] = v1&0xFFFF;		
	if(v2 != -1) MEM[IP++] = v2&0xFFFF;
}

void assemble_dat() {
	u16 b = 0;
	size_t i;

	next();
	while(cf->token == tCOMMA) {
		next();
		if(cf->token == tNUM) {
			MEM[IP++] = cf->tokennum;
		} else if(cf->token == tQSTR) {
			for(i = 0; i < strlen(cf->tokenstr); i++) {
				b = 0;
				b |= cf->tokenstr[i];
				MEM[IP++] = b; 
			}
			MEM[IP++] = 0;		
		}
		next();
	}
}

int assemble() {
	u16 s = 0, d = 0;
	int v1 = -1, v2 = -1;
	int t;
	u16 b = 0;
	size_t i;

	for(;;) {
		next();
again:
		v1 = -1;
		v2 = -1;
		t = cf->token;
		char sym[128];

		switch(t) {
			case tEOF: goto done;
			case tCOLON: {
				expect(tSTR);
				add_symbol(cf->tokenstr, IP, &IP, NULL);
				break;
			}
			case tHASH: {
				expect(tSTR);
				if(!strcmp(cf->tokenstr, "include")) {			
					expect(tQSTR);

					struct file_asm tf = *cf;
					struct file_asm nf;
					
					init_asm(&nf, cf->tokenstr);

					cf = &nf;
					assemble();
					*cf = tf;
				}
				break;
			}
			case tDOT: {
				zero(sym);
				expect(tSTR);
				strcpy(sym, cf->tokenstr);
				expect(tDAT);
				next();
				if(cf->token == tNUM) {
					add_symbol(sym, IP, &cf->tokennum, NULL);
					MEM[IP++] = cf->tokennum;
					
					assemble_dat();
					goto again;
				} else if(cf->token == tQSTR) {
					add_symbol(sym, IP, NULL, cf->tokenstr);
					for(i = 0; i < strlen(cf->tokenstr); i++) {
						b = 0;
						b |= cf->tokenstr[i];
						MEM[IP++] = b; 
					}
					MEM[IP++] = 0;

					assemble_dat();
					goto again;
				}
				break;
			}
			
			case tSET: case tCMP:
			case tADD: case tSUB: case tMUL: case tDIV:
			case tMOD: case tAND: case tOR: case tXOR:
			case tSHL: case tSHR: {
				assemble_o(&d, &v1);
				expect(tCOMMA);				
				assemble_o(&s, &v2);
				assemble_i(t, d, s, v1, v2);
				break;
			}
			case tNOT: case tJMP: case tJE: case tJNE: case tJG:
			case tJL: case tJGE: case tJLE: case tJTR:
			case tPUSH: case tPOP:
			{
				assemble_o(&d, &v1);
				assemble_i(t, d, 0, v1, -1);
				break;	
			}

			case tRET: {
				assemble_i(t, 0, 0, -1, -1);
				break;
			}
			case tEND: {
				assemble_i(t, 0, 0, -1, -1);
				break;
			}
			default: {
				error("invalid instruction, line %d, \"%s\"", cf->linenumber, cf->tokenstr);
				break;
			}
		}
	}
done:
	fclose(cf->fp);
	return 1;
}

int main(int argc, char **argv) {
	if(argc < 4) {
		error("%s <source_file> <program> <symbols>", argv[0]);
	}

	o_fn = argv[2];
	sym_fn = argv[3];

	zero(MEM);

	// Init file_asm structure for the main file
	struct file_asm mf;
	init_asm(&mf, argv[1]); 

	cf = &mf;
	assemble();
	fix_symbols();

	dump_symbols(sym_fn);
	output(o_fn);

	return 0;
}