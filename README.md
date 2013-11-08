pvm
===

a 16-bit virtual machine, 0x8000 words of memory available (0x10000 bytes) with its own assembly language


```
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
```