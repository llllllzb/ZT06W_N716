#include "app_central.h"
#include "app_sys.h"
#include "app_bleRelay.h"
#include "app_task.h"
#include "app_server.h"
#include "app_param.h"
//全局变量

tmosTaskID bleCentralTaskId = INVALID_TASK_ID;
static gapBondCBs_t bleBondCallBack;
static gapCentralRoleCB_t bleRoleCallBack;
static deviceConnInfo_s devInfoList[DEVICE_MAX_CONNECT_COUNT];
static bleScheduleInfo_s bleSchedule;


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

static void bleSchduleChangeFsm(bleFsm nfsm);
static void bleScheduleTask(void);
static void bleDevDiscoverCharByUuid(void);
static void bleDevDiscoverServByUuid(void);

static void bleOtaFsmChange(ble_ota_fsm_e fsm);



/**************************************************
@bref       BLE主机初始化
@param
@return
@note
**************************************************/

void bleCentralInit(void)
{
    bleDevConnInit();
    bleRelayInit();
    GAPRole_CentralInit();
    GAP_SetParamValue(TGAP_DISC_SCAN, 12800);
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
		if (devInfoList[i].use && devInfoList[i].discState       == BLE_DISC_STATE_CHAR
							   && devInfoList[i].findServiceDone == 1)
		{
			if (pMsgEvt->msg.readByTypeRsp.numPairs > 0)
			{
                bleDevSetCharHandle(devInfoList[i].connHandle, 
        			   BUILD_UINT16(pMsgEvt->msg.readByTypeRsp.pDataList[0],
                                    pMsgEvt->msg.readByTypeRsp.pDataList[1]));
			}
			if ((pMsgEvt->method == ATT_READ_BY_TYPE_RSP && pMsgEvt->hdr.status == bleProcedureComplete))
            {
            	if (sysparam.relayUpgrade[i] == BLE_UPGRADE_FLAG)
            	{
					bleDevConnectComplete(devInfoList[i].connHandle);
            	}
            	else
            	{
					bleDevDiscoverNotify(devInfoList[i].connHandle);
				}
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
    LogPrintf(DEBUG_ALL, "Handle[%d],Method[0x%02X],Status[0x%02X]", pMsgEvt->connHandle, pMsgEvt->method,
              pMsgEvt->hdr.status);
    switch (pMsgEvt->method)
    {
        case ATT_ERROR_RSP:
            LogPrintf(DEBUG_BLE, "Error,Handle[%d],ReqOpcode[0x%02X],ErrCode[0x%02X]", pMsgEvt->msg.errorRsp.handle,
                      pMsgEvt->msg.errorRsp.reqOpcode, pMsgEvt->msg.errorRsp.errCode);
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
            LogPrintf(DEBUG_BLE, "Handle[%d] send %s!", pMsgEvt->connHandle, pMsgEvt->hdr.status == SUCCESS ? "success" : "fail");
            break;
		//读取数据
        case ATT_READ_RSP:
        	//tmos_memcpy((unsigned char *)&iap_read_data, pMsgEvt->msg.handleValueNoti.pValue, pMsgEvt->msg.handleValueNoti.len);
        	dataLen = pMsgEvt->msg.handleValueNoti.len > 50 ? 50 : pMsgEvt->msg.handleValueNoti.len;
        	byteToHexString(pMsgEvt->msg.handleValueNoti.pValue, debug, dataLen);
        	debug[dataLen * 2] = 0;
			LogPrintf(DEBUG_BLE, "^^Handle[%d] Read:%s, len:%d", pMsgEvt->connHandle, debug, pMsgEvt->msg.handleValueNoti.len);
			//bleOtaReadDataParser(pMsgEvt->connHandle, iap_read_data, pMsgEvt->msg.readRsp.len);
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
				sysparam.relayUpgrade[i]  == BLE_UPGRADE_FLAG)
			{
				LogPrintf(DEBUG_BLE, "Dev(%d) Read connHandle[0x%02x] charHandle[0x%02x]", i, devInfoList[i].connHandle, devInfoList[i].charHandle);
				status = bleCentralRead(devInfoList[i].connHandle, devInfoList[i].charHandle);
			}
		}
		if (status == SUCCESS)
		{
			tmos_stop_task(bleCentralTaskId, BLE_TASK_OTA_READ_EVENT);
		}
		return event ^ BLE_TASK_OTA_READ_EVENT;
    }

    if (event & BLE_TASK_OTA_INFO_EVENT)
    {
    	status = bleIncorrectMode;
		for (uint8_t i = 0; i < DEVICE_MAX_CONNECT_COUNT; i++)
		{
			if (devInfoList[i].use && 
				devInfoList[i].connHandle != INVALID_CONNHANDLE &&
				sysparam.relayUpgrade[i]  == BLE_UPGRADE_FLAG)
			{
				LogPrintf(DEBUG_BLE, "Dev(%d) Write connHandle[0x%02x] charHandle[0x%02x]", i, devInfoList[i].connHandle, devInfoList[i].charHandle);
				status = bleOtaSendProtocol(devInfoList[i].connHandle, devInfoList[i].charHandle, CMD_IAP_INFO, NULL, 0);
			}
		}
		if (status == SUCCESS)
		{
			tmos_stop_task(bleCentralTaskId, BLE_TASK_OTA_INFO_EVENT);
			bleOtaFsmChange(BLE_OTA_FSM_INFO);
			//tmos_start_reload_task(bleCentralTaskId, BLE_TASK_OTA_READ_EVENT, MS1_TO_SYSTEM_TIME(1000));
		}
		return event ^ BLE_TASK_OTA_INFO_EVENT;
    }

    if (event & BLE_TASK_UPDATE_PARAM_EVENT)
    {
    	status = bleIncorrectMode;
		for (uint8_t i = 0; i < DEVICE_MAX_CONNECT_COUNT; i++)
		{
			if (devInfoList[i].use &&
				devInfoList[i].connHandle != INVALID_CONNHANDLE)
			{
				status = GAPRole_UpdateLink(devInfoList[i].connHandle,
											20,
											100,
											0,
											600);
				LogPrintf(DEBUG_BLE, "Dev(%d) Updataparam, ret:0x%02x", i, status);
			}
		}
		return event ^ BLE_TASK_UPDATE_PARAM_EVENT;
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
        	LogPrintf(DEBUG_BLE, "param update");
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
    LogPrintf(DEBUG_BLE, "Start connect [%s](%d),ret=0x%02X", debug, addrType, status);
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
    req.handle = devInfoList[0].charHandle;
    ret = GATT_ReadCharValue(devInfoList[0].connHandle, &req, bleCentralTaskId);
    LogPrintf(DEBUG_BLE, "bleCentralRead==>Ret:0x%02X", ret);
    return ret;
}

/**************************************************
@bref       修改MTU
@param
@return
@note
**************************************************/

static void bleCentralChangeMtu(uint16_t connHandle)
{
	bStatus_t ret;
    attExchangeMTUReq_t req = {
        .clientRxMTU = BLE_BUFF_MAX_LEN - 4,
    };
    ret = GATT_ExchangeMTU(connHandle, &req, bleCentralTaskId);
    LogPrintf(DEBUG_BLE, "bleCentralRead==>Ret:0x%02X", ret);
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
            //不用添加devInfoList[i].discState = BLE_DISC_STATE_IDLE,由蓝牙状态回调来改写
            LogPrintf(DEBUG_BLE, "bleDevConnAdd==>[%d]ok", i);
            return i;
        }
    }
    return -1;
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
            if (devInfoList[i].connHandle != INVALID_CONNHANDLE)
            {
                bleCentralDisconnect(devInfoList[i].connHandle);
            }
            return i;
        }
    }
    return -1;
}

void bleDevConnDelAll(void)
{
    uint8_t i;
    for (i = 0; i < DEVICE_MAX_CONNECT_COUNT; i++)
    {
        if (devInfoList[i].use)
        {
//            if (devInfoList[i].connHandle != INVALID_CONNHANDLE)
//            {
//                bleCentralDisconnect(devInfoList[i].connHandle);
//            }
            devInfoList[i].use = 0;
        }
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
                LogPrintf(DEBUG_BLE, "Get device conn handle [%d]", connHandle);
                /* 更改MTU */
                //bleCentralChangeMtu(devInfoList[i].connHandle);
                /* 更新参数 */
                tmos_start_task(bleCentralTaskId, BLE_TASK_UPDATE_PARAM_EVENT, MS1_TO_SYSTEM_TIME(2000));
                /*  */
                tmos_start_task(bleCentralTaskId, BLE_TASK_SVC_DISCOVERY_EVENT, MS1_TO_SYSTEM_TIME(100));
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
            devInfoList[i].discState       = BLE_DISC_STATE_IDLE;
            byteToHexString(devInfoList[i].addr, debug, 6);
            debug[12] = 0;
            LogPrintf(DEBUG_BLE, "bleDevDisconnect==>Device (%d)[%s] disconnect,Handle[%d]", i, debug, connHandle);
            //bleSchduleChangeFsm(BLE_SCHEDULE_IDLE);		//不能添加这个，因为可能存在正在连第二个链路而主机又想断开第一条链路
            bleOtaFsmChange(BLE_OTA_FSM_IDLE);
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
    bStatus_t status;
    for (i = 0; i < DEVICE_MAX_CONNECT_COUNT; i++)
    {
        if (devInfoList[i].use && devInfoList[i].connHandle      != INVALID_CONNHANDLE
        					   && devInfoList[i].findServiceDone == 0
        					   && devInfoList[i].discState       == BLE_DISC_STATE_CONN)
        {
        	if (sysparam.relayUpgrade[i] == BLE_UPGRADE_FLAG)
        	{
	        	devInfoList[i].discState = BLE_DISC_STATE_SVC;
	            uuid[0] = LO_UINT16(OTA_SERVER_UUID);
	            uuid[1] = HI_UINT16(OTA_SERVER_UUID);
	            status = GATT_DiscPrimaryServiceByUUID(devInfoList[i].connHandle, uuid, 2, bleCentralTaskId);
	            LogPrintf(DEBUG_BLE, "Dev(%d)Discover Handle[%d]services by ota uuid[0x%04x],ret=0x%02X", 
	            						i, devInfoList[i].connHandle, OTA_SERVER_UUID, status);
            }
            else
            {
	        	devInfoList[i].discState = BLE_DISC_STATE_SVC;
	            uuid[0] = LO_UINT16(SERVICE_UUID);
	            uuid[1] = HI_UINT16(SERVICE_UUID);
	            status = GATT_DiscPrimaryServiceByUUID(devInfoList[i].connHandle, uuid, 2, bleCentralTaskId);
	            LogPrintf(DEBUG_BLE, "Dev(%d)Discover Handle[%d]services by normal uuid[0x%04x],ret=0x%02X", 
	            						i, devInfoList[i].connHandle, SERVICE_UUID, status);
            }
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
	attReadByTypeReq_t req;
	bStatus_t status;
	for (i = 0; i < DEVICE_MAX_CONNECT_COUNT; i++)
	{
		if (devInfoList[i].use && devInfoList[i].startHandle     != 0
							   && devInfoList[i].findServiceDone == 1
							   && devInfoList[i].discState       == BLE_DISC_STATE_SVC)
		{
			devInfoList[i].discState = BLE_DISC_STATE_CHAR;
			req.startHandle = devInfoList[i].startHandle;
            req.endHandle   = devInfoList[i].endHandle;
            req.type.len    = ATT_BT_UUID_SIZE;
            if (sysparam.relayUpgrade[i] == BLE_UPGRADE_FLAG)
            {
				req.type.uuid[0] = LO_UINT16(OTA_CHAR_UUID);
	            req.type.uuid[1] = HI_UINT16(OTA_CHAR_UUID);
	            status = GATT_ReadUsingCharUUID(devInfoList[i].connHandle, &req, bleCentralTaskId);
            	LogPrintf(DEBUG_BLE, "Dev(%d)Discover handle[%d]chars by ota uuid[0x%04X],ret=0x%02X", 
            				          i, devInfoList[i].connHandle, OTA_CHAR_UUID, status);
            }
            else
            {
	            req.type.uuid[0] = LO_UINT16(CHAR_UUID);
	            req.type.uuid[1] = HI_UINT16(CHAR_UUID);
	            status = GATT_ReadUsingCharUUID(devInfoList[i].connHandle, &req, bleCentralTaskId);
            	LogPrintf(DEBUG_BLE, "Dev(%d)Discover handle[%d]chars by normal uuid[0x%04X],ret=0x%02X", 
            						  i, devInfoList[i].connHandle, CHAR_UUID, status);
            }
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
@bref       完成连接
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
			devInfoList[i].notifyDone = 1;
			bleSchduleChangeFsm(BLE_SCHEDULE_DONE);
			LogPrintf(DEBUG_BLE, "Dev(%d)Handle[%d] skip notify and connect finish", i, connHandle);
			tmos_start_reload_task(bleCentralTaskId, BLE_TASK_OTA_INFO_EVENT, MS1_TO_SYSTEM_TIME(5000));
			bleOtaFsmChange(BLE_OTA_FSM_IDLE);
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
            LogPrintf(DEBUG_BLE, "bleDevTerminateProcess==>Dev(%d)Terminate Handle[%d]", i, devInfoList[i].connHandle);
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
	for (uint8_t i = 0; i < sysparam.bleMacCnt; i++)
	{
		if (devInfoList[i].use == 0)
		{
			if (devInfoList[i].connHandle == INVALID_CONNHANDLE)
			{
				LogPrintf(DEBUG_BLE, "蓝牙添加");
				bleRelayInsert(sysparam.bleConnMac[i], 0);
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
	bleDevInsertTask();
	bleConnPermissonManger();
	bleDevTerminateProcess();
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
                LogPrintf(DEBUG_BLE, "bleSchedule==>timeout!!!");
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

static ble_ota_fsm_e ble_ota_fsm = 0;
static void bleOtaFsmChange(ble_ota_fsm_e fsm)
{
	ble_ota_fsm = fsm;
	LogPrintf(DEBUG_BLE, "bleOtaFsmChange==>%d", ble_ota_fsm);
}

/**************************************************
@bref       蓝牙ota初始化
@param
@return
@note
**************************************************/

void bleOtaInit(void)
{
	bleOtaFsmChange(BLE_OTA_FSM_IDLE);
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
    uint32_t image_b_addr;
    uint16_t block_size;
    uint16_t chip_id;
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
			LogPrintf(DEBUG_BLE, " ota is not ready");
			break;
		/* 0200A001000010730000000010000000F0FF0000 */
		case BLE_OTA_FSM_INFO:

			LogPrintf(DEBUG_BLE, "--------------ota info---------------");
			LogPrintf(DEBUG_BLE, "IMAGE%s, ", iap_read_data.other.buf[0] == 2 ? "B" : "A");
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
			LogPrintf(DEBUG_BLE, "BLOCK  SIZE:0x%04X", block_size);
			chip_id   = iap_read_data.other.buf[8];
			chip_id <<= 8;
			chip_id  |= iap_read_data.other.buf[7];
			LogPrintf(DEBUG_BLE, "CHIP ID:0x%04X", chip_id);
			break;
		case BLE_OTA_FSM_EASER:

			break;
	}
	
}

