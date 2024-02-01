#include "app_central.h"
#include "app_sys.h"
#include "app_bleRelay.h"
#include "app_task.h"
#include "app_server.h"
#include "app_param.h"
#include "app_protocol.h"
#include "app_file.h"
#include "app_kernal.h"
//全局变量

tmosTaskID bleCentralTaskId = INVALID_TASK_ID;
static gapBondCBs_t bleBondCallBack;
static gapCentralRoleCB_t bleRoleCallBack;
static deviceConnInfo_s devInfoList[DEVICE_MAX_CONNECT_COUNT];
static bleScheduleInfo_s bleSchedule;
static int16_t ble_conn_now_id;
static OTA_IAP_CMD_t iap_send_buff;
static ble_ota_fsm_e ble_ota_fsm  = 0;
static uint16_t      ble_ota_tick = 0;
static fileRxInfo    otaRxInfo;


//函数声明
static tmosEvents bleCentralTaskEventProcess(tmosTaskID taskID, tmosEvents event);
static void bleCentralEventCallBack(gapRoleEvent_t *pEvent);
static void bleCentralHciChangeCallBack(uint16_t connHandle, uint16_t maxTxOctets, uint16_t maxRxOctets);
static void bleCentralRssiCallBack(uint16_t connHandle, int8_t newRSSI);
static void bleDevConnInit(void);

static void bleDevConnSuccess(uint8_t *addr, uint16_t connHandle);
static void bleDevDisconnect(uint16_t connHandle);
static void bleDevSetCharHandle(uint16_t connHandle, uint16_t handle);
static void bleDevSetNotifyHandle(uint16_t connHandle, uint16_t handle);
static void bleDevConnectComplete(uint16_t connHandle);
static void bleDevSetServiceHandle(uint16_t connHandle, uint16_t findS, uint16_t findE);
static void bleDevDiscoverAllServices(void);
static void bleDevDiscoverAllChars(uint16_t connHandle);
static void bleDevDiscoverNotify(uint16_t connHandle);
static uint8_t bleDevEnableNotify(void);
static uint8_t bleDevSendDataTest(void);
bStatus_t bleCentralChangeMtu(uint16_t connHandle);
static void bleDevSetConnUuid(uint8_t ind, uint16_t serUuid, uint16_t charUuid);

static void bleSchduleChangeFsm(bleFsm nfsm);
static void bleScheduleTask(void);
static void bleDevDiscoverCharByUuid(void);
static void bleDevDiscoverServByUuid(void);
static void bleDevSetConnUuid(uint8_t ind, uint16_t serUuid, uint16_t charUuid);





/**************************************************
@bref       BLE主机初始化
@param
@return
@note
**************************************************/

void bleCentralInit(void)
{
	otaRxInfoInit(0,NULL, 0);
    bleDevConnInit();
    bleRelayInit();
    GAPRole_CentralInit();
    GAP_SetParamValue(TGAP_DISC_SCAN, 2400);
    GAP_SetParamValue(TGAP_CONN_EST_INT_MIN, 20);
    GAP_SetParamValue(TGAP_CONN_EST_INT_MAX, 100);
    GAP_SetParamValue(TGAP_CONN_EST_SUPERV_TIMEOUT, 100);

    bleCentralTaskId = TMOS_ProcessEventRegister(bleCentralTaskEventProcess);
    GATT_InitClient();
    GATT_RegisterForInd(bleCentralTaskId);

    bleBondCallBack.pairStateCB = NULL;
    bleBondCallBack.passcodeCB = NULL;

    bleRoleCallBack.eventCB = bleCentralEventCallBack;
    bleRoleCallBack.ChangCB = bleCentralHciChangeCallBack;
    bleRoleCallBack.rssiCB = bleCentralRssiCallBack;

    tmos_set_event(bleCentralTaskId, BLE_TASK_START_EVENT);
    tmos_start_reload_task(bleCentralTaskId, BLE_TASK_SCHEDULE_EVENT, MS1_TO_SYSTEM_TIME(1000));
}


/**************************************************
@bref      ATT_FIND_BY_TYPE_VALUE_RSP回调
@param
@return
@note
**************************************************/
static void attFindByTypeValueRspCB(gattMsgEvent_t *pMsgEvt)
{
	uint8_t i;
	for (i = 0; i < DEVICE_MAX_CONNECT_COUNT; i++)
	{
		if (devInfoList[i].use && devInfoList[i].discState == BLE_DISC_STATE_SVC)
		{
			if (pMsgEvt->msg.findByTypeValueRsp.numInfo > 0)
			{
				bleDevSetServiceHandle(pMsgEvt->connHandle, 
					   ATT_ATTR_HANDLE(pMsgEvt->msg.findByTypeValueRsp.pHandlesInfo, 0), 
					ATT_GRP_END_HANDLE(pMsgEvt->msg.findByTypeValueRsp.pHandlesInfo, 0));
			}
			if (pMsgEvt->hdr.status == bleProcedureComplete || pMsgEvt->hdr.status == bleTimeout)
	        {	
				bleDevDiscoverCharByUuid();
	        }
		}
	}	
}

/**************************************************
@bref	   ATT_READ_BY_TYPE_RSP回调
@param
@return
@note
**************************************************/
static void attReadByTypeRspCB(gattMsgEvent_t *pMsgEvt)
{
	uint8_t i;
	for (i = 0; i < DEVICE_MAX_CONNECT_COUNT; i++)
	{
		if (devInfoList[i].use && 
			devInfoList[i].discState == BLE_DISC_STATE_CHAR && 
			devInfoList[i].findServiceDone == 1)
		{
			if (pMsgEvt->msg.readByTypeRsp.numPairs > 0)
			{
                bleDevSetCharHandle(devInfoList[i].connHandle, 
        			   BUILD_UINT16(pMsgEvt->msg.readByTypeRsp.pDataList[0],
                                    pMsgEvt->msg.readByTypeRsp.pDataList[1]));
			}
			if ((pMsgEvt->method == ATT_READ_BY_TYPE_RSP && pMsgEvt->hdr.status == bleProcedureComplete))
            {
            	if (devInfoList[i].findServiceUuid == OTA_SERVER_UUID && 
            		devInfoList[i].findCharUuid    == OTA_CHAR_UUID)
            	{
					bleDevConnectComplete(devInfoList[i].connHandle);
            	}
            	else if (devInfoList[i].findServiceUuid == SERVICE_UUID &&
            	  		 devInfoList[i].findCharUuid    == CHAR_UUID)
            	{
					bleDevDiscoverNotify(devInfoList[i].connHandle);
				}
				else
				{
					LogPrintf(DEBUG_BLE, "attReadByTypeRspCB==>Error, serUuid:0x%04x charUuid:0x%04x",
      						  			  devInfoList[i].findServiceUuid,
      						              devInfoList[i].findCharUuid);
				}
				ble_conn_now_id = -1;
            }
		}
		if (devInfoList[i].use && devInfoList[i].discState    == BLE_DISC_STATE_CCCD
							   && devInfoList[i].findCharDone == 1)
		{
			if (pMsgEvt->msg.readByTypeRsp.numPairs > 0)
			{
				bleDevSetNotifyHandle(devInfoList[i].connHandle, 
						 BUILD_UINT16(pMsgEvt->msg.readByTypeRsp.pDataList[0],
                                      pMsgEvt->msg.readByTypeRsp.pDataList[1]));
               	tmos_start_task(bleCentralTaskId, BLE_TASK_NOTIFYEN_EVENT, MS1_TO_SYSTEM_TIME(1000));
			}
		}
	}
}

/**************************************************
@bref       GATT系统消息事件处理
@param
@return
@note
**************************************************/

static void gattMessageHandler(gattMsgEvent_t *pMsgEvt)
{
    char debug[101] = { 0 };
    uint8 dataLen, infoLen, numOf, i;
    uint8 *pData;
    uint8_t uuid16[16];
    uint16 uuid, startHandle, endHandle, findHandle;
    bStatus_t status;
    OTA_IAP_CMD_t iap_read_data;
    LogPrintf(DEBUG_BLE, "Handle[%d],Method[0x%02X],Status[0x%02X]", 
					    						pMsgEvt->connHandle, 
					    						pMsgEvt->method,
					             				pMsgEvt->hdr.status);
    switch (pMsgEvt->method)
    {
        case ATT_ERROR_RSP:
            LogPrintf(DEBUG_BLE, "Error,Handle[%d],ReqOpcode[0x%02X],ErrCode[0x%02X]", 
					            						pMsgEvt->connHandle,	//注意这里的handle不是连接句柄
					                      				pMsgEvt->msg.errorRsp.reqOpcode, 
					                      				pMsgEvt->msg.errorRsp.errCode);
            /* 找不到对应的服务 */
            if (ATT_ERR_ATTR_NOT_FOUND == pMsgEvt->msg.errorRsp.errCode)
            {
            	/* 
					情况1：主机往BLE写文件的时候，主机复位了，继电器处于IAP层
					情况2：设备刚开始升级，继电器还没擦除APP，继电器（断电）复位，继电器跑回APP层
            	*/
            	int8_t id = bleDevGetIdByHandle(pMsgEvt->connHandle);
            	if (id >= 0)
            	{
//					bleRelayDeleteAll();
//					sysparam.relayUpgrade[id] = BLE_UPGRADE_FLAG;
//					LogPrintf(DEBUG_BLE, "Dev(%d) connhandle[%d] was in ota program,change it", 
//										  id, pMsgEvt->connHandle);
					/* 情况2 */
					if (devInfoList[id].findServiceUuid == OTA_SERVER_UUID && 
	            		devInfoList[id].findCharUuid    == OTA_CHAR_UUID)
	            	{
						LogPrintf(DEBUG_BLE, "gattMessageHandler==>Error,Dev(%d)Handle[%d] is in app", 
										      id, pMsgEvt->connHandle);
						for (i = 0; i < DEVICE_MAX_CONNECT_COUNT; i++)
						{
							sysparam.relayUpgrade[i] = 0;
						}
						otaRxInfoInit(0, NULL, 0);
						paramSaveAll();
						bleRelayDeleteAll();
//						/* 切换normal蓝牙连接 */
//						bleDevSetConnUuid(i, SERVICE_UUID, CHAR_UUID);
	            	}
	            	else if (devInfoList[id].findServiceUuid == SERVICE_UUID &&
	            	  		 devInfoList[id].findCharUuid    == CHAR_UUID)
	            	{
	            		LogPrintf(DEBUG_BLE, "gattMessageHandler==>Error,Dev(%d)Handle[%d] is in boot", 
										      id, pMsgEvt->connHandle);
						sysparam.relayUpgrade[id] = BLE_UPGRADE_FLAG;
						paramSaveAll();
						bleRelayDeleteAll();
						/* 载入升级包信息 */
						
					}
					/* UUID错乱 */
					else
					{
						LogPrintf(DEBUG_BLE, "gattMessageHandler==>Error,serUuid:0x%04x charUuid:0x%04x",
	      						  			  devInfoList[id].findServiceUuid,
	      						              devInfoList[id].findCharUuid);
	      				bleRelayDeleteAll();
					}
            	}
            	else
            	{
					LogPrintf(DEBUG_BLE, "gattMessageHandler==>Error,id:%d, connhandle:%d", 
										  id, pMsgEvt->connHandle);
            	}
//            	else
//            	{
//					sysparam.relayUpgrade[getBleOtaStatus()] = 0;
//					paramSaveAll();
//					LogPrintf(DEBUG_BLE, "Dev connhandle[%d] was in normal program,change it", pMsgEvt->msg.errorRsp.handle);
//            	}
            }
            break;
        //查找服务 BY UUID
        case ATT_FIND_BY_TYPE_VALUE_RSP:
			attFindByTypeValueRspCB(pMsgEvt);
            break;
        //查找服务 ALL
        case ATT_READ_BY_GRP_TYPE_RSP:
            infoLen = pMsgEvt->msg.readByGrpTypeRsp.len;
            numOf   = pMsgEvt->msg.readByGrpTypeRsp.numGrps;
            pData   = pMsgEvt->msg.readByGrpTypeRsp.pDataList;
            dataLen = infoLen * numOf;
            if (infoLen != 0)
            {
                byteArrayInvert(pData, dataLen);
                for (i = 0; i < numOf; i++)
                {
                    uuid = 0;
                    switch (infoLen)
                    {
                        case 6:
                            uuid = pData[6 * i];
                            uuid <<= 8;
                            uuid |= pData[6 * i + 1];

                            endHandle = pData[6 * i + 2];
                            endHandle <<= 8;
                            endHandle |= pData[6 * i + 3];

                            startHandle = pData[6 * i + 4];
                            startHandle <<= 8;
                            startHandle |= pData[6 * i + 5];

                            LogPrintf(DEBUG_BLE, "ServUUID: [%04X],Start:0x%04X,End:0x%04X", uuid, startHandle, endHandle);
                            break;
                        case 20:
                            memcpy(uuid16, pData + (20 * i), 16);
                            endHandle = pData[20 * i + 16];
                            endHandle <<= 8;
                            endHandle |= pData[20 * i + 17];

                            startHandle = pData[20 * i + 18];
                            startHandle <<= 8;
                            startHandle |= pData[20 * i + 19];
                            byteToHexString(uuid16, debug, 16);
                            debug[32] = 0;
                            LogPrintf(DEBUG_BLE, "ServUUID: [%s],Start:0x%04X,End:0x%04X", debug, startHandle, endHandle);
                            break;
                    }
                    if (uuid == SERVICE_UUID)
                    {
                        LogPrintf(DEBUG_BLE, "Find my services uuid [%04X]", uuid);
                        bleDevSetServiceHandle(pMsgEvt->connHandle, startHandle, endHandle);
                    }
                }
            }
            if (pMsgEvt->hdr.status == bleProcedureComplete || pMsgEvt->hdr.status == bleTimeout)
            {
                LogPrintf(DEBUG_BLE, "Discover all services done!");
                bleDevDiscoverAllChars(pMsgEvt->connHandle);
            }
            break;
        //查找特征
        case ATT_READ_BY_TYPE_RSP:
			attReadByTypeRspCB(pMsgEvt);
            break;
        //数据发送回复
        case ATT_WRITE_RSP:
            LogPrintf(DEBUG_BLE, "^^Handle[%d] send %s!", pMsgEvt->connHandle, pMsgEvt->hdr.status == SUCCESS ? "success" : "fail");
            break;
		//读取数据
        case ATT_READ_RSP:
        	tmos_memcpy((unsigned char *)&iap_read_data, pMsgEvt->msg.readRsp.pValue, pMsgEvt->msg.readRsp.len);
        	dataLen = pMsgEvt->msg.readRsp.len > 50 ? 50 : pMsgEvt->msg.readRsp.len;
        	byteToHexString(pMsgEvt->msg.readRsp.pValue, debug, dataLen);
        	debug[dataLen * 2] = 0;
			LogPrintf(DEBUG_BLE, "^^Handle[%d] Read:%s, len:%d", pMsgEvt->connHandle, debug, pMsgEvt->msg.readRsp.len);
			if (pMsgEvt->msg.readRsp.len == 0)
			{
				tmos_start_reload_task(bleCentralTaskId, BLE_TASK_OTA_READ_EVENT, MS1_TO_SYSTEM_TIME(BLE_WRITE_OR_READ_MS));
				break;
			}
			bleOtaReadDataParser(pMsgEvt->connHandle, iap_read_data, pMsgEvt->msg.readRsp.len);
        	break;
        //收到notify数据
        case ATT_HANDLE_VALUE_NOTI:
            pData   = pMsgEvt->msg.handleValueNoti.pValue;
            dataLen = pMsgEvt->msg.handleValueNoti.len;
            byteToHexString(pData, debug, dataLen);
            debug[dataLen * 2] = 0;
            LogPrintf(DEBUG_BLE, "^^Handle[%d],Recv:[%s]", pMsgEvt->connHandle, debug);
            bleRelayRecvParser(pMsgEvt->connHandle, pData, dataLen);
            break;
        //修改MTU请求
        case ATT_EXCHANGE_MTU_RSP:

        	break;
        //修改MTU结果
        case ATT_MTU_UPDATED_EVENT:
			LogPrintf(DEBUG_BLE, "^^Handle[%d] MTU update:%d", pMsgEvt->connHandle, pMsgEvt->msg.mtuEvt.MTU);
        	break;
        default:
            LogPrintf(DEBUG_BLE, "It is unprocessed!!!");
            break;
    }
    GATT_bm_free(&pMsgEvt->msg, pMsgEvt->method);
}


/**************************************************
@bref       系统消息事件处理
@param
@return
@note
**************************************************/

static void sysMessageHandler(tmos_event_hdr_t *pMsg)
{
    switch (pMsg->event)
    {
        case GATT_MSG_EVENT:
            gattMessageHandler((gattMsgEvent_t *)pMsg);
            break;
        default:
            LogPrintf(DEBUG_BLE, "Unprocessed Event 0x%02X", pMsg->event);
            break;
    }
}


/**************************************************
@bref       主机任务事件处理
@param
@return
@note
**************************************************/

static tmosEvents bleCentralTaskEventProcess(tmosTaskID taskID, tmosEvents event)
{
    uint8_t *pMsg;
    bStatus_t status;
    if (event & SYS_EVENT_MSG)
    {
        if ((pMsg = tmos_msg_receive(bleCentralTaskId)) != NULL)
        {
            sysMessageHandler((tmos_event_hdr_t *) pMsg);
            tmos_msg_deallocate(pMsg);
        }
        return (event ^ SYS_EVENT_MSG);
    }
    if (event & BLE_TASK_START_EVENT)
    {
        status = GAPRole_CentralStartDevice(bleCentralTaskId, &bleBondCallBack, &bleRoleCallBack);
        if (status == SUCCESS)
        {
			LogMessage(DEBUG_BLE, "master role init..");
        }
        else 
        {
			LogPrintf(DEBUG_BLE, "master role init error, ret:0x%02x", status);
        }
        return event ^ BLE_TASK_START_EVENT;
    }
    if (event & BLE_TASK_NOTIFYEN_EVENT)
    {
        if (bleDevEnableNotify() == SUCCESS)
        {
            if (sysinfo.logLevel == 4)
            {
                LogMessage(DEBUG_FACTORY, "+FMPC:BLE CONNECT SUCCESS");
            }
            LogMessage(DEBUG_BLE, "Notify done!");
            bleSchduleChangeFsm(BLE_SCHEDULE_DONE);
        }
        return event ^ BLE_TASK_NOTIFYEN_EVENT;
    }
	if (event & BLE_TASK_SVC_DISCOVERY_EVENT)
	{
		bleDevDiscoverServByUuid();
		return event ^ BLE_TASK_SVC_DISCOVERY_EVENT;
	}

	if (event & BLE_TASK_READ_RSSI_EVENT)
	{
		for (uint8_t i = 0; i < DEVICE_MAX_CONNECT_COUNT; i++)
		{
			if (devInfoList[i].use && devInfoList[i].connHandle != INVALID_CONNHANDLE)
			{
				LogPrintf(DEBUG_BLE, "bleDevReadAllRssi==>Dev(%d) connhandle:%d", i, devInfoList[i].connHandle);
				GAPRole_ReadRssiCmd(devInfoList[i].connHandle);
			}
		}		
		return event ^ BLE_TASK_READ_RSSI_EVENT;
	}
	
    if (event & BLE_TASK_SCHEDULE_EVENT)
    {
        bleScheduleTask();
        blePeriodTask();
        bleRelaySendDataTry();
        return event ^ BLE_TASK_SCHEDULE_EVENT;
    }

    if (event & BLE_TASK_OTA_READ_EVENT)
    {
		status = bleIncorrectMode;
		for (uint8_t i = 0; i < DEVICE_MAX_CONNECT_COUNT; i++)
		{
			if (devInfoList[i].use && 
				devInfoList[i].connHandle != INVALID_CONNHANDLE &&
				sysparam.relayUpgrade[i]   == BLE_UPGRADE_FLAG)
			{
				LogPrintf(DEBUG_BLE, "Dev(%d) Read connHandle[0x%02x] charHandle[0x%02x]", i, devInfoList[i].connHandle, devInfoList[i].charHandle);
				status = bleCentralRead(devInfoList[i].connHandle, devInfoList[i].charHandle);
				break;
			}
		}
		if (status == SUCCESS)
		{
			tmos_stop_task(bleCentralTaskId, BLE_TASK_OTA_READ_EVENT);
		}
		return event ^ BLE_TASK_OTA_READ_EVENT;
    }

    if (event & BLE_TASK_OTA_WRITE_EVENT)
    {
    	status = bleIncorrectMode;
		for (uint8_t i = 0; i < DEVICE_MAX_CONNECT_COUNT; i++)
		{
			if (devInfoList[i].use && 
				devInfoList[i].connHandle != INVALID_CONNHANDLE &&
				sysparam.relayUpgrade[i]   == BLE_UPGRADE_FLAG)
			{
				LogPrintf(DEBUG_BLE, "Dev(%d) Write connHandle[0x%02x] charHandle[0x%02x]", i, devInfoList[i].connHandle, devInfoList[i].charHandle);
				status = bleOtaSendProtocol(devInfoList[i].connHandle, devInfoList[i].charHandle);
				break;
			}
		}
		return event ^ BLE_TASK_OTA_WRITE_EVENT;
    }

    if (event & BLE_TASK_UPDATE_PARAM_EVENT)
    {
    	status = bleIncorrectMode;
		if (devInfoList[0].use &&
			devInfoList[0].connHandle != INVALID_CONNHANDLE)
		{
			status = GAPRole_UpdateLink(devInfoList[0].connHandle,
										20,
										100,
										0,
										100);
			LogPrintf(DEBUG_BLE, "Dev(%d)[%d] Updataparam, ret:0x%02x", 0, devInfoList[0].connHandle, status);
		}
		if (status == blePending)
		{
			tmos_start_task(bleCentralTaskId, BLE_TASK_UPDATE_PARAM_EVENT, MS1_TO_SYSTEM_TIME(100));	
		}
		
		return event ^ BLE_TASK_UPDATE_PARAM_EVENT;
    }

    if (event & BLE_TASK_UPDATE_MTU_EVENT)
    {
    	status = bleIncorrectMode;

			if (devInfoList[0].use &&
				devInfoList[0].connHandle != INVALID_CONNHANDLE)
			{
				status = bleCentralChangeMtu(devInfoList[0].connHandle);
			}
		
		if (status == blePending)
		{
			tmos_start_task(bleCentralTaskId, BLE_TASK_UPDATE_MTU_EVENT, MS1_TO_SYSTEM_TIME(100));	
		}
		return event ^ BLE_TASK_UPDATE_MTU_EVENT;
    }
    
    if (event & BLE_TASK_UPDATE_PARAM2_EVENT)
    {
    	status = bleIncorrectMode;
		if (devInfoList[1].use &&
			devInfoList[1].connHandle != INVALID_CONNHANDLE)
		{
			status = GAPRole_UpdateLink(devInfoList[1].connHandle,
										20,
										100,
										0,
										100);
			LogPrintf(DEBUG_BLE, "Dev(%d)[%d] Updataparam, ret:0x%02x", 1, devInfoList[1].connHandle, status);
		}
		if (status == blePending)
		{
			tmos_start_task(bleCentralTaskId, BLE_TASK_UPDATE_PARAM2_EVENT, MS1_TO_SYSTEM_TIME(100));	
		}
		
		return event ^ BLE_TASK_UPDATE_PARAM2_EVENT;
    }

    if (event & BLE_TASK_UPDATE_MTU2_EVENT)
    {
    	status = bleIncorrectMode;
		if (devInfoList[1].use &&
			devInfoList[1].connHandle != INVALID_CONNHANDLE)
		{
			status = bleCentralChangeMtu(devInfoList[1].connHandle);
		}
		
		if (status == blePending)
		{
			tmos_start_task(bleCentralTaskId, BLE_TASK_UPDATE_MTU2_EVENT, MS1_TO_SYSTEM_TIME(100));	
		}
		return event ^ BLE_TASK_UPDATE_MTU2_EVENT;
    }

    

    if (event & BLE_TASK_100MS_EVENT)
    {
		return event ^ BLE_TASK_100MS_EVENT;
    }
    return 0;
}
/**-----------------------------------------------------------------**/
/**-----------------------------------------------------------------**/
/**************************************************
@bref       处理扫描时扫到的蓝牙设备
@param
@return
@note
**************************************************/

static void deviceInfoEventHandler(gapDeviceInfoEvent_t *pEvent)
{
    uint8 i, dataLen, cmd;
    char debug[100];
    deviceScanInfo_s scaninfo;

    byteToHexString(pEvent->addr, debug, B_ADDR_LEN);
    debug[B_ADDR_LEN * 2] = 0;
    LogPrintf(DEBUG_BLE, "MAC:[%s],TYPE:0x%02X,RSSI:%d", debug, pEvent->addrType, pEvent->rssi);

    tmos_memset(&scaninfo, 0, sizeof(deviceScanInfo_s));
    tmos_memcpy(scaninfo.addr, pEvent->addr, B_ADDR_LEN);
    scaninfo.rssi = pEvent->rssi;
    scaninfo.addrType = pEvent->addrType;
    scaninfo.eventType = pEvent->eventType;


    if (pEvent->pEvtData != NULL && pEvent->dataLen != 0)
    {

        byteToHexString(pEvent->pEvtData, debug, pEvent->dataLen);
        debug[pEvent->dataLen * 2] = 0;
        //LogPrintf(DEBUG_ALL, "BroadCast:[%s]", debug);

        for (i = 0; i < pEvent->dataLen; i++)
        {
            dataLen = pEvent->pEvtData[i];
            if ((dataLen + i + 1) > pEvent->dataLen)
            {
                return ;
            }
            cmd = pEvent->pEvtData[i + 1];
            switch (cmd)
            {
                case GAP_ADTYPE_LOCAL_NAME_SHORT:
                case GAP_ADTYPE_LOCAL_NAME_COMPLETE:
                    if (dataLen > 30)
                    {
                        break;
                    }
                    tmos_memcpy(scaninfo.broadcaseName, pEvent->pEvtData + i + 2, dataLen - 1);
                    scaninfo.broadcaseName[dataLen - 1] = 0;
                    LogPrintf(DEBUG_BLE, "<---->BroadName:[%s]", scaninfo.broadcaseName);
                    break;
                default:
                    //LogPrintf(DEBUG_ALL, "UnsupportCmd:0x%02X", cmd);
                    break;
            }

            i += dataLen;
        }
    }

}
/**************************************************
@bref       与从机建立链接成功
@param
@return
@note
**************************************************/

void linkEstablishedEventHandler(gapEstLinkReqEvent_t *pEvent)
{
    char debug[20];

    if (pEvent->hdr.status != SUCCESS)
    {
        LogPrintf(DEBUG_BLE, "Link established error,Status:[0x%X]", pEvent->hdr.status);
        return;
    }
    byteToHexString(pEvent->devAddr, debug, 6);
    debug[12] = 0;
    LogPrintf(DEBUG_BLE, "Device [%s] connect success", debug);
    bleDevConnSuccess(pEvent->devAddr, pEvent->connectionHandle);
    //bleDevDiscoverAllServices();
}

/**************************************************
@bref       与从机断开链接
@param
@return
@note
**************************************************/

void linkTerminatedEventHandler(gapTerminateLinkEvent_t *pEvent)
{
    LogPrintf(DEBUG_BLE, "Device disconnect,Handle [%d],Reason [0x%02X]", pEvent->connectionHandle, pEvent->reason);
    bleDevDisconnect(pEvent->connectionHandle);
}
/**-----------------------------------------------------------------**/
/**-----------------------------------------------------------------**/
/**************************************************
@bref       主机底层事件回调
@param
@return
@note
**************************************************/

static void bleCentralEventCallBack(gapRoleEvent_t *pEvent)
{
    LogPrintf(DEBUG_BLE, "bleCentral Event==>[0x%02X]", pEvent->gap.opcode);
    switch (pEvent->gap.opcode)
    {
        case GAP_DEVICE_INIT_DONE_EVENT:
            LogPrintf(DEBUG_BLE, "bleCentral init done!");
            break;
        case GAP_DEVICE_DISCOVERY_EVENT:
            LogPrintf(DEBUG_BLE, "bleCentral discovery done!");
            break;
        case GAP_ADV_DATA_UPDATE_DONE_EVENT:
            break;
        case GAP_MAKE_DISCOVERABLE_DONE_EVENT:
            break;
        case GAP_END_DISCOVERABLE_DONE_EVENT:
            break;
        case GAP_LINK_ESTABLISHED_EVENT:
            linkEstablishedEventHandler(&pEvent->linkCmpl);
            break;
        case GAP_LINK_TERMINATED_EVENT:
            linkTerminatedEventHandler(&pEvent->linkTerminate);
            break;
        case GAP_LINK_PARAM_UPDATE_EVENT:
        	LogPrintf(DEBUG_BLE, "Dev connhandle:%d param update", pEvent->linkUpdate.connectionHandle);
        	LogPrintf(DEBUG_BLE, "connInterval:%d", pEvent->linkUpdate.connInterval);
        	LogPrintf(DEBUG_BLE, "connLatency:%d", pEvent->linkUpdate.connLatency);
        	LogPrintf(DEBUG_BLE, "connTimeout:%d", pEvent->linkUpdate.connTimeout);
            break;
        case GAP_RANDOM_ADDR_CHANGED_EVENT:
            break;
        case GAP_SIGNATURE_UPDATED_EVENT:
            break;
        case GAP_AUTHENTICATION_COMPLETE_EVENT:
            break;
        case GAP_PASSKEY_NEEDED_EVENT:
            break;
        case GAP_SLAVE_REQUESTED_SECURITY_EVENT:
            break;
        case GAP_DEVICE_INFO_EVENT:
            deviceInfoEventHandler(&pEvent->deviceInfo);
            break;
        case GAP_BOND_COMPLETE_EVENT:
            break;
        case GAP_PAIRING_REQ_EVENT:
            break;
        case GAP_DIRECT_DEVICE_INFO_EVENT:
            break;
        case GAP_PHY_UPDATE_EVENT:
            break;
        case GAP_EXT_ADV_DEVICE_INFO_EVENT:
            break;
        case GAP_MAKE_PERIODIC_ADV_DONE_EVENT:
            break;
        case GAP_END_PERIODIC_ADV_DONE_EVENT:
            break;
        case GAP_SYNC_ESTABLISHED_EVENT:
            break;
        case GAP_PERIODIC_ADV_DEVICE_INFO_EVENT:
            break;
        case GAP_SYNC_LOST_EVENT:
            break;
        case GAP_SCAN_REQUEST_EVENT:
            break;
    }
}
/**************************************************
@bref       MTU改变时系统回调
@param
@return
@note
**************************************************/

static void bleCentralHciChangeCallBack(uint16_t connHandle, uint16_t maxTxOctets, uint16_t maxRxOctets)
{
    LogPrintf(DEBUG_BLE, "Handle[%d] MTU change ,TX:%d , RX:%d", connHandle, maxTxOctets, maxRxOctets);
}
/**************************************************
@bref       主动读取rssi回调
@param
@return
@note
**************************************************/

static void bleCentralRssiCallBack(uint16_t connHandle, int8_t newRSSI)
{
    LogPrintf(DEBUG_BLE, "Handle[%d] respon rssi %d dB", connHandle, newRSSI);
    int8_t id = bleDevGetIdByHandle(connHandle);
    if (id >= 0)
    {
		devInfoList[id].rssi = newRSSI;
    }
}
/**************************************************
@bref       主动扫描
@param
@return
@note
**************************************************/

void bleCentralStartDiscover(void)
{
    bStatus_t status;
    status = GAPRole_CentralStartDiscovery(DEVDISC_MODE_ALL, TRUE, FALSE);
    LogPrintf(DEBUG_BLE, "Start discovery,ret=0x%02X", status);
}

/**************************************************
@bref       主动链接从机
@param
    addr        从机地址
    addrType    从机类型
@return
@note
**************************************************/

void bleCentralStartConnect(uint8_t *addr, uint8_t addrType)
{
    char debug[13];
    bStatus_t status;
    byteToHexString(addr, debug, 6);
    debug[12] = 0;
    status = GAPRole_CentralEstablishLink(FALSE, FALSE, addrType, addr);
    LogPrintf(DEBUG_BLE, "Start connect [%s]addrType(%d),ret=0x%02X", debug, addrType, status);
    if (status != SUCCESS)
    {
        LogPrintf(DEBUG_BLE, "Startconnect error:0x%02x", status);
        GAPRole_TerminateLink(INVALID_CONNHANDLE);
    }
    else if (status == bleAlreadyInRequestedMode)
    {
		
    }
}
/**************************************************
@bref       主动断开与从机的链接
@param
    connHandle  从机句柄
@return
@note
**************************************************/

void bleCentralDisconnect(uint16_t connHandle)
{
    bStatus_t status;
    status = GAPRole_TerminateLink(connHandle);
    LogPrintf(DEBUG_BLE, "ble terminate Handle[%X],ret=0x%02X", connHandle, status);
}

/**************************************************
@bref       主动向从机发送数据
@param
    connHandle  从机句柄
    attrHandle  从机属性句柄
    data        数据
    len         长度
@return
    bStatus_t
@note
**************************************************/

uint8 bleCentralSend(uint16_t connHandle, uint16 attrHandle, uint8 *data, uint8 len)
{
    attWriteReq_t req;
    bStatus_t ret;
    req.handle = attrHandle;
    req.cmd = FALSE;
    req.sig = FALSE;
    req.len = len;
    req.pValue = GATT_bm_alloc(connHandle, ATT_WRITE_REQ, req.len, NULL, 0);
    if (req.pValue != NULL)
    {
        tmos_memcpy(req.pValue, data, len);
        ret = GATT_WriteCharValue(connHandle, &req, bleCentralTaskId);
        if (ret != SUCCESS)
        {
            GATT_bm_free((gattMsg_t *)&req, ATT_WRITE_REQ);
        }
    }
    LogPrintf(DEBUG_BLE, "bleCentralSend==>Ret:0x%02X", ret);
    return ret;
}

/**************************************************
@bref       主机读取从机参数
@param
    connHandle  从机句柄
    attrHandle  从机属性句柄
    data        数据
    len         长度
@return
    bStatus_t
@note
**************************************************/

uint8_t bleCentralRead(uint16_t connHandle, uint16_t attrHandle)
{
	attReadReq_t req;
	bStatus_t ret;
    req.handle = attrHandle;
    ret = GATT_ReadCharValue(connHandle, &req, bleCentralTaskId);
    LogPrintf(DEBUG_BLE, "bleCentralRead==>Ret:0x%02X", ret);
    return ret;
}

/**************************************************
@bref       修改MTU
@param
@return
@note
**************************************************/

bStatus_t bleCentralChangeMtu(uint16_t connHandle)
{
	bStatus_t ret;
    attExchangeMTUReq_t req = {
        .clientRxMTU = BLE_BUFF_MAX_LEN - 4,
    };
    ret = GATT_ExchangeMTU(connHandle, &req, bleCentralTaskId);
    LogPrintf(DEBUG_BLE, "bleCentralChangeMtu[%d]==>Ret:0x%02X", connHandle, ret);
    return ret;
}

/*--------------------------------------------------*/


/**************************************************
@bref       链接列表初始化
@param
@return
@note
**************************************************/

static void bleDevConnInit(void)
{
    tmos_memset(&devInfoList, 0, sizeof(devInfoList));
    /* 把调度器用到的状态参数统一初始化 */
    for (uint8_t i = 0; i < DEVICE_MAX_CONNECT_COUNT; i++)
    {
		devInfoList[i].connHandle      = INVALID_CONNHANDLE;
		devInfoList[i].charHandle      = INVALID_CONNHANDLE;
		devInfoList[i].notifyHandle    = INVALID_CONNHANDLE;
		devInfoList[i].findServiceDone = 0;
		devInfoList[i].findCharDone    = 0;
		devInfoList[i].notifyDone      = 0;
		devInfoList[i].connStatus      = 0;
		devInfoList[i].startHandle     = 0;
		devInfoList[i].endHandle       = 0;
    }
}

/**************************************************
@bref       添加新的链接对象到链接列表中
@param
@return
    >0      添加成功，返回所添加的位置
    <0      添加失败
@note
**************************************************/

int8_t bleDevConnAdd(uint8_t *addr, uint8_t addrType)
{
    uint8_t i;

    for (i = 0; i < DEVICE_MAX_CONNECT_COUNT; i++)
    {
        if (devInfoList[i].use == 0)
        {
            tmos_memcpy(devInfoList[i].addr, addr, 6);
            devInfoList[i].addrType        = addrType;
            devInfoList[i].connHandle      = INVALID_CONNHANDLE;
            devInfoList[i].charHandle      = INVALID_CONNHANDLE;
            devInfoList[i].notifyHandle    = INVALID_CONNHANDLE;
            devInfoList[i].findServiceDone = 0;
            devInfoList[i].findCharDone    = 0;
            devInfoList[i].notifyDone      = 0;
            devInfoList[i].connStatus      = 0;
            devInfoList[i].startHandle     = 0;
            devInfoList[i].endHandle       = 0;
            devInfoList[i].use             = 1;
            if (sysparam.relayUpgrade[i] != BLE_UPGRADE_FLAG)
	        {
				bleDevSetConnUuid(i, SERVICE_UUID, CHAR_UUID);
	        }
	        else
	        {
				bleDevSetConnUuid(i, OTA_SERVER_UUID, OTA_CHAR_UUID);
	        }
            //不用添加devInfoList[i].discState = BLE_DISC_STATE_IDLE,由蓝牙状态回调来改写
            LogPrintf(DEBUG_BLE, "bleDevConnAdd(%d)==>ok", i);
            return i;
        }
    }
    return -1;
}

/**************************************************
@bref       添加新的链接对象到指定链接列表中
@param
@return
    >0      添加成功，返回所添加的位置
    <0      添加失败
@note
**************************************************/

int8_t bleDevConnAddIndex(uint8_t index, uint8_t *addr, uint8_t addrType)
{
   	if (devInfoList[index].use)
	{
		LogPrintf(DEBUG_BLE, "bleDevConnAddIndex(%d)==>Occupied", index);
		return -1;
	}
    else
    {
        tmos_memcpy(devInfoList[index].addr, addr, 6);
        devInfoList[index].addrType        = addrType;
        devInfoList[index].connHandle      = INVALID_CONNHANDLE;
        devInfoList[index].charHandle      = INVALID_CONNHANDLE;
        devInfoList[index].notifyHandle    = INVALID_CONNHANDLE;
        devInfoList[index].findServiceDone = 0;
        devInfoList[index].findCharDone    = 0;
        devInfoList[index].notifyDone      = 0;
        devInfoList[index].connStatus      = 0;
        devInfoList[index].startHandle     = 0;
        devInfoList[index].endHandle       = 0;
        devInfoList[index].use             = 1;
        if (sysparam.relayUpgrade[index] != BLE_UPGRADE_FLAG)
        {
			bleDevSetConnUuid(index, SERVICE_UUID, CHAR_UUID);
        }
        else
        {
			bleDevSetConnUuid(index, OTA_SERVER_UUID, OTA_CHAR_UUID);
        }
        //不用添加devInfoList[i].discState = BLE_DISC_STATE_IDLE,由蓝牙状态回调来改写
        LogPrintf(DEBUG_BLE, "bleDevConnAddIndex(%d)==>ok", index);
        return index;
    }
}


/**************************************************
@bref       删除链接列表中的对象
@param
@return
    >0      删除成功，返回所添加的位置
    <0      删除失败
@note
**************************************************/

int8_t bleDevConnDel(uint8_t *addr)
{
    uint8_t i;
    for (i = 0; i < DEVICE_MAX_CONNECT_COUNT; i++)
    {
        if (devInfoList[i].use && tmos_memcmp(addr, devInfoList[i].addr, 6) == TRUE)
        {
            devInfoList[i].use = 0;
            devInfoList[i].findServiceUuid = 0;
            devInfoList[i].findCharUuid    = 0;
            if (devInfoList[i].connHandle != INVALID_CONNHANDLE)
            {
                bleCentralDisconnect(devInfoList[i].connHandle);
            }
            return i;
        }
    }
    return -1;
}

/**************************************************
@bref       删除链接列表中的全部对象
@param
@return
@note
**************************************************/


void bleDevConnDelAll(void)
{
    uint8_t i;
    for (i = 0; i < DEVICE_MAX_CONNECT_COUNT; i++)
    {
        if (devInfoList[i].use)
        {
            devInfoList[i].use = 0;
            devInfoList[i].findServiceUuid = 0;
            devInfoList[i].findCharUuid    = 0;
        }
    }
}

/**************************************************
@bref       设置连接的uuid
@param
    serUuid   服务uuid
    charUuid  特征uuid
@return
@note
**************************************************/

static void bleDevSetConnUuid(uint8_t ind, uint16_t serUuid, uint16_t charUuid)
{
	if (devInfoList[ind].use)
	{
		devInfoList[ind].findServiceUuid = serUuid;
      	devInfoList[ind].findCharUuid    = charUuid;
      	LogPrintf(DEBUG_BLE, "bleDevSetConnUuid==>serUuid:0x%04x charUuid:0x%04x",
      						  devInfoList[ind].findServiceUuid,
      						  devInfoList[ind].findCharUuid);
	}
	else
	{
		LogPrintf(DEBUG_BLE, "bleDevSetConnUuid==>error");
	}
}

/**************************************************
@bref       链接成功，查找对象并赋值句柄
@param
    addr        对象的mac地址
    connHandle  赋值对象的句柄
@return
@note
**************************************************/

static void bleDevConnSuccess(uint8_t *addr, uint16_t connHandle)
{
    uint8_t i;

    for (i = 0; i < DEVICE_MAX_CONNECT_COUNT; i++)
    {
        if (devInfoList[i].use && devInfoList[i].connStatus == 0)
        {
            if (tmos_memcmp(devInfoList[i].addr, addr, 6) == TRUE)
            {
                devInfoList[i].connHandle      = connHandle;
                devInfoList[i].connStatus      = 1;
                devInfoList[i].notifyHandle    = INVALID_CONNHANDLE;
                devInfoList[i].charHandle      = INVALID_CONNHANDLE;
                devInfoList[i].discState       = BLE_DISC_STATE_CONN;
                devInfoList[i].findServiceDone = 0;
                devInfoList[i].findCharDone    = 0;
                devInfoList[i].notifyDone      = 0;
                ble_conn_now_id				   = i;
                LogPrintf(DEBUG_BLE, "Get device conn handle [%d]", connHandle);
                if (i == 0)
                {
	                /* 更改MTU */
	                tmos_start_task(bleCentralTaskId, BLE_TASK_UPDATE_MTU_EVENT, MS1_TO_SYSTEM_TIME(1600));
	                /* 更新参数 */
	                tmos_start_task(bleCentralTaskId, BLE_TASK_UPDATE_PARAM_EVENT, MS1_TO_SYSTEM_TIME(1800));
                }
                else if (i == 1)
                {
	                /* 更改MTU */
	                tmos_start_task(bleCentralTaskId, BLE_TASK_UPDATE_MTU2_EVENT, MS1_TO_SYSTEM_TIME(1600));
	                /* 更新参数 */
	                tmos_start_task(bleCentralTaskId, BLE_TASK_UPDATE_PARAM2_EVENT, MS1_TO_SYSTEM_TIME(1800));
                }
                /* 查找服务 */
                tmos_start_task(bleCentralTaskId, BLE_TASK_SVC_DISCOVERY_EVENT, MS1_TO_SYSTEM_TIME(800));
                bleDevReadAllRssi();
                return;
            }
        }
        /* 如果use=0还进入这里，说明在连上之前就执行了断开的指令，所以再次断开即可 */
        if (devInfoList[i].use == 0)
        {
			
        }
    }
}

/**************************************************
@bref       链接被断开时调用
@param
    connHandle  对象的句柄
@return
@note
**************************************************/

static void bleDevDisconnect(uint16_t connHandle)
{
    uint8_t i;
    char debug[20];
    for (i = 0; i < DEVICE_MAX_CONNECT_COUNT; i++)
    {
        if (devInfoList[i].connHandle == connHandle)
        {
            devInfoList[i].connHandle      = INVALID_CONNHANDLE;
            devInfoList[i].connStatus      = 0;
            devInfoList[i].notifyHandle    = INVALID_CONNHANDLE;
            devInfoList[i].charHandle      = INVALID_CONNHANDLE;
            devInfoList[i].findServiceDone = 0;
            devInfoList[i].findCharDone    = 0;
            devInfoList[i].notifyDone      = 0;
            devInfoList[i].rssi			   = 0;
            devInfoList[i].otaStatus	   = 0;
            devInfoList[i].discState       = BLE_DISC_STATE_IDLE;
            byteToHexString(devInfoList[i].addr, debug, 6);
            debug[12] = 0;
            LogPrintf(DEBUG_BLE, "bleDevDisconnect==>Device (%d)[%s] disconnect,Handle[%d]", i, debug, connHandle);
            //bleSchduleChangeFsm(BLE_SCHEDULE_IDLE);		//不能添加这个，因为可能存在正在连第二个链路而主机又想断开第一条链路

            /* 停止循环执行的tmos任务 */
            tmos_stop_task(bleCentralTaskId, BLE_TASK_OTA_WRITE_EVENT);
            tmos_stop_task(bleCentralTaskId, BLE_TASK_OTA_READ_EVENT);
            /* 表示已通过指令解绑 */
            if (devInfoList[i].use == 0)
            {
				
            }
            return;
        }
    }
}

/**************************************************
@bref       获取到uuid的句柄
@param
    connHandle  对象的句柄
    handle      uuid属性句柄
@return
@note
**************************************************/

static void bleDevSetCharHandle(uint16_t connHandle, uint16_t handle)
{
    uint8_t i;
    for (i = 0; i < DEVICE_MAX_CONNECT_COUNT; i++)
    {
        if (devInfoList[i].use && devInfoList[i].connHandle == connHandle 
       	 					   && devInfoList[i].discState  == BLE_DISC_STATE_CHAR)
        {
            devInfoList[i].charHandle   = handle;
            devInfoList[i].findCharDone = 1;
            LogPrintf(DEBUG_BLE, "Dev(%d)[%d]set charHandle [0x%04X]", i, connHandle, handle);
            return;
        }
    }
}

/**************************************************
@bref       获取到notify的句柄
@param
    connHandle  对象的句柄
    handle      notify属性句柄
@return
@note
**************************************************/

static void bleDevSetNotifyHandle(uint16_t connHandle, uint16_t handle)
{
    uint8_t i;
    for (i = 0; i < DEVICE_MAX_CONNECT_COUNT; i++)
    {
        if (devInfoList[i].use && devInfoList[i].connHandle   == connHandle
        					   && devInfoList[i].discState    == BLE_DISC_STATE_CCCD
        					   && devInfoList[i].notifyHandle == INVALID_CONNHANDLE)
        {
            devInfoList[i].notifyHandle = handle;
            LogPrintf(DEBUG_BLE, "Dev(%d)[%d]set Notify Handle:[0x%04X]", i, connHandle, handle);
            return;
        }
    }
}

/**************************************************
@bref       获取到services的句柄区间
@param
    connHandle  对象的句柄
    findS       起始句柄
    findE       结束句柄
@return
@note
**************************************************/

static void bleDevSetServiceHandle(uint16_t connHandle, uint16_t findS, uint16_t findE)
{
    uint8_t i;
    for (i = 0; i < DEVICE_MAX_CONNECT_COUNT; i++)
    {
        if (devInfoList[i].use && devInfoList[i].connHandle == connHandle 
        					   && devInfoList[i].discState  == BLE_DISC_STATE_SVC)
        {
            devInfoList[i].startHandle     = findS;
            devInfoList[i].endHandle       = findE;
            devInfoList[i].findServiceDone = 1;
            LogPrintf(DEBUG_BLE, "Dev(%d)[%d]Set service handle [0x%04X~0x%04X]", i, connHandle, findS, findE);
            return;
        }
    }
}

/**************************************************
@bref       查找所有服务
@param
@return
@note
**************************************************/

static void bleDevDiscoverAllServices(void)
{
    uint8_t i;
    bStatus_t status;
    for (i = 0; i < DEVICE_MAX_CONNECT_COUNT; i++)
    {
        if (devInfoList[i].use && 
        	devInfoList[i].connHandle != INVALID_CONNHANDLE && 
        	devInfoList[i].findServiceDone == 0)
        {
            status = GATT_DiscAllPrimaryServices(devInfoList[i].connHandle, bleCentralTaskId);
            LogPrintf(DEBUG_BLE, "Discover Handle[%d] all services,ret=0x%02X", devInfoList[i].connHandle, status);
            return;
        }
    }
}
/**************************************************
@bref       根据UUID查找服务
@param
@return
@note
**************************************************/
static void bleDevDiscoverServByUuid(void)
{
	uint8_t i;
    uint8_t uuid[2];
    uint16_t serUuid;
    bStatus_t status;
    for (i = 0; i < DEVICE_MAX_CONNECT_COUNT; i++)
    {
        if (devInfoList[i].use && devInfoList[i].connHandle      != INVALID_CONNHANDLE
        					   && devInfoList[i].findServiceDone == 0
        					   && devInfoList[i].discState       == BLE_DISC_STATE_CONN
        					   && devInfoList[i].findServiceUuid != 0)
        {
        	serUuid = devInfoList[i].findServiceUuid;
			devInfoList[i].discState = BLE_DISC_STATE_SVC;
			uuid[0] = LO_UINT16(serUuid);
			uuid[1] = HI_UINT16(serUuid);
			status = GATT_DiscPrimaryServiceByUUID(devInfoList[i].connHandle, uuid, 2, bleCentralTaskId);
			LogPrintf(DEBUG_BLE, "Dev(%d)Discover Handle[%d]services by uuid[0x%04X],ret=0x%02X", 
									i, 
									devInfoList[i].connHandle, 
									devInfoList[i].findServiceUuid, 
									status);
            return;
        }
    }
    LogPrintf(DEBUG_ALL, "bleDevDiscoverServByUuid==>Error");
}

/**************************************************
@bref       根据UUID找特征
@param
@return
@note
**************************************************/
static void bleDevDiscoverCharByUuid(void)
{
	uint8_t i;
	uint16_t charUuid;
	attReadByTypeReq_t req;
	bStatus_t status;
	for (i = 0; i < DEVICE_MAX_CONNECT_COUNT; i++)
	{
		if (devInfoList[i].use && devInfoList[i].startHandle     != 0
							   && devInfoList[i].findServiceDone == 1
							   && devInfoList[i].discState       == BLE_DISC_STATE_SVC
							   && devInfoList[i].findCharUuid    != 0)
		{
			devInfoList[i].discState = BLE_DISC_STATE_CHAR;
			charUuid = devInfoList[i].findCharUuid;
			req.startHandle = devInfoList[i].startHandle;
            req.endHandle   = devInfoList[i].endHandle;
            req.type.len    = ATT_BT_UUID_SIZE;
        	req.type.uuid[0] = LO_UINT16(charUuid);
            req.type.uuid[1] = HI_UINT16(charUuid);
            status = GATT_ReadUsingCharUUID(devInfoList[i].connHandle, &req, bleCentralTaskId);
        	LogPrintf(DEBUG_BLE, "Dev(%d)Discover handle[%d]chars by uuid[0x%04X],ret=0x%02X", 
        				          i, 
        				          devInfoList[i].connHandle, 
        				          devInfoList[i].findCharUuid, 
        				          status);
			return;
		}
	}
	LogPrintf(DEBUG_ALL, "bleDevDiscoverCharByUuid==>Error");
}


/**************************************************
@bref       查找所有特征
@param
@return
@note
**************************************************/

static void bleDevDiscoverAllChars(uint16_t connHandle)
{
    uint8_t i;
    bStatus_t status;
    for (i = 0; i < DEVICE_MAX_CONNECT_COUNT; i++)
    {
        if (devInfoList[i].use && 
            devInfoList[i].connHandle      == connHandle && 
            devInfoList[i].findServiceDone == 1)
        {
            status = GATT_DiscAllChars(devInfoList[i].connHandle, 
            						   devInfoList[i].startHandle, 
            						   devInfoList[i].endHandle,
                                       bleCentralTaskId);
            LogPrintf(DEBUG_BLE, "Discover handle[%d] all chars,ret=0x%02X", devInfoList[i].connHandle, status);
            return;
        }
    }
}

/**************************************************
@bref       查找所有notify
@param
@return
@note
**************************************************/

static void bleDevDiscoverNotify(uint16_t connHandle)
{
    uint8_t i;
    bStatus_t status;
    attReadByTypeReq_t req;
    for (i = 0; i < DEVICE_MAX_CONNECT_COUNT; i++)
    {
        if (devInfoList[i].use && devInfoList[i].connHandle   == connHandle
                			   && devInfoList[i].findCharDone == 1)
        {
            tmos_memset(&req, 0, sizeof(attReadByTypeReq_t));
			req.startHandle  = devInfoList[i].startHandle;
			req.endHandle    = devInfoList[i].endHandle;
			req.type.len     = ATT_BT_UUID_SIZE;
			req.type.uuid[0] = LO_UINT16(GATT_CLIENT_CHAR_CFG_UUID);
			req.type.uuid[1] = HI_UINT16(GATT_CLIENT_CHAR_CFG_UUID);
			devInfoList[i].discState = BLE_DISC_STATE_CCCD;
			status = GATT_ReadUsingCharUUID(devInfoList[i].connHandle, &req, bleCentralTaskId);
            LogPrintf(DEBUG_BLE, "Dec(%d)[%d]Discover notify,ret=0x%02X", 
            				      i, devInfoList[i].connHandle, status);
            return;
        }
    }
	LogPrintf(DEBUG_BLE, "bleDevDiscoverNotify==>Error");
}

/**************************************************
@bref       ota蓝牙完成连接
@param
@return
@note
连接ota蓝牙的时候不需要找Notify
**************************************************/

static void bleDevConnectComplete(uint16_t connHandle)
{
	uint8_t i;
	for (i = 0; i < DEVICE_MAX_CONNECT_COUNT; i++)
	{
		if (devInfoList[i].use && devInfoList[i].connHandle   == connHandle
							   && devInfoList[i].findCharDone == 1)
		{
			devInfoList[i].discState  = BLE_DISC_STATE_COMP;
			devInfoList[i].otaStatus  = 1;
			//devInfoList[i].notifyDone = 1;				//不置1就不会发送app指令
			bleSchduleChangeFsm(BLE_SCHEDULE_DONE);
			ble_ota_tick = 0;
			LogPrintf(DEBUG_BLE, "Dev(%d)handle[%d] skip notify and connect finish", i, connHandle);
			/* 如果OTA设备已经处于IAP状态了，那么normal蓝牙的ota指令可能会一直在发送，这里需要再次清除normal蓝牙的发送事件 */
			bleRelayClearReq(i, BLE_EVENT_ALL);
		}
	}
}

/**************************************************
@bref       使能notify
@param
@return
@note
**************************************************/

static uint8_t bleDevEnableNotify(void)
{
    uint8_t i;
    uint8_t data[2];
    bStatus_t status;
    for (i = 0; i < DEVICE_MAX_CONNECT_COUNT; i++)
    {
        if (devInfoList[i].use && devInfoList[i].notifyHandle != INVALID_CONNHANDLE 
        					   && devInfoList[i].notifyDone   == 0
        					   && devInfoList[i].connHandle   != INVALID_CONNHANDLE
        					   && devInfoList[i].discState    == BLE_DISC_STATE_CCCD)
        {
            data[0] = 0x01;
            data[1] = 0x00;
            LogPrintf(DEBUG_BLE, "Dev(%d)Handle[%d] try to notify", i, devInfoList[i].connHandle);
            status = bleCentralSend(devInfoList[i].connHandle, devInfoList[i].notifyHandle, data, 2);
            if (status == SUCCESS)
            {
                devInfoList[i].notifyDone = 1;
                devInfoList[i].discState  = BLE_DISC_STATE_COMP;
            }
            return status;
        }
    }
    return 0;
}


/**************************************************
@bref       终止链路进程
@param
@return
@note  
不修改useflag及其他的标志位,且每秒中断一个
**************************************************/

void bleDevTerminateProcess(void)
{
    uint8_t i;
    bStatus_t status;
    for (i = 0; i < DEVICE_MAX_CONNECT_COUNT; i++)
    {
        if (((devInfoList[i].use && devInfoList[i].connPermit == 0) ||
	          devInfoList[i].use == 0) &&
	          devInfoList[i].connHandle != INVALID_CONNHANDLE) 
        {
            status = GAPRole_TerminateLink(devInfoList[i].connHandle);
            LogPrintf(DEBUG_BLE, "bleDevTerminateProcess==>Dev(%d)Handle[%d] Terminate,status:0x%02x", i, devInfoList[i].connHandle, status);
            return;
        }
    }
}

/**************************************************
@bref       蓝牙添加
@param
@return
@note
**************************************************/

static void bleDevInsertTask(void)
{
	/* 如果有要升级的设备直接添加该设备，另一台不管 */
	if (getBleOtaStatus() >= 0)
	{
		if (devInfoList[getBleOtaStatus()].use == 0)
		{
			/* 这个状态标志很关键，必须等到蓝牙协议栈断连回调产生才能把设备添加进来 */
			if (devInfoList[getBleOtaStatus()].connHandle == INVALID_CONNHANDLE)
			{
				bleRelayInsertIndex(getBleOtaStatus(), sysparam.bleConnMac[getBleOtaStatus()], 0);
				fileRxInfo *info = getOtaInfo();
				if (info->totalsize == 0)
				{
					otaRxInfoInit(1, sysparam.file.fileName, sysparam.file.fileSize);
				}
			}
		}
		return;
	}
	for (uint8_t i = 0; i < sysparam.bleMacCnt; i++)
	{
		if (devInfoList[i].use == 0)
		{
			/* 这个状态标志很关键，必须等到蓝牙协议栈断连回调产生才能把设备添加进来 */
			if (devInfoList[i].connHandle == INVALID_CONNHANDLE)
			{
				LogPrintf(DEBUG_BLE, "蓝牙添加链接列表");
				bleRelayInsertIndex(i, sysparam.bleConnMac[i], 0);
			}
		}
	}
}

/**************************************************
@bref       读取信号值
@param
@return
@note
**************************************************/

void bleDevReadAllRssi(void)
{
	tmos_start_task(bleCentralTaskId, BLE_TASK_READ_RSSI_EVENT, MS1_TO_SYSTEM_TIME(1500));
}

/**************************************************
@bref       通过地址查找对象
@param
@return
@note
**************************************************/

deviceConnInfo_s *bleDevGetInfo(uint8_t *addr)
{
    uint8_t i;
    for (i = 0; i < DEVICE_MAX_CONNECT_COUNT; i++)
    {
        if (devInfoList[i].use && tmos_memcmp(devInfoList[i].addr, addr, 6) == TRUE)
        {
            return &devInfoList[i];
        }
    }
    return NULL;
}

/**************************************************
@bref       通过id查找对象
@param
@return
@note
**************************************************/

deviceConnInfo_s *bleDevGetInfoById(uint8_t id)
{
	if (devInfoList[id].use)
		return &devInfoList[id];
	return NULL;
}


/**************************************************
@bref       获取绑定继电器个数
@param
@return
@note
**************************************************/
uint8_t bleDevGetCnt(void)
{
	uint8_t cnt = 0, i;
	for (i = 0; i < DEVICE_MAX_CONNECT_COUNT; i++)
	{
		if (devInfoList[i].use) {
			cnt++;
		}
	}
	return cnt;
}

/**************************************************
@bref       通过链接句柄，获取对应mac地址
@param
@return
@note
**************************************************/

uint8_t *bleDevGetAddrByHandle(uint16_t connHandle)
{
    uint8_t i;
    for (i = 0; i < DEVICE_MAX_CONNECT_COUNT; i++)
    {
        if (devInfoList[i].use && devInfoList[i].connHandle == connHandle)
        {
            return devInfoList[i].addr;
        }
    }
    return NULL;
}

/**************************************************
@bref       允许指定ID蓝牙进行连接
@param
@return
@note
**************************************************/

void bleDevSetPermit(uint8 id, uint8_t enabled)
{
	if (enabled)
	{
		if (devInfoList[id].use)
		{
			devInfoList[id].connPermit = 1;
		}
	}
	else
	{
		if (devInfoList[id].use)
		{
			devInfoList[id].connPermit = 0;
		}
	}
	//LogPrintf(DEBUG_BLE, "bleDevSetPermit==>id:%d %s", id, enabled ? "enable" : "disable");
}

/**************************************************
@bref       全部蓝牙禁止连接
@param
@return
@note
**************************************************/

void bleDevAllPermitDisable(void)
{
	for (uint8_t i = 0; i < DEVICE_MAX_CONNECT_COUNT; i++)
	{
		devInfoList[i].connPermit = 0;
	}
}

/**************************************************
@bref       蓝牙连接时间设置
@param
@return
@note
**************************************************/

void bleDevSetConnTick(uint8_t id, uint16_t tick)
{
	devInfoList[id].connTick = tick;
	LogPrintf(DEBUG_BLE, "bleDevSetConnTick==>%d", devInfoList[id].connTick);
}

/**************************************************
@bref       获取指定蓝牙是否能连接
@param
@return
@note
**************************************************/

uint8_t bleDevGetPermit(uint8_t id)
{
	if (devInfoList[id].use)
		return devInfoList[id].connPermit;
	return 0;
}

/**************************************************
@bref       通过链接句柄找到设备ID
@param
@return
@note
**************************************************/

int bleDevGetIdByHandle(uint16_t connHandle)
{
	uint8_t i;
	for (i = 0; i < DEVICE_MAX_CONNECT_COUNT; i++)
	{
		if (devInfoList[i].use && devInfoList[i].connHandle == connHandle)
		{
			return i;
		}
	}
	return  -1;
}

/**************************************************
@bref       通过指针获取ota状态
@param
@return
@note
**************************************************/

int bleDevGetOtaStatusByIndex(uint8_t index)
{
	return devInfoList[index].otaStatus;
}

/**************************************************
@bref       蓝牙链接管理调度器状态切换
@param
@return
@note
**************************************************/

static void bleSchduleChangeFsm(bleFsm nfsm)
{
    bleSchedule.fsm     = nfsm;
    bleSchedule.runTick = 0;
    LogPrintf(DEBUG_BLE, "bleSchduleChangeFsm==>%d", nfsm);
}

/**************************************************
@bref       蓝牙链接管理调度器
@param
@return
@note
**************************************************/

static void bleScheduleTask(void)
{
    static uint8_t ind = 0;
	
	bleConnPermissonManger();
	bleDevTerminateProcess();
	bleDevInsertTask();
    switch (bleSchedule.fsm)
    {
    	case BLE_SCHEDULE_IDLE:
			ind = ind % DEVICE_MAX_CONNECT_COUNT;
			//LogPrintf(DEBUG_BLE, "ind:%d USE:%d,con:%d,dis:%d,conpermisson:%d", ind, devInfoList[ind].use, devInfoList[ind].connHandle,devInfoList[ind].discState, devInfoList[ind].connPermit);
			//查找是否有未链接的设备，需要进行链接
			for (; ind < DEVICE_MAX_CONNECT_COUNT; ind++)
			{
				if (devInfoList[ind].use 							   && 
					devInfoList[ind].connHandle == INVALID_CONNHANDLE  && 
					devInfoList[ind].discState  == BLE_DISC_STATE_IDLE &&
					devInfoList[ind].connPermit == 1)
				{
					bleCentralStartConnect(devInfoList[ind].addr, devInfoList[ind].addrType);
					bleSchduleChangeFsm(BLE_SCHEDULE_WAIT);
					break;
				}
			}
			break;
        case BLE_SCHEDULE_WAIT:
            if (bleSchedule.runTick >= 15)
            {
                //链接超时
                LogPrintf(DEBUG_BLE, "bleSchedule(%d)[%d]==>timeout!!!", ind, devInfoList[ind].connHandle);
                bleCentralDisconnect(devInfoList[ind].connHandle);
                bleSchduleChangeFsm(BLE_SCHEDULE_DONE);
                if (sysinfo.logLevel == 4)
                {
                    LogMessage(DEBUG_FACTORY, "+FMPC:BLE CONNECT FAIL");
                }
            }
            else
            {
                break;
            }
        case BLE_SCHEDULE_DONE:
            ind++;
            bleSchduleChangeFsm(BLE_SCHEDULE_IDLE);
            break;
        default:
            bleSchduleChangeFsm(BLE_SCHEDULE_IDLE);
            break;
    }
    bleSchedule.runTick++;
}

/**************************************************
@bref       蓝牙连接方式切换
@param
@return
@note
**************************************************/

void bleConnTypeChange(uint8_t type, uint8_t index)
{
	if (type == 0)
	{
		for (uint8_t i = 0; i < DEVICE_MAX_CONNECT_COUNT; i++)
		{
		    sysparam.relayUpgrade[i] = 0;
		}
	}
	else
	{
	    sysparam.relayUpgrade[index] = BLE_UPGRADE_FLAG;
	}
	LogPrintf(DEBUG_BLE, "bleConnTypeChange==>%s", type ? "normal" : "ota");
	paramSaveAll();
}

/**************************************************
@bref       ota接收数据初始化
@param
@return
@note
**************************************************/

void otaRxInfoInit(uint8_t onoff, uint8_t *file, uint32_t total)
{
	if (onoff)
	{
		sysinfo.bleUpgradeOnoff = 1;
		tmos_memset(&otaRxInfo, 0, sizeof(fileRxInfo));
		if (file != NULL)
		{
			strncpy(otaRxInfo.fileName, file, 20);
			otaRxInfo.fileName[strlen(file)] = 0;
		}
		otaRxInfo.readLen   = 240;
		otaRxInfo.totalsize = total;
		bleOtaFsmChange(BLE_OTA_FSM_IDLE);
		LogPrintf(DEBUG_BLE, "otaRxInfoInit==>Startup, file:%s, size:%d", file, total);
	}
	else
	{
		sysinfo.bleUpgradeOnoff = 0;
		tmos_memset(&otaRxInfo, 0, sizeof(fileRxInfo));
		bleOtaFsmChange(BLE_OTA_FSM_IDLE);
		LogPrintf(DEBUG_BLE, "otaRxInfoInit==>Close, size:%d", file, total);
	}
}


/**************************************************
@bref       获取ota信息
@param
@return
@note
**************************************************/

fileRxInfo *getOtaInfo(void)
{
	return &otaRxInfo;
}


/**************************************************
@bref		执行升级
@param
	upgradeResult  执行类型
	offset		   当前地址
	size 		   单包长度
@return
@note
**************************************************/

void upgradeResultProcess(uint8_t upgradeResult, uint32_t offset, uint32_t size)
{
	/* 编程部分 */
    if (upgradeResult == 0)
    {
        otaRxInfo.rxoffset = offset + size;
        LogPrintf(DEBUG_BLE, "upgradeResultProcess==>rxoffset:%d", otaRxInfo.rxoffset);
        LogPrintf(DEBUG_BLE, ">>>>>>>>>> rx progress %.1f%% <<<<<<<<<<",
                  ((float)otaRxInfo.rxoffset / otaRxInfo.totalsize) * 100);
        if (otaRxInfo.rxoffset == otaRxInfo.totalsize)
        {
        	/* 开始校验 */
            bleOtaFsmChange(BLE_OTA_FSM_VERI_READ);
            LogPrintf(DEBUG_BLE, "Program ok!!!!!!!!!");
        }
        else if (otaRxInfo.rxoffset > otaRxInfo.totalsize)
        {
            LogMessage(DEBUG_ALL, "Recevie complete ,but total size is different,retry again\n");
            otaRxInfo.totalsize = 0;
            bleOtaFsmChange(BLE_OTA_FSM_INFO);
        }
        else
        {
			bleOtaFsmChange(BLE_OTA_FSM_READ);
        }
    }
    /* 校验部分 */
    else if (upgradeResult == 1)
    {
		otaRxInfo.veroffset = offset + size;
        LogPrintf(DEBUG_BLE, "upgradeResultProcess==>veroffset:%d", otaRxInfo.veroffset);
        LogPrintf(DEBUG_BLE, ">>>>>>>>>> ver progress %.1f%% <<<<<<<<<<",
                  ((float)otaRxInfo.veroffset / otaRxInfo.totalsize) * 100);
        if (otaRxInfo.veroffset == otaRxInfo.totalsize)
        {
        	/* 开始校验 */
            bleOtaFsmChange(BLE_OTA_FSM_END);
            LogPrintf(DEBUG_BLE, "Verify ok!!!!!!!!!");
        }
        else if (otaRxInfo.veroffset > otaRxInfo.totalsize)
        {
            LogMessage(DEBUG_ALL, "Recevie complete ,but total size is different,retry again\n");
            otaRxInfo.totalsize = 0;
            bleOtaFsmChange(BLE_OTA_FSM_INFO);
        }
        else
        {
			bleOtaFsmChange(BLE_OTA_FSM_VERI_READ);
        }
    }
    else
    {
        LogPrintf(DEBUG_ALL, "Writing firmware error at 0x%X\n", otaRxInfo.rxoffset);
        //upgradeServerChangeFsm(NETWORK_DOWNLOAD_ERROR);
    }

}


/**************************************************
@bref       
@param
@return
@note
**************************************************/

int8_t getBleOtaStatus(void)
{
	for (uint8_t i = 0; i < DEVICE_MAX_CONNECT_COUNT; i++)
	{
		if (sysparam.relayUpgrade[i] == BLE_UPGRADE_FLAG)
			return i;
	}
	return -1;
}

/**************************************************
@bref       获取蓝牙ota当前状态
@param
@return
@note
**************************************************/

ble_ota_fsm_e getBleOtaFsm(void)
{
	return ble_ota_fsm;
}

/**************************************************
@bref       切换蓝牙ota状态
@param
@return
@note
**************************************************/

void bleOtaFsmChange(ble_ota_fsm_e fsm)
{
	ble_ota_fsm  = fsm;
	ble_ota_tick = 0;
	LogPrintf(DEBUG_BLE, "bleOtaFsmChange==>%d", ble_ota_fsm);
}


/**************************************************
@bref       蓝牙编程数据封装
@param
@return
@note
**************************************************/

void bleOtaFilePackage(ota_package_t file)
{
	if (ble_ota_fsm != BLE_OTA_FSM_READ && ble_ota_fsm != BLE_OTA_FSM_PROM)
	{
		LogPrintf(DEBUG_UP, "bleOtaFilePackage==>OTA FSM ERROR");
		return;
	}
	otaRxInfo.len      = file.len;	    //此包文件的长度
	uint16_t prom_addr = (IMAGE_A_START_ADD + otaRxInfo.rxoffset) / 16;
	iap_send_buff.program.cmd     = CMD_IAP_PROM;
	iap_send_buff.program.len     = file.len;
	iap_send_buff.program.addr[0] = (prom_addr >> 8) & 0xff;
	iap_send_buff.program.addr[1] =  prom_addr & 0xff;
	tmos_memcpy(iap_send_buff.program.buf, file.data, iap_send_buff.program.len);
	bleOtaFsmChange(BLE_OTA_FSM_PROM);
}

/**************************************************
@bref       蓝牙校验数据封装
@param
@return
@note
**************************************************/

void bleVerifyFilePackage(ota_package_t file)
{
	if (ble_ota_fsm != BLE_OTA_FSM_VERI_READ && ble_ota_fsm != BLE_OTA_FSM_VERI)
	{
		LogPrintf(DEBUG_UP, "bleVerifyFilePackage==>OTA FSM ERROR");
		return;
	}
	otaRxInfo.len      = file.len;	    //此包文件的长度
	uint16_t prom_addr = (IMAGE_A_START_ADD + otaRxInfo.veroffset) / 16;
	iap_send_buff.verify.cmd     = CMD_IAP_VERIFY;
	iap_send_buff.verify.len     = file.len;
	iap_send_buff.verify.addr[0] = (prom_addr >> 8) & 0xff;
	iap_send_buff.verify.addr[1] =  prom_addr & 0xff;
	tmos_memcpy(iap_send_buff.verify.buf, file.data, iap_send_buff.verify.len);
	bleOtaFsmChange(BLE_OTA_FSM_VERI);
}


/**************************************************
@bref       蓝牙Ota发送协议
@param
	connhandle 连接句柄
	charhandle 特征句柄
@return
@note
**************************************************/

uint8_t bleOtaSendProtocol(uint16_t connHandle, uint16_t charHandle)
{
	OTA_IAP_CMD_t iap_send_data;
	uint8_t ret = bleIncorrectMode;
	char message[255] = { 0 };
	uint8_t size_len;
	uint8_t send_len;
	uint8_t block_num   = 0;
	uint16_t erase_addr = 0;
	tmos_memset(&iap_send_data, 0, sizeof(OTA_IAP_CMD_t));
	switch (ble_ota_fsm)
	{
		/*
			84 12 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
		*/
		case BLE_OTA_FSM_INFO:
			iap_send_data.info.cmd = CMD_IAP_INFO;
			iap_send_data.info.len = 12;
			send_len 			   = 12;
			break;
		case BLE_OTA_FSM_PROM:
			iap_send_data.program.cmd     = CMD_IAP_PROM;
			iap_send_data.program.len     = iap_send_buff.program.len;
			iap_send_data.program.addr[0] = iap_send_buff.program.addr[1];
			iap_send_data.program.addr[1] = iap_send_buff.program.addr[0];
			tmos_memcpy(iap_send_data.program.buf, iap_send_buff.program.buf, iap_send_data.program.len);
			send_len					  = iap_send_buff.program.len + 4;
			break;
		/*
			81 00 00 01 0b 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
		*/
		//这里务必确保擦除地址正确
		case BLE_OTA_FSM_EASER:
			/* 擦除起始地址写死,目标设备会乘16 */
			erase_addr = IMAGE_A_START_ADD / 16;
			iap_send_buff.erase.addr[0] = (erase_addr >> 8) & 0xff;
			iap_send_buff.erase.addr[1] =  erase_addr & 0xff;
			LogPrintf(DEBUG_BLE, "IMAGEA ADDR:0x%02X%02X", iap_send_buff.erase.addr[0], 
														   iap_send_buff.erase.addr[1]);
			block_num = otaRxInfo.totalsize / EEPROM_BLOCK_SIZE;
			if (otaRxInfo.totalsize % EEPROM_BLOCK_SIZE != 0)
			{
				block_num++;
			}
			iap_send_buff.erase.block_num[0] = 0;
			iap_send_buff.erase.block_num[1] = block_num;
			LogPrintf(DEBUG_BLE, "ERASE BLOCK NUM:%d%d", iap_send_buff.erase.block_num[1],
														 iap_send_buff.erase.block_num[0]);
			iap_send_data.erase.cmd     	 = CMD_IAP_ERASE;
			iap_send_data.erase.len     	 = 0;
			iap_send_data.erase.addr[0] 	 = iap_send_buff.erase.addr[1];
			iap_send_data.erase.addr[1] 	 = iap_send_buff.erase.addr[0];
			iap_send_data.erase.block_num[0] = iap_send_buff.erase.block_num[1];
			iap_send_data.erase.block_num[1] = iap_send_buff.erase.block_num[0];
			send_len 						 = 20;
			if (otaRxInfo.totalsize >= 102400)
			{
				tmos_stop_task(bleCentralTaskId, BLE_TASK_OTA_WRITE_EVENT);
				return 0;
			}
			break;
		case BLE_OTA_FSM_VERI:
			iap_send_data.verify.cmd     = CMD_IAP_VERIFY;
			iap_send_data.verify.len     = iap_send_buff.verify.len;
			iap_send_data.verify.addr[0] = iap_send_buff.verify.addr[1];
			iap_send_data.verify.addr[1] = iap_send_buff.verify.addr[0];
			tmos_memcpy(iap_send_data.verify.buf, iap_send_buff.verify.buf, iap_send_data.verify.len);
			send_len					  = iap_send_buff.verify.len + 4;
			break;
		case BLE_OTA_FSM_END:
			iap_send_data.end.cmd	= CMD_IAP_END;
			iap_send_data.end.len   = 12;
			send_len				= 12;
			break;
		default:
			iap_send_data.info.cmd = CMD_IAP_ERR;
			send_len               = 0;
			LogPrintf(DEBUG_BLE, "send error");
			break;
	}
	size_len = send_len > 127 ? 127 : send_len;
	byteToHexString((uint8_t *)iap_send_data.other.buf, (uint8_t *)message, size_len);
	message[size_len * 2] = 0;
	LogPrintf(DEBUG_BLE, "ble send [%d]:%s", size_len, message);
	if (bleCentralSend(connHandle, charHandle, iap_send_data.other.buf, send_len) == SUCCESS)
	{
		tmos_stop_task(bleCentralTaskId, BLE_TASK_OTA_WRITE_EVENT);
		if (ble_ota_fsm == BLE_OTA_FSM_END)
		{
			LogPrintf(DEBUG_BLE, "Ble ota finish");
			sysinfo.bleUpgradeOnoff = 0;
			for (uint8_t i = 0; i < DEVICE_MAX_CONNECT_COUNT; i++)
			{
			    sysparam.relayUpgrade[i] = 0;
			}
			otaRxInfoInit(0, NULL, 0);
			tmos_memset(&sysparam.file, 0, sizeof(fileInfo_s));
			paramSaveAll();
			/* 注意这里不能立马断，会导致从机接收不到 */
			startTimer(10, bleRelayDeleteAll, 0);
			bleOtaFsmChange(BLE_OTA_FSM_IDLE);
		}
		else
		{
			tmos_start_reload_task(bleCentralTaskId, BLE_TASK_OTA_READ_EVENT, MS1_TO_SYSTEM_TIME(BLE_WRITE_OR_READ_MS));
		}
	}
	return ret;
}

/**************************************************
@bref       蓝牙ota进程
@param
@return
@note
**************************************************/

void bleOtaReadDataParser(uint16_t connHandle, OTA_IAP_CMD_t iap_read_data, uint16_t len)
{
	int ind;
	uint8_t  block_num    = 0;
    uint32_t image_b_addr = 0;
    uint16_t block_size   = 0;
    uint16_t chip_id      = 0;
    static uint8_t errcnt = 0;
	if (len == 0)
	{
		return;
	}
	
	ind = bleDevGetIdByHandle(connHandle);
	if (ind >= DEVICE_MAX_CONNECT_COUNT || ind < 0)
	{
		return;
	}

	switch (ble_ota_fsm)
	{
		case BLE_OTA_FSM_IDLE:
			LogPrintf(DEBUG_BLE, "ota is not ready");
			break;
		/* 0200A001000010730000000010000000F0FF0000 */
		case BLE_OTA_FSM_INFO:
			LogPrintf(DEBUG_BLE, "--------------ota info---------------");
			LogPrintf(DEBUG_BLE, "Dev(%d) in IMAGE%s ", ind, iap_read_data.other.buf[0] == 2 ? "B" : "A");
			image_b_addr   = iap_read_data.other.buf[4];
			image_b_addr <<= 8;
			image_b_addr  |= iap_read_data.other.buf[3];
			image_b_addr <<= 8;
			image_b_addr  |= iap_read_data.other.buf[2];
			image_b_addr <<= 8;
			image_b_addr  |= iap_read_data.other.buf[1];
			LogPrintf(DEBUG_BLE, "IMAGEB ADDR:0x%04X", image_b_addr);
			block_size   = iap_read_data.other.buf[6];
			block_size <<= 8;
			block_size  |= iap_read_data.other.buf[5];
			LogPrintf(DEBUG_BLE, "ONE BLOCK SIZE:%d", block_size);
			chip_id   = iap_read_data.other.buf[8];
			chip_id <<= 8;
			chip_id  |= iap_read_data.other.buf[7];
			LogPrintf(DEBUG_BLE, "CHIP ID:0x%04X", chip_id);
			if (chip_id == 0x73)
			{
				block_num = otaRxInfo.totalsize / EEPROM_BLOCK_SIZE;
				if (block_num > 11)
				{
					LogPrintf(DEBUG_BLE, "earse block num[%d] is more than CH573 max", block_num);
					bleOtaFsmChange(BLE_OTA_FSM_END);
				}
			}
			else if (chip_id == 0x92)
			{
				block_num = otaRxInfo.totalsize / EEPROM_BLOCK_SIZE;
				if (block_num > 100)
				{
					{
						LogPrintf(DEBUG_BLE, "earse block num[%d] is more than CH592 max", block_num);
						bleOtaFsmChange(BLE_OTA_FSM_END);
					}
				}
			}
			LogPrintf(DEBUG_BLE, "-------------------------------------");
			if (iap_read_data.other.buf[0] == 2)
			{
				bleOtaFsmChange(BLE_OTA_FSM_EASER);
			}
			else
			{
				bleOtaFsmChange(BLE_OTA_FSM_END_ERR);
			}
			break;
		case BLE_OTA_FSM_EASER:
			if (iap_read_data.other.buf[0] == 0x00)
			{
				bleOtaFsmChange(BLE_OTA_FSM_READ);				
			}
			else 
			{
				LogPrintf(DEBUG_BLE, "The file is too large. Erasing failed");
				bleOtaFsmChange(BLE_OTA_FSM_END_ERR);
			}
			break;
		case BLE_OTA_FSM_PROM:
			if (iap_read_data.other.buf[0] == 0x00)
			{
				upgradeResultProcess(0, otaRxInfo.rxoffset, otaRxInfo.len);
				errcnt = 0;
			}
			else
			{
				LogPrintf(DEBUG_BLE, "The file write error");
				errcnt++;
				if (errcnt >= 5)
				{
					//这里服务器的状态需不需要发送cmd=3还待考虑
					bleOtaFsmChange(BLE_OTA_FSM_END_ERR);
					errcnt = 0;
				}
			}
			break;
		case BLE_OTA_FSM_VERI:
			if (iap_read_data.other.buf[0] == 0x00)
			{
				upgradeResultProcess(1, otaRxInfo.veroffset, otaRxInfo.len);
				errcnt = 0;
			}
			else
			{
				bleOtaFsmChange(BLE_OTA_FSM_VERI_READ);
				errcnt++;
				if (errcnt >= 5)
				{
					LogPrintf(DEBUG_BLE, "The file verify error");
					bleOtaFsmChange(BLE_OTA_FSM_INFO);
				}
			}
			break;
		case BLE_OTA_FSM_END:
			
			break;
	}
	
}

/**************************************************
@bref       蓝牙ota发数据
@param
@return
@note
**************************************************/

void bleOtaSend(void)
{
	tmos_start_reload_task(bleCentralTaskId, BLE_TASK_OTA_WRITE_EVENT, MS1_TO_SYSTEM_TIME(BLE_WRITE_OR_READ_MS));
}


/**************************************************
@bref       蓝牙ota任务
@param
@return
@note
**************************************************/

#define OTA_TIMEBASE_MULTIPLE			10
void bleOtaTask(void)
{
	uint16_t readlen;
	if (sysinfo.bleUpgradeOnoff == 0)
	{
		return;
	}
	if (getBleOtaStatus() < 0 || getBleOtaStatus() >= DEVICE_MAX_CONNECT_COUNT)
	{
		ble_ota_fsm = BLE_OTA_FSM_IDLE;
		return;
	}
	/* 断连不重置状态 */
	if (bleDevGetOtaStatusByIndex(getBleOtaStatus()) == 0)
	{
		//LogPrintf(DEBUG_BLE, "bleOtaTask==>waitting connect");
		return;
	}
	/* 载入升级包信息才能通过 */
	if (otaRxInfo.totalsize == 0)
	{
		return;
	}
	switch (ble_ota_fsm)
	{
		case BLE_OTA_FSM_IDLE:
			if (otaRxInfo.totalsize != 0)
			{
				bleOtaSend();
				bleOtaFsmChange(BLE_OTA_FSM_INFO);
			}
			/* 没配置升级文件就跑到这里来 */
			else
			{
				
			}
			break;
		case BLE_OTA_FSM_INFO:
			if (ble_ota_tick == 0)
			{
				bleOtaSend();
			}
			if (ble_ota_tick >= 10 * OTA_TIMEBASE_MULTIPLE)
			{
				bleOtaFsmChange(BLE_OTA_FSM_IDLE);
				bleRelayDeleteAll();
			}
			break;
		case BLE_OTA_FSM_EASER:
			if (ble_ota_tick == 0)
			{
				bleOtaSend();
			}
			if (ble_ota_tick >= 10 * OTA_TIMEBASE_MULTIPLE)
			{
				bleOtaFsmChange(BLE_OTA_FSM_IDLE);
				bleRelayDeleteAll();
			}
			break;
		case BLE_OTA_FSM_READ:
			if (ble_ota_tick == 0)
			{
				readlen = otaRxInfo.totalsize - otaRxInfo.rxoffset;
				if (readlen > otaRxInfo.readLen)
				{
					readlen = otaRxInfo.readLen;
				}
	            fileReadData(otaRxInfo.fileName, readlen, otaRxInfo.rxoffset);
			}
			if (ble_ota_tick >= 10 * OTA_TIMEBASE_MULTIPLE)
			{

				readlen = otaRxInfo.totalsize - otaRxInfo.rxoffset;
				if (readlen > otaRxInfo.readLen)
				{
					readlen = otaRxInfo.readLen;
				}
	            fileReadData(otaRxInfo.fileName, readlen, otaRxInfo.rxoffset);
			}
			break;	
		case BLE_OTA_FSM_PROM:
			if (ble_ota_tick == 0)
			{
				bleOtaSend();
			}
			if (ble_ota_tick >= 20 * OTA_TIMEBASE_MULTIPLE)
			{
				bleOtaFsmChange(BLE_OTA_FSM_READ);
			}
			break;
		case BLE_OTA_FSM_VERI_READ:
			if (ble_ota_tick == 0)
			{
				readlen = otaRxInfo.totalsize - otaRxInfo.veroffset;
				if (readlen > otaRxInfo.readLen)
				{
					readlen = otaRxInfo.readLen;
				}
	            fileReadData(otaRxInfo.fileName, readlen, otaRxInfo.veroffset);
			}
			if (ble_ota_tick >= 10 * OTA_TIMEBASE_MULTIPLE)
			{
				readlen = otaRxInfo.totalsize - otaRxInfo.veroffset;
				if (readlen > otaRxInfo.readLen)
				{
					readlen = otaRxInfo.readLen;
				}
	            fileReadData(otaRxInfo.fileName, readlen, otaRxInfo.veroffset);
			}
			break;
		case BLE_OTA_FSM_VERI:
			if (ble_ota_tick == 0)
			{
				bleOtaSend();
			}
			if (ble_ota_tick >= 20 * OTA_TIMEBASE_MULTIPLE)
			{
				bleOtaFsmChange(BLE_OTA_FSM_VERI_READ);
			}
			break;
		/* 只有全部正确才能到达这里 */
		case BLE_OTA_FSM_END:
			if (ble_ota_tick == 0)
			{
				bleOtaSend();
			}
			if (ble_ota_tick >= 20)
			{
				bleOtaSend();
			}
			break;
		/* 异常退出不发送END指令 */
		case BLE_OTA_FSM_END_ERR:
			
			break;
	}
	ble_ota_tick++;
}


