#include "pxml.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>

#define RED "\033[38;5;1m"
#define GRN "\033[38;5;2m"
#define YLW "\033[38;5;3m"
#define PRP "\033[38;5;5m"
#define BLU "\033[38;5;6m"
#define RST "\033[0m"

#define ERR(MSG) (fprintf(stderr, RED"%s/%s:%d: ERROR: %s, %s(%d)"RST"\n", __FILE__, __func__, __LINE__, 		\
			MSG, strerror(errno), errno))
#define ERRF(MSG, ...) (fprintf(stderr, RED"%s/%s:%d: ERROR: " MSG ", %s(%d)"RST"\n", __FILE__, __func__, __LINE__, 	\
			__VA_ARGS__, strerror(errno), errno))

static void parseElement(struct XmlElement *ret, const char *restrict text, int depth, char *symbol_sp) {
	char *symbol = NULL;
	if(depth == 0)
		symbol = strtok_r((char *restrict)text, " \n\t", &symbol_sp);
	else
		symbol = strtok_r(NULL, " \n\t", &symbol_sp);
	for(;symbol != NULL;
	    symbol = strtok_r(NULL, " \n\t", &symbol_sp)) {
		printf(BLU"symbol: %s\n"RST, symbol);

		if(*symbol != '<') {
			ERRF("Error while parsing, '%s' doesn't begin with '<'", symbol);
			ret->isValid = false;
			return;
		} else { // == <
			if(*(symbol+1) == '/') {
				if(strncmp(ret->name, symbol+2, strlen(ret->name)-1) == 0) {
					symbol_sp += strlen(symbol);

					return;
				}
				ERRF("Unexpected closing %s %s %d", ret->name, symbol, depth);
				ret->isValid = false;
				return;
			}
			char name[32];
			const size_t slen = strnlen(symbol, 32);
			printf(YLW"depth: %d, symbol: '%s', slen: %ld, symbol_sp: '%s'\n"RST, depth, symbol, slen, symbol_sp);
			strncpy(name, symbol+1, slen-2);
			name[slen-2] = '\0';

			if(depth == 0)
				strncpy(ret->name, name, 32);

			if(*symbol_sp == '\0')
				return;

			while(strncmp(symbol_sp, "</", 2)) {
				if(*symbol_sp == '<' && *(symbol_sp+1) != '/') {
					parseElement(ret, text, depth+1, symbol_sp);
					puts("HERER");
				}
				symbol_sp++;
			}

			printf(GRN"name: '%s', symbol: '%s'\n"RST, name, symbol);

			if(strncmp(name, symbol_sp+2, strlen(name)) != 0) {
				ERRF("Begin(%s) is not closed", name);
				printf("\nsymbol: \n\t'%s'\nsymbol_sp: \n\t'%s'\n\n", symbol, symbol_sp);
			}
			symbol_sp += strnlen(symbol, 31)+1;
			printf(PRP"%s\n"RST, symbol_sp);
		}
	}
}

struct XmlElement XMLParse(const char *restrict fname) {
	// Open file
	FILE *file;
	file = fopen(fname, "r");
	if(!file) {
		ERRF("Couldn't open file: %s", fname);
		return (struct XmlElement){0};
	}
	fseek(file, 0, SEEK_END);
	const long flen = ftell(file);
	fseek(file, 0, SEEK_SET);

	char *text = malloc(flen+1);
	if(!text) {
		ERR("Couldn't allocate for char *text");
		fclose(file);
		return (struct XmlElement){0};
	}

	fread(text, flen, 1, file);
	//printf("%s\n", text);

	struct XmlElement ret = {0};

	char *symbol_sp = NULL;
	parseElement(&ret, text, 0, symbol_sp);

	free(text);
	fclose(file);
	return ret;
}

