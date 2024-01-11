/*
 * app_central.h
 *
 *  Created on: Jun 27, 2022
 *      Author: idea
 */

#ifndef APP_INCLUDE_APP_CENTRAL_H_
#define APP_INCLUDE_APP_CENTRAL_H_

#include "config.h"
#include "app_bleRelay.h"
#include "app_protocol.h"

//通用继电器UUID
#define SERVICE_UUID                    0xFFE0
#define CHAR_UUID                       0xFFE1

//OTA继电器UUID
#define OTA_SERVER_UUID					0xFEE0
#define OTA_CHAR_UUID					0xFEE1

#define DEVICE_MAX_CONNECT_COUNT        2

//OTA起始地址
#define IMAGE_A_START_ADD			    0x1000

#define BLE_TASK_START_EVENT            0x0001
#define BLE_TASK_NOTIFYEN_EVENT         0x0002
#define BLE_TASK_SCHEDULE_EVENT         0x0004
#define BLE_TASK_SVC_DISCOVERY_EVENT	0x0008
#define BLE_TASK_TERMINATE_EVENT		0x0010
#define BLE_TASK_READ_RSSI_EVENT		0x0020
#define BLE_TASK_OTA_WRITE_EVENT		0x0040
#define BLE_TASK_OTA_READ_EVENT			0x0080
#define BLE_TASK_UPDATE_PARAM_EVENT		0x1000
#define BLE_TASK_UPDATE_MTU_EVENT		0x2000

#define BLE_ID_0						0
#define BLE_ID_1						1

#define BLE_CONN_ENABLE					1
#define BLE_CONN_DISABLE				0

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
	/* 应用层蓝牙调度器改写参数 */
    uint8_t  connStatus      :1;
    uint8_t  findServiceDone :1;
    uint8_t  findCharDone	 :1;
    uint8_t  notifyDone      :1;	//这个可以作为ota还是normal的标志
    uint8_t  use             :1;
    uint8_t  otaStatus		 :1;
    uint8_t  addr[6];
    uint8_t  addrType;
    uint16_t connHandle;
    uint16_t startHandle;
    uint16_t endHandle;
    uint16_t charHandle;
    uint16_t notifyHandle;
    int8_t	 rssi;
 	uint8_t  connPermit		 :1;	// 该参数会一直判断并改写，所以可以在连接的时候改也可以不改 
 	uint16_t connTick;
 	/* 蓝牙协议栈改写参数 */
 	uint8_t  discState;				//该状态除了第一次上电，其余时间由蓝牙协议栈改写
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
} bleFsm;

typedef enum
{
	BLE_OTA_FSM_IDLE,
	BLE_OTA_FSM_INFO,
	BLE_OTA_FSM_EASER,
	BLE_OTA_FSM_PROM,
	BLE_OTA_FSM_VERI,
	BLE_OTA_FSM_END,
	BLE_OTA_FSM_FINISH,
}ble_ota_fsm_e;


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
void bleDevSetConnTick(uint8_t id, uint16_t tick);
uint8_t bleCentralRead(uint16_t connHandle, uint16_t attrHandle);
void bleDevAllPermitDisable(void);

static void bleDevDiscoverServByUuid(void);
int bleDevGetIdByHandle(uint16_t connHandle);
deviceConnInfo_s *bleDevGetInfoById(uint8_t id);
void bleDevReadAllRssi(void);
int bleDevGetOtaStatusByIndex(uint8_t index);
int8_t bleDevConnAdd(uint8_t *addr, uint8_t addrType);
int8_t bleDevConnDel(uint8_t *addr);
void bleDevConnDelAll(void);
void bleOtaFsmChange(ble_ota_fsm_e fsm);
void otaRxInfoInit(void);

void bleOtaInit(void);
ble_ota_fsm_e getBleOtaFsm(void);
void bleOtaSend(void);
void bleOtaFilePackage(ota_package_t file);
void bleConnTypeChange(uint8_t type, uint8_t index);


void bleOtaReadDataParser(uint16_t connHandle, OTA_IAP_CMD_t iap_read_data, uint16_t len);
uint8_t bleOtaSendProtocol(uint16_t connHandle, uint16_t charHandle);


#endif /* APP_INCLUDE_APP_CENTRAL_H_ */
