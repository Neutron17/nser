#include <stdio.h>
#include "defs.h"

unsigned char lut[5][2] = {
	{ 0xBE, SIT_FILE_BEGIN },
	{ 0xED, SIT_FILE_END },
	{ 0x00, SIT_NONE },
	{ 0xFF, SIT_DATA_BEGIN },
	{ 0xEE, SIT_DATA_END },
};

void dataer_cb(struct SerCBParam param) {
	printf("---DATA at: %d---\n", param.data.act_causer);
	for(int i = 0; i < param.data.data_len; i++) {
		printf("%d(%c)\t", param.data.data[i], param.data.data[i]);
	}
	puts("");
}

ser_callback_fn cb_arr[5] = {
	NULL, NULL, NULL, NULL, dataer_cb
};

void logger(enum SerLogType type, int status, const char *restrict message) {
	printf("%d:%d - %s\n", type, status, message);
}

int main(int argc, char *argv[]) {
	printf("Hello World\n");
	Serializer ser = serCreate(cb_arr, 5, &**lut, logger);
	serParse(&ser, "ser.dat");
	return 0;
}

