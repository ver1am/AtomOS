#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>

uint8_t msize(uint8_t* m) {
	uint16_t i = 0;
	for (;m[i] != 0x00;i++);
	return i;
}

// For delete comments in compilation
void trim_c(char* text,char c) {
	for (uint8_t i = 0; text[i] != '\0'; i++) {
		if (text[i] == c) {
			text[i] = '\0';
			break;
		}
	}
}

/*
    \n \r ' '
*/
void trim_end(char* text) {
	uint8_t end = 0;
	while(text[end+1]) end++;
	while(
		text[end] == '\n' ||
		text[end] == '\r' ||
		text[end] == ' '  ||
		text[end] == '\t'
		) {
        text[end--] = '\0';
	}
}

void skip_chars(char* txt, uint8_t* i) {
	while (
		txt[*i] == ' '  ||
		txt[*i] == '\t' || // TAB - 0x09
		txt[*i] == ','  ||
		txt[*i] == '\n' ||
		txt[*i] == '\r'
	) (*i)++;
}

void skip__chars(char** txt) {
	uint8_t i = 0;
	while (
		(*txt)[i] == ' '  ||
		(*txt)[i] == '\t' ||
		(*txt)[i] == ','  ||
		(*txt)[i] == '\n' ||
		(*txt)[i] == '\r'
	) i++;
	*txt = &(*txt)[i];
}

void arg_len(char* txt, uint8_t* i, uint8_t byte) {
	while (
		txt[*i+1] != '\0' &&
		txt[*i+1] != ' '  &&
		txt[*i+1] != ','  &&
		txt[*i+1] != '\n' &&
		txt[*i+1] != '\r' &&
		txt[*i+1] != byte
	) (*i)++;
}

typedef struct {
	char text[20];
	uint32_t address;
} SYMBOL_T;

typedef enum {
	ARG_NONE,
	ARG_REG,
	ARG_INT32,
	ARG_INT8,
	ARG_TAG
} ARGS_T;

typedef struct {
	char* cmdf;
	uint8_t byte[3];
	ARGS_T arg_t;
} CMD_T;

int to_num(char* txt, uint8_t size) {
	int num = 0;
	bool neg = (txt[0] == '-');
	uint8_t i = neg ? 1 : 0;
	for (uint8_t i = 0; i <= size;i++) {
		num = num * 10 + (txt[i] - '0');
	}
	return neg ? -num : num;
}

const char hex_chars[] = "0123456789ABCDEF";

uint8_t hexindex(char hex) {
	uint8_t c = 0;
	for (; hex_chars[c] != '\0' && hex_chars[c] != hex; c++);
	return c;
}

int16_t stepen(int16_t num,uint16_t mon) {
	if (mon == 0) return 1;

	int16_t rnum = num;
	while (--mon) rnum = rnum*num;

	return rnum;
}

/*
	%d - to number
	%s - to tag
*/
bool stringscanf(char* text, char* format,...) {
	va_list args;
	va_start(args,format);

	trim_end(text);
	for (uint8_t i = 0;text[i] != '\0';i++) {
		if (format[i] == '%') {
			if (format[i+1] == 'd') {
				int* num = va_arg(args, int*);
				uint8_t last = i;
				if (text[i] == '0' && text[i+1] == 'x') {
					i += 2;
					last = i;
					arg_len(text,&i,format[i+2]);
					uint8_t idx = 0;
					for (int8_t c = i;last <= c;c--) {
						*num += hexindex(text[c]) * stepen(16,idx++);
					}
				} else {
					arg_len(&text[i],&i,format[i+2]);
					*num = to_num(&text[last],i-last);
				}
				i++;
			} else if (format[i+1] == 's') {
				char* rtxt = va_arg(args,char*);
				uint8_t j = 0;
				while (text[i] != '\0' && text[i] != format[i+2]) {
					if (text[i] != '\n' && text[i] != '\r') {
						rtxt[j++] = text[i];
					}
					i++;
				}
				rtxt[j] = '\0';
			} else if (format[i+1] == 'r') {
				uint8_t* rbyte = va_arg(args,uint8_t*);
				uint8_t first = i;
				arg_len(text,&i,format[i+2]);
				text[i+1] = '\0';

				uint8_t byte = 0xB0;

				// 8 bits B0-B7
				if (i-first == 1) {// If AL CL DL BL AH CH DH BH
					if (strcmp(&text[first],"al")) {
						byte += 0;
					} else if (strcmp(&text[first],"cl")) {
						byte += 0x01;
					} else if (strcmp(&text[first],"dl")) {
						byte += 0x02;
					} else if (strcmp(&text[first],"bl")) {
						byte += 0x03;
					} else if (strcmp(&text[first],"ah")) {
						byte += 0x04;
					} else if (strcmp(&text[first],"ch")) {
						byte += 0x05;
					} else if (strcmp(&text[first],"dh")) {
						byte += 0x06;
					} else if (strcmp(&text[first],"bh")) {
						byte += 0x07;
					}
				} else if (i-first == 2) {
					byte += 0x08;
					if (strcmp(&text[first],"eax")) {
						byte += 0;
					} else if (strcmp(&text[first],"ecx")) {
						byte += 0x01;
					} else if (strcmp(&text[first],"edx")) {
						byte += 0x02;
					} else if (strcmp(&text[first],"ebx")) {
						byte += 0x03;
					} else if (strcmp(&text[first],"esp")) {
						byte += 0x04;
					} else if (strcmp(&text[first],"ebp")) {
						byte += 0x05;
					} else if (strcmp(&text[first],"esi")) {
						byte += 0x06;
					} else if (strcmp(&text[first],"edi")) {
						byte += 0x07;
					}
				}
				*rbyte = byte;
				i += 2;
			}
		} else if (text[i] != format[i]) {
			va_end(args);
			return false;
		}
	}

	va_end(args);
	return true;
}

/*
	%r - Registers REG
	%d - Number ex. F6 = -10 00 00 00 00 INT32
	%s - Tag TAG
*/
CMD_T x86[] = {
	// mov
	{.cmdf = "mov bl, al",.byte = {0x88,0xC3,0},   .arg_t = ARG_NONE},
	{.cmdf = "mov ebx, eax",.byte = {0x89,0xC3,0}, .arg_t = ARG_NONE},
	{.cmdf = "mov bl, al",.byte = {0x88,0xC3,0},   .arg_t = ARG_NONE},
	{.cmdf = "mov %r, %d",.byte = {0,0,0},        .arg_t = ARG_REG},

	{.cmdf = "inc eax", .byte = {0x40,0,0},       .arg_t = ARG_NONE},
	{.cmdf = "cmp eax, %d", .byte = {0x3D,0,0},   .arg_t = ARG_INT32},
	{.cmdf = "jl %s", .byte = {0x7C,0,0},         .arg_t = ARG_TAG},
	{.cmdf = "syscall", .byte = {0x0F,0x05,0},    .arg_t = ARG_NONE},
	{.cmdf = "ret", .byte = {0xC3,0,0},           .arg_t = ARG_NONE},
};

void asm_setsymbol(char* asmcode,SYMBOL_T symbols[],int8_t lastsymbol,uint32_t* pc) {
	char temp[20] = {0};
	char temp0[20] = {0};
	uint8_t ptr = 0;
	trim_c(asmcode,';');
	trim_end(asmcode);
	if (asmcode[0] == '\0') return; // For comments NEEDED REWORK
	for (uint8_t i = 0; i < sizeof(x86) / sizeof(x86[0]); i++) {
		if (stringscanf(asmcode,x86[i].cmdf,temp,temp0)) {

			for (uint8_t j = 0;x86[i].byte[j] != 0;j++) ptr++;

			if (x86[i].arg_t == ARG_INT8) {
				ptr++;
			}
			else if (x86[i].arg_t == ARG_INT32) {
				ptr++;
				ptr++;
				ptr++;
				ptr++;
			} else if (x86[i].arg_t == ARG_TAG) {
				ptr++;
			} else if (x86[i].arg_t == ARG_REG) {
				ptr++;
				ptr++; // 1 bytes TODO
			}

			*pc += ptr;
		} else {
			uint8_t c = 0; // For labels
			arg_len(asmcode,&c,':');
			if (asmcode[c+1] == ':') {
				asmcode[++c] = '\0';
				for (uint8_t i = 0;i < lastsymbol;i++) {
					if (strcmp(symbols[i].text,asmcode) == 0) {
						symbols[i].address = *pc;
						break;
			        	}
				}
		        }
	        }
	}
}

int8_t asm_tobyte(char* asmcode, uint8_t* bytes, SYMBOL_T symbols[], int8_t lastsymbol,uint32_t* pc) {
	char temp[20] = {0};
	char temp0[20] = {0};
	uint8_t ptr = 0;
	bool found = false;
	trim_c(asmcode,';');
	trim_end(asmcode);
	if (asmcode[0] == '\0') goto nfound;
	for (uint8_t i = 0; i < sizeof(x86) / sizeof(x86[0]); i++) {
		if (stringscanf(asmcode,x86[i].cmdf,temp,temp0)) {
			for (uint8_t j = 0; x86[i].byte[j] != 0 ;j++) {
				bytes[ptr++] = x86[i].byte[j];
			}

			if (x86[i].arg_t == ARG_INT8) {
				bytes[ptr++] = (uint8_t)(temp[0] & 0xFF);
			}
			else if (x86[i].arg_t == ARG_INT32) {
				bytes[ptr++] = (uint8_t)(temp[0] & 0xFF);
				bytes[ptr++] = (uint8_t)((temp[1] >> 8) & 0xFF);
				bytes[ptr++] = (uint8_t)((temp[2] >> 16) & 0xFF);
				bytes[ptr++] = (uint8_t)((temp[3] >> 24) & 0xFF);
			} else if (x86[i].arg_t == ARG_TAG) {
				for (;lastsymbol >= 0;lastsymbol--) {
					if (strcmp(symbols[lastsymbol].text,temp) == 0) {
						bytes[ptr] = symbols[lastsymbol].address - *pc;
						break;
					}
				}
				ptr++;
			} else if (x86[i].arg_t == ARG_REG) {
				bytes[ptr++] = temp[0];
				bytes[ptr++] = temp0[0]; // 1 byte TODO
			}
			*pc += ptr;
			found = true;
		}
	}

nfound:
	// For label:
	if (!found) {
		uint8_t i = 0;
		skip_chars(asmcode,&i);
		arg_len(asmcode,&i,':');
		if (asmcode[i+1] != ':' && asmcode[0] != '\0') ptr = -1;
		// Function on start trims ; and
		// if ; is first, asmcode[0] == '\0'
	}
	return ptr;
}

void asm_psymbol(char* buffer,SYMBOL_T symbols[],uint8_t* symbol) {
	uint8_t i = 0;
	skip_chars(buffer,&i);
	char* ptr = &buffer[i];
	arg_len(buffer,&i,':');
	if (buffer[i+1] == ':') {
		buffer[i+1] = '\0';
		uint8_t idx = *symbol;
		strcpy(symbols[idx].text,ptr);
		*symbol = idx + 1;
	}
}

typedef struct {
	bool showbytes;
} SETTINGS_CMP;

void asm_compile(FILE* file,FILE* bin,SETTINGS_CMP* settings) {
	char buffer[128] = {0};
	char* bufferptr = buffer; // Use pointer!!
	// bufferptr is pointer on char starting without \t ' '

	SYMBOL_T symbols[50] = {0}; // For "label:"
	int8_t lastsymbol = 0;

	uint32_t pc = 0;

	// Symbol parser
	while (fgets(buffer,sizeof(buffer),file) != NULL) {
		asm_psymbol(bufferptr,symbols,&lastsymbol);
	}

	clearerr(file);
	fseek(file,0,SEEK_SET);

	while (fgets(buffer,sizeof(buffer),file) != NULL) {
		asm_setsymbol(buffer,symbols,lastsymbol,&pc);
	}

	clearerr(file);
	fseek(file,0,SEEK_SET);

	pc = 0;

	uint8_t error = 0; // 1 = error
	uint16_t lineasm = 0;   // For error debug

	while (fgets(buffer,sizeof(buffer),file) != NULL) {
		skip__chars(&bufferptr);
		uint8_t byte[20] = {0};
		lineasm++;
		int8_t b_c = asm_tobyte(bufferptr,byte,symbols,lastsymbol,&pc);
		/*
			-1 : Not finded asm cmd
			0  : Label/Tag ex. output:
			>0 : Bytes of code
		*/
		if (b_c == -1) {
			error = true;
			break;
		} else if (b_c > 0) {
			fwrite(byte,b_c,1,bin);
			if (settings->showbytes) {
				printf("Line: %u | ",lineasm);
				for (uint8_t i = 0; i < b_c;i++) {
					printf("%X ",byte[i]);
				}
				printf("| %s |\n\r",bufferptr);
			}
		}
	}
	if (error) {
		printf("Error at line: %u | Text: [%s]\n\r",lineasm,buffer);
	} else {
		printf("Compiled!\n\r");
		printf("PC: %u\n\r",pc);
	}
}

int main(int argc,char* argv[]) {
	if (argc < 3) return 1;

	// Compile settings
	SETTINGS_CMP settings = {.showbytes = false};
	if (argc >= 4) { // atasm test bin -bytes
		if (strcmp(argv[3],"-bytes") == 0) {
			settings.showbytes = true;
		}
	}

	FILE* file = fopen(argv[1],"r");
	FILE* bin =  fopen(argv[2],"w");

	if (file == NULL || bin == NULL) {
		perror("Failed to open file");
		return 1;
	}

	asm_compile(file,bin,&settings);

	fclose(file);
	fclose(bin);
	return 0;
}
