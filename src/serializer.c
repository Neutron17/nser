#include "defs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define BIT(OFF) (1 << OFF)

// 0101010
const static unsigned short pairLUT =     BIT(SIT_FILE_BEGIN) | BIT(SIT_FILE_END)
					| BIT(SIT_DATA_BEGIN) | BIT(SIT_DATA_END)
					| BIT(SIT_TXT_BEGIN)  | BIT(SIT_TXT_END);
#define IS_X_PAIR(X) ((pairLUT & X) >> X)

static void def_logger(enum SerLogType type, int status, const char *restrict message) {}

Serializer serCreate(ser_callback_fn *restrict callback_array, SER_LEN_TYPE callback_sz, SER_LEN_TYPE *restrict ser_actlut, const ser_log_fn log_cb) {
	Serializer ret = {0};
	if(!log_cb)
		ret.log_cb = def_logger;
	if(!callback_array || callback_sz < 1 || !ser_actlut) {
		if(!callback_array)
			log_cb(SFT_ERR, 0, "callback_array is NULL");
		if(callback_sz < 1)
			log_cb(SFT_ERR, 1, "callback size too small");
		if(!ser_actlut)
			log_cb(SFT_ERR, 2, "ser_actlut is NULL");
		return ret;
	}
	ret = (Serializer){
		.callback_array = callback_array,
		.actlut = ser_actlut,
		.callback_sz = callback_sz,
		.isValid = true,
		.log_cb = log_cb,
		// WARNING: pointer to local variable
		// 	    done this way, because ret should be NULL after a failed call,
		// 	    hence isValid refers to the whole Serializer,
		// 	    but ret can be used to indicate failure
		.ret = &ret
	};
	return ret;
}

// identifier byte
inline static SER_LEN_TYPE pairGetFirst (const SER_LEN_TYPE *restrict set, unsigned index) { return set[2*index]; 	}
// SIT
inline static SER_LEN_TYPE pairGetSecond(const SER_LEN_TYPE *restrict set, unsigned index) { return set[2*index+1];	}

// Find the first element of pair with 2nd element(SIT) being searched SIT
// WARNING: Only use if occurance is garanteed
static SER_LEN_TYPE pairFirstSIT(const SER_LEN_TYPE *restrict pair, enum SerItemType SIT) {
	SER_LEN_TYPE symbol = SIT_NONE;
	for(int i=0;; i++)
		if((symbol = pairGetSecond(pair, i)) == SIT)
			return pairGetFirst(pair, i);
	return SIT_NONE;
}

// Find the first element of pair with 2nd element(SIT) being searched SIT, but safely
static SER_LEN_TYPE pairFirstSIT_safe(const SER_LEN_TYPE *restrict pair, enum SerItemType SIT, SER_LEN_TYPE pair_len) {
	SER_LEN_TYPE symbol = SIT_NONE;
	for(int i=0; i < pair_len; i++)
		if((symbol = pairGetSecond(pair, i)) == SIT)
			return pairGetFirst(pair, i);
	return SIT_NONE;
}

static inline unsigned long nextSIT_FileOffset(
		FILE *restrict file,
		const SER_LEN_TYPE *restrict actlut,
		SER_LEN_TYPE begin_index /* index(in actlut) of begin SIT */) {
	int c;
	while(((c = getc(file)) != EOF) && c != pairGetFirst(actlut, begin_index+1)) {}
	return ftell(file);
}

void serParse(Serializer *restrict ser, const char *restrict fname) {
	if(!ser->isValid)
		return;
	// ret should be a non NULL value after all successful calls
	ser->ret = ser;
// Open file
	FILE *file = fopen(fname, "rb");
	if(!file) {
		ser->log_cb(SFT_ERR, 1, "Couldn't open file"); // Probably check errno
		ser->ret = NULL;
		return;
	}
// find some elements
	SER_LEN_TYPE begin_symbol = pairFirstSIT_safe(ser->actlut, SIT_FILE_BEGIN,	ser->callback_sz);
	SER_LEN_TYPE end_symbol	  = pairFirstSIT_safe(ser->actlut, SIT_FILE_END,	ser->callback_sz);
	SER_LEN_TYPE none_symbol  = pairFirstSIT_safe(ser->actlut, SIT_NONE,		ser->callback_sz);

	register int c;
	while(((c = getc(file)) != EOF) && (c != begin_symbol));
	if(c == EOF) {
		ser->log_cb(SFT_ERR, -1, "Reached end of file before file begin symbol");
		goto err;
	}
	while (((c = getc(file)) != EOF) && (c != end_symbol)) {
		if(c == none_symbol)
			continue;
		for(int i = 0; i < ser->callback_sz; i++) {
			if(pairGetFirst(ser->actlut, i) == c) {
				if(IS_X_PAIR(pairGetSecond(ser->actlut, i))) {
					ser->log_cb(SFT_ERR, 0, "actlut structure invalid: SIT_???_BEGIN must be followed by SIT_???_END");
					goto err;
				}
				switch (pairGetSecond(ser->actlut, i)) {
					case SIT_DATA_BEGIN: {
						const unsigned long begin_offs = ftell(file);
						const unsigned long end_offs = nextSIT_FileOffset(file, ser->actlut, i);

						if(c == EOF) {
							ser->log_cb(SFT_ERR, end_offs, "Unexpected end of file");
							goto err;
						}
						unsigned char *buff = malloc(end_offs - begin_offs);
						if(!buff) {
							ser->log_cb(SFT_ERR, -2, "Couldn't allocate for SerData buffer");
							goto err;
						}
						fseek(file, begin_offs, SEEK_SET);
						{
							unsigned j = 0;
							while((c = getc(file)) != pairGetFirst(ser->actlut, i+1)) { // Can't be EOF
								// ISO C forbids braced-groups within expressions :(
								buff[j] = c;
								j++;
							}
						}
						struct SerData data = {
							.act_causer = i,
							.data_len   = end_offs - begin_offs,
							.data 	    = buff
						};
						ser->callback_array[i+1]((struct SerCBParam) {
								.type	= SCB_DATA,
								.arg	= { data }
						});
					} break;

					case SIT_TXT_BEGIN: {
						const unsigned long begin_offs = ftell(file);
						const unsigned long end_offs = nextSIT_FileOffset(file, ser->actlut, i);

						if(c == EOF) {
							ser->log_cb(SFT_ERR, end_offs, "Unexpected end of file");
							goto err;
						}
						char *buff = malloc((end_offs - begin_offs)+1);
						if(!buff) {
							ser->log_cb(SFT_ERR, -2, "Couldn't allocate for SerData buffer");
							goto err;
						}
						fseek(file, begin_offs, SEEK_SET);
						{
							unsigned j = 0;
							while((c = getc(file)) != pairGetFirst(ser->actlut, i+1)) { // Can't be EOF
								// ISO C forbids braced-groups within expressions :(
								buff[j] = c;
								j++;
							}
							buff[j] = '\0';
						}
						struct SerText text = {
							.act_causer = i,
							.data_len   = end_offs - begin_offs,
							.data 	    = buff
						};
						ser->callback_array[i+1]((struct SerCBParam) {
								.type	= SCB_TEXT,
								.arg	= { .text = text }
						});
					} break;

					default:
						//ser->callback_array[i]((struct SerCBParam){0});
						break;
				}
			}
		}
	}
	if(c == EOF)
		ser->log_cb(SFT_WARN, -2, "Reached end of file before file end symbol");
	goto cleanUp;
err:
	ser->ret = NULL;
cleanUp:
	fclose(file);
}

void serParseAsync(Serializer *restrict ser, const char *restrict fname);

