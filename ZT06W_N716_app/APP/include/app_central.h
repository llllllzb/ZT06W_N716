/*
 * app_central.h
 *
 *  Created on: Jun 27, 2022
 *      Author: idea
 */

#ifndef APP_INCLUDE_APP_CENTRAL_H_
#define APP_INCLUDE_APP_CENTRAL_H_

#include "config.h"

#define SERVICE_UUID                    0xFFE0
#define CHAR_UUID                       0xFFE1

#define DEVICE_MAX_CONNECT_COUNT        2

#define BLE_TASK_START_EVENT            0x0001
#define BLE_TASK_NOTIFYEN_EVENT         0x0002
#define BLE_TASK_SENDTEST_EVENT         0x0004
#define BLE_TASK_SCHEDULE_EVENT         0x0008
#define BLE_TASK_SVC_DISCOVERY_EVENT	0x0010
#define BLE_TASK_TERMINATE_EVENT		0x0020



typedef struct
{
    uint8 addr[B_ADDR_LEN];
    uint8 addrType;
    uint8 eventType;
    uint8 broadcaseName[31];
    int8  rssi;
} deviceScanInfo_s;

typedef struct
{
    uint8_t connStatus      :1;
    uint8_t findServiceDone :1;
    uint8_t findCharDone	:1;
    uint8_t notifyDone      :1;
    uint8_t use             :1;
    uint8_t connPermit		:1;
    uint8_t addr[6];
    uint8_t addrType;
    uint8_t discState;		//��״̬���˵�һ���ϵ磬����ʱ��������Э��ջ��д

    uint16_t connHandle;
    uint16_t startHandle;
    uint16_t endHandle;
    uint16_t charHandle;
    uint16_t notifyHandle;
    
} deviceConnInfo_s;

typedef struct
{
    uint8_t fsm;
    uint16_t runTick;
    uint8_t disconnIng;
} bleScheduleInfo_s;

typedef enum
{
	BLE_DISC_STATE_IDLE, // Idle
	BLE_DISC_STATE_CONN, // Connect success
    BLE_DISC_STATE_SVC,  // Service discovery
    BLE_DISC_STATE_CHAR, // Characteristic discovery
    BLE_DISC_STATE_CCCD, // client characteristic configuration discovery
    BLE_DISC_STATE_COMP, // Ble connect complete
}bleDevDiscState_e;

typedef enum
{
    BLE_SCHEDULE_IDLE,
    BLE_SCHEDULE_WAIT,
    BLE_SCHEDULE_DONE,
    BLE_SCHEDULE_NONET,
} bleFsm;


extern tmosTaskID bleCentralTaskId;
void bleCentralInit(void);
void bleCentralStartDiscover(void);
void bleCentralStartConnect(uint8_t *addr, uint8_t addrType);
void bleCentralDisconnect(uint16_t connHandle);
uint8 bleCentralSend(uint16_t connHandle, uint16 attrHandle, uint8 *data, uint8 len);
void bleDevTerminate(void);
deviceConnInfo_s *bleDevGetInfo(uint8_t *addr);
uint8_t bleDevGetCnt(void);
uint8_t *bleDevGetAddrByHandle(uint16_t connHandle);
void bleDevSetPermit(uint8 id, uint8_t enabled);
uint8_t bleDevGetPermit(uint8_t id);

static void bleDevDiscoverServByUuid(void);
int bleDevGetIdByHandle(uint16_t connHandle);

int8_t bleDevConnAdd(uint8_t *addr, uint8_t addrType);
int8_t bleDevConnDel(uint8_t *addr);
void bleDevConnDelAll(void);

#endif /* APP_INCLUDE_APP_CENTRAL_H_ */
