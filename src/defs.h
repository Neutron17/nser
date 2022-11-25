#ifndef _NTR_SER_DEFIN_H_
#define _NTR_SER_DEFIN_H_ 1
#include <stdbool.h>

#define SER_LEN_TYPE unsigned char

enum SerLogType {
	SFT_ERR,
	SFT_WARN,
	SFT_DEBUG
};

struct SerData {
	// element of Serializer.actlut (SER_LEN_TYPE [])
	SER_LEN_TYPE act_causer;
	unsigned data_len;
	// WARNING: Needs freeing
	unsigned char *data;
};

enum SerCBParamType {
	SCB_DATA,
	SCB_UNKNOWN
};

struct SerCBParam {
	enum SerCBParamType type;
	union {
		struct SerData data;
	};
};

enum SerItemType {
	SIT_NONE = 0,
	// Only one
	SIT_FILE_BEGIN,
	// Only one
	SIT_FILE_END,
	SIT_DATA_BEGIN,
	// Callback data structure
	SIT_DATA_END,
	SIT_MAX
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
	void *ret;
} Serializer;

Serializer serCreate(ser_callback_fn *restrict callback_array, SER_LEN_TYPE callback_sz, SER_LEN_TYPE *restrict ser_actlut, const ser_log_fn log_cb);
void serParse(Serializer *restrict ser, const char *restrict fname);
void serParseAsync(Serializer *restrict ser, const char *restrict fname);

#endif /* ifndef _NTR_SER_DEFIN_H_ */
