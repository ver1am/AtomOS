#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>

uint8_t msize(uint8_t* m) {
	uint16_t i = 0;
	for (;m[i] != 0x00;i++);
	return i;
}

void skip_chars(char* txt, uint8_t* i) {
	while (
		txt[*i] == ' '  ||
		txt[*i] == ','  ||
		txt[*i] == '\n' ||
		txt[*i] == '\r'
	) (*i)++;
}

void arg_len(char* txt, uint8_t* i, uint8_t byte) {
	while (
		txt[*i+1] != ' '  &&
		txt[*i+1] != ','  &&
		txt[*i+1] != '\n' &&
		txt[*i+1] != '\r' &&
		txt[*i+1] != ':'  &&
		txt[*i+1] != byte
	) (*i)++;
}

typedef struct {
	char text[20];
	uint32_t adress;
} SYMBOL_T;

typedef struct {
	char* cmdf;
	uint8_t byte[3];
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

/*
	%d - to number
*/
bool stringscanf(char* text, char* format,...) {
	va_list args;
	va_start(args,format);

	for (uint8_t i = 0;text[i] != '\0';i++) {
		if (format[i] == '%') {
			if (format[i+1] == 'd') {
				int* num = va_arg(args, int*);
				uint8_t last = i;
				arg_len(&text[i],&i,format[i+2]);
				*num = to_num(&text[last],i-last);
				i++;
			}
		} else if (text[i] != format[i]) {
			// Probably missed i-s
			if (text[i] == '\n' || text[i] == '\r') continue;
			va_end(args);
			return false;
		}
	}

	va_end(args);
	return true;
}

/*
	%r - Registers
	%d - Number ex. F6 = -10
*/
CMD_T x86[] = {
	{.cmdf = "inc eax", .byte = {0x40,0,0}},
	{.cmdf = "cmp eax, %d", .byte = {0x3D,0,0}},
	{.cmdf = "jl %d", .byte = {0x7C,0,0}}
};

void asm_tobyte(char* asmcode, uint8_t* bytes) {
	int temp0 = 0;
	for (uint8_t i = 0; i < sizeof(x86) / sizeof(x86[0]); i++) {
		if (stringscanf(asmcode,x86[i].cmdf,&temp0)) {
			uint8_t last = 0;
			for (uint8_t j = 0;x86[i].byte[j] != 0;j++) {
				byte[j] = x86[i].byte[j];
				last = j;
			}
			byte[++j] = temp0;
		}
	}
}

void asm_symbol(char* buffer,SYMBOL_T symbols[]) {
	uint8_t symbol = 0;
	uint8_t i = 0;
	for (;buffer[i] != '\0';) {
		skip_chars(buffer,&i);
		char* ptr = &buffer[i];
		arg_len(buffer,&i,0x00);
		if (buffer[i+1] == ':') {
			buffer[i+1] = '\0';
			strcpy(symbols[symbol++].text,ptr);
		}
		if (buffer[i+1] == '\0') break;
		buffer[++i] = '\0';
	}
}

void asm_compile(FILE* file,FILE* bin) {
	char buffer[128] = {0};

	SYMBOL_T symbols[50] = {0}; // For "label:"

	uint32_t pc = 0;

	// Symbol parser
	while (fgets(buffer,sizeof(buffer),file) != NULL) {
		asm_symbol(&buffer[0],symbols);
	}

	fseek(file,0,SEEK_SET);

	while (fgets(buffer,sizeof(buffer),file) != NULL) {
		uint8_t byte[20] = {0};
		asm_tobyte(buffer,byte);
		printf("%s\n\r",buffer);
		printf("BYTES: %x\n\r",byte[0]);
		printf("SIZEOFBYTES: %u\r\n",msize(byte));
		fwrite(byte,msize(byte),1,bin);
	}
	printf("%s",symbols[0].text);
}


int main(int argc,char* argv[]) {
	if (argc < 3) return 1;

	FILE* file = fopen(argv[1],"r");
	FILE* bin =  fopen(argv[2],"w");

	if (file == NULL || bin == NULL) {
		perror("Failed to open file");
		return 1;
	}

	asm_compile(file,bin);

	fclose(file);
	fclose(bin);
	return 0;
}
