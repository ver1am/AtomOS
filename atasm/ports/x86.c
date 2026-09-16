#include "port.h"

#ifdef x86

typedef enum {
	ARG_NONE,
	ARG_REGB,
	ARG_REGD,
	ARG_INT32,
	ARG_INT8,
	ARG_TAG,
} ARGS_T;

/*
	%d - to number
	%s - to tag
*/
bool stringscanf(char* text, char* format,...) {
	va_list args;
	va_start(args,format);

	for (uint8_t i = 0;text[i] != '\0';i++) {
		if (format[i] == '%') {
			if (format[i+1] == 'd') {
				int* num = va_arg(args, int*);
				uint8_t last = i;
				*num = 0;
				if (text[i] == '0' && text[i+1] == 'x') {
					i += 2;
					last = i;
					arg_len(text,&i,format[i+2]);
					uint8_t idx = 0;
					for (int8_t c = i;last <= c;c--) {
						*num += hexindex(text[c]) * stepen(16,idx++);
					}
				} else if (text[i] >= '0' && text[i] <= '9') { // TODO For numbers only
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
				char txt[5] = {0};

				uint8_t* rbyte = NULL;
				uint8_t byte = 0;

				uint8_t first = i;
				arg_len(text,&i,format[i+2]);

				uint8_t len = i-first;

				for (uint8_t c = 0;c < (i+1)-first;c++) {
					txt[c] = text[first+c];
				}

				txt[(i+1)-first] = '\0';

				// 8 bits 0-7
				if (len == 1 || len == 2) {// If AL CL DL BL AH CH DH BH
					rbyte = va_arg(args,uint8_t*);
					byte = *rbyte;
					if (strcmp(txt,"al") == 0        || strcmp(txt,"eax") == 0) {
						byte += 0;
					} else if (strcmp(txt,"cl") == 0 || strcmp(txt,"ecx") == 0) {
						byte += 0x01;
					} else if (strcmp(txt,"dl") == 0 || strcmp(txt,"edx") == 0) {
						byte += 0x02;
					} else if (strcmp(txt,"bl") == 0 || strcmp(txt,"ebx") == 0) {
						byte += 0x03;
					} else if (strcmp(txt,"ah") == 0 || strcmp(txt,"esp") == 0) {
						byte += 0x04;
					} else if (strcmp(txt,"ch") == 0 || strcmp(txt,"ebp") == 0) {
						byte += 0x05;
					} else if (strcmp(txt,"dh") == 0 || strcmp(txt,"esi") == 0) {
						byte += 0x06;
					} else if (strcmp(txt,"bh") == 0 || strcmp(txt,"edi") == 0) {
						byte += 0x07;
					}
				} else {
					i = first;
					continue;
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

CMD_T arc[] = {
	// mov
	{.cmdf = "mov bl, al",.byte = {0x88,0xC3,0},   .arg_t = {ARG_NONE,ARG_NONE,0}},
	{.cmdf = "mov ebx, eax",.byte = {0x89,0xC3,0}, .arg_t = {ARG_NONE,ARG_NONE,0}},
	{.cmdf = "mov %r, %d",.byte = {0,0,0},         .arg_t = {ARG_REGB,ARG_INT8,0}},

	// call
	{.cmdf = "call %r",.byte = {0xFF,0,0},         .arg_t = {ARG_REGD,ARG_NONE,0}},
	{.cmdf = "call %d",.byte = {0xE8,0,0},         .arg_t = {ARG_INT32,ARG_NONE,0}},

	{.cmdf = "inc eax", .byte = {0x40,0,0},        .arg_t = {ARG_NONE,ARG_NONE,0}},
	{.cmdf = "cmp eax, %d", .byte = {0x3D,0,0},    .arg_t = {ARG_INT32,ARG_NONE,0}},
	{.cmdf = "jl %s", .byte = {0x7C,0,0},          .arg_t = {ARG_TAG,ARG_NONE,0}},
	{.cmdf = "syscall", .byte = {0x0F,0x05,0},     .arg_t = {ARG_NONE,ARG_NONE,0}},
	{.cmdf = "ret", .byte = {0xC3,0,0},            .arg_t = {ARG_NONE,ARG_NONE,0}},
};

uint8_t arc_count = sizeof(arc) / sizeof(arc[0]);

GENED_T genbytecode(COMP_T* s) { // mode | 0 for gen | 1 for calc size
	GENED_T result;
	result.error = 1;

	uint8_t ptr = 0;
	for (uint8_t i = 0; i < arc_count; i++) {
		char temp[20];
		char temp0[20];
		if (arc[i].arg_t[0] == ARG_REGB) {
			temp0[0] = 0xB0;
			temp[0] =  0xB0;
		}  else if (arc[i].arg_t[0] == ARG_REGD) {
			temp0[0] = 0xD0;
			temp[0] =  0xD0;
		}

		if (stringscanf(s->asmcode,arc[i].cmdf,temp,temp0)) {
			for (uint8_t j = 0; arc[i].byte[j] != 0 ;j++) {
				if (!s->mode) s->bytes[ptr++] = arc[i].byte[j]; else ptr++;
			}

			if (arc[i].arg_t[0] == ARG_INT8) {
				if (!s->mode) s->bytes[ptr++] = (uint8_t)(temp[0] & 0xFF); else ptr++;
			} else if (arc[i].arg_t[0] == ARG_INT32) {
				if (!s->mode) {
					s->bytes[ptr++] = (uint8_t)(temp[0] & 0xFF);
					s->bytes[ptr++] = (uint8_t)((temp[1] >> 8) & 0xFF);
					s->bytes[ptr++] = (uint8_t)((temp[2] >> 16) & 0xFF);
					s->bytes[ptr++] = (uint8_t)((temp[3] >> 24) & 0xFF);
				} else ptr += 4;
			} else if (arc[i].arg_t[0] == ARG_TAG) {
				if (!s->mode) {
					for (;s->sym->lastsymbol >= 0;s->sym->lastsymbol--) {
						if (strcmp(s->sym->symbols[s->sym->lastsymbol].text,temp) == 0) {
							s->bytes[ptr] = s->sym->symbols[s->sym->lastsymbol].address - *s->pc;
							break;
						}
					}
				}
				ptr++;
			} else if (arc[i].arg_t[0] == ARG_REGB || arc[i].arg_t[0] == ARG_REGD) {
				if (!s->mode) s->bytes[ptr++] = temp[0]; else ptr++;
				if (arc[i].arg_t[1] == ARG_INT8 && !s->mode) s->bytes[ptr++] = temp0[0]; else if (arc[i].arg_t[1] == ARG_INT8 && s->mode) ptr++;
			}

			result.error = 0;
			break;
		}
        }
	result.gened = ptr;

	return result;
}

#endif
