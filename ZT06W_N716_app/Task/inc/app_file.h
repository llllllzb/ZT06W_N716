/*
 * app_file.h
 *
 *  Created on: Jan 11, 2024
 *      Author: nimo
 */

#ifndef TASK_INC_APP_FILE_H_
#define TASK_INC_APP_FILE_H_

#include "config.h"
#include "app_net.h"
#include "app_sys.h"

#define OTA_FILE_NAME				"BR06"



typedef struct
{
	uint8_t   fileName[20];
	uint8_t  *data;
	uint16_t  readLen;
	uint16_t  len;
	uint16_t  rxoffset;
	uint16_t  veroffset;
	uint32_t  totalsize;
}fileRxInfo;

void fileQueryList(void);
void fileQueryFileSize(char *file);
void fileDelete(char *file);
void fileWriteData(uint8_t *file, uint8_t *data, uint16_t len, uint8_t type);
void fileReadData(uint8_t *file, uint16_t len, uint16_t readind);




#endif /* TASK_INC_APP_FILE_H_ */
