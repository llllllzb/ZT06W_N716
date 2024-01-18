/*
 * app_server.h
 *
 *  Created on: Jul 14, 2022
 *      Author: idea
 */

#ifndef TASK_INC_APP_SERVER_H_
#define TASK_INC_APP_SERVER_H_

#include "CH58x_common.h"

typedef enum
{
    SERV_LOGIN,
    SERV_LOGIN_WAIT,
    SERV_READY,
    
    SERV_END,
} NetWorkFsmState;

typedef enum
{
    NETWORK_LOGIN,					//0
    NETWORK_LOGIN_WAIT,				//1
    NETWORK_LOGIN_READY,			//2


    NETWORK_DOWNLOAD_DOING,			//3
    NETWORK_DOWNLOAD_WAIT,			//4
    

    NETWORK_DOWNLOAD_DONE,			//5
    NETWORK_VERIFY_LENGTH,			//6
    NETWORK_VERIFY_ERROR,
    NETWORK_DOWNLOAD_END
} UpgradeFsmState;

typedef struct
{
    NetWorkFsmState fsmstate;
    unsigned int heartbeattick;
    unsigned short serial;
    uint8_t logintick;
    uint8_t loginCount;
} netConnectInfo_s;
typedef struct bleDev
{
    char imei[16];
    uint8_t batLevel;
    uint16_t startCnt;
    float   vol;
    struct bleDev *next;
} bleInfo_s;


typedef enum
{
    JT808_REGISTER,
    JT808_AUTHENTICATION,
    JT808_NORMAL,
} jt808_connfsm_s;

typedef struct
{
    jt808_connfsm_s connectFsm;
    uint8_t runTick;
    uint8_t regCnt;
    uint8_t authCnt;
    uint16_t hbtTick;
} jt808_Connect_s;

typedef struct
{
	UpgradeFsmState fsm;
	uint16_t 		logintick;
	uint8_t 		loginCount;
	uint8_t         getVerCount;
	uint16_t 		runTick;
	uint8_t 		index;
	uint32_t		fileSize;
	
}upgrade_Connect_s;

void moduleRspSuccess(void);
void hbtRspSuccess(void);

void privateServerReconnect(void);
void privateServerLoginSuccess(void);

void hiddenServerLoginSuccess(void);
void hiddenServerCloseRequest(void);
void hiddenServerCloseClear(void);


void jt808ServerReconnect(void);
void jt808ServerAuthSuccess(void);


void bleServerAddInfo(bleInfo_s dev);
void showBleList(void);
void bleSerLoginReady(void);

void agpsRequestSet(void);
void agpsRequestClear(void);
void upgradeServerInit(uint8_t index);
void upgradeServerCancel(void);
void upgradeServerLoginSuccess(void);
int8_t upgradeDevIndex(void);
UpgradeFsmState getUpgradeServerFsm(void);
void upgradeServerChangeFsm(UpgradeFsmState state);

uint8_t primaryServerIsReady(void);
uint8_t hiddenServerIsReady(void);
uint8_t upgradeServerIsReady(void);
void upgradeServerConnTask(void);

void serverManageTask(void);

#endif /* TASK_INC_APP_SERVER_H_ */
