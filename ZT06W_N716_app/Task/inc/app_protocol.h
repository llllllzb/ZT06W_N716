#ifndef APPSERVERPROTOCOL
#define APPSERVERPROTOCOL

#include <stdint.h>
#include "app_gps.h"

#define WIFIINFOMAX		10
typedef enum{
	PROTOCOL_01,//登录
	PROTOCOL_12,//定位
	PROTOCOL_13,//心跳
	PROTOCOL_16,//定位
	PROTOCOL_19,//多基站
	PROTOCOL_21,//蓝牙
	PROTOCOL_61,
	PROTOCOL_62,
	PROTOCOL_F1,//ICCID
	PROTOCOL_F3,
	PROTOCOL_8A,
	PROTOCOL_A0,
	PROTOCOL_0B,
	PROTOCOL_UP,
}PROTOCOLTYPE;

typedef struct
{
    char curCODEVERSION[50];
    char newCODEVERSION[50];
    char rxsn[50];
    char rxcurCODEVERSION[50];
    uint8_t updateOK;
	uint8_t updateFirst;
    uint32_t file_id;
    uint32_t file_offset;
    uint32_t file_len;
    uint32_t file_totalsize;
    uint32_t rxfileOffset;//已接收文件长度
} UndateInfoStruct;

typedef struct _ota_package
{
    unsigned int offset;
    unsigned int len;			//单包文件长度
    unsigned char *data;
} ota_package_t;


//总过17个字节，存储20个字节
typedef struct{
	uint8_t year;
	uint8_t month;
	uint8_t day;
	uint8_t hour;
	uint8_t minute;
	uint8_t second;
	uint8_t latititude[4];
	uint8_t longtitude[4];
	uint8_t speed;
	uint8_t coordinate[2];
	uint8_t temp[3];
}gpsRestore_s;

typedef struct
{
    uint8_t ssid[6];
    int8_t signal;
} WIFIINFOONE;
typedef struct
{
    WIFIINFOONE ap[WIFIINFOMAX];
    uint8_t apcount;
} WIFIINFO;

typedef struct{
	char IMEI[17];
	uint8_t batLevel;
	float Voltage;
	uint16_t startUpcnt;
	uint16_t runCnt;
	uint16_t serialNum;
}protocolInfo_s;


typedef struct
{
    char *dest;
    char *dateTime;
    uint8_t fileType;
    uint8_t *recData;
    uint16_t packNum;
    uint16_t recLen;
    uint32_t totalSize;
    uint16_t packSize;
} recordUploadInfo_s;

void protocolSend(uint8_t link,PROTOCOLTYPE protocol,void * param);

uint8_t getBatteryLevel(void);
void gpsRestoreDataSend(gpsRestore_s *grs, char *dest	, uint16_t *len);

void save123InstructionId(void);
void reCover123InstructionId(void);

void getInsid(void);
void setInsId(void);

void getLastInsid(void);
void setLastInsid(void);
void createProtocolA0(char *DestBuf, uint16_t *len);

uint32_t getFileTotalSize(void);
uint32_t getRxfileOffset(void);

void updateUISInit(uint8_t object);
void updateUISVersion(uint8_t *version);
uint8_t *getNewCodeVersion(void);

void gpsRestoreUpload(void);

void updateHistoryGpsTime(gpsinfo_s *gpsinfo);
void protocolSnRegister(char *sn);
void protocolInfoResiter(uint8_t batLevel, float vol, uint16_t sCnt, uint16_t runCnt);

void protocolRxParser(uint8_t link, char *protocol, uint16_t size);


#endif
