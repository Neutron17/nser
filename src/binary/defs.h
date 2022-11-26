#ifndef _NTR_SER_DEFIN_H_
#define _NTR_SER_DEFIN_H_ 1
#include <stdbool.h>

#define SER_LEN_TYPE unsigned short

enum SerLogType {
	SFT_ERR,
	SFT_WARN,
	SFT_DEBUG
};

struct SerGeneric {
	// element of Serializer.actlut (SER_LEN_TYPE [])
	SER_LEN_TYPE act_causer;
};

struct SerData {
	struct SerGeneric parent;
	SER_LEN_TYPE data_len;
	// WARNING: Needs freeing
	unsigned char *data;
};

struct SerText {
	struct SerGeneric parent;
	SER_LEN_TYPE txt_len;
	// WARNING: Needs freeing
	char *txt;
};

struct SerArrayItem {
	SER_LEN_TYPE data_len;
	// WARNING: Needs freeing
	unsigned char *data;
};
struct SerArray {
	struct SerGeneric parent;
	SER_LEN_TYPE arr_len;
	// WARNING: Needs freeing
	struct SerArrayItem *items;
};
void serArrayFree(struct SerArray *restrict array);

enum SerCBParamType {
	SCB_NONE,
	SCB_DATA,
	SCB_TEXT,
	SCB_UINT,
	SCB_UNKNOWN
};

struct SerCBParam {
	enum SerCBParamType type;
	union arg {
		struct SerData data;
		struct SerText text;
		unsigned uinteger;
	} arg;
};

enum SerItemType {
	SIT_NONE = 0,
	// Only one
	SIT_FILE_BEGIN,
	// Only one
	SIT_FILE_END,
	SIT_DATA_BEGIN,
	// Callback: SerData
	SIT_DATA_END,

	SIT_TXT_BEGIN,
	// Callback: SerText
	SIT_TXT_END,

	/*********
	* ARRAYS *
	*********/
	// Syntax:
	//   SIT_ARRAY_BEGIN <size> SIT_ARRAY_ITEM <data> SIT_ARRAY_ITEM <data> ... SIT_ARRAY_END
	SIT_ARR_BEGIN,
	// Callback: SerArray
	SIT_ARR_END,
	SIT_ARR_ITEM,

	// SIT_UINT <BYTE1> ... <BYTE4>
	SIT_UINT,

	/*  MODES
	 *         1: 0->little endian 1-> big endian
	 *        10
	 *       100
	 *      1000
	 *
	 *    1 0000
	 *   10 0000
	 *  100 0000
	 * 1000 0000
	 */
	SIT_MODE,

	SIT_MAX
};

enum SerMode {
	SM_LENDIAN = 0,
	SM_BENDIAN = 1,
};

typedef void (*ser_callback_fn)(struct SerCBParam);
// status usually is the number0 with which the log is associated
typedef void (*ser_log_fn)(enum SerLogType type, int status, const char *restrict message);

typedef ser_callback_fn callback_array[];
typedef SER_LEN_TYPE ser_actlut[];

typedef struct {
	ser_callback_fn *callback_array;
	// required structure:
	// {
	//    { #act, SerItemType },
	//    { #act, SerItemType },
	//    ...
	// } // where #indicator is an element of ser_actlut
	SER_LEN_TYPE *actlut;
	SER_LEN_TYPE callback_sz;
	bool isValid;
	ser_log_fn log_cb;
	// only NULL after a failed function call
	void *ret;
} Serializer;

Serializer serCreate(ser_callback_fn *restrict callback_array, SER_LEN_TYPE callback_sz, SER_LEN_TYPE *restrict ser_actlut, const ser_log_fn log_cb);
void serParse(Serializer *restrict ser, const char *restrict fname);
void serParseAsync(Serializer *restrict ser, const char *restrict fname);

#endif /* ifndef _NTR_SER_DEFIN_H_ */

