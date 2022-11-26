#include "defs.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <endian.h>
#include <arpa/inet.h>

#define BIT(OFF) (1 << OFF)

// Does SIT have a pair(open/close)
const static unsigned short pairLUT =     BIT(SIT_FILE_BEGIN) | BIT(SIT_FILE_END)
					| BIT(SIT_DATA_BEGIN) | BIT(SIT_DATA_END)
					| BIT(SIT_TXT_BEGIN)  | BIT(SIT_TXT_END)
					| BIT(SIT_ARR_BEGIN)  | BIT(SIT_ARR_END);
#define IS_X_PAIR(X) ((pairLUT & X) >> X)

static void def_logger(enum SerLogType type, int status, const char *restrict message) {}

	/***************************/
	/*   ActLUT manipulation   */
	/***************************/

// identifier byte
inline static SER_LEN_TYPE pairGetFirst (const SER_LEN_TYPE *restrict set, unsigned index) {
	/*static SER_LEN_TYPE cache_value = 0;
	static SER_LEN_TYPE *cache_key1 = 0;
	static SER_LEN_TYPE cache_key2  = 0;
	if(cache_key1 == set && cache_key2 == index)
		return cache_value;
	cache_value = set[2*index];
	cache_key1 = set;
	cache_key2 = index;*/

	return set[2*index];
}
// SIT
inline static SER_LEN_TYPE pairGetSecond(const SER_LEN_TYPE *restrict set, unsigned index) {
	return set[2*index+1];
}

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

/* actlut is vaild if and only if:
 *  - all the first elements(identifiers) are unique
 *  - there are no invalid(over SIT_MAX) second elements(SITs)
 * WARNING: Doesn't check for NULL, bc: serCreate checks
 */
static bool isActLUTValid(const SER_LEN_TYPE *restrict ser_actlut, SER_LEN_TYPE actlut_len, const ser_log_fn log_cb) {
	SER_LEN_TYPE *arr = malloc(sizeof(SER_LEN_TYPE) * actlut_len);
	if(!arr) {
		log_cb(SFT_ERR, actlut_len, "Couldn't allocate for actlut validity check");
		// Assuming invalidity
		return false;
	}
	int fb_c = 0, fe_c = 0, n_c = 0;
	for(int i = 0; i < actlut_len; i++) {
		if(pairGetSecond(ser_actlut, i) >= SIT_MAX) {
			printf("DEBUG: i: %d\n", i);
			log_cb(SFT_ERR, 1, "actlut validation failed: Unknown SIT");
			return false;
		}
		// Checking for multiple accourances of first element
		for(int j = 0; (j != 0) && (j < i); j++) {
			if(arr[j] == pairGetFirst(ser_actlut, i)) {
				log_cb(SFT_ERR, 1, "actlut validation failed: Multiple definitions for the same identifier");
				return false;
			}
		}
		arr[i] = pairGetFirst(ser_actlut, i);
		switch (pairGetSecond(ser_actlut, i)) {
			case SIT_FILE_BEGIN:
				fb_c++;
				break;
			case SIT_FILE_END:
				fe_c++;
				break;
			case SIT_NONE:
				n_c++;
				break;
			default:;
		}
		if(fb_c > 1 || fe_c > 1 || n_c > 1)
			log_cb(SFT_WARN, 2, "WARNING: multiple definitions of file_begin/end or none");
	}
	free(arr);
	return true;
}



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
	if(!isActLUTValid(ser_actlut, callback_sz, log_cb)) {
		log_cb(SFT_ERR, 2, "Invalid ser_actlut");
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

	bool isBigEndian = true;

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
							.parent	  = { i },
							.data_len = end_offs - begin_offs,
							.data 	  = buff
						};
						ser->callback_array[SIT_DATA_END]((struct SerCBParam) {
								.type	= SCB_DATA,
								.arg	= { data }
						});
					} break;

					case SIT_TXT_BEGIN: {
						const unsigned long begin_offs = ftell(file);
						const unsigned long end_offs = nextSIT_FileOffset(file, ser->actlut, i);

						if(c == EOF) {
							ser->log_cb(SFT_ERR, end_offs, "Unexpected end of file during text");
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
							.parent	 =  { i },
							.txt_len = end_offs - begin_offs,
							.txt 	 = buff
						};
						ser->callback_array[SIT_TXT_END]((struct SerCBParam) {
								.type	= SCB_TEXT,
								.arg	= { .text = text }
						});
					} break;

					case SIT_ARR_BEGIN: {
						register int c = getc(file);
						if(c == EOF) {
							ser->log_cb(SFT_ERR, ftell(file), "Unexpected end of file during array");
							goto err;
						}
						const unsigned char arr_len = c;
						struct SerArray *array = malloc(sizeof(struct SerArray) * arr_len);
						if(!array) {
							ser->log_cb(SFT_ERR, ftell(file), "Couldn't allocate for array");
							goto err;
						}
						if((c = getc(file)) == EOF || pairGetSecond(ser->actlut, i+1)) {
						}
					} break;

					case SIT_UINT: {
						unsigned char b1 = getc(file), b2 = getc(file), b3 = getc(file), b4 = getc(file);
						unsigned integer;
						if(isBigEndian)
							integer  = (((unsigned)b1) << 0) | (((unsigned)b2) << 2)
								 | (((unsigned)b3) << 4) | (((unsigned)b4) << 6);
						else
							integer  = (((unsigned)b4) << 0) | (((unsigned)b3) << 2)
								 | (((unsigned)b2) << 4) | (((unsigned)b1) << 6);


						//integer = ntohl(integer);
						struct SerCBParam param = {
							.type = SCB_UINT,
							.arg = { .uinteger = integer }
						};
						ser->callback_array[SIT_UINT](param);
					} break;

					case SIT_MODE: {
						int c;
						if((c = getc(file)) == EOF) {
							ser->log_cb(SFT_ERR, ftell(file), "Unexpected end of file during mode");
							goto err;
						}
						unsigned char arg = c;
						if((arg & SM_LENDIAN) == SM_LENDIAN)
							isBigEndian = false;
						else
							isBigEndian = true;
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

void serArrayFree(struct SerArray *restrict array) {
	for(int i = 0; i < array->arr_len; i++)
		free(array->items[i].data);
	free(array);
}

