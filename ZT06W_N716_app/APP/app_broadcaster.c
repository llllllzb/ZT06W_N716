/*
 * app_broadcaster.c
 *
 *  Created on: Feb 23, 2022
 *      Author: idea
 */
#include "config.h"
#include "app_broadcaster.h"
#include "app_sys.h"
static uint8 appBroadcastTaskId = INVALID_TASK_ID;
static gapRolesBroadcasterCBs_t appBroadcasterStruct;


static tmosEvents appBroadcasterEventProcess(tmosTaskID taskID, tmosEvents event);
static void appBroadcasterNofifyCallBack(gapRole_States_t newState);


/**
 * @brief               ���ù㲥��Ϣ
 * @param
 *      broadcastnmae   �㲥����
 * @return  none
 */
void appBroadcastCfg(uint8 *broadcastnmae)
{
    uint8 len, advLen;
    uint8 advertData[31];
    len = tmos_strlen(broadcastnmae);

    advLen = 0;
    advertData[advLen++] = len + 1;
    advertData[advLen++] = GAP_ADTYPE_LOCAL_NAME_COMPLETE;
    tmos_memcpy(advertData + advLen, broadcastnmae, len);
    advLen += len;
	//���ù㲥��Ϣ
    GAPRole_SetParameter(GAPROLE_ADVERT_DATA, advLen, advertData);
}

/**
 * @brief               �㲥��ʼ��
 * @param
 * @return  none
 */
void appBroadCasterInit(void)
{

    uint8 value;
    //�㲥��ʼ��
    GAPRole_BroadcasterInit();
    //����ע��
    appBroadcastTaskId = TMOS_ProcessEventRegister(appBroadcasterEventProcess);
    //�ص�������ʼ��
    appBroadcasterStruct.pfnStateChange = appBroadcasterNofifyCallBack;
    appBroadcasterStruct.pfnScanRecv = NULL;
    //ʹ�ܹ㲥
    value = ENABLE;
    GAPRole_SetParameter(GAPROLE_ADVERT_ENABLED, sizeof(uint8), &value);
    //��������
    value = GAP_ADTYPE_ADV_NONCONN_IND;
    GAPRole_SetParameter(GAPROLE_ADV_EVENT_TYPE, sizeof(uint8), &value);
    //���ù㲥��Ϣ
    appBroadcastCfg("Ch582F");
    //�����㲥����
    tmos_set_event(appBroadcastTaskId, APP_BROADCAST_START_EVENT);

}
/**
 * @brief         �㲥����ص�
 * @param
 *      taskID
 *      events
 * @return
 */
static tmosEvents appBroadcasterEventProcess(tmosTaskID taskID, tmosEvents events)
{
    if (events & SYS_EVENT_MSG)
    {
        uint8 *pMsg;
        if ((pMsg = tmos_msg_receive(appBroadcastTaskId)) != NULL)
        {
            tmos_msg_deallocate(pMsg);
        }
        return (events ^ SYS_EVENT_MSG);
    }
    if (events & APP_BROADCAST_START_EVENT)
    {
        GAPRole_BroadcasterStartDevice(&appBroadcasterStruct);
        return events ^ APP_BROADCAST_START_EVENT;
    }

    return 0;
}
/**
 * @brief         �㲥״̬֪ͨ
 * @param
 *      newState
 * @return
 */
static void appBroadcasterNofifyCallBack(gapRole_States_t newState)
{
    switch (newState)
    {
        case GAPROLE_STARTED:
            LogPrintf(DEBUG_ALL,"GAPRole started");
            break;

        case GAPROLE_ADVERTISING:
            LogPrintf(DEBUG_ALL,"GAPRole advertising");
            break;

        case GAPROLE_WAITING:
            LogPrintf(DEBUG_ALL,"GAPRole waitting");
            break;

        case GAPROLE_ERROR:
            LogPrintf(DEBUG_ALL,"GAPRole error");
            break;

        default:
            break;
    }
}
