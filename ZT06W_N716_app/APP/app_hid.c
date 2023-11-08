/*
 * app_blehid.c
 *
 *  Created on: Oct 21, 2022
 *      Author: nimo
 */

#include "app_hid.h"
#include "app_task.h"

/*
* ȫ�ֱ�������
*/
tmosTaskID appHidTaskId = INVALID_TASK_ID;
static gapBondCBs_t appHidGapBondCallback;
static gapRolesCBs_t appHidGapRolesCallback;
hidConnectionInfoStruct appHidConn;
hidRptInfoStruct appHidRptInfo;

/*********************************BLE������������*************************************/
#define APP_SERVICE_UUID            0xFFE0
#define APP_CHARACTERISTIC1_UUID    0xFFE1

static uint8 ServiceUUID[ATT_BT_UUID_SIZE] = { LO_UINT16(APP_SERVICE_UUID), HI_UINT16(APP_SERVICE_UUID)};
static uint8 Char1UUID[ATT_BT_UUID_SIZE] = { LO_UINT16(APP_CHARACTERISTIC1_UUID), HI_UINT16(APP_CHARACTERISTIC1_UUID)};

static gattAttrType_t ServiceProfile =
{
    ATT_BT_UUID_SIZE, ServiceUUID
};

static uint8 char1_Properties = GATT_PROP_READ | GATT_PROP_WRITE
                                | GATT_PROP_NOTIFY;
static uint8 char1ValueStore[4];
static gattCharCfg_t char1ClientConfig[4];

static uint8 char1Description[] = "appchar1";

static gattAttribute_t appAttributeTable[] =
{
    //Service
    {   { ATT_BT_UUID_SIZE, primaryServiceUUID }, //type
        GATT_PERMIT_READ, 0,
        (uint8 *)& ServiceProfile
    },
    //��������
    {   { ATT_BT_UUID_SIZE, characterUUID },
        GATT_PERMIT_READ, 0, &char1_Properties
    },
    //��������ֵ
    {   { ATT_BT_UUID_SIZE, Char1UUID },
        GATT_PERMIT_READ | GATT_PERMIT_WRITE, 0, char1ValueStore
    },
    //�ͻ�����������NOTIFY
    {   { ATT_BT_UUID_SIZE, clientCharCfgUUID },
        GATT_PERMIT_READ | GATT_PERMIT_WRITE, 0, (uint8 *) char1ClientConfig
    },
    //�����������û�����
    {   { ATT_BT_UUID_SIZE, charUserDescUUID },
        GATT_PERMIT_READ, 0, char1Description
    }
};

static bStatus_t appReadAttrCB(uint16 connHandle, gattAttribute_t *pAttr,
                               uint8 *pValue, uint16 *pLen, uint16 offset, uint16 maxLen, uint8 method)
{
    bStatus_t ret = SUCCESS;
    uint16 uuid;
    if (gattPermitAuthorRead(pAttr->permissions))
    {
        return ATT_ERR_INSUFFICIENT_AUTHOR;
    }
    if (offset > 0)
    {
        return ATT_ERR_ATTR_NOT_LONG;
    }
    if (pAttr->type.len == ATT_BT_UUID_SIZE)
    {
        uuid = BUILD_UINT16(pAttr->type.uuid[0], pAttr->type.uuid[1]);
        LogPrintf(DEBUG_ALL, "UUID[0x%X]==>Read Request", uuid);
        switch (uuid)
        {
            case APP_CHARACTERISTIC1_UUID:
                *pLen = 4;
                tmos_memcpy(pValue, pAttr->pValue, 4);
                break;
        }
    }
    else
    {
        *pLen = 0;
        ret = ATT_ERR_INVALID_HANDLE;
    }

    return ret;
}

static bStatus_t appWriteAttrCB(uint16 connHandle, gattAttribute_t *pAttr,
                                uint8 *pValue, uint16 len, uint16 offset, uint8 method)
{
    bStatus_t ret = SUCCESS;
    uint16 uuid;
    uint8_t debugStr[101], debugLen;
    if (gattPermitAuthorWrite(pAttr->permissions))
    {
        return ATT_ERR_INSUFFICIENT_AUTHOR;
    }
    if (pAttr->type.len == ATT_BT_UUID_SIZE)
    {
        uuid = BUILD_UINT16(pAttr->type.uuid[0], pAttr->type.uuid[1]);
        //LogPrintf(DEBUG_ALL, "UUID[0x%X]==>Write Request,size:%d", uuid, len);
        debugLen = len > 50 ? 50 : len;
        byteToHexString(pValue, debugStr, debugLen);
        debugStr[debugLen * 2] = 0;

        switch (uuid)
        {
            case APP_CHARACTERISTIC1_UUID:
                LogMessage(DEBUG_ALL, "- + - - + - - + - - + - - + - - + - - + - ^");
                LogPrintf(DEBUG_ALL, "BLE receive:%s", debugStr);
                LogMessage(DEBUG_ALL, "- + - - + - - + - - + - - + - - + - - + -");
                //bleProtoclRecvParser(pValue, len);

                break;
            case GATT_CLIENT_CHAR_CFG_UUID:
                ret = GATTServApp_ProcessCCCWriteReq(connHandle, pAttr, pValue, len,
                                                     offset, GATT_CLIENT_CFG_NOTIFY);
                if(pValue[0] == 1)
                {
                    LogMessage(DEBUG_ALL, "Ble Connected Success");
                    sysinfo.bleConnStatus = 1;
                }
                break;
        }

    }
    else
    {
        ret = ATT_ERR_INVALID_HANDLE;
    }
    return ret;
}
static gattServiceCBs_t gattServiceCallBack = { appReadAttrCB, appWriteAttrCB,
                                                NULL,
                                              };






/*********************************HID������������*************************************/

/********************************************************
 * UUID����С��
 ********************************************************/
static uint8_t hidServUUID[ATT_BT_UUID_SIZE] = {
		LO_UINT16(HID_SERV_UUID), HI_UINT16(HID_SERV_UUID)};

static uint8_t hidProtocolModeUUID[ATT_BT_UUID_SIZE] = {
		LO_UINT16(PROTOCOL_MODE_UUID), HI_UINT16(PROTOCOL_MODE_UUID)};
		
static uint8_t hidReportUUID[ATT_BT_UUID_SIZE] = {
		LO_UINT16(REPORT_UUID), HI_UINT16(REPORT_UUID)};

const uint8_t InfoUUID[ATT_BT_UUID_SIZE] = {
    LO_UINT16(HID_INFORMATION_UUID), HI_UINT16(HID_INFORMATION_UUID)};
const uint8_t ReportMapUUID[ATT_BT_UUID_SIZE] = {
    LO_UINT16(REPORT_MAP_UUID), HI_UINT16(REPORT_MAP_UUID)};

/********************************************************
* HID�����UUID��Ϣ
********************************************************/
//hid����
static const gattAttrType_t hidService = {ATT_BT_UUID_SIZE, hidServUUID};


/********************************************************
 * ����������ֵ
 ********************************************************/
static uint8_t hidProtocolModeProps = GATT_PROP_READ | GATT_PROP_WRITE_NO_RSP;
static uint8_t hidReportProps = GATT_PROP_READ | GATT_PROP_NOTIFY;
static uint8_t hidInfoProps = GATT_PROP_READ;
static uint8_t hidReportMapProps = GATT_PROP_READ;
/********************************************************
 * �����洢��
 ********************************************************/
static uint8_t hidProtocolModeStore = HID_PROTOCOL_MODE_REPORT;
static uint8_t hidReportStore;
static const uint8_t hidInfo[HID_INFORMATION_LEN] = {
    LO_UINT16(0x0111), HI_UINT16(0x0111), // �汾�ţ�bcdHID (USB HID version)
    0x00,                                 // Ŀ�Ĺ��ҵ�ʶ���룺bCountryCode
    0x01                                  // ���Ա�־��Flags
};
static gattCharCfg_t hidReportClientCharCfg[GATT_MAX_NUM_CONN];

/********************************************************
 * ��������
 ********************************************************/
static uint8_t hidReportRef[2] = {0, HID_REPORT_TYPE_INPUT};



// HID Report Map characteristic value
static const uint8_t appHidReportMap[] = {
        // Consumer
//        0x05, 0x0c, // USAGE_PAGE (Consumer Devices)
//        0x09, 0x01, // USAGE (Consumer Control)
//        0xa1, 0x01, // COLLECTION (Application)
//        0x85, 0x01, //   REPORT_ID (1)        //���ֻ��һ�����͵�report ���Բ���д��һ��
//
//        0x15, 0x00, //   LOGICAL_MINIMUM (0)
//        0x25, 0x01, //   LOGICAL_MAXIMUM (1)
//        0x15, 0x00, //   LOGICAL_MINIMUM (0)
//        0x25, 0x01, //   LOGICAL_MAXIMUM (1)
//        0x75, 0x08, //   REPORT_SIZE (8)
//        0x95, 0x01, //   REPORT_COUNT (1)
//        0x09, 0xe9, //   USAGE (Volume Up)
//        0x81, 0x06, //   INPUT (Data,Var,Rel)
//        0x09, 0xea, //   USAGE (Volume Down)
//        0x81, 0x06, //   INPUT (Data,Var,Rel)
//
//        0xc0 // END_COLLECTION


        //���
//        0x05, 0x01, // USAGE_PAGE (Generic Desktop)
//        0x09, 0x02, // USAGE (Mouse)
//        0xa1, 0x01, // COLLECTION (Application)
//        //ָ���豸
//        0x09, 0x01, //   USAGE (Pointer)
//        0xa1, 0x00, //   COLLECTION (Physical)
//
//        0x05, 0x09, //     USAGE_PAGE (Button)
//        0x19, 0x01, //     USAGE_MINIMUM (Button 1)
//        //ʹ�����ֵ3��1��ʾ�����2��ʾ�Ҽ���3��ʾ�м�
//        0x29, 0x03, //     USAGE_MAXIMUM (Button 3)
//        0x15, 0x00, //     LOGICAL_MINIMUM (0)
//        0x25, 0x01, //     LOGICAL_MAXIMUM (1)
//        //��СΪ1��bit
//        0x75, 0x01, //     REPORT_SIZE (1)
//        //��8��bit���һ���ֽ�
//        0x95, 0x08, //     REPORT_COUNT (8)
//        //����ֵ
//        0x81, 0x02, //     INPUT (Data,Var,Abs)
//        //��;ҳ
//        0x05, 0x01, //     USAGE_PAGE (Generic Desktop)
//        0x09, 0x30, //     USAGE (X)
//        0x09, 0x31, //     USAGE (Y)
//        0x09, 0x38, //     USAGE (Wheel)
//        0x15, 0x81, //     LOGICAL_MINIMUM (-127)
//        0x25, 0x7f, //     LOGICAL_MAXIMUM (127)
//        0x75, 0x08, //     REPORT_SIZE (8)
//        0x95, 0x03, //     REPORT_COUNT (3)
//        0x81, 0x06, //     INPUT (Data,Var,Rel)
//        0xc0,       //     END_COLLECTION
//        0xc0        // END_COLLECTION

        //�Զ���map
        0x05, 0x06, // Usage Pg (Generic Desktop)
        0x09, 0x0D, // Usage (Portable Device Control)
        0xA1, 0x01, // Collection: (Application)

        0x75, 0x08, // Report Size (8)
        0x95, 0x01, // Report Count (1)
        0x15, 0x00, // LOGICAL_MINIMUM (0)
        0x25, 0xFF, // LOGICAL_MAXIMUM (255)
        0x19, 0x00, // Usage Min (0)
        0x29, 0xFF, // Usage Max (255)
        0x09, 0x3F, // �Զ���
        0x81, 0x02, // Iutput: (Data, Variable, Absolute)
        0x09, 0x3F, // �Զ���
        0x91, 0x02, // Output: (Data, Variable, Absolute)
        0xc0        // END_COLLECTION
};
/********************************************************
* HID����������
********************************************************/
static gattAttribute_t appHidAttrTbl[] = {

    // HID Service
    {
        {ATT_BT_UUID_SIZE, primaryServiceUUID}, /* type */			
        GATT_PERMIT_READ,                       /* permissions */
        0,                                      /* handle */
        (uint8_t *)&hidService                  /* pValue */
    },

    // HID Information characteristic declaration
    {
        {ATT_BT_UUID_SIZE, characterUUID},
        GATT_PERMIT_READ,
        0,
        &hidInfoProps
    },
    // HID Information characteristic
    {
        {ATT_BT_UUID_SIZE, InfoUUID},
        GATT_PERMIT_ENCRYPT_READ,
        0,
        (uint8_t *)hidInfo
    },

    // HID Protocol Mode characteristic declaration
    {
		{ATT_BT_UUID_SIZE, characterUUID},
		GATT_PERMIT_READ,
		0,
		&hidProtocolModeProps
    },
    {
    	{ATT_BT_UUID_SIZE, hidProtocolModeUUID},
    	GATT_PERMIT_ENCRYPT_READ | GATT_PERMIT_ENCRYPT_WRITE,
    	0,
    	&hidProtocolModeStore
    },
    // HID Report Map characteristic declaration
    {
        {ATT_BT_UUID_SIZE, characterUUID},
        GATT_PERMIT_READ,
        0,
        &hidReportMapProps
    },

    // HID Report Map characteristic
    {
        {ATT_BT_UUID_SIZE, ReportMapUUID},
        GATT_PERMIT_ENCRYPT_READ,
        0,
        (uint8_t *)appHidReportMap
    },

	// HID Report characteristic, key input declaration
	{
		{ATT_BT_UUID_SIZE, characterUUID},
		GATT_PERMIT_READ,
		0,
		&hidReportProps
	},
	{
		{ATT_BT_UUID_SIZE, hidReportUUID},
		GATT_PERMIT_ENCRYPT_READ,
		0,
		&hidReportStore
	},
	{
		{ATT_BT_UUID_SIZE, clientCharCfgUUID},
		GATT_PERMIT_READ | GATT_PERMIT_ENCRYPT_WRITE,
		0,
		(uint8_t *)&hidReportClientCharCfg
	},
	{															//������
		{ATT_BT_UUID_SIZE, reportRefUUID},					
		GATT_PERMIT_READ,
		0,
		hidReportRef},

};
/********************************************************
* �������ص�����
********************************************************/
uint8_t appHidReadAttrCB( uint16_t connHandle, gattAttribute_t *pAttr, uint8_t *pValue,
                            uint16_t *pLen, uint16_t offset, uint16_t maxLen, uint8_t method )
{
    bStatus_t ret = SUCCESS;
    uint16_t uuid;
    uint16_t maplen = sizeof(appHidReportMap);

    uuid = BUILD_UINT16(pAttr->type.uuid[0], pAttr->type.uuid[1]);
    LogPrintf(DEBUG_ALL, "read, uuid:%x", uuid);
    if(offset > 0 && uuid != REPORT_MAP_UUID)
    {
        return ATT_ERR_ATTR_NOT_LONG;
    }

    switch(uuid)
    {
        case REPORT_UUID:
            if(appHidRptInfo.handle == pAttr->handle)
            {
                if(appHidRptInfo.type == HID_REPORT_TYPE_OUTPUT)
                {
                    tmos_memcpy(pValue, pAttr->pValue, *pLen);
                }
            }
            else
            {
                *pLen = 0;
            }

            break;
        case REPORT_MAP_UUID:


            if(offset >= maplen)
            {
                ret = ATT_ERR_INVALID_OFFSET;
            }
            else
            {
                // determine read length
                *pLen = MIN(maxLen, (maplen - offset));

                // copy data
                tmos_memcpy(pValue, pAttr->pValue + offset, *pLen);
            }
            break;
        case HID_INFORMATION_UUID:
            //��Ҫ�������ݷŽ�pValue, Ҫ�����ݵĳ��ȷŽ�pLen
            *pLen = HID_INFORMATION_LEN;
            tmos_memcpy(pValue, pAttr->pValue, HID_INFORMATION_LEN);

            break;
        case PROTOCOL_MODE_UUID:
            LogMessage(DEBUG_ALL, "protocol mode");
            *pLen = HID_PROTOCOL_MODE_LEN;
            //tmos_memcpy(pValue, pAttr->pValue, PROTOCOL_MODE_UUID);
            pValue[0] = pAttr->pValue[0];
            break;
        case GATT_REPORT_REF_UUID:
            *pLen = HID_REPORT_REF_LEN;
            tmos_memcpy(pValue, pAttr->pValue, HID_REPORT_REF_LEN);
            break;
        default:
            LogPrintf(DEBUG_ALL, "uuid:%x", uuid);

            break;

    }
    return  ret;
}
/********************************************************
* ����д�ص�����(pValue����д���������֣� pAttr->pValueָ����Ƕ�Ӧ�������洢��)
********************************************************/
uint8_t appHidWriteAttrCB( uint16_t connHandle, gattAttribute_t *pAttr, uint8_t *pValue,
                            uint16_t len, uint16_t offset, uint8_t method )
{
    bStatus_t ret = SUCCESS;
    uint16_t uuid;

    if(offset > 0)
    {
        return ATT_ERR_ATTR_NOT_LONG;
    }
    uuid = BUILD_UINT16(pAttr->type.uuid[0], pAttr->type.uuid[1]);
    LogPrintf(DEBUG_ALL, "write, uuid:%x", uuid);
    switch(uuid)
    {
        case REPORT_UUID:
            if(appHidRptInfo.handle == pAttr->handle)
            {
                if(appHidRptInfo.type == HID_REPORT_TYPE_OUTPUT)
                {
                    ret = SUCCESS;
                    //����ӻص�����
                }
            }
            break;
        case PROTOCOL_MODE_UUID:
            if(len == HID_PROTOCOL_MODE_LEN)
            {
                //��ΪHID Protocol Mode characteristic�������洢��������1
                if(pValue[0] == HID_PROTOCOL_MODE_REPORT || pValue[0] == HID_PROTOCOL_MODE_BOOT)
                {
                    pAttr->pValue[0] = pValue[0];
                    //����ӻص�����
                }
            }
            else
            {
                //���Ȳ���
                ret = ATT_ERR_INVALID_VALUE_SIZE;
            }
            break;
        case GATT_CLIENT_CHAR_CFG_UUID:
            ret = GATTServApp_ProcessCCCWriteReq(connHandle, pAttr, pValue, len,
                                                     offset, GATT_CLIENT_CFG_NOTIFY);
            break;
        default :
            ret = ATT_ERR_INVALID_HANDLE;
            break;

    }

    return ret;
}
/*********************************************************************
 * @fn          hidDevHandleConnStatusCB
 *
 * @brief       Reset client char config.
 *
 * @param       connHandle - connection handle
 * @param       changeType - type of change
 *
 * @return      none
 */
static void appHidHandleConnStatusCB(uint16 connHandle, uint8 changeType)
{
    // Make sure this is not loopback connection
    if (connHandle != LOOPBACK_CONNHANDLE)
    {
        // Reset Client Char Config if connection has dropped
        if ((changeType == LINKDB_STATUS_UPDATE_REMOVED)
                || ((changeType == LINKDB_STATUS_UPDATE_STATEFLAGS)
                    && (!linkDB_Up(connHandle))))
        {
            GATTServApp_InitCharCfg(connHandle, char1ClientConfig);
            GATTServApp_InitCharCfg(connHandle, hidReportClientCharCfg);
            sysinfo.bleConnStatus = 0;
        }
    }

}

static gattServiceCBs_t hidServiceCallback = {appHidReadAttrCB, appHidWriteAttrCB, NULL};


void appHidAddService(void)
{
    GGS_AddService(GATT_ALL_SERVICES);         // GAP
    GATTServApp_AddService(GATT_ALL_SERVICES); // GATT attributes
	// Initialize Client Characteristic Configuration attributes
	//report��notify����
	GATTServApp_InitCharCfg(INVALID_CONNHANDLE, hidReportClientCharCfg);
    // Initialize Client Characteristic Configuration attributes
    GATTServApp_InitCharCfg(INVALID_CONNHANDLE, char1ClientConfig);
    // Register with Link DB to receive link status change callback
    linkDB_Register(appHidHandleConnStatusCB);
    //linkDB_Register(appHidHandleConnStatusCB);
    GATTServApp_RegisterService(appHidAttrTbl,
                                GATT_NUM_ATTRS(appHidAttrTbl), GATT_MAX_ENCRYPT_KEY_SIZE,
                                &hidServiceCallback);
    GATTServApp_RegisterService(appAttributeTable,
                                GATT_NUM_ATTRS(appAttributeTable), GATT_MAX_ENCRYPT_KEY_SIZE,
                                &gattServiceCallBack);

	//����Report��������Ϣ���������
	appHidRptInfo.id = hidReportRef[0];
	appHidRptInfo.type = hidReportRef[1];
	appHidRptInfo.handle = appHidAttrTbl[8].handle;
	appHidRptInfo.cccdHandle = appHidAttrTbl[9].handle;
	appHidRptInfo.mode = HID_PROTOCOL_MODE_REPORT;

}


/***********************************HID��������***************************************/

/*
*	GapBond״̬�ص�����
*/
static void appHidPasscodeCB(uint8_t *deviceAddr, uint16_t connectionHandle,
                             uint8_t uiInputs, uint8_t uiOutputs)
{
    LogPrintf(DEBUG_ALL, "uiInput:%d, uiOutput:%d\n", uiInputs, uiOutputs);
    uint32_t passkey;
    GAPBondMgr_GetParameter(GAPBOND_PERI_DEFAULT_PASSCODE, &passkey);
//    LogPrintf(DEBUG_ALL, "passkey:%d", passkey);
//    GAPBondMgr_PasscodeRsp(connectionHandle, SUCCESS, passkey);
}

//#define GAPBOND_PAIRING_STATE_STARTED           0x00  //!< Pairing started
//#define GAPBOND_PAIRING_STATE_COMPLETE          0x01  //!< Pairing complete
//#define GAPBOND_PAIRING_STATE_BONDED            0x02  //!< Devices bonded
//#define GAPBOND_PAIRING_STATE_BOND_SAVED        0x03  //!< Bonding record saved in NV
static void appHidPairStateCB(uint16_t connHandle, uint8_t state, uint8_t status)
{
    LogPrintf(DEBUG_ALL, "appHidPairStateCB==>0x%02x", state);
	switch(state)
	{
		case GAPBOND_PAIRING_STATE_STARTED:
			LogMessage(DEBUG_ALL, "Pairing Start");
			break;
		//��ʾ��ԵĹ�������ˣ�����һ���ǳɹ���
		case GAPBOND_PAIRING_STATE_COMPLETE:
		    if(status == SUCCESS)
		    {
		        LogMessage(DEBUG_ALL, "Pairing Success");
	            tmos_set_event(appHidTaskId, APP_HID_VERIFY_EVENT);
	            //sysinfo.bleConnStatus = 1;
		    }
		    else
		    {
                LogMessage(DEBUG_ALL, "Pairing Fail");
            }
			break;
		case GAPBOND_PAIRING_STATE_BOND_SAVED:
			LogMessage(DEBUG_ALL, "Pairing Bond Save");
            //��ȡ�Ѱ󶨵��豸
            uint8_t u8Value;
            GAPBondMgr_GetParameter( GAPBOND_BOND_COUNT, &u8Value );
            LogPrintf(DEBUG_ALL, "Bonded device count :%d", u8Value);

//            if(u8Value)
//            {
//                uint8_t sync_resolve_list = TRUE;
//                GAPBondMgr_SetParameter(GAPBOND_AUTO_SYNC_RL,sizeof(sync_resolve_list),&sync_resolve_list);
//
//                uint8_t filter_policy = GAP_FILTER_POLICY_WHITE;//GAP_FILTER_POLICY_WHITE_SCAN;
//                GAPRole_SetParameter( GAPROLE_ADV_FILTER_POLICY, sizeof( uint8_t ), &filter_policy );
//            }
			break;
		//�Ѿ�����Թ����豸��ֱ�ӽ������״̬�����ϵ�״̬����
		case GAPBOND_PAIRING_STATE_BONDED:
			LogMessage(DEBUG_ALL, "Paring Bonded");
			tmos_set_event(appHidTaskId, APP_HID_VERIFY_EVENT);
			//sysinfo.bleConnStatus = 1;
			break;
		default :
			LogMessage(DEBUG_ALL, "Unknow Pairing State");
			break;
			
	}
}






/*
*	GapRole״̬�ص�����
*/
//#define GAPROLE_STARTED                     1       //!< Started but not advertising
//#define GAPROLE_ADVERTISING                 2       //!< Currently Advertising
//#define GAPROLE_WAITING                     3       //!< Device is started but not advertising, is in waiting period before advertising again
//#define GAPROLE_CONNECTED                   4       //!< In a connection
//#define GAPROLE_CONNECTED_ADV               5       //!< In a connection + advertising
//#define GAPROLE_ERROR                       6       //!< Error occurred - invalid state
static void appHidStateNotify(gapRole_States_t newState, gapRoleEvent_t *pEvent)
{
	uint8_t u8Value;
	char debug[20];
	LogPrintf(DEBUG_ALL, "appHidStateNotify=>newstate:0x%02x, opcode:0x%02x", (newState & GAPROLE_STATE_ADV_MASK), pEvent->gap.opcode);
	switch (newState & GAPROLE_STATE_ADV_MASK)
	{
		case GAPROLE_STARTED:
			LogMessage(DEBUG_ALL, "GAPROLE Initialized..");
			//���óɾ�̬��ַ
			uint8_t addr[6];
			GAPRole_GetParameter(GAPROLE_BD_ADDR, addr);
			GAP_ConfigDeviceAddr(ADDRTYPE_STATIC, addr);
			break;

		case GAPROLE_ADVERTISING:
			if (pEvent->gap.opcode == GAP_MAKE_DISCOVERABLE_DONE_EVENT)
			{
				LogMessage(DEBUG_ALL, "GAPROLE Advertising..");
			}
			/*�㲥״̬�³��ֶ����¼�*/
			else if (pEvent->gap.opcode == GAP_LINK_TERMINATED_EVENT)
			{
				LogPrintf(DEBUG_ALL, "Disconnected.. Reason:0x%x", pEvent->linkTerminate.reason);
				appHidHandleConnStatusCB(appHidConn.connectionHandle, LINKDB_STATUS_UPDATE_REMOVED);

					/*�ȹع㲥���ٿ���*/
					appHidBroadcastTypeCtl(0, 0);

			}
			break;
		case GAPROLE_WAITING:
			/*�޹㲥״̬�¹㲥�ر��¼�*/
			if (pEvent->gap.opcode == GAP_END_DISCOVERABLE_DONE_EVENT)
            {
                LogMessage(DEBUG_ALL, "Waiting for advertising..");

                	/*���������ӹ㲥*/
					appHidBroadcastTypeCtl(1, 0);

            }
            /*�޹㲥״̬�³��ֶ����¼�*/
            else if (pEvent->gap.opcode == GAP_LINK_TERMINATED_EVENT)
            {
				LogPrintf(DEBUG_ALL, "Disconnected.. Reason:0x%x", pEvent->linkTerminate.reason);
				appHidHandleConnStatusCB(appHidConn.connectionHandle, LINKDB_STATUS_UPDATE_REMOVED);

					/*�ȹع㲥���ٿ���*/
					appHidBroadcastTypeCtl(0, 0);

            }
			break;
		case GAPROLE_CONNECTED:
			if(pEvent->gap.opcode == GAP_LINK_ESTABLISHED_EVENT)
			{
			    appHidConn.connectionHandle = pEvent->linkCmpl.connectionHandle;
				LogMessage(DEBUG_ALL, "GAPROLE Connecting..");
				tmos_start_task(appHidTaskId, APP_HID_PARAM_UPDATE_EVENT, MS1_TO_SYSTEM_TIME(800));
                memcpy(debug, pEvent->linkCmpl.devAddr, 6);
                byteToHexString(pEvent->linkCmpl.devAddr, debug, 6);
                debug[12] = 0;
                LogPrintf(DEBUG_ALL, "addr:%s, addrtype:%d", debug, pEvent->linkCmpl.devAddrType);
                tmos_memcpy(appHidConn.addr, pEvent->linkCmpl.devAddr, 6);
                appHidConn.addrType = pEvent->linkCmpl.devAddrType;

				//�����������ӹ㲥
				appHidBroadcastTypeCtl(1, 1);
                //��Ȩ
                //tmos_set_event(appHidTaskId, APP_HID_VERIFY_EVENT);
			}
			break;
		case GAPROLE_CONNECTED_ADV:
			LogMessage(DEBUG_ALL, "GAPROLE_CONNECTED_ADV");
			break;
		case GAPROLE_ERROR:
			LogMessage(DEBUG_ALL, "Ble error");
			break;
		
	}
}

static void appHidRssiRead(uint16_t connHandle, int8_t newRSSI)
{
	LogPrintf(DEBUG_ALL, "ConnHanle %d, Rssi %d", connHandle, newRSSI);
}

static void appHidParamUpdate( uint16_t connHandle, uint16_t connInterval,
    uint16_t connSlaveLatency, uint16_t connTimeout)
{
    LogPrintf(DEBUG_ALL, "Param Update==>connectHandle %d, connInterval %d", connHandle, connInterval);
//	LogMessage(DEBUG_ALL, "-------------------------------------\r\n");
//    LogMessage(DEBUG_ALL, "*****ParamUpdate*****\r\n");
//    LogPrintf(DEBUG_ALL, "connectionHandle:%d\r\n", connHandle);
//    LogPrintf(DEBUG_ALL, "connInterval:%d\r\n", connInterval);
//    LogPrintf(DEBUG_ALL, "connLatency:%d\r\n", connSlaveLatency);
//    LogPrintf(DEBUG_ALL, "connTimeout:%d\r\n", connTimeout);
//    LogMessage(DEBUG_ALL, "-------------------------------------\r\n");
}



static void appGapMsgProcess(gapRoleEvent_t *pMsg)
{
	char debug[20];
	LogPrintf(DEBUG_ALL, "appGapMsgProcess=>0x%02x", pMsg->gap.opcode);
	switch (pMsg->gap.opcode)
	{
		case GAP_SCAN_REQUEST_EVENT:
			//����ɨ���ӡ��ɨ���豸��mac

			break;
		case GAP_PHY_UPDATE_EVENT:
//            LogMessage(DEBUG_ALL, "-------------------------------------");
//            LogMessage(DEBUG_ALL, "*****LinkPhyUpdate*****\r\n");
//            LogPrintf(DEBUG_ALL, "connHandle:%d\r\n", pMsg->linkPhyUpdate.connectionHandle);
//            LogPrintf(DEBUG_ALL, "connRxPHYS:%d\r\n", pMsg->linkPhyUpdate.connRxPHYS);
//            LogPrintf(DEBUG_ALL, "connTxPHYS:%d\r\n", pMsg->linkPhyUpdate.connTxPHYS);
//            LogMessage(DEBUG_ALL, "-------------------------------------");
			break;
		case GAP_LINK_PARAM_UPDATE_EVENT:
		    LogMessage(DEBUG_ALL, "Param Update");

		    break;

	}
}

static void appGattMsgProcess(gattMsgEvent_t *pMsg)
{
    if(pMsg->method == ATT_MTU_UPDATED_EVENT)
    {
        LogPrintf(DEBUG_ALL, "MTU Exchange: %d", pMsg->msg.exchangeMTUReq.clientRxMTU);
    }
}

static void appHidProcessTMOSMsg(tmos_event_hdr_t *Msg)
{
	LogPrintf(DEBUG_ALL, "appHidProcessTMOSMsg==>MsgEvent:0x%x, MsgStatus:0x%x", Msg->event, Msg->status);
	switch(Msg->event)
	{
		case GAP_MSG_EVENT:
			appGapMsgProcess((gapRoleEvent_t *)Msg);
			break;
		case GATT_MSG_EVENT:
		    appGattMsgProcess((gattMsgEvent_t *)Msg);
		    break;
	}
}

uint16_t apphidProcessEvent(tmosTaskID task_id, tmosEvents events)
{
	if (events & SYS_EVENT_MSG)
	{
		uint8_t *Msg;
		if((Msg = tmos_msg_receive(appHidTaskId)) != NULL)
		{
			appHidProcessTMOSMsg((tmos_event_hdr_t *)Msg);
			tmos_msg_deallocate(Msg);
		}
		return events ^ SYS_EVENT_MSG;
	}

	if (events & APP_HID_SATRT_EVENT)
	{
		GAPRole_PeripheralStartDevice(appHidTaskId, &appHidGapBondCallback, &appHidGapRolesCallback );
		return events ^ APP_HID_SATRT_EVENT;
	}

	if (events & APP_HID_PARAM_UPDATE_EVENT)
	{
	    uint8_t u8val;

		u8val = GAPRole_PeripheralConnParamUpdateReq(appHidConn.connectionHandle, 8, 8, 0, 500, appHidTaskId);
		return events ^ APP_HID_PARAM_UPDATE_EVENT;
	}
	
	if (events & APP_HID_SEND_DATA_TEST_EVENT)
	{


	    return events ^ APP_HID_SEND_DATA_TEST_EVENT;
	}

	if (events & APP_HID_ENABLE_NOTIFY_EVENT)
	{
	    uint8_t ret;
	    ret = appEnableNotifyCtl(appHidConn.connectionHandle, 1);
	    if(ret == SUCCESS)
	    {
	        LogMessage(DEBUG_ALL, "Notify Enable Success");
	    }
	    else
	    {
            LogMessage(DEBUG_ALL, "Notify Enable Fail");
        }
	    return events ^ APP_HID_ENABLE_NOTIFY_EVENT;
	}

	if (events & APP_HID_VERIFY_EVENT)
	{
	    appHidSendNotifyData(dynamicParam.SN, 15);
	    return events ^ APP_HID_VERIFY_EVENT;
	}

}

void appHidBroadcasterCfg(uint8_t *devName)
{
	uint8_t advertData[31];
	uint8_t len, u8Value, advertLen = 0;
	len = strlen(devName);
	advertData[advertLen++] = len + 1;
	advertData[advertLen++] = GAP_ADTYPE_LOCAL_NAME_COMPLETE;
	tmos_memcpy(advertData + advertLen, devName, len);
	advertLen += len;
	advertData[advertLen++] = 0x03;
	advertData[advertLen++] = GAP_ADTYPE_APPEARANCE;
	advertData[advertLen++] = LO_UINT16(GAP_APPEARE_GENERIC_HID);
	advertData[advertLen++] = HI_UINT16(GAP_APPEARE_GENERIC_HID);
	advertData[advertLen++] = 0x02;
	advertData[advertLen++] = GAP_ADTYPE_FLAGS;
	advertData[advertLen++] = GAP_ADTYPE_FLAGS_LIMITED | GAP_ADTYPE_FLAGS_BREDR_NOT_SUPPORTED;

	GAPRole_SetParameter(GAPROLE_ADVERT_DATA, advertLen, advertData);
					u8Value = GAP_ADTYPE_ADV_IND;
	GAPRole_SetParameter(GAPROLE_ADV_EVENT_TYPE, sizeof(uint8_t), &u8Value);
	u8Value = TRUE;
	GAPRole_SetParameter(GAPROLE_ADVERT_ENABLED, sizeof(u8Value), &u8Value);

	//����GGS�����Device name����������ֵ
	GGS_SetParameter(GGS_DEVICE_NAME_ATT, GAP_DEVICE_NAME_LEN, (void *)devName);
}

void appHidDeviveNameCfg(void)
{
    char name[30];
    sprintf(name, "EMSD-%s", dynamicParam.SN + 11);
    appHidBroadcasterCfg(name);
}

void appHidScanRspDataCfg(void)
{
    uint8_t scanRspData[] = {
            0x05, // length of this data
            GAP_ADTYPE_16BIT_MORE,
            LO_UINT16(HID_SERV_UUID),
            HI_UINT16(HID_SERV_UUID),
            LO_UINT16(BATT_SERV_UUID),
            HI_UINT16(BATT_SERV_UUID),
    };

    GAPRole_SetParameter(GAPROLE_SCAN_RSP_DATA, sizeof(scanRspData), scanRspData);
}
/*
 * HID��ʼ��
 * */
void appHidPeripheralInit(void)
{
	//�ӻ���ɫ��ʼ��
	GAPRole_PeripheralInit();
	//ע��tmos����
	appHidTaskId = TMOS_ProcessEventRegister(apphidProcessEvent);
	//ע��״̬�ص�����
	appHidGapBondCallback.pairStateCB = appHidPairStateCB;
	appHidGapBondCallback.passcodeCB = appHidPasscodeCB;
	appHidGapRolesCallback.pfnRssiRead = appHidRssiRead;
	appHidGapRolesCallback.pfnParamUpdate = appHidParamUpdate;
	appHidGapRolesCallback.pfnStateChange = appHidStateNotify;

	//������ԺͰ�
	uint32_t passkey = 123456;
	uint8_t  pairMode = GAPBOND_PAIRING_MODE_WAIT_FOR_REQ;//GAPBOND_PAIRING_MODE_INITIATE��ʾ�������������������Ҳ�������á� GAPBOND_PAIRING_MODE_WAIT_FOR_REQ��ʾ�ȴ��Է��������
	uint8_t  mitm = TRUE;
	uint8_t  ioCap = GAPBOND_IO_CAP_NO_INPUT_NO_OUTPUT;
	uint8_t  bonding = TRUE;
	GAPBondMgr_SetParameter(GAPBOND_PERI_DEFAULT_PASSCODE, sizeof(uint32_t), &passkey);
	GAPBondMgr_SetParameter(GAPBOND_PERI_PAIRING_MODE, sizeof(uint8_t), &pairMode);
	GAPBondMgr_SetParameter(GAPBOND_PERI_MITM_PROTECTION, sizeof(uint8_t), &mitm);
	GAPBondMgr_SetParameter(GAPBOND_PERI_IO_CAPABILITIES, sizeof(uint8_t), &ioCap);
	GAPBondMgr_SetParameter(GAPBOND_PERI_BONDING_ENABLED, sizeof(uint8_t), &bonding);

//	//������ģʽ
//	uint8_t policy = GAP_FILTER_POLICY_ALL;
//	GAPBondMgr_SetParameter(GAPROLE_ADV_FILTER_POLICY, sizeof(policy), &policy);
	uint8_t u8Value;
	u8Value = GAP_ADTYPE_ADV_IND;
	GAPRole_SetParameter(GAPROLE_ADV_EVENT_TYPE, sizeof(uint8_t), &u8Value);

	//�㲥���ò�ʹ��
	GAP_SetParamValue(TGAP_DISC_ADV_INT_MIN, 160);
	GAP_SetParamValue(TGAP_DISC_ADV_INT_MAX, 160);
	//���ù㲥����
	appHidDeviveNameCfg();
	//ɨ��Ӧ����������
	appHidScanRspDataCfg();
	//��ӷ���(ƻ���ֻ��޷���Ժ��������й�)
	appHidAddService();

	tmos_set_event(appHidTaskId, APP_HID_SATRT_EVENT);

}

/*
 * Notify ��������
 * */
static uint8_t appHidNotify(uint16_t handle, uint8_t len, uint8_t *pData)
{
    uint8_t ret;
    attHandleValueNoti_t notify;

    notify.pValue = GATT_bm_alloc(appHidConn.connectionHandle, ATT_HANDLE_VALUE_NOTI, len, NULL, 0);
    if(notify.pValue != NULL)
    {
        notify.handle = handle;
        notify.len = len;
        tmos_memcpy(notify.pValue, pData, len);

        ret = GATT_Notification(appHidConn.connectionHandle, &notify, FALSE);
        if(ret != SUCCESS)
        {
            GATT_bm_free((gattMsg_t *)&notify, ATT_HANDLE_VALUE_NOTI);

            //LogPrintf(DEBUG_ALL, "Notify Fail~~ret :0x%02x",ret);
        }
        else
        {
            //LogMessage(DEBUG_ALL, "Notify Success~");
        }
    }
    else
    {
        ret = bleMemAllocError;
    }
    return ret;
}

void appHidSendReport(uint8_t id, uint8_t type, uint16_t len, uint8_t *pData)
{
    gattAttribute_t *pAttr;
    uint16_t pHandle;
    uint16_t notifyVal;
    uint8_t ret = bleNoResources;
    if(appHidRptInfo.id == id && appHidRptInfo.type == type &&
            appHidRptInfo.mode == HID_PROTOCOL_MODE_REPORT)
    {
        if((pAttr = GATT_FindHandle(appHidRptInfo.cccdHandle, &pHandle)) != NULL)
        {
            notifyVal = GATTServApp_ReadCharCfg(appHidConn.connectionHandle, (gattCharCfg_t *)pAttr->pValue);
            if(notifyVal & GATT_CLIENT_CFG_NOTIFY)
            {
                ret = appHidNotify(appHidRptInfo.handle, len, pData);
            }
        }
    }
}

/*
 * ������notify
 * */
uint8_t appEnableNotifyCtl(uint16_t connHandle, uint8_t enable)
{
    uint8_t cccd;
    if(enable)
    {
        cccd |= GATT_CLIENT_CFG_NOTIFY;
    }
    else
    {
        cccd &= ~GATT_CLIENT_CFG_NOTIFY;
    }
    return GATTServApp_WriteCharCfg(connHandle, hidReportClientCharCfg, cccd);
}

/*
 * FFE1 notify��������
 */
static bStatus_t appNotify(uint16 connHandle, attHandleValueNoti_t *pNoti)
{
    uint16 value = GATTServApp_ReadCharCfg(connHandle, char1ClientConfig);

    // If notifications enabled
    if (value & GATT_CLIENT_CFG_NOTIFY)
    {
        // Set the handle
        pNoti->handle = appAttributeTable[2].handle;

        // Send the notification
        return GATT_Notification(connHandle, pNoti, FALSE);
    }
    return bleIncorrectMode;
}

void appHidSendNotifyData(uint8 *data, uint16 len)
{
    attHandleValueNoti_t notify;
    uint8_t ret;
    notify.len = len;
    notify.pValue = GATT_bm_alloc(appHidConn.connectionHandle, ATT_HANDLE_VALUE_NOTI, notify.len, NULL, 0);
    if (notify.pValue == NULL)
    {
        LogPrintf(DEBUG_ALL, "appSendNotifyData==>alloc memory fail");
        return;
    }
    tmos_memcpy(notify.pValue, data, notify.len);
    if ((ret = appNotify(appHidConn.connectionHandle, &notify)) != SUCCESS)
    {
        GATT_bm_free((gattMsg_t *)&notify, ATT_HANDLE_VALUE_NOTI);
        //LogPrintf(DEBUG_ALL, "Notify fail, ret:0x02%x", ret);
    }
    else
    {
        //LogPrintf(DEBUG_ALL, "Notify success");
    }
}
/*�����*/
void appHidRemoveBond(uint8_t number)
{
    uint8_t bondcount;
    uint8_t addr[6] = {0};
    uint8_t buf[7] = {0};
    uint8_t debug[50];
    GAPBondMgr_GetParameter(GAPBOND_BOND_COUNT, &bondcount);
    if(bondcount <= 0)
    {
        LogMessage(DEBUG_ALL, "No bond");
        return;
    }
    tmos_snv_read(mainRecordNvID(number), 6, addr);
    byteToHexString(addr, debug, 6);
    debug[12] = 0;
    LogPrintf(DEBUG_ALL, "Ble Mac:%s", debug);

    memcpy(buf + 1, addr, 6);
    buf[0] = appHidConn.addrType;
    GAPBondMgr_SetParameter(GAPBOND_ENABLE_SINGLEBOND, 6 + 1, buf);
}

/*����*/
void appHidTerminalLink(void)
{
    GAPRole_TerminateLink(appHidConn.connectionHandle);
    sysinfo.bleConnStatus = 0;
}

void appHidBroadcastTypeCtl(uint8_t onoff, uint8_t type)
{
	uint8_t u8Value;
	if (onoff)
	{
		if (type == 0)
		{
			u8Value = GAP_ADTYPE_ADV_IND;
			GAPRole_SetParameter(GAPROLE_ADV_EVENT_TYPE, sizeof(uint8_t), &u8Value);
			u8Value = TRUE;
			GAPRole_SetParameter(GAPROLE_ADVERT_ENABLED, sizeof(uint8_t), &u8Value);
		}
		else if (type == 1)
		{
			u8Value = GAP_ADTYPE_ADV_NONCONN_IND;
			GAPRole_SetParameter(GAPROLE_ADV_EVENT_TYPE, sizeof(uint8_t), &u8Value);
			u8Value = TRUE;
			GAPRole_SetParameter(GAPROLE_ADVERT_ENABLED, sizeof(uint8_t), &u8Value);
		}
	}
	else 
	{
		u8Value = FALSE;
		GAPRole_SetParameter(GAPROLE_ADVERT_ENABLED, sizeof(uint8_t), &u8Value);
	}
}

