/*
 * app_bleRelay.h
 *
 *  Created on: Oct 27, 2022
 *      Author: idea
 */

#ifndef TASK_INC_APP_BLERELAY_H_
#define TASK_INC_APP_BLERELAY_H_

#include "CH58x_common.h"

#define BLE_CONNECT_LIST_SIZE   2

#define BLE_UPGRADE_FLAG		0x66

#define CMD_GET_SHIELD_CNT        			0x00  //获取屏蔽次数
#define CMD_CLEAR_SHIELD_CNT      			0x01  //清除计数器，并接通继电器
#define CMD_DEV_ON                			0x02  //主动接通继电器
#define CMD_DEV_OFF               			0x03  //主动断开继电器
#define CMD_SET_VOLTAGE           			0x04  //设置屏蔽电压阈值
#define CMD_GET_VOLTAGE           			0x05  //读取屏蔽电压阈值
#define CMD_GET_ADCV              			0x06  //读取屏蔽电压
#define CMD_SET_OUTVOLTAGE        			0x07  //设置外部电压阈值
#define CMD_GET_OUTVOLTAGE        			0x08  //读取外部电压阈值
#define CMD_GET_OUTV              			0x09  //读取外部电压值
#define CMD_GET_ONOFF             			0x0A  //读取继电器状态
#define CMD_ALARM                 			0x0B  //报警发送
#define CMD_SPEED_FLAG            			0x0C  //速度标志
#define CMD_AUTODIS               			0x0D  //设置倒计时参数
#define CMD_CLEAR_ALARM           			0x0E  //清除报警
#define CMD_PREALARM              			0x0F  //预警发送
#define CMD_CLEAR_PREALARM        			0x10  //清除预警
#define CMD_OTA                             0x11  //进入OTA
#define CMD_VER                             0x12  //读version
#define CMD_SET_PRE_ALARM_PARAM   			0x13  //设置屏蔽预警参数
#define CMD_GET_PRE_ALARM_PARAM   			0x14  //读取屏蔽预警参数
#define CMD_GET_DISCONNECT_PARAM  			0x15  //读取倒计时参数
#define CMD_CHK_SOCKET_STATUS				0x18  //获取主机网络状态
#define CMD_SEND_SHIELD_LOCK_ALARM			0x19  //发送屏蔽锁车报警
#define CMD_RES_SHIELD_LOCK_ALRAM			0x20  //屏蔽锁车应答
#define CMD_CLEAR_SHIELD_LOCK_ALRAM			0x21  //清除屏蔽锁车事件
#define CMD_SEND_FAST_SHIELD_ALARM			0x22  //发送快速屏蔽报警
#define CMD_CLEAR_FAST_SHIELD_ALARM			0x23  //清除快速屏蔽报警


#define CMD_SET_TXPOWER             		0xA7  //发射功率控制
#define CMD_SET_RTC               			0xA8  //更新RTC时间
#define CMD_RESET							0xA9  //复位



/* IAP定义 */
/* 以下为IAP下载命令定义 */
#define CMD_IAP_PROM         0x80               // IAP编程命令
#define CMD_IAP_ERASE        0x81               // IAP擦除命令
#define CMD_IAP_VERIFY       0x82               // IAP校验命令
#define CMD_IAP_END          0x83               // IAP结束标志
#define CMD_IAP_INFO         0x84               // IAP获取设备信息
#define CMD_IAP_ERR			 0xfe


/* 数据帧长度定义 */
#define IAP_LEN              247




#define BLE_EVENT_GET_OUTV        0x00000001 //读取蓝牙继电器的外部电压
#define BLE_EVENT_GET_RFV         0x00000002 //读取蓝牙继电器的屏蔽电压
#define BLE_EVENT_GET_RF_THRE     0x00000004 //读取屏蔽电压阈值
#define BLE_EVENT_GET_OUT_THRE    0x00000008 //读取外部电压阈值
#define BLE_EVENT_SET_TXPOWER	  0x00000010 //设置发射功率
#define BLE_EVENT_RESET			  0x00000020 //复位

#define BLE_EVENT_SET_DEVON       0x00000100 //继电器on
#define BLE_EVENT_SET_DEVOFF      0x00000200 //继电器off
#define BLE_EVENT_CLR_CNT         0x00000400 //清除屏蔽次数
#define BLE_EVENT_SET_RF_THRE     0x00000800 //设置屏蔽电压阈值
#define BLE_EVENT_SET_OUTV_THRE   0x00001000 //设置acc检测电压阈值
#define BLE_EVENT_SET_AD_THRE     0x00002000 //设置自动断连参数阈值
#define BLE_EVENT_CLR_ALARM       0x00004000 //清除报警
#define BLE_EVENT_CLR_PREALARM    0x00008000 //清除预警
#define BLE_EVENT_SET_PRE_PARAM   0x00010000 //设置预警参数
#define BLE_EVENT_GET_PRE_PARAM   0x00020000 //读取预警参数
#define BLE_EVENT_GET_AD_THRE     0x00040000 //读取自动断连参数
#define BLE_EVENT_SET_RTC         0x00080000 //设置RTC参数
#define BLE_EVENT_CHK_SOCKET	  0x00100000 //获取网络链路状态
#define BLE_EVENT_RES_LOCK_ALRAM  0x00200000 //应答屏蔽锁车报警
#define BLE_EVENT_VERSION		  0x00400000 //读取继电器Version
#define BLE_EVENT_OTA			  0x00800000 //OTA

#define BLE_EVENT_CLR_FAST_ALARM  0x01000000 //清除快速预报警
#define BLE_EVENT_OTA_EARSE		  0x10000000
#define BLE_EVENT_OTA_VERIFY	  0x20000000
#define BLE_EVENT_OTA_END	  	  0x40000000
#define BLE_EVENT_OTA_INFO        0x80000000

#define BLE_EVENT_ALL			  0xFFFFFFFF



#define APP_TX_POWEER_0_DBM                        0
#define APP_TX_POWEER_1_DBM                        1
#define APP_TX_POWEER_2_DBM                        2
#define APP_TX_POWEER_3_DBM                        3
#define APP_TX_POWEER_4_DBM                        4
#define APP_TX_POWEER_5_DBM                        5
#define APP_TX_POWEER_6_DBM                        6





typedef struct
{
    float outV;         //外电电压
    float rfV;          //屏蔽电压

    float rf_threshold; //屏蔽电压阈值
    float out_threshold;//外电电压阈值

    uint32_t updateTick;//更新时间
	
    uint8_t bleLost;    //链路状态
    uint8_t preV_threshold;         //预警阈值
    uint8_t preDetCnt_threshold;    //预警检测次数
    uint8_t preHold_threshold;      //预警保持
    uint8_t fastPreVDetTime;		//快速预屏蔽检测时间
    uint8_t disc_threshold;         //断连阈值
    uint8_t version[25];			//继电器版本
    uint8_t upgradeFlag;			//是否需要升级状态
} bleRelayInfo_s;


typedef struct
{
    uint8_t addr[6];        //mac
    uint8_t addrType;       //type
    uint8_t used;           //是否已使用
    uint32_t dataReq;       //需要发送的指令类型
    bleRelayInfo_s bleInfo; //蓝牙继电器信息
} bleRelayDev_s;


/* OTA IAP通讯协议定义 */
/* 地址使用4倍偏移 */
typedef union
{
    struct
    {
        unsigned char cmd;          /* 命令码 0x81 */
        unsigned char len;          /* 后续数据长度 */
        unsigned char addr[2];      /* 擦除地址 */
        unsigned char block_num[2]; /* 擦除块数 */

    } erase; /* 擦除命令 */
    struct
    {
        unsigned char cmd;       /* 命令码 0x83 */
        unsigned char len;       /* 后续数据长度 */
        unsigned char status[2]; /* 两字节状态，保留 */
    } end;                       /* 结束命令 */
    struct
    {
        unsigned char cmd;              /* 命令码 0x82 */
        unsigned char len;              /* 后续数据长度 */
        unsigned char addr[2];          /* 校验地址 */
        unsigned char buf[IAP_LEN - 4]; /* 校验数据 */
    } verify;                           /* 校验命令 */
    struct
    {
        unsigned char cmd;              /* 命令码 0x80 */
        unsigned char len;              /* 后续数据长度 */
        unsigned char addr[2];          /* 地址 */
        unsigned char buf[IAP_LEN - 4]; /* 后续数据 */
    } program;                          /* 编程命令 */
    struct
    {
        unsigned char cmd;              /* 命令码 0x84 */
        unsigned char len;              /* 后续数据长度 */
        unsigned char buf[IAP_LEN - 2]; /* 后续数据 */
    } info;                             /* 编程命令 */
    struct
    {
        unsigned char buf[IAP_LEN]; /* 接收数据包*/
    } other;
} OTA_IAP_CMD_t;


void bleRelaySetReq(uint8_t ind, uint32_t req);
void bleRelaySetAllReq(uint32_t req);
void bleRelayClearReq(uint8_t ind, uint32_t req);
void bleRelayClearAllReq(uint32_t req);
bleRelayInfo_s *bleRelayGeInfo(uint8_t i);
void bleRelayDeleteAll(void);
void bleRelayInit(void);
int8_t bleRelayInsert(uint8_t *addr, uint8_t addrType);
int8_t bleRelayInsertIndex(uint8_t index, uint8_t *addr, uint8_t addrType);
void bleRelayRecvParser(uint16_t connHandle, uint8_t *data, uint8_t len);
void bleRelaySendDataTry(void);
void blePeriodTask(void);
void bleConnPermissonManger(void);


#endif /* TASK_INC_APP_BLERELAY_H_ */
