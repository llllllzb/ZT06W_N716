#ifndef APP_NET
#define APP_NET
#include <app_protocol.h>
#include <stdint.h>

#include "app_port.h"

#define POWER_ON			PORT_SUPPLY_ON
#define POWER_OFF			PORT_SUPPLY_OFF

#define PWRKEY_HIGH         PORT_PWRKEY_H
#define PWRKEY_LOW          PORT_PWRKEY_L

#define RSTKEY_HIGH         PORT_RSTKEY_H
#define RSTKEY_LOW          PORT_RSTKEY_L
//DTR
#define WAKEMODULE			DTR_LOW
#define SLEEPMODULE			DTR_HIGH


#define FILE_MAX_CNT		5


#define MODULE_RX_BUFF_SIZE		896

typedef enum
{
    AT_CMD = 1,
    CPIN_CMD,
    CGSN_CMD,
    CIMI_CMD,
    CSQ_CMD,
    CREG_CMD,
    CEREG_CMD,
    CGATT_CMD,
    MYSYSINFO_CMD,
    CGDCONT_CMD,
    XGAUTH_CMD,
    XIIC_CMD,
   	TCPSETUP_CMD,
    IPSTATUS_CMD,
    TCPSEND_CMD,
    TCPACK_CMD,
    TCPCLOSE_CMD,
    CCLK_CMD,
    ATA_CMD,
    ATH_CMD,
    CMGF_CMD,
    CMGS_CMD,
    CMGD_CMD,
    CMGR_CMD,
    CNMI_CMD,
    MYGPSPWR_CMD,
    MYGPSPOS_CMD,
    MYGPSSTATE_CMD,
    MYGNSSSEL_CMD,
    MYPOWEROFF_CMD,
    TTSPLAY_CMD,
    PLAYTTS_CMD,
    FSLIST_CMD,
    FSWF_CMD,
    FSRF_CMD,
    FSDF_CMD,
    FSFAT_CMD,
    RECMODE_CMD,
    RECF_CMD,
    WIFIAPSCAN_CMD,
    CCID_CMD,
    SETSERVER_CMD,
    ENPWRSAVE_CMD,
    CFUN_CMD,
   	MICL_CMD,
    MYLACID_CMD,
    NWBTBLEPWR_CMD,
    NWBTBLENAME_CMD,
    NWBLEROLE_CMD,
    NWBLEPSEND_CMD,
    NWBLEDISCON_CMD,
    NWBLEMAC_CMD,
    NWBLECCON_CMD,
    NWBTBLEMAC_CMD,
    NWBLEADVEN_CMD,
    HTTPSCFG_CMD,
    HTTPSPARA_CMD,
    HTTPSSETUP_CMD,
    HTTPSACTION_CMD,
    HTTPPARA_CMD,
    HTTPSETUP_CMD,
    HTTPACTION_CMD,
    GMR_CMD,
    SIMHOTSWAP,
    POWEROFF_CMD,
    NETDMSG_CMD,
	RECVMODE_CMD,
	TCPREAD_CMD,
	SIMCROSS_CMD,
	READADC_CMD,
	CLIP_CMD,
	FSFS_CMD,
	CLVL_CMD,
} atCmdType_e;


/*定义系统运行状态*/
typedef enum
{
    AT_STATUS,	  //0
    CPIN_STATUS,
    CSQ_STATUS,
    CGREG_STATUS,
    CONFIG_STATUS,
    QIACT_STATUS,
    XIIC_STATUS,
    NORMAL_STATUS,
} moduleStatus_s;

/*指令集对应结构体*/
typedef struct
{
    atCmdType_e cmd_type;
    char *cmd;
} atCmd_s;

//发送队列结构体
typedef struct cmdNode
{
    char *data;
    uint16_t datalen;
    uint8_t currentcmd;
    struct cmdNode *nextnode;
} cmdNode_s;


typedef struct
{
	uint8_t  fileName[20];
	uint16_t fileSize;
}fileInfo_s;

typedef struct
{
    uint8_t powerState			: 1;
    uint8_t atResponOK			: 1;
    uint8_t cpinResponOk		: 1;
    uint8_t csqOk				: 1;
    uint8_t cgregOK				: 1;
    uint8_t ceregOK				: 1;
    uint8_t qipactSet			: 1;
    uint8_t qipactOk			: 1;
    uint8_t xiicSet				: 1;
    uint8_t xiicOk				: 1;
    uint8_t noGpsFile			: 1;

    uint8_t normalLinkQird		: 1;
    uint8_t agpsLinkQird		: 1;
    uint8_t bleLinkQird 		: 1;
    uint8_t jt808LinkQird		: 1;
    uint8_t hideLinkQird		: 1;
    uint8_t upgradeQird			: 1;

    uint8_t curQirdId;
    uint8_t rdyQirdId;
	uint8_t curSendId;

    uint8_t fsmState;
    uint8_t cmd;
    uint8_t rssi;

    uint8_t IMEI[21];
    uint8_t IMSI[21];
    uint8_t ICCID[21];
    uint8_t messagePhone[20];

    uint8_t gpsUpFsm;
    uint8_t gpsFileHandle;


    uint8_t mnc;
    uint16_t mcc;
    uint16_t lac;

    uint32_t cid;
    uint32_t powerOnTick;
    uint32_t gpsUpTotalSize;
    uint32_t gpsUpHadRead;
    uint32_t tcpTotal;
    uint32_t tcpAck;
    uint32_t tcpNack;
    uint32_t fsmtick;
	fileInfo_s  file[FILE_MAX_CNT];
	uint8_t  fileNum;
	uint8_t ataflag             :1;
} moduleState_s;

typedef struct
{
    uint8_t scanMode;
    uint8_t atCount;
    uint8_t cpincount;
    uint8_t csqCount;
    uint8_t cgregCount;
    uint8_t qipactCount;
    uint8_t xiicCount;
    uint8_t qiopenCount;
    uint16_t csqTime;
} moduleCtrl_s;


uint8_t createNode(char *data, uint16_t datalen, uint8_t currentcmd);
void outputNode(void);
uint8_t  sendModuleCmd(uint8_t cmd, char *param);

void modulePowerOn(void);
void modulePowerOff(void);
void moduleReset(void);

void openSocket(uint8_t link, char *server, uint16_t port);
void closeSocket(uint8_t link);
void netSetSim(uint8_t sim);

void netConnectTask(void);
void moduleRecvParser(uint8_t *buf, uint16_t bufsize);

void netResetCsqSearch(void);
int socketSendData(uint8_t link, uint8_t *data, uint16_t len);
void moduleSleepCtl(uint8_t onoff);
void moduleFileInfoClear(void);

void moduleGetCsq(void);
void moduleGetLbs(void);
void moduleGetWifiScan(void);

void sendMessage(uint8_t *buf, uint16_t len, char *telnum);
void deleteMessage(void);
void querySendData(uint8_t link);
void queryBatVoltage(void);

uint8_t isAgpsDataRecvComplete(void);
fileInfo_s* getFileList(uint8_t *num);

uint8_t isModulePowerOn(void);

uint8_t compareFile(uint8_t *file, uint8_t len);

uint8_t getModuleRssi(void);
uint8_t *getModuleIMSI(void);
uint8_t *getModuleIMEI(void);
uint8_t *getModuleICCID(void);
uint16_t getMCC(void);
uint8_t getMNC(void);
uint16_t getLac(void);
uint32_t getCid(void);
uint32_t getTcpNack(void);

uint8_t isModuleRunNormal(void);
uint8_t isModulePowerOnOk(void);
void stopCall(void);
void callPhone(char *tel);


#endif

