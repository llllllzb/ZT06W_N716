/*
 * app_file.c
 *
 *  Created on: Jan 11, 2024
 *      Author: nimo
 */
#include "app_file.h"


/**************************************************
@bref		�ļ���ѯ
@param
@return
@note
**************************************************/

void fileQueryList(void)
{
	sendModuleCmd(FSLIST_CMD, "?");
}

/**************************************************
@bref		ָ���ļ���ѯ��С
@param
@return
@note
**************************************************/

void fileQueryFileSize(char *file)
{
	uint8_t param[20] = { 0 };
	sprintf(param, "%s", file);
	sendModuleCmd(FSFS_CMD, param);
}

/**************************************************
@bref		ָ���ļ�ɾ��
@param
@return
@note
**************************************************/

void fileDelete(char *file)
{
	uint8_t param[20] = { 0 };
	sprintf(param, "%s", file);
	sendModuleCmd(FSDF_CMD, param);
}

/**************************************************
@bref		д�ļ�
@param
@return
@note
**************************************************/

void fileWriteData(uint8_t *file, uint8_t *data, uint16_t len, uint8_t type)
{
	uint8_t param[50] = { 0 };
	sprintf(param, "\"%s\",%d,%d,1500", file, type, len);
	sendModuleCmd(FSWF_CMD, param);
	createNode(data, len, FSWF_CMD);
	LogPrintf(DEBUG_UP, "fileWriteData==>File:%s len:%d", file, len);
}

/**************************************************
@bref		���ļ�
@param
@return
@note
**************************************************/

void fileReadData(uint8_t *file, uint16_t len, uint16_t readind)
{
	uint8_t param[50] = { 0 };
	sprintf(param, "\"%s\",1,%d,%d", file, len, readind);
	sendModuleCmd(FSRF_CMD, param);
	LogPrintf(DEBUG_UP, "fileReadData==>File:%s, offset:%d, readlen:%d", file, readind, len);
}

/**************************************************
@bref		����ota�ļ�����
@param
@return
@note
**************************************************/

void fileOtaFileDownloadTask(void)
{
	
	
}



