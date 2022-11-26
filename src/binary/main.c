#include <stdio.h>
#include <stdlib.h>
#include "defs.h"

unsigned char lut[7][2] = {
	{ 0xBE, SIT_FILE_BEGIN },
	{ 0xED, SIT_FILE_END },
	{ 0x00, SIT_NONE },
	{ 0xF0, SIT_DATA_BEGIN },
	{ 0xE0, SIT_DATA_END },
	{ 0xF1, SIT_TXT_BEGIN },
	{ 0xE1, SIT_TXT_END },
};

void dataer_cb(struct SerCBParam param) {
	printf("---DATA at: %d---\n", param.arg.data.act_causer);
	for(int i = 0; i < param.arg.data.data_len; i++) {
		printf("%d(%c)\t", param.arg.data.data[i], param.arg.data.data[i]);
	}
	puts("");
	free(param.arg.data.data);
}

void texter_cb(struct SerCBParam param) {
	printf("---TEXT at: %d---\n", param.arg.data.act_causer);
	printf("%s\n", param.arg.text.data);
	free(param.arg.text.data);
}

ser_callback_fn cb_arr[SIT_MAX-1];

void logger(enum SerLogType type, int status, const char *restrict message) {
	printf("%d:%d - %s\n", type, status, message);
}

int main(int argc, char *argv[]) {
	cb_arr[SIT_DATA_END] = dataer_cb;
	cb_arr[SIT_TXT_END]  = texter_cb;

	Serializer ser = serCreate(cb_arr, 7, &**lut, logger);
	serParse(&ser, "ser.dat");
	return 0;
}

