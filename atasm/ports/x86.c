#include "port.h"

#ifdef x86

typedef enum {
	ARG_NONE,
	ARG_REGB,
	ARG_REGD,
	ARG_INT32,
	ARG_INT8,
	ARG_UNINT,
	ARG_TAG,
} ARGS_T;

/*
    %d - to number
    %s - to tag
    %r - to register
*/
bool stringscanf(char* text, char* format, ...) {
	va_list args;
	va_start(args, format);

	uint8_t text_idx = 0;
	uint8_t fmt_idx = 0;

	while (text[text_idx] != '\0' && format[fmt_idx] != '\0') {
		// In future in "debug" printf("idx: %d | %c | %c%c\n\r",text_idx,text[text_idx],format[fmt_idx],format[fmt_idx+1]);
		if (format[fmt_idx] == '%') {
			char spec = format[fmt_idx + 1];
			char delim = format[fmt_idx + 2]; // Next symbol after format symbol

			if (spec == 'd') {
				SCANR_T* rscan = va_arg(args, SCANR_T*);

				rscan->text = false; // This is BOOL
				rscan->num = 0; // Returnable value

				// HEX
				if (text[text_idx] == '0' && (text[text_idx + 1] == 'x' || text[text_idx + 1] == 'X')) {
					text_idx += 2;
					uint8_t last = text_idx;
					arg_len(text,&text_idx,delim);

					uint8_t idx = 0;
					for (int8_t c = text_idx; c >= (int8_t)last; c--) {
						rscan->num += hexindex(text[c]) * stepen(16, idx++);
					}
				} else if (text[text_idx] >= '0' && text[text_idx] <= '9') { // Just numbers
					uint8_t last = text_idx;
					// 56
					// 01
					arg_len(text,&text_idx,delim);
					rscan->num = to_num(&text[last], (text_idx+1) - last); // +1 NOTE
				} else {
					va_end(args);
					return false;
				}
				fmt_idx += 2; // skip %d
			} else if (spec == 's') {
				SCANR_T* rscan = va_arg(args, SCANR_T*);

				rscan->text = true; // returns text
				uint8_t j = 0;

				while (text[text_idx] != '\0' && text[text_idx] != delim &&
					text[text_idx] != '\n' && text[text_idx] != '\r')
				{
					rscan->txt[j++] = text[text_idx++]; // Copying text
				}
				rscan->txt[j] = '\0';
				fmt_idx += 2;
			} else if (spec == 'r') {
				char txt[5] = {0};
				uint8_t byte = 0;

				SCANR_T* rscan = va_arg(args, SCANR_T*);

				uint8_t j = arg_copy(&text[text_idx],txt,delim);
				txt[j] = '\0';

				byte = rscan->bytes[0]; // Taking byte ex. 0xB0 0xD0

				if (strcmp(txt, "al") == 0 || strcmp(txt, "eax") == 0 || strcmp(txt, "ax") == 0) {
					byte += 0;
					if (strcmp(txt, "eax") == 0) rscan->bytes[5] = 0x04; // How much bytes written
					else if (strcmp(txt, "ax") == 0) rscan->bytes[5] = 0x02;
					else rscan->bytes[5] = 0x01;
				} else if (strcmp(txt, "cl") == 0 || strcmp(txt, "ecx") == 0 || strcmp(txt, "cx") == 0) {
					byte += 0x01;
					if (strcmp(txt, "ecx") == 0) rscan->bytes[5] = 0x04;
					else if (strcmp(txt, "cx") == 0) rscan->bytes[5] = 0x02;
					else rscan->bytes[5] = 0x01;
				} else if (strcmp(txt, "dl") == 0 || strcmp(txt, "edx") == 0 || strcmp(txt, "dx") == 0) {
					byte += 0x02;
					if (strcmp(txt, "edx") == 0) rscan->bytes[5] = 0x04;
					else if (strcmp(txt, "dx") == 0) rscan->bytes[5] = 0x02;
					else rscan->bytes[5] = 0x01;
				} else if (strcmp(txt, "bl") == 0 || strcmp(txt, "ebx") == 0 || strcmp(txt, "bx") == 0) {
					byte += 0x03;
					if (strcmp(txt, "ebx") == 0) rscan->bytes[5] = 0x04;
					else if (strcmp(txt, "bx") == 0) rscan->bytes[5] = 0x02;
					else rscan->bytes[5] = 0x01;
				} else if (strcmp(txt, "ah") == 0 || strcmp(txt, "esp") == 0 || strcmp(txt, "sp") == 0) {
					byte += 0x04;
					if (strcmp(txt, "esp") == 0) rscan->bytes[5] = 0x04;
					else if (strcmp(txt, "sp") == 0) rscan->bytes[5] = 0x02;
					else rscan->bytes[5] = 0x01;
				} else if (strcmp(txt, "ch") == 0 || strcmp(txt, "ebp") == 0 || strcmp(txt, "bp") == 0) {
					byte += 0x05;
					if (strcmp(txt, "ebp") == 0) rscan->bytes[5] = 0x04;
					else if (strcmp(txt, "bp") == 0) rscan->bytes[5] = 0x02;
					else rscan->bytes[5] = 0x01;
				} else if (strcmp(txt, "dh") == 0 || strcmp(txt, "esi") == 0 || strcmp(txt, "si") == 0) {
					byte += 0x06;
					if (strcmp(txt, "esi") == 0) rscan->bytes[5] = 0x04;
					else if (strcmp(txt, "si") == 0) rscan->bytes[5] = 0x02;
					else rscan->bytes[5] = 0x01;
				} else if (strcmp(txt, "bh") == 0 || strcmp(txt, "edi") == 0 || strcmp(txt, "di") == 0) {
					byte += 0x07;
					if (strcmp(txt, "edi") == 0) rscan->bytes[5] = 0x04;
					else if (strcmp(txt, "di") == 0) rscan->bytes[5] = 0x02;
                	   		else rscan->bytes[5] = 0x01;
                		} else {
					va_end(args); // Not finded register
					return false;
                		}
				rscan->bytes[0] = byte; // Main byte

				text_idx += j;
				fmt_idx += 2;
			} // END OF %
		} else if (text[text_idx] != format[fmt_idx]) {
			va_end(args);
			return false;
		} else {
			text_idx++; // Not %
			fmt_idx++;
		}
	}

	va_end(args);
	return (format[fmt_idx] == '\0');
}

CMD_T arc[] = {
	// mov
	{.cmdf = "mov bl, al",.byte = {0x88,0xC3,0},   .arg_t = {ARG_NONE,ARG_NONE,0}},
	{.cmdf = "mov al, bl",.byte = {0x88,0xD8,0},   .arg_t = {ARG_NONE,ARG_NONE,0}},
	{.cmdf = "mov ebx, eax",.byte = {0x89,0xC3,0}, .arg_t = {ARG_NONE,ARG_NONE,0}},
	{.cmdf = "mov eax, ebx",.byte = {0x89,0xD8,0}, .arg_t = {ARG_NONE,ARG_NONE,0}},
	{.cmdf = "mov %r, %d",.byte = {0,0,0},         .arg_t = {ARG_REGB,ARG_UNINT,0}},

	// call
	{.cmdf = "call %r",.byte = {0xFF,0,0},         .arg_t = {ARG_REGD,ARG_NONE,0}},
	{.cmdf = "call %d",.byte = {0xE8,0,0},         .arg_t = {ARG_INT32,ARG_NONE,0}},

	{.cmdf = "nop",.byte = {0x90,0,0},             .arg_t = {ARG_NONE,ARG_NONE,0}},

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
		SCANR_T temp = {};
		SCANR_T temp0 = {};
		if (arc[i].arg_t[0] == ARG_REGB) {
			temp0.bytes[0] = 0xB0;
			temp.bytes[0] =  0xB0;
		}  else if (arc[i].arg_t[0] == ARG_REGD) {
			temp0.bytes[0] = 0xD0;
			temp.bytes[0] =  0xD0;
		}
		if (stringscanf(s->asmcode,arc[i].cmdf,&temp,&temp0)) {
			for (uint8_t j = 0; arc[i].byte[j] != 0 ;j++) {
				if (!s->mode) s->bytes[ptr++] = arc[i].byte[j]; else ptr++;
			}

			if (arc[i].arg_t[0] == ARG_INT8) {
				if (!s->mode) s->bytes[ptr++] = (uint8_t)(temp.num & 0xFF); else ptr++;
			} else if (arc[i].arg_t[0] == ARG_INT32) {
				if (!s->mode) {
					s->bytes[ptr++] = (uint8_t)(temp.num & 0xFF);
					s->bytes[ptr++] = (uint8_t)((temp.num >> 8) & 0xFF);
					s->bytes[ptr++] = (uint8_t)((temp.num >> 16) & 0xFF);
					s->bytes[ptr++] = (uint8_t)((temp.num >> 24) & 0xFF);
				} else ptr += 4;
			} else if (arc[i].arg_t[0] == ARG_TAG) {
				if (!s->mode) {
					for (;s->sym->lastsymbol >= 0;s->sym->lastsymbol--) {
						if (strcmp(s->sym->symbols[s->sym->lastsymbol].text,temp.txt) == 0) {
							s->bytes[ptr] = s->sym->symbols[s->sym->lastsymbol].address - *s->pc;
							break;
						}
					}
				}
				ptr++;
			} else if (arc[i].arg_t[0] == ARG_REGB || arc[i].arg_t[0] == ARG_REGD) {
				if (!s->mode) s->bytes[ptr++] = temp.bytes[0]; else ptr++;
				if (arc[i].arg_t[1] == ARG_UNINT && !s->mode) {
					for (uint8_t ofs = 0; ofs<temp.bytes[5];ofs++) {
						s->bytes[ptr++] = (uint8_t)((temp0.num >> ofs*8) & 0xFF);
					}
				} else if (arc[i].arg_t[1] == ARG_UNINT && s->mode) {
					ptr += temp.bytes[5];
				}
			}

			result.error = 0;
			break;
		}
        }
	result.gened = ptr;

	return result;
}

#endif
