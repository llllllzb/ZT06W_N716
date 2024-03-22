#ifndef APP_EEPROM_H
#define APP_EEPROM_H
#include <stdint.h>

#include "app_sys.h"
#include "app_file.h"

/*
 * DataFlash 分布
 * [0~1)        Total:1KB   bootloader
 * [1~9)        Total:8KB   systemparam
 * [9~10)		Total:1KB	dynamicparam
 * [10~30)      Total:20KB  database
 * [30~31)		Total:1KB	databaseInfo
 * [31~32)      Total:1KB   blestack
 */

//目前预留前1K字节给bootloader，所以app从0x400开始
#define BOOT_USER_PARAM_ADDR    0x00
#define APP_USER_PARAM_ADDR     0x400  //实际是0x00070000+APP_USER_PARAM_ADDR
#define APP_DYNAMIC_PARAM_ADDR	0x2400 //实际是0x00070000+APP_DYNAMIC_PARAM_ADDR
#define APP_PARAM_FLAG          0x1A
#define BOOT_PARAM_FLAG         0xB0
#define	OTA_PARAM_FLAG			0x20

#define EEPROM_VERSION									"ZT06W_N716_ZKW_V2.1.3"


#define JT808_PROTOCOL_TYPE			8
#define ZT_PROTOCOL_TYPE			0



typedef struct
{
    uint8_t VERSION;        /*当前软件版本号*/
    uint8_t updateStatus;
    uint8_t apn[50];
    uint8_t apnuser[50];
    uint8_t apnpassword[50];
    uint8_t SN[20];
    uint8_t updateServer[50];
    uint8_t codeVersion[50];
    uint16_t updatePort;
} bootParam_s;

/*存储在EEPROM中的数据*/
typedef struct
{
    uint8_t VERSION;        /*当前软件版本号*/
    uint8_t MODE;           /*系统工作模式*/
    uint8_t ldrEn;
    uint8_t gapDay;
    uint8_t heartbeatgap;
    uint8_t ledctrl;
    uint8_t accdetmode;
    uint8_t poitype;
    uint8_t accctlgnss;
    uint8_t apn[50];
    uint8_t apnuser[50];
    uint8_t apnpassword[50];
    uint8_t apnAuthPort;
    uint8_t lowvoltage;
    uint8_t fence;
    
    uint8_t Server[50];
    uint8_t agpsServer[50];
    uint8_t agpsUser[50];
    uint8_t agpsPswd[50];
    uint8_t protocol;

    uint8_t hiddenServOnoff;
    uint8_t hiddenServer[50];
    uint8_t jt808Server[50];
    uint8_t bleServer[50];
    uint8_t upgradeServer[50];

    uint8_t bleAutoDisc;
    uint8_t bleRfThreshold;
    uint8_t blePreShieldVoltage;
    uint8_t blePreShieldDetCnt;
    uint8_t blePreShieldHoldTime;
    uint8_t relayCtl;


    uint8_t bleConnMac[2][6];
    uint8_t relaySpeed;
    uint8_t bf;
    int8_t utc;

    uint16_t gpsuploadgap;
    uint16_t AlarmTime[5];  /*每日时间，0XFFFF，表示未定义，单位分钟*/
    uint16_t gapMinutes;    /*模式三间隔周期，单位分钟*/
    
    
    uint16_t agpsPort;
    uint16_t jt808Port;
    uint16_t hiddenPort;
    uint16_t ServerPort;
    uint16_t bleServerPort;
    uint16_t upgradePort;
    uint16_t bleOutThreshold;

    float  adccal;
    float  accOnVoltage;
    float accOffVoltage;
    float protectVoltage;

	

	uint8_t cm;
	uint8_t sosNum[3][20];
	uint8_t centerNum[20];
	uint8_t sosalm;

    uint8_t gsdettime;
    uint8_t gsValidCnt;
    uint8_t gsInvalidCnt;

    uint8_t bleen;
    uint8_t agpsen;
    uint8_t cm2;

    uint8_t bleRelay;
    float bleVoltage;
    uint8_t relayFun;
    
    float batLowLevel;
    float batHighLevel;
	uint8_t otaParamFlag;
	
	double mileage;
    uint8_t milecal;

    uint8_t simpulloutalm;
	uint8_t simSel;
	uint8_t simpulloutLock;
	uint32_t alarmRequest;
	uint8_t uncapalm;
	uint8_t uncapLock;
	uint8_t bleMacCnt;
	uint8_t txPower;
	uint8_t updateStatus;
	uint8_t relayUpgrade[2];	//表示设备需要升级，一旦设置，在设备升级完成或者手动取消之前都不会置0
	fileInfo_s file;
	uint8_t blefastShieldTime;
	uint8_t shutdownalm;
	uint8_t shutdownLock;
	uint8_t relayCloseCmd;
	uint8_t shieldDetTime;	  // 屏蔽锁车时间构成：
	uint8_t netConnDetTime;	  // 屏蔽锁车时间 = 屏蔽干扰检测时间 + 网络信号检测时间

	uint8_t rfDetType;

} systemParam_s;

/*存在EEPROM里的动态参数*/
typedef struct
{
	uint16_t runTime;  /*模式二的工作时间，单位分钟*/
	uint16_t startUpCnt;
	uint8_t SN[20];
	
	uint8_t jt808isRegister;
	uint8_t jt808sn[7];
	uint8_t jt808AuthCode[50];
    uint8_t jt808AuthLen;

	uint16_t noNmeaRstCnt;
	int16_t saveLat;
	int16_t saveLon;
	int32_t rtcOffset;
	uint8_t sim;
	uint8_t lowPowerFlag;		//当读取外电小于5时，且电池小于3.5V时，该标志位置1，直到有外电接入为止再置0
}dynamicParam_s;


typedef enum{
	ALARM_TYPE_NONE,
	ALARM_TYPE_GPRS,
	ALARM_TYPE_GPRS_SMS,
	ALARM_TYPE_GPRS_SMS_TEL,
}AlarmType_e;

extern systemParam_s sysparam;
extern bootParam_s bootparam;
extern dynamicParam_s dynamicParam;

void bootParamSaveAll(void);
void bootParamGetAll(void);

void paramSaveAll(void);
void paramGetAll(void);

void dynamicParamSaveAll(void);
void dynamicParamGetAll(void);

void paramInit(void);
void paramDefaultInit(uint8_t type);


#endif

