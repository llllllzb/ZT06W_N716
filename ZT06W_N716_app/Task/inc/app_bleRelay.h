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

#define CMD_GET_SHIELD_CNT        			0x00  //��ȡ���δ���
#define CMD_CLEAR_SHIELD_CNT      			0x01  //���������������ͨ�̵���
#define CMD_DEV_ON                			0x02  //������ͨ�̵���
#define CMD_DEV_OFF               			0x03  //�����Ͽ��̵���
#define CMD_SET_VOLTAGE           			0x04  //�������ε�ѹ��ֵ
#define CMD_GET_VOLTAGE           			0x05  //��ȡ���ε�ѹ��ֵ
#define CMD_GET_ADCV              			0x06  //��ȡ���ε�ѹ
#define CMD_SET_OUTVOLTAGE        			0x07  //�����ⲿ��ѹ��ֵ
#define CMD_GET_OUTVOLTAGE        			0x08  //��ȡ�ⲿ��ѹ��ֵ
#define CMD_GET_OUTV              			0x09  //��ȡ�ⲿ��ѹֵ
#define CMD_GET_ONOFF             			0x0A  //��ȡ�̵���״̬
#define CMD_ALARM                 			0x0B  //��������
#define CMD_SPEED_FLAG            			0x0C  //�ٶȱ�־
#define CMD_AUTODIS               			0x0D  //���õ���ʱ����
#define CMD_CLEAR_ALARM           			0x0E  //�������
#define CMD_PREALARM              			0x0F  //Ԥ������
#define CMD_CLEAR_PREALARM        			0x10  //���Ԥ��
#define CMD_SET_PRE_ALARM_PARAM   			0x13  //��������Ԥ������
#define CMD_GET_PRE_ALARM_PARAM   			0x14  //��ȡ����Ԥ������
#define CMD_GET_DISCONNECT_PARAM  			0x15  //��ȡ����ʱ����
#define CMD_SET_RTC               			0xA8  //����RTCʱ��
#define CMD_CHK_SOCKET_STATUS				0x18  //��ȡ��������״̬
#define CMD_SEND_SHIELD_LOCK_ALARM			0x19  //����������������
#define CMD_RES_SHIELD_LOCK_ALRAM			0x20  //��������Ӧ��
#define CMD_CLEAR_SHIELD_LOCK_ALRAM			0x21  //������������¼�



#define BLE_EVENT_GET_OUTV        0x00000001 //��ȡ�����̵������ⲿ��ѹ
#define BLE_EVENT_GET_RFV         0x00000002 //��ȡ�����̵��������ε�ѹ
#define BLE_EVENT_GET_RF_THRE     0x00000004 //��ȡ���ε�ѹ��ֵ
#define BLE_EVENT_GET_OUT_THRE    0x00000008 //��ȡ�ⲿ��ѹ��ֵ
#define BLE_EVENT_SET_DEVON       0x00000100 //�̵���on
#define BLE_EVENT_SET_DEVOFF      0x00000200 //�̵���off
#define BLE_EVENT_CLR_CNT         0x00000400 //������δ���
#define BLE_EVENT_SET_RF_THRE     0x00000800 //�������ε�ѹ��ֵ
#define BLE_EVENT_SET_OUTV_THRE   0x00001000 //����acc����ѹ��ֵ
#define BLE_EVENT_SET_AD_THRE     0x00002000 //�����Զ�����������ֵ
#define BLE_EVENT_CLR_ALARM       0x00004000 //�������
#define BLE_EVENT_CLR_PREALARM    0x00008000 //���Ԥ��
#define BLE_EVENT_SET_PRE_PARAM   0x00010000 //����Ԥ������
#define BLE_EVENT_GET_PRE_PARAM   0x00020000 //��ȡԤ������
#define BLE_EVENT_GET_AD_THRE     0x00040000 //��ȡ�Զ���������
#define BLE_EVENT_SET_RTC         0x00080000 //����RTC����
#define BLE_EVENT_CHK_SOCKET	  0x00100000 //��ȡ������·״̬
#define BLE_EVENT_RES_LOCK_ALRAM  0x00200000 //Ӧ��������������
#define BLE_EVENT_CLR_LOCK_ALARM  0x00400000 //���������������



typedef struct
{
    float outV;         //����ѹ
    float rfV;          //���ε�ѹ

    float rf_threshold; //���ε�ѹ��ֵ
    float out_threshold;//����ѹ��ֵ

    uint32_t updateTick;//����ʱ��

    uint8_t bleLost;    //��·״̬
    uint8_t preV_threshold;         //Ԥ����ֵ
    uint8_t preDetCnt_threshold;    //Ԥ��������
    uint8_t preHold_threshold;      //Ԥ������
    uint8_t disc_threshold;         //������ֵ
} bleRelayInfo_s;


typedef struct
{
    uint8_t addr[6];        //mac
    uint8_t addrType;       //type
    uint8_t used;           //�Ƿ���ʹ��
    uint32_t dataReq;       //��Ҫ���͵�ָ������
    bleRelayInfo_s bleInfo; //�����̵�����Ϣ
} bleRelayDev_s;

void bleRelaySetReq(uint8_t ind, uint32_t req);
void bleRelaySetAllReq(uint32_t req);
void bleRelayClearReq(uint8_t ind, uint32_t req);
void bleRelayClearAllReq(uint32_t req);
bleRelayInfo_s *bleRelayGeInfo(uint8_t i);
void bleRelayDeleteAll(void);
void bleRelayInit(void);
int8_t bleRelayInsert(uint8_t *addr, uint8_t addrType);
void bleRelayRecvParser(uint8_t connHandle, uint8_t *data, uint8_t len);
void bleRelaySendDataTry(void);
void blePeriodTask(void);
uint8_t bleNoNetHandShakeTask(void);
uint8 bleHandShakeTask(void);

#endif /* TASK_INC_APP_BLERELAY_H_ */
