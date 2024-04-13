#include <app_protocol.h>
#include "app_net.h"

#include "app_db.h"
#include "app_gps.h"
#include "app_instructioncmd.h"
#include "app_kernal.h"
#include "app_param.h"
#include "app_sys.h"
#include "app_task.h"
#include "app_server.h"
#include "app_socket.h"
#include "app_jt808.h"
#include "app_central.h"

//ÁªÍøÏà¹Ø½á¹¹Ìå

static moduleState_s  moduleState;
static moduleCtrl_s moduleCtrl;
static cmdNode_s *headNode = NULL;


//Ö¸Áî±í
const atCmd_s cmdtable[] =
{
    {AT_CMD, "ATE0"},
    {CPIN_CMD, "AT+CPIN"},
    {CGSN_CMD, "AT+CGSN"},
    {CIMI_CMD, "AT+CIMI"},
    {CSQ_CMD, "AT+CSQ"},
    {CREG_CMD, "AT+CREG"},
    {CGATT_CMD, "AT+CGATT"},
    {MYSYSINFO_CMD, "AT$MYSYSINFO"},
    {CGDCONT_CMD, "AT+CGDCONT"},
    {XGAUTH_CMD, "AT+XGAUTH"},
    {XIIC_CMD, "AT+XIIC"},
    {TCPSETUP_CMD, "AT+TCPSETUP"},
    {IPSTATUS_CMD, "AT+IPSTATUS"},
    {TCPSEND_CMD, "AT+TCPSEND"},
    {TCPACK_CMD, "AT+TCPACK"},
    {TCPCLOSE_CMD, "AT+TCPCLOSE"},
    {CMGF_CMD, "AT+CMGF"},
    {CMGS_CMD, "AT+CMGS"},
    {CMGD_CMD, "AT+CMGD"},
    {CMGR_CMD, "AT+CMGR"},
    {CNMI_CMD, "AT+CNMI"},
    {ATA_CMD, "ATA"},
    {ATH_CMD, "ATH"},
    {CCLK_CMD, "AT+CCLK"},
    {MYGPSPWR_CMD, "AT$MYGPSPWR"},
    {MYGPSPOS_CMD, "AT$MYGPSPOS"},
    {MYGPSSTATE_CMD, "AT$MYGPSSTATE"},
    {MYPOWEROFF_CMD, "AT$MYPOWEROFF"},
    {MYGNSSSEL_CMD, "AT$MYGNSSSEL"},
    {TTSPLAY_CMD, "AT+TTSPLAY"},
    {PLAYTTS_CMD, "AT+PLAYTTS"},
    {FSLIST_CMD, "AT+FSLIST"},
    {FSWF_CMD, "AT+FSWF"},
    {FSRF_CMD, "AT+FSRF"},
    {FSDF_CMD, "AT+FSDF"},
    {FSFAT_CMD, "AT+FSFAT"},
    {RECMODE_CMD, "AT+RECMODE"},
    {RECF_CMD, "AT+RECF"},
    {WIFIAPSCAN_CMD, "AT+WIFIAPSCAN"},
    {CCID_CMD, "AT+CCID"},
    {SETSERVER_CMD, "AT+SETSERVER"},
    {ENPWRSAVE_CMD, "AT+ENPWRSAVE"},
    {CFUN_CMD, "AT+CFUN"},
    {MICL_CMD, "AT+MICL"},
    {MYLACID_CMD, "AT$MYLACID"},
    {NWBTBLEPWR_CMD, "AT+NWBTBLEPWR"},
    {NWBTBLENAME_CMD, "AT+NWBTBLENAME"},
    {NWBLEROLE_CMD, "AT+NWBLEROLE"},
    {NWBLEPSEND_CMD, "AT+NWBLEPSEND"},
    {NWBLEDISCON_CMD, "AT+NWBLEDISCON"},
    {NWBLEMAC_CMD, "AT+NWBLEMAC"},
    {NWBLECCON_CMD, "AT+NWBLECCON"},
    {NWBTBLEMAC_CMD, "AT+NWBTBLEMAC"},
    {NWBLEADVEN_CMD, "AT+NWBLEADVEN"},
    {HTTPSCFG_CMD, "AT+HTTPSCFG"},
    {HTTPSPARA_CMD, "AT+HTTPSPARA"},
    {HTTPSSETUP_CMD, "AT+HTTPSSETUP"},
    {HTTPSACTION_CMD, "AT+HTTPSACTION"},
    {HTTPPARA_CMD, "AT+HTTPPARA"},
    {HTTPSETUP_CMD, "AT+HTTPSETUP"},
    {HTTPACTION_CMD, "AT+HTTPACTION"},
    {GMR_CMD, "AT+GMR"},
    {SIMHOTSWAP, "AT+SIMHOTSWAP"},
    {POWEROFF_CMD, "AT$MYPOWEROFF"},
	{NETDMSG_CMD, "AT+NETDMSG"},
	{RECVMODE_CMD, "AT+RECVMODE"},
	{TCPREAD_CMD, "AT+TCPREAD"},
	{SIMCROSS_CMD, "AT+SIMCROSS"},
	{READADC_CMD, "AT+READADC"},
	{CLIP_CMD, "AT+CLIP"},
	{FSFS_CMD, "AT+FSFS"},
	{CLVL_CMD, "AT+CLVL"},
};

/**************************************************
@bref		´´½¨Ö¸ÁîÊä³ö¶ÓÁÐ£¬ÓÃÓÚË³ÐòÊä³ö
@param
@return
@note
**************************************************/

uint8_t createNode(char *data, uint16_t datalen, uint8_t currentcmd)
{
    cmdNode_s *nextnode;
    cmdNode_s *currentnode;
    //Èç¹ûÁ´±íÍ·Î´´´½¨£¬Ôò´´½¨Á´±íÍ·¡£
    WAKEMODULE;
    if (currentcmd == WIFIAPSCAN_CMD)
    {
		wakeUpByInt(1, 20);
    }
    else
    {
		wakeUpByInt(1, 15);
    }
    if (headNode == NULL)
    {
        headNode = malloc(sizeof(cmdNode_s));
        if (headNode != NULL)
        {
            headNode->currentcmd = currentcmd;
            headNode->data = NULL;
            headNode->data = malloc(datalen);
            if (headNode->data != NULL)
            {
                memcpy(headNode->data, data, datalen);
                headNode->datalen = datalen;
                headNode->nextnode = NULL;
                return 1;
            }
            else
            {
                free(headNode);
                headNode = NULL;
                return 0;
            }
        }
        else
        {
            return 0;
        }
    }
    currentnode = headNode;
    do
    {
        nextnode = currentnode->nextnode;
        if (nextnode == NULL)
        {
            nextnode = malloc(sizeof(cmdNode_s));
            if (nextnode != NULL)
            {

                nextnode->currentcmd = currentcmd;
                nextnode->data = NULL;
                nextnode->data = malloc(datalen);
                if (nextnode->data != NULL)
                {
                    memcpy(nextnode->data, data, datalen);
                    nextnode->datalen = datalen;
                    nextnode->nextnode = NULL;
                    currentnode->nextnode = nextnode;
                    nextnode = nextnode->nextnode;
                }
                else
                {
                    free(nextnode);
                    nextnode = NULL;
                    return 0;
                }
            }
            else
            {
                return 0;
            }
        }
        currentnode = nextnode;
    }
    while (nextnode != NULL);

    return 1;
}

/**************************************************
@bref		Êý¾Ý¶ÓÁÐÊä³ö
@param
@return
@note
**************************************************/

void outputNode(void)
{
    static uint8_t lockFlag = 0;
    static uint8_t lockTick = 0;
    static uint8_t sleepTick = 0;
    static uint8_t tickRange = 50;
    cmdNode_s *nextnode;
    cmdNode_s *currentnode;
    if (lockFlag)
    {
        if (lockTick++ >= tickRange)
        {
            lockFlag = 0;
            lockTick = 0;
            LogMessage(DEBUG_ALL, "outputNode==>Unlock");
        }
        return ;
    }
    if (headNode == NULL)
    {
        if (sleepTick > 0)
        {
            sleepTick--;
        }
        else
        {
            SLEEPMODULE;
        }
        return ;
    }
    sleepTick = 2;
    currentnode = headNode;
    if (currentnode != NULL)
    {
        nextnode = currentnode->nextnode;
        moduleState.cmd = currentnode->currentcmd;

        //Êý¾Ý·¢ËÍ
        portUartSend(MODULE_UART, (uint8_t *)currentnode->data, currentnode->datalen);
        if (currentnode->data[0] != 0X78 && currentnode->data[0] != 0x79 && currentnode->data[0] != 0x7E)
        {
            LogMessageWL(DEBUG_ALL, currentnode->data, currentnode->datalen);
        }
        free(currentnode->data);
        free(currentnode);
        if (currentnode->currentcmd == TCPCLOSE_CMD || currentnode->currentcmd == CMGS_CMD || 
        		currentnode->currentcmd == WIFIAPSCAN_CMD)
        {
            lockFlag = 1;
            if (currentnode->currentcmd == TCPCLOSE_CMD)
            {
                tickRange = 20;
            }
            else if (currentnode->currentcmd == WIFIAPSCAN_CMD)
            {
				tickRange = 10;
            }
            else
            {
                tickRange = 10;
            }
            LogMessage(DEBUG_ALL, "outputNode==>Lock");
        }
    }
    headNode = nextnode;

}

/**************************************************
@bref		Ä£×éÖ¸Áî·¢ËÍ
@param
@return
@note
**************************************************/

uint8_t  sendModuleCmd(uint8_t cmd, char *param)
{
    uint8_t i;
    int16_t cmdtype = -1;
    char sendData[256];
    for (i = 0; i < sizeof(cmdtable) / sizeof(cmdtable[0]); i++)
    {
        if (cmd == cmdtable[i].cmd_type)
        {
            cmdtype = i;
            break;
        }
    }
    if (cmdtype < 0)
    {
        snprintf(sendData, 255, "sendModuleCmd==>No cmd");
        LogMessage(DEBUG_ALL, sendData);
        return 0;
    }
    if (param != NULL && strlen(param) <= 240)
    {
        if (param[0] == '?')
        {
            snprintf(sendData, 255, "%s?\r\n", cmdtable[cmdtype].cmd);

        }
        else
        {
            snprintf(sendData, 255, "%s=%s\r\n", cmdtable[cmdtype].cmd, param);
        }
    }
    else if (param == NULL)
    {
        snprintf(sendData, 255, "%s\r\n", cmdtable[cmdtype].cmd);
    }
    else
    {
        return 0;
    }
    createNode(sendData, strlen(sendData), cmd);
    return 1;
}

void sendModuleDirect(char *param)
{
    createNode(param, strlen(param), 0);
}

/**************************************************
@bref		³õÊ¼»¯Ä£¿éÏà¹ØÊ¹ÓÃ½á¹¹Ìå
@param
@return
@note
**************************************************/

static void moduleInit(void)
{
    memset(&moduleState, 0, sizeof(moduleState_s));
}


/**************************************************
@bref		°´ÏÂ¿ª»ú°´¼ü
@param
@return
@note
**************************************************/

static void modulePressPowerKey(void)
{
    PWRKEY_LOW;
    moduleState.powerState = 1;
    LogPrintf(DEBUG_ALL, "PowerOn Done");
}
/**************************************************
@bref		Ä£×é¿ª»ú
@param
@return
@note
**************************************************/

void modulePowerOn(void)
{
    LogMessage(DEBUG_ALL, "modulePowerOn");
    moduleInit();
    sysinfo.moduleRstFlag = 1;
    portUartCfg(APPUSART3, 1, 115200, moduleRecvParser);
    POWER_ON;
    PWRKEY_HIGH;
    RSTKEY_HIGH;
    startTimer(6, modulePressPowerKey, 0);
    moduleState.gpsFileHandle = 1;
    moduleCtrl.scanMode = 0;
    
    socketDelAll();
}

/**************************************************
@bref		Ä£×é¹Ø»ú
@param
@return
@note
**************************************************/

void modulePowerOff(void)
{
    LogMessage(DEBUG_ALL, "modulePowerOff");
    moduleInit();
    portUartCfg(APPUSART3, 0, 115200, NULL);
    POWER_OFF;
    PWRKEY_HIGH;
    RSTKEY_HIGH;
    sysinfo.moduleRstFlag = 1;
    socketDelAll();
    moduleRspSuccess();
    hbtRspSuccess();
}


/**************************************************
@bref		Ä£×é¸´Î»
@param
@return
@note
**************************************************/

void moduleReset(void)
{
    LogMessage(DEBUG_ALL, "moduleReset");
    moduleInit();
    POWER_OFF;
    startTimer(20, modulePowerOn, 0);
    socketDelAll();
    moduleRspSuccess();
    hbtRspSuccess();
}

/**************************************************
@bref		ÇÐ»»ÁªÍø×´Ì¬»ú
@param
@return
@note
**************************************************/
static void changeProcess(uint8_t fsm)
{
    moduleState.fsmState = fsm;
    moduleState.fsmtick = 0;
    LogPrintf(DEBUG_ALL, "changeProcess==>%d", fsm);
    if (moduleState.fsmState != NORMAL_STATUS)
    {
        ledStatusUpdate(SYSTEM_LED_NETOK, 0);
    }
}

/**************************************************
@bref		´´½¨socket
@param
@return
@note
**************************************************/

void openSocket(uint8_t link, char *server, uint16_t port)
{
    char param[100];
    sprintf(param, "%d,%s,%d", link, server, port);
    sendModuleCmd(TCPSETUP_CMD, param);
}

/**************************************************
@bref		¹Ø±Õsocket
@param
@return
@note
**************************************************/

void closeSocket(uint8_t link)
{
    char param[10];
    sprintf(param, "%d", link);
    sendModuleCmd(AT_CMD, NULL);
    sendModuleCmd(TCPCLOSE_CMD, param);
}

/**************************************************
@bref		apnÅäÖÃ
@param
@return
@note
**************************************************/

static void netSetCgdcong(char *apn)
{
    char param[100]={0};
    sprintf(param, "1,\"IP\",\"%s\"", apn);
    sendModuleCmd(CGDCONT_CMD, param);
}

/**************************************************
@bref		apnÅäÖÃ
@param
@return
@note
**************************************************/

static void netSetApn(char *apn, char *apnname, char *apnpassword, uint8_t apnauthport)
{
    char param[100]={0};
    sprintf(param, "1,%d,\"%s\",\"%s\"", apnauthport, apnname, apnpassword,apnauthport);
    sendModuleCmd(XGAUTH_CMD, param);
}

/**************************************************
@bref		ÇÐ»»SIM
@param
@return
@note
**************************************************/

void netSetSim(uint8_t sim)
{
	char param[10] = {0};
	sprintf(param, "%d", sim);
	sendModuleCmd(AT_CMD, NULL);
	sendModuleCmd(SIMCROSS_CMD, param);
	dynamicParam.sim = sim;
	LogPrintf(DEBUG_ALL, "netSetSim==>Sim%d", dynamicParam.sim);
	dynamicParamSaveAll();
}

/**************************************************
@bref		Ä£×é½øÈë·ÉÐÐÄ£Ê½
@param
@return
@note
**************************************************/

static void moduleEnterFly(void)
{
    sendModuleCmd(CFUN_CMD, "4,0");
}

/**************************************************
@bref		Ä£×é½øÈëÕý³£Ä£Ê½
@param
@return
@note
**************************************************/

static void moduleExitFly(void)
{
    sendModuleCmd(CFUN_CMD, "1,0");
}

/**************************************************
@bref		·¢ËÍsocket¶ÁÈ¡»º´æÖ¸Áî
@param
@return
@note
**************************************************/

static void qirdCmdSend(uint8_t link)
{
    char param[10];
	sprintf(param, "%d,768", link);
	moduleState.curQirdId = link;
    sendModuleCmd(TCPREAD_CMD, param);
}

/**************************************************
@bref		¶ÁÈ¡»º´æ
@param
@return
@note
**************************************************/

static void queryRecvBuffer(void)
{
    if (moduleState.normalLinkQird)
    {
        qirdCmdSend(NORMAL_LINK);
    }
    else if (moduleState.agpsLinkQird)
    {
        qirdCmdSend(AGPS_LINK);
    }
    else if (moduleState.bleLinkQird)
    {
        qirdCmdSend(BLE_LINK);
    }
    else if (moduleState.jt808LinkQird)
    {
        qirdCmdSend(JT808_LINK);
    }
    else if (moduleState.hideLinkQird)
    {
        qirdCmdSend(HIDDEN_LINK);
    }
    else if (moduleState.upgradeQird)
    {
		qirdCmdSend(UPGRADE_LINK);
    }
}

/**************************************************
@bref		²éÑ¯Á´Â·×´Ì¬
@param
@return
@note
**************************************************/
static void querySocketStatus(uint8_t link)
{
	char param[10]={0};
	sprintf(param, "%d", link);
	sendModuleCmd(IPSTATUS_CMD, param);
}

/**************************************************
@bref		ÁªÍø×¼±¸ÈÎÎñ
@param
@return
@note
**************************************************/

void netConnectTask(void)
{
    if (moduleState.powerState == 0)
    {
        return;
    }

    moduleState.powerOnTick++;
    switch (moduleState.fsmState)
    {
        case AT_STATUS:
            if (moduleState.atResponOK)
            {
                moduleCtrl.atCount = 0;
                moduleState.atResponOK = 0;
                moduleState.cpinResponOk = 0;
                sendModuleCmd(CGSN_CMD, NULL);
                sendModuleCmd(SIMCROSS_CMD, "?");
                changeProcess(CPIN_STATUS);
                break;
            }
            else
            {
                if (moduleState.fsmtick % 2 == 0)
                {
                    moduleState.powerOnTick = 0;
                    sendModuleCmd(AT_CMD, NULL);
                }
                if (moduleState.fsmtick >= 30)
                {
                    moduleCtrl.atCount++;
                    if (moduleCtrl.atCount >= 2)
                    {
                        moduleCtrl.atCount = 0;
                        modulePowerOn();
                    }
                    else
                    {
                        moduleReset();
                    }
                }
                break;
            }
        case CPIN_STATUS:
            if (moduleState.cpinResponOk)
            {
                moduleState.cpinResponOk = 0;
                moduleState.csqOk = 0;
                moduleCtrl.cpincount = 0;
                netSetCgdcong((char *)sysparam.apn);
                netSetApn(sysparam.apn, sysparam.apnuser, sysparam.apnpassword, sysparam.apnAuthPort);
                changeProcess(CSQ_STATUS);

            }
            else
            {
                if (moduleState.fsmtick % 2 == 0)
                {
                    sendModuleCmd(CPIN_CMD, "?");
                }
                if (moduleState.fsmtick >= 30)
                {
                	changeProcess(AT_STATUS);
                	if (sysparam.simSel == SIM_MODE_1)
                	{
						if (dynamicParam.sim == SIM_1)
						{
							netSetSim(SIM_2);
						}
						else if (dynamicParam.sim == SIM_2)
						{
							netSetSim(SIM_1);
						}
                	}
                    startTimer(10, moduleReset, 0);
                    
                }
                break;
            }
        case CSQ_STATUS:
            if (moduleState.csqOk)
            {
                moduleState.csqOk = 0;
                moduleState.cgregOK = 0;
                moduleCtrl.csqCount = 0;
                sendModuleCmd(CMGF_CMD, "1");
                sendModuleCmd(CNMI_CMD, "0,1");
                changeProcess(CGREG_STATUS);
                netResetCsqSearch();
            }
            else
            {
                sendModuleCmd(CSQ_CMD, NULL);
                if (moduleCtrl.csqTime == 0)
                {
                    moduleCtrl.csqTime = 90;
                }
                if (moduleState.fsmtick >= 90)
                {
                    moduleCtrl.csqCount++;
                    if (moduleCtrl.csqCount >= 2)
                    {
                        if (sysparam.simSel == SIM_MODE_1)
                        {
	                        if (dynamicParam.sim == SIM_1)
							{
								netSetSim(SIM_2);
							}
							else if (dynamicParam.sim == SIM_2)
							{
								netSetSim(SIM_1);
							}
						}
						startTimer(10, moduleReset, 0);
						changeProcess(AT_STATUS);
					}
                    else if (moduleCtrl.csqCount >= 4)
                    {
                        moduleCtrl.csqCount = 0;
                        if (sysparam.simSel == SIM_MODE_1)
                        {
	                        if (dynamicParam.sim == SIM_1)
							{
								netSetSim(SIM_2);
							}
							else if (dynamicParam.sim == SIM_2)
							{
								netSetSim(SIM_1);
							}
						}
                        changeProcess(AT_STATUS);
                        //2´Î×¢²á²»ÉÏ»ùÕ¾Ê±£¬Èç¹ûÃ»ÓÐgpsÇëÇó£¬Ôò¹Ø»ú
                        if (sysinfo.gpsRequest == 0)
                        {
                            startTimer(30, modeTryToStop, 0);
                        }
                        else
                        {
                            startTimer(10, moduleReset, 0);
                        }
                    }
                    else
                    {
                        moduleEnterFly();
                        startTimer(80, moduleExitFly, 0);
                    }
                    changeProcess(AT_STATUS);
                }
                break;
            }
        case CGREG_STATUS:
            if (moduleState.cgregOK)
            {
                moduleCtrl.cgregCount = 0;
                moduleState.cgregOK = 0;
                changeProcess(CONFIG_STATUS);
            }
            else
            {
                sendModuleCmd(CREG_CMD, "?");
                if (moduleState.fsmtick >= 90)
                {
                    moduleCtrl.cgregCount++;
                    if (moduleCtrl.cgregCount >= 2)
                    {
                        if (sysparam.simSel == SIM_MODE_1)
                        {
	                        if (dynamicParam.sim == SIM_1)
							{
								netSetSim(SIM_2);
							}
							else if (dynamicParam.sim == SIM_2)
							{
								netSetSim(SIM_1);
							}
						}
						startTimer(10, moduleReset, 0);
						changeProcess(AT_STATUS);
					}
                    else if (moduleCtrl.cgregCount >= 4)
                    {
                        moduleCtrl.cgregCount = 0;
                        if (sysparam.simSel == SIM_MODE_1)
                        {
	                        if (dynamicParam.sim == SIM_1)
							{
								netSetSim(SIM_2);
							}
							else if (dynamicParam.sim == SIM_2)
							{
								netSetSim(SIM_1);
							}
						}
                        //2´Î×¢²á²»ÉÏ»ùÕ¾Ê±£¬Èç¹ûÃ»ÓÐgpsÇëÇó£¬Ôò¹Ø»ú
                        if (sysinfo.gpsRequest == 0)
                        {
                            startTimer(30, modeTryToStop, 0);
                        }
                        else
                        {
                            startTimer(10, moduleReset, 0);
                        }
						changeProcess(AT_STATUS);
                        LogMessage(DEBUG_ALL, "Register timeout");
                    }
                    else
                    {
                        sendModuleCmd(CGATT_CMD, "1");
                        changeProcess(AT_STATUS);
                    }
                }
                break;
            }
        case CONFIG_STATUS:
			sendModuleCmd(CCID_CMD, NULL);
			sendModuleCmd(CIMI_CMD, NULL);
			sendModuleCmd(CGSN_CMD, NULL);
			sendModuleCmd(FSLIST_CMD, "?");
			sendModuleCmd(RECVMODE_CMD, "0");
			sendModuleCmd(CLIP_CMD, "1");
			sendModuleCmd(MICL_CMD, "9");
			moduleGetLbs();
			queryBatVoltage();
            changeProcess(QIACT_STATUS);
            break;
        case QIACT_STATUS:
            if (moduleState.qipactOk)
            {
                moduleState.qipactOk = 0;
                moduleCtrl.qipactCount = 0;
                changeProcess(XIIC_STATUS);
                
            }
            else
            {
                if (moduleState.qipactSet == 0)
                {
                    moduleState.qipactSet = 1;
                    sendModuleCmd(CGATT_CMD, "1");
                }
                else
                {
                    sendModuleCmd(CGATT_CMD, "?");
                }
                if (moduleState.fsmtick >= 45)
                {
                    LogMessage(DEBUG_ALL, "try QIPACT again");
                    moduleState.qipactSet = 0;
                    moduleState.fsmtick = 0;
                    moduleCtrl.qipactCount++;
                    if (moduleCtrl.qipactCount >= 3)
                    {
                        moduleCtrl.qipactCount = 0;
                        moduleReset();
                    }
                    else
                    {
                        changeProcess(CPIN_STATUS);
                    }
                }
                break;
            }
        case XIIC_STATUS:
			if (moduleState.xiicOk)
			{
				moduleState.xiicOk = 0;
				moduleState.xiicSet = 0;
				moduleCtrl.xiicCount = 0;
				changeProcess(NORMAL_STATUS);
			}
			else
			{
				if (moduleState.xiicSet == 0)
				{
					moduleState.xiicSet = 1;
					sendModuleCmd(XIIC_CMD, "1");
				}
				else
				{
					sendModuleCmd(XIIC_CMD, "?");
					if (moduleState.fsmtick >= 30)
					{
						moduleState.fsmtick = 0;
						moduleState.xiicSet = 0;
						moduleCtrl.xiicCount++;
						if (moduleCtrl.xiicCount >= 3)
						{
							moduleReset();
							moduleCtrl.xiicCount = 0;
						}
					}
				}
			}
        	break;
        case NORMAL_STATUS:
            socketSchedule();
            queryRecvBuffer();
            break;
        default:
            changeProcess(AT_STATUS);
            break;
    }
    moduleState.fsmtick++;
}


/**************************************************
@bref		AT+CSQ	Ö¸Áî½âÎö
@param
@return
@note
**************************************************/

static void csqParser(uint8_t *buf, uint16_t len)
{
    int index, indexa, datalen;
    uint8_t *rebuf;
    uint16_t  relen;
    char restore[5];
    index = my_getstrindex((char *)buf, "+CSQ:", len);
    if (index >= 0)
    {
        rebuf = buf + index;
        relen = len - index;
        indexa = getCharIndex(rebuf, relen, ',');
        if (indexa > 6)
        {
            datalen = indexa - 6;
            if (datalen > 5)
                return;
            memset(restore, 0, 5);
            strncpy(restore, (char *)rebuf + 6, datalen);
            moduleState.rssi = atoi(restore);
            if (moduleState.rssi >= 6 && moduleState.rssi <= 31)
                moduleState.csqOk = 1;
        }
    }
}

/**************************************************
@bref		AT+CREG	Ö¸Áî½âÎö
@param
@return
@note
**************************************************/

static void cgregParser(uint8_t *buf, uint16_t len)
{
    int index, datalen;
    uint8_t *rebuf;
    uint16_t  relen, i;
    char restore[50];
    uint8_t cnt;
    uint8_t type = 0;
    index = my_getstrindex((char *)buf, "+CREG:", len);
    if (index >= 0)
    {
        rebuf = buf + index;
        relen = len - index;
        datalen = 0;
        cnt = 0;
        restore[0] = 0;
        for (i = 0; i < relen; i++)
        {
            if (rebuf[i] == ',' || rebuf[i] == '\r' || rebuf[i] == '\n')
            {
                if (restore[0] != 0)
                {
                    restore[datalen] = 0;
                    cnt++;
                    datalen = 0;
                    switch (cnt)
                    {
                        case 2:
                            if (restore[0] == '1' || restore[0] == '5')
                            {
                                if (type)
                                {
                                    moduleState.ceregOK = 1;
                                }
                                else
                                {
                                    moduleState.cgregOK = 1;
                                }
                            }
                            else
                            {
                                return ;
                            }
                            break;
                    }
                    restore[0] = 0;
                }
            }
            else
            {
                restore[datalen] = rebuf[i];
                datalen++;
                if (datalen >= 50)
                {
                    return ;
                }

            }
        }
    }
}

/**************************************************
@bref		$MYLACID	Ö¸Áî½âÎö
@param
@return
@note
$MYLACID: 758A,0D6241F2

**************************************************/
void mylacidParser(uint8_t *buf, uint16_t len)
{
	int index;
	uint8_t *rebuf;
	uint16_t relen;
	char restore[50];
	rebuf = buf;
	relen = len;
	index = my_getstrindex((char *)rebuf, "$MYLACID:", relen);
	if (index < 0)
		return;
	rebuf += index + 10;
	relen -= index + 10;
	index = getCharIndex(rebuf, relen, ',');
	if (index < 0)
		return;
	memcpy(restore, rebuf, index);
	restore[index] = 0;
	moduleState.lac = strtoul(restore, NULL, 16);
	LogPrintf(DEBUG_ALL, "LAC=%s,0x%X\n", restore, moduleState.lac);
	rebuf += index + 1;
	relen -= index + 1;
	index = getCharIndex(rebuf, relen, '\r');
	if (index < 0)
		return;
	memcpy(restore, rebuf, index);
	restore[index] = 0;
	moduleState.cid = strtoul(restore, NULL, 16);
	LogPrintf(DEBUG_ALL, "CID=%s,0x%X\n", restore, moduleState.cid);
}

/**************************************************
@bref		AT+NETDMSG	Ö¸Áî½âÎö
@param
@return
@note
+NETDMSG: "460+01", 0x758A, 0x06040021, 122, "FDD LTE", LTE BAND 3, 1506, -72, 23, 0x0, 0x0, 0x0, -1020, -135, 115

**************************************************/
static void netdmsgParser(uint8_t *buf, uint16_t len)
{
	int index, indexa;
	uint8_t *rebuf;
	uint16_t relen;
	char restore[10];

	index = my_getstrindex(buf, "+NETDMSG:", len);
	if (index >= 0)
	{
		index = getCharIndexWithNum(buf, len, '"', 1);
		if (index >= 0)
		{
			indexa = getCharIndexWithNum(buf, len, '"', 2);
			if ((indexa - index) > 2 && (indexa - index) <= 10)
			{
				rebuf = buf + index + 1;
				relen = len - index - 1;
				index = getCharIndex(rebuf, relen, '+');
				memcpy(restore, rebuf, index);
				restore[index] = 0;
				moduleState.mcc = atoi(restore);
				rebuf += index + 1;
				relen -= index + 1;
				index = getCharIndex(rebuf, relen, '"');
				memcpy(restore, rebuf, index);
				restore[index] = 0;
				moduleState.mnc = atoi(restore);
				LogPrintf(DEBUG_ALL, "MCC=%d,MNC=%d\n", moduleState.mcc, moduleState.mnc);
			}
		}
	}
}


/**************************************************
@bref		AT+CIMI	Ö¸Áî½âÎö
@param
@return
@note
+CIMI: 460064814034211

**************************************************/

static void cimiParser(uint8_t *buf, uint16_t len)
{
    int16_t index;
    uint8_t *rebuf;
    uint16_t  relen;
    uint8_t i;
    rebuf = buf;
    relen = len;
    index = my_getstrindex(rebuf, "+CIMI:", relen);
    if (index < 0)
    {
        return;
    }
    rebuf += index + 7;
    relen -= index + 7;
    index = getCharIndex(rebuf, relen, '\r');
    if (index == 15)
    {
        for (i = 0; i < index; i++)
        {
            moduleState.IMSI[i] = rebuf[i];
        }
        moduleState.IMSI[index] = 0;
        LogPrintf(DEBUG_ALL, "IMSI:%s", moduleState.IMSI);
    }
}


/**************************************************
@bref		AT+ICCID	Ö¸Áî½âÎö
@param
@return
@note
+CCID: 89860620040007641589

**************************************************/

static void iccidParser(uint8_t *buf, uint16_t len)
{
    int16_t index;
    uint8_t *rebuf;
    uint16_t  relen;
    uint8_t i;
    char debug[70];
    index = my_getstrindex((char *)buf, "+CCID:", len);
    if (index >= 0)
    {
        rebuf = buf + index + 7;
        relen = len - index - 7;
        index = getCharIndex(rebuf, relen, '\r');
        if (index >= 0)
        {
            if (index == 20)
            {
                for (i = 0; i < index; i++)
                {
                    moduleState.ICCID[i] = rebuf[i];
                }
                moduleState.ICCID[index] = 0;
                sprintf(debug, "ICCID:%s", moduleState.ICCID);
                LogMessage(DEBUG_ALL, debug);
            }
        }
    }

}


/**************************************************
@bref		socket»º´æÊý¾ÝÉÏ±¨½âÎö
@param
@return
@note
+TCPRECV: 0

**************************************************/

static void tcprecvParser(uint8_t *buf, uint16_t len)
{
    char *rebuf;
    int index, relen;
    int sockId;
    char restore[10];
    rebuf = buf;
    relen = len;
    index = my_getstrindex((char *)rebuf, "+TCPRECV:", relen);
    if (index < 0)
        return;
    while (index >= 0)
    {
		rebuf += index + 10;
		relen -= index + 10;
		index = getCharIndex(rebuf, relen, '\r');
		if (index >= 0 && index <= 2)
		{
			memcpy(restore, rebuf, index);
			restore[index] = 0;
			sockId = atoi(restore);
			LogPrintf(DEBUG_ALL, "Socket [%d] recv data", sockId);
			switch (sockId)
			{
				case NORMAL_LINK:
					moduleState.normalLinkQird = 1;
					break;
				case BLE_LINK:
					moduleState.bleLinkQird = 1;
					break;
				case JT808_LINK:
					moduleState.jt808LinkQird = 1;
					break;
				case HIDDEN_LINK:
					moduleState.hideLinkQird = 1;
					break;
				case AGPS_LINK:
					moduleState.agpsLinkQird = 1;
					break;
				case UPGRADE_LINK:
					moduleState.upgradeQird = 1;
					break;
			}
			rebuf += index + 1;
			relen -= index + 1;
		}
		index = my_getstrindex((char *)rebuf, "+TCPRECV:", relen);
 	}
}
/**************************************************
@bref		socketÊý¾Ý½âÎö
@param
@return
@note
+TCPREAD: 0,46,xx\0ÙÜ
xxñ\0v?
xx?
;\0P?
xx‹\0àn

+TCPREAD: 0,0

**************************************************/

static void tcpreadParser(uint8_t *buf, uint16_t len)
{
	int index;
	uint8_t *rebuf;
	uint16_t relen, recvlen;
	int sockId;
	char restore[10];
	rebuf = buf;
	relen = len;
	index = my_getstrindex(rebuf, "+TCPREAD:", relen);

	while (index >= 0)
	{
		rebuf += index + 9;
		relen -= index + 9;
		index = getCharIndex(rebuf, relen, ',');
		if (index < 0)
			return;
		memcpy(restore, rebuf, index);
		restore[index] = 0;
		sockId = atoi(restore);
		rebuf += index + 1;
		relen -= index + 1;
		index = getCharIndex(rebuf, relen, ',');
		//+TCPREAD: 0,0
		if (index < 0)
		{
			index = getCharIndex(rebuf, relen, '\r');
		}
		if (index >= 0)
		{
			memcpy(restore, rebuf, index);
			restore[index] = 0;
			recvlen = atoi(restore);
			
		}
		if (recvlen != 0)
		{
			rebuf += index + 1;
			relen -= index + 1;
			/* 
				Ä£×é¿ÉÄÜ»á·µ»Ø±È²¥±¨¶ÌµÄÊý¾Ý»ØÀ´£¬Èç£º
				+TCPREAD: 0,46,xx\0\0ÈU
				xxñ\0gs
				Êµ¼ÊÓ¦¸Ã·µ»Ø£º
				+TCPREAD: 0,46,xx\0ÙÜ
				xxñ\0v?
				xx?
				;\0P?
				xx‹\0àn
				ÉÙÁËÁ½ÌõÊý¾Ý£¬relen -= recvlen;»áµ¼ÖÂrelen²»¹»¼õ,È¥µ½60000+
				ËùÒÔÕâÀïÒªÇó³öÊµ¼ÊÊ£ÓàµÄ³¤¶È
			*/
			//char debug[769] = { 0 };
			recvlen = relen >= recvlen ? recvlen : relen;
			LogPrintf(DEBUG_ALL, "Socket [%d] Recv %d bytes:", sockId, recvlen);
			
			socketRecv(sockId, rebuf, recvlen);
//			recvlen = recvlen > 384 ? 38 4 : recvlen;
//			byteToHexString(rebuf, debug, recvlen);
//			debug[recvlen * 2] = 0;
//			LogMessageWL(DEBUG_ALL, debug, recvlen * 2);

			rebuf += index;
			relen -= index;
		}
		else 
		{
			switch (sockId)
			{
				case NORMAL_LINK:
					moduleState.normalLinkQird = 0;
					break;
				case BLE_LINK:
					moduleState.bleLinkQird    = 0;
					break;
				case JT808_LINK:
					moduleState.jt808LinkQird  = 0;
					break;
				case HIDDEN_LINK:
					moduleState.hideLinkQird   = 0;
					break;
				case AGPS_LINK:
					moduleState.agpsLinkQird   = 0;
					break;
				case UPGRADE_LINK:
					moduleState.upgradeQird    = 0;
					break;
			}
			LogPrintf(DEBUG_ALL, "Socket [%d] Recv done", sockId);
		}
		index = my_getstrindex(rebuf, "+TCPREAD:", relen);
	}
}


/**************************************************
@bref		¶ÌÐÅ½ÓÊÕ
@param
@return
@note
**************************************************/


static void cmtiParser(uint8_t *buf, uint16_t len)
{
    uint8_t i;
    int16_t index;
    uint8_t *rebuf;
    char restore[5];
    index = my_getstrindex((char *)buf, "+CMTI:", len);
    if (index >= 0)
    {
        rebuf = buf + index;
        index = getCharIndex(rebuf, len, ',');
        if (index < 0)
            return;
        rebuf = rebuf + index + 1;
        index = getCharIndex(rebuf, len, '\r');
        if (index > 5 || index < 0)
            return ;
        for (i = 0; i < index; i++)
        {
            restore[i] = rebuf[i];
        }
        restore[index] = 0;
        LogPrintf(DEBUG_ALL, "Message index=%d", atoi(restore));
        sendModuleCmd(CMGR_CMD, restore);
    }
}

/**************************************************
@bref		CMGR	Ö¸Áî½âÎö
@param
@return
@note
**************************************************/

static void cmgrParser(uint8_t *buf, uint16_t len)
{
    int index;
    uint8_t *rebuf;
    uint8_t *numbuf;
    uint16_t  relen, i, renumlen;
    char restore[100];
    insParam_s insparam;
    //ÕÒµ½ÌØ¶¨×Ö·û´®ÔÚbufµÄÎ»ÖÃ
    index = my_getstrindex((char *)buf, "+CMGR:", len);
    if (index >= 0)
    {
        //µÃµ½ÌØ¶¨×Ö·û´®µÄ¿ªÊ¼Î»ÖÃºÍÊ£Óà³¤¶È
        rebuf = buf + index;
        relen = len - index;
        //Ê¶±ðÊÖ»úºÅÂë
        index = getCharIndexWithNum(rebuf, relen, '"', 3);
        if (index < 0)
            return;
        numbuf = rebuf + index + 1;
        renumlen = relen - index - 1;
        index = getCharIndex(numbuf, renumlen, '"');
        if (index > 100 || index < 0)
            return ;
        for (i = 0; i < index; i++)
        {
            restore[i] = numbuf[i];
        }
        restore[index] = 0;

        if (index > sizeof(moduleState.messagePhone))
            return ;
        strcpy((char *)moduleState.messagePhone, restore);
        LogPrintf(DEBUG_ALL, "Tel:%s", moduleState.messagePhone);
        //µÃµ½µÚÒ»¸ö\nµÄÎ»ÖÃ
        index = getCharIndex(rebuf, len, '\n');
        if (index < 0)
            return;
        //Æ«ÒÆµ½ÄÚÈÝ´¦
        rebuf = rebuf + index + 2;
        //µÃµ½´ÓÄÚÈÝ´¦¿ªÊ¼µÄµÚÒ»¸ö\n£¬²âÊÔindex¾ÍÊÇÄÚÈÝ³¤¶È
        index = getCharIndex(rebuf, len, '"');
        if (index > 100 || index < 0)
            return ;
        for (i = 0; i < index; i++)
        {
            restore[i] = rebuf[i];
        }
        restore[index] = 0;
        LogPrintf(DEBUG_ALL, "Message:%s", restore);
        insparam.telNum = moduleState.messagePhone;
        lastparam.telNum = moduleState.messagePhone;
        instructionParser((uint8_t *)restore, index, SMS_MODE, &insparam);
    }
}

/**************************************************
@bref		AT+QISEND	Ö¸Áî½âÎö
@param
@return
@note
+QISEND: 212,212,0

**************************************************/

static void qisendParser(uint8_t *buf, uint16_t len)
{
    int index;
    uint8_t *rebuf;
    uint8_t i, datalen, cnt, sockId;
    uint16_t relen;
    char restore[51];

    index = my_getstrindex((char *)buf, "ERROR", len);
    if (index >= 0)
    {
        //²»ÄÜºÜºÃÇø·Öµ½µ×ÊÇÄÄÌõÁ´Â·³öÏÖ´íÎó
        socketResetConnState();
        moduleSleepCtl(0);
        changeProcess(CGREG_STATUS);
        return ;
    }
    index = my_getstrindex((char *)buf, "+QISEND:", len);
    if (index < 0)
    {
        return ;
    }

    rebuf = buf + index + 9;
    relen = len - index - 9;
    index = getCharIndex(rebuf, relen, '\n');
    datalen = 0;
    cnt = 0;
    if (index > 0 && index < 50)
    {
        restore[0] = 0;
        for (i = 0; i < index; i++)
        {
            if (rebuf[i] == ',' || rebuf[i] == '\r' || rebuf[i] == '\n')
            {
                if (restore[0] != 0)
                {
                    restore[datalen] = 0;
                    cnt++;
                    datalen = 0;
                    switch (cnt)
                    {
                        case 1:
                            moduleState.tcpTotal = atoi(restore);
                            break;
                        case 2:
                            moduleState.tcpAck = atoi(restore);
                            break;
                        case 3:
                            moduleState.tcpNack = atoi(restore);
                            break;
                    }
                    restore[0] = 0;
                }
            }
            else
            {
                restore[datalen] = rebuf[i];
                datalen++;
                if (datalen >= 50)
                {
                    return ;
                }

            }
        }
    }
}





/**************************************************
@bref		WIFIAPSCAN	Ö¸Áî½âÎö
@param
@return
@note
+WIFIAPSCAN: ec41180c8209,-45,8
+WIFIAPSCAN: f88c21a2c6e9,-70,4
+WIFIAPSCAN: 4022301af801,-66,9
+WIFIAPSCAN: dc9fdb1c1d76,-65,11
+WIFIAPSCAN: 086bd10b5060,-73,11
+WIFIAPSCAN: 500ff5511aeb,-57,13
+WIFIAPSCAN: accb51ae14e1,-71,7


OK
**************************************************/
static void wifiapscanParser(uint8_t *buf, uint16_t len)
{
    int index;
    uint8_t *rebuf, i;
    int16_t relen;
    char restore[20];
    WIFIINFO wifiList;
    rebuf = buf;
    relen = len;
    index = my_getstrindex((char *)rebuf, "+WIFIAPSCAN:", relen);
    wifiList.apcount = 0;
    while (index >= 0)
    {
        rebuf += index + 13;
        relen -= index + 13;

        index = getCharIndex(rebuf, relen, ',');
        if (index == 12 && wifiList.apcount < WIFIINFOMAX)
        {
            memcpy(restore, rebuf, index);
            restore[12] = 0;
            LogPrintf(DEBUG_ALL, "WIFI:[%s]", restore);

            wifiList.ap[wifiList.apcount].signal = 0;
            changeHexStringToByteArray(wifiList.ap[wifiList.apcount].ssid, restore, 6);
            wifiList.apcount++;
        }
        index = getCharIndex(rebuf, relen, '\n');
        rebuf += index;
        relen -= index;

        index = my_getstrindex((char *)rebuf, "+WIFIAPSCAN:", relen);
    }
	if (wifiList.apcount != 0)
    {
    	if (wifiList.apcount < 4 && sysinfo.wifiExtendEvt != 0)
		{
			lbsRequestSet(DEV_EXTEND_OF_MY);
		}
		else
		{
			if (sysinfo.wifiExtendEvt & DEV_EXTEND_OF_MY)
		    {
		    	jt808UpdateWifiinfo(&wifiList);
		        protocolSend(NORMAL_LINK, PROTOCOL_F3, &wifiList);
		        jt808SendToServer(TERMINAL_POSITION, getCurrentGPSInfo());
		    }
		    if (sysinfo.wifiExtendEvt & DEV_EXTEND_OF_BLE)
		    {
		        protocolSend(BLE_LINK, PROTOCOL_F3, &wifiList);
		    }
		    lbsRequestClear();
		}
    	wifiRspSuccess();
        sysinfo.wifiExtendEvt = 0;
    }
}

/**************************************************
@bref		AT+CGSN	Ö¸Áî½âÎö
@param
@return
@note
+CGSN: 864161050720320

**************************************************/

static void cgsnParser(uint8_t *buf, uint16_t len)
{
    int16_t index;
    uint8_t *rebuf;
    uint16_t  relen;
    uint8_t i;
    rebuf = buf;
    relen = len;
    
    index = my_getstrindex((char *)rebuf, "+CGSN:", relen);
    rebuf += index + 7;
    relen -= index + 7;
    
    index = getCharIndex(rebuf, relen, '\r');
    if (index >= 0 && index < 20)
    {
        for (i = 0; i < index; i++)
        {
            moduleState.IMEI[i] = rebuf[i];
        }
        moduleState.IMEI[index] = 0;
        LogPrintf(DEBUG_ALL, "module IMEI [%s]", moduleState.IMEI);
        if (tmos_memcmp(moduleState.IMEI, dynamicParam.SN, 15) == FALSE)
        {
            tmos_memset(dynamicParam.SN, 0, sizeof(dynamicParam.SN));
            strncpy(dynamicParam.SN, moduleState.IMEI, 15);
            jt808CreateSn(dynamicParam.jt808sn, dynamicParam.SN + 3, 12);
            dynamicParam.jt808isRegister = 0;
            dynamicParam.jt808AuthLen = 0;
            dynamicParamSaveAll();
        }
    }

}


/**************************************************
@bref		+CGATT	Ö¸Áî½âÎö
@param
@return
@note
	+CGATT: 1

	OK
**************************************************/

static void cgattParser(uint8_t *buf, uint16_t len)
{
    uint8_t ret;
    int index;
    uint8_t *rebuf;
    int16_t relen;
    rebuf = buf;
    relen = len;
    index = my_getstrindex((char *)rebuf, "+CGATT:", relen);
    if (index < 0)
    {
        return;
    }
    rebuf += index;
    relen -= index;
    ret = rebuf[8] - '0';
    if (ret == 1)
    {
        moduleState.qipactOk = 1;
    }
    else
    {
        moduleState.qipactOk = 0;
    }
}

/**************************************************
@bref		+XIIC	Ö¸Áî½âÎö
@param
@return
@note
+XIIC:    1,10.107.216.162
+XIIC:    0,0.0.0.0

**************************************************/
void xiicParser(uint8_t *buf, uint16_t len)
{
	uint8_t ret;
	int index;
	uint8_t *rebuf;
	uint16_t relen;
	char restore[50] = {0};

	rebuf = buf;
	relen = len;
	index = my_getstrindex(rebuf, "+XIIC:", relen);
	if (index < 0)
		return;
	rebuf += index + 10;
	relen -= index + 10;
	index = getCharIndex(rebuf, relen, ',');
	if (index >= 0 && index <= 2)
	{
		memcpy(restore, rebuf, index);
		restore[index] = 0;
		ret = atoi(restore);
		if (ret != 0)
		{
			moduleState.xiicOk = 1;
		}
	}
}


/**************************************************
@bref		CBC	Ö¸Áî½âÎö
@param
@return
@note
	+CBC: 0,80,3909

	OK
**************************************************/

void cbcParser(uint8_t *buf, uint16_t len)
{
    char *rebuf;
    int index, relen;
    ITEM item;
    rebuf = buf;
    relen = len;
    index = my_getstrindex((char *)rebuf, "+CBC:", relen);
    if (index < 0)
    {
        return;
    }
    rebuf += index + 6;
    relen -= index + 6;
    stringToItem(&item, rebuf, relen);
    if (item.item_cnt >= 3)
    {
        sysinfo.insidevoltage = atoi(item.item_data[2]) / 1000.0;
        LogPrintf(DEBUG_ALL, "batttery voltage %.2f", sysinfo.insidevoltage);
    }
}


/**************************************************
@bref		PDP	Ö¸Áî½âÎö
@param
@return
@note
+PDP DEACT


**************************************************/
void gprsDisconnectionParser(uint8_t *buf, uint16_t len)
{
    int index;
    index = my_getstrindex(buf, "GPRS DISCONNECTION", len);
    if (index < 0)
    {
        return;
    }
    socketDelAll();
    changeProcess(CPIN_STATUS);
}


/**************************************************
@bref		CMT	Ö¸Áî½âÎö
@param
@return
@note
+CMT: "1064899195049",,"2022/09/05 17:46:53+32"
PARAM

**************************************************/

void cmtParser(uint8_t *buf, uint16_t len)
{
    uint8_t *rebuf;
    char restore[130];
    int relen;
    int index;
    insParam_s insparam;
    rebuf = buf;
    relen = len;
    index = my_getstrindex(rebuf, "+CMT:", relen);
    if (index < 0)
    {
        return;
    }
    rebuf += index;
    relen -= index;
    while (index >= 0)
    {
        rebuf += 7;
        relen -= 7;
        index = getCharIndex(rebuf, relen, '\"');
        if (index < 0 || index >= 20)
        {
            return;
        }
        memcpy(moduleState.messagePhone, rebuf, index);
        moduleState.messagePhone[index] = 0;
        LogPrintf(DEBUG_ALL, "TEL:%s", moduleState.messagePhone);
        index = getCharIndex(rebuf, relen, '\n');
        if (index < 0)
        {
            return;
        }
        rebuf += index + 1;
        relen -= index - 1;
        index = getCharIndex(rebuf, relen, '\r');
        if (index < 0 || index >= 128)
        {
            return ;
        }
        memcpy(restore, rebuf, index);
        restore[index] = 0;
        LogPrintf(DEBUG_ALL, "Content:[%s]", restore);
        insparam.telNum = moduleState.messagePhone;
        instructionParser((uint8_t *)restore, index, SMS_MODE, &insparam);
    }
}


/**************************************************
@bref		×Ô¶¯½ÓÌý
@param
@return
@note

**************************************************/

void ringParser(uint8_t *buf, uint16_t len)
{
    int index;
    index = my_getstrindex(buf, "RING", len);
    if (index < 0)
    {
        return;
    }
    sendModuleCmd(ATA_CMD, NULL);
}


/**************************************************
@bref		TCPÁ¬½Ó½âÎöÆ÷
@param
@return
@note
+TCPSETUP: 0,OK
+TCPSETUP: 0,FAIL
+TCPSETUP: ERROR

**************************************************/

void tcpsetupRspParser(uint8_t *buf, uint16_t len)
{
    int index;
    int relen;
    uint8_t *rebuf;
    rebuf = buf;
    relen = len;

    index = my_getstrindex(rebuf, "+TCPSETUP:", relen);
    if (index < 0)
    {
        return;
    }
    //Õâ¸ö¼È¿ÉÄÜÊÇ²ÎÊýÐ´´í£¬Ò²¿ÉÄÜÊÇÍø¶ÏÁË
	index = my_getstrindex(rebuf, "+TCPSETUP: ERROR", relen);
	if (index >= 0)
	{
		socketDelAll();
		changeProcess(CPIN_STATUS);
	}
    index = my_getstrindex(rebuf, "+TCPSETUP: 0,OK", relen);
    if (index >= 0)
    {
        socketSetConnState(NORMAL_LINK, SOCKET_CONN_SUCCESS);
        moduleCtrl.qiopenCount = 0;
    }
    index = my_getstrindex(rebuf, "+TCPSETUP: 1,OK", relen);
    if (index >= 0)
    {
        socketSetConnState(BLE_LINK, SOCKET_CONN_SUCCESS);
    }
    index = my_getstrindex(rebuf, "+TCPSETUP: 2,OK", relen);
    if (index >= 0)
    {
        socketSetConnState(JT808_LINK, SOCKET_CONN_SUCCESS);
        moduleCtrl.qiopenCount = 0;
    }
    index = my_getstrindex(rebuf, "+TCPSETUP: 3,OK", relen);
    if (index >= 0)
    {
        socketSetConnState(HIDDEN_LINK, SOCKET_CONN_SUCCESS);
    }
    index = my_getstrindex(rebuf, "+TCPSETUP: 4,OK", relen);
    if (index >= 0)
    {
        socketSetConnState(AGPS_LINK, SOCKET_CONN_SUCCESS);
    }
    index = my_getstrindex(rebuf, "+TCPSETUP: 5,OK", relen);
    if (index >= 0)
    {
        socketSetConnState(UPGRADE_LINK, SOCKET_CONN_SUCCESS);
    }

    index = my_getstrindex(rebuf, "+TCPSETUP: 0,FAIL", relen);
    if (index >= 0)
    {
        socketSetConnState(NORMAL_LINK, SOCKET_CONN_ERR);
        moduleCtrl.qiopenCount++;
    }
    index = my_getstrindex(rebuf, "+TCPSETUP: 1,FAIL", relen);
    if (index >= 0)
    {
        socketSetConnState(BLE_LINK, SOCKET_CONN_ERR);
    }
    index = my_getstrindex(rebuf, "+TCPSETUP: 2,FAIL", relen);
    if (index >= 0)
    {
        socketSetConnState(JT808_LINK, SOCKET_CONN_ERR);
        moduleCtrl.qiopenCount++;
    }
    index = my_getstrindex(rebuf, "+TCPSETUP: 3,FAIL", relen);
    if (index >= 0)
    {
        socketSetConnState(HIDDEN_LINK, SOCKET_CONN_ERR);
    }
    index = my_getstrindex(rebuf, "+TCPSETUP: 4,FAIL", relen);
    if (index >= 0)
    {
        socketSetConnState(AGPS_LINK, SOCKET_CONN_ERR);
    }
    index = my_getstrindex(rebuf, "+TCPSETUP: 5,FAIL", relen);
    if (index >= 0)
    {
        socketSetConnState(UPGRADE_LINK, SOCKET_CONN_ERR);
    }

    index = my_getstrindex(rebuf, "+TCPSETUP: 0,ERROR1", relen);
    if (index >= 0)
    {
        socketSetConnState(NORMAL_LINK, SOCKET_CONN_ERR);
    }
    index = my_getstrindex(rebuf, "+TCPSETUP: 1,ERROR1", relen);
    if (index >= 0)
    {
        socketSetConnState(BLE_LINK, SOCKET_CONN_ERR);
    }
    index = my_getstrindex(rebuf, "+TCPSETUP: 2,ERROR1", relen);
    if (index >= 0)
    {
        socketSetConnState(JT808_LINK, SOCKET_CONN_ERR);
    }
    index = my_getstrindex(rebuf, "+TCPSETUP: 3,ERROR1", relen);
    if (index >= 0)
    {
        socketSetConnState(HIDDEN_LINK, SOCKET_CONN_ERR);
    }
    index = my_getstrindex(rebuf, "+TCPSETUP: 4,ERROR1", relen);
    if (index >= 0)
    {
        socketSetConnState(AGPS_LINK, SOCKET_CONN_ERR);
    }
    index = my_getstrindex(rebuf, "+TCPSETUP: 5,ERROR1", relen);
    if (index >= 0)
    {
        socketSetConnState(UPGRADE_LINK, SOCKET_CONN_ERR);
    }
	if (moduleCtrl.qiopenCount >= 10)
	{
		moduleCtrl.qiopenCount = 0;
		moduleReset();
	}
}

/**************************************************
@bref		TCP¹Ø±Õ½âÎöÆ÷
@param
@return
@note
+TCPCLOSE: 0,Link Closed//±»¶¯¹Ø±Õ

**************************************************/
static void tcpcloseParser(uint8_t *buf, uint16_t len)
{
	int index;
	uint8_t *rebuf;
	uint16_t relen;
	uint8_t sockId;
	char restore[20] = {0};
	rebuf = buf;
	relen = len;
	index = my_getstrindex(rebuf, "+TCPCLOSE:", relen);
	if (index < 0)
		return;
	rebuf += index + 11;
	relen -= index + 11;
	index = getCharIndex(rebuf, relen, ',');
	if (index >= 3)	
		return;
	memcpy(restore, rebuf, index);
	restore[index] = 0;
	sockId = atoi(restore);
	rebuf += index + 1;
	relen -= index + 1;
	index = getCharIndex(rebuf, relen, '\r');
	LogPrintf(DEBUG_ALL, "index:%d", index);
	if (index < 0 || index > 20)
		return;
	memcpy(restore, rebuf, index);
	restore[index] = 0;
	if (my_strstr(rebuf, "Link Closed", index))
	{
		LogPrintf(DEBUG_ALL, "Socket [%d] is close", sockId);
		socketSetConnState(sockId, SOCKET_CONN_ERR);
		if (sockId == NORMAL_LINK || sockId == JT808_LINK)
		{
			moduleCtrl.qiopenCount++;
			if (moduleCtrl.qiopenCount >= 4)
			{
				moduleCtrl.qiopenCount = 0;
				moduleReset();
			}
		}
	}
}

uint8_t isAgpsDataRecvComplete(void)
{
	return moduleState.agpsLinkQird;
}


/**************************************************
@bref		TCPÊý¾Ý×´Ì¬½âÎöÆ÷
@param
@return
@note
+TCPACK: 0,20,20
+TCPACK: 0,DISCONNECT

**************************************************/

void tcpackParser(uint8_t *buf, uint16_t len)
{
    int index;
    ITEM item;
    uint8_t *rebuf;
    int16_t relen;
    rebuf = buf;
    relen = len;
	static uint8_t cnt;
	uint8_t socketid;
	uint16_t sent = 0, recv = 0;
	index = my_getstrindex(rebuf, "ERROR", relen);
	if (index >= 0)
	{
		cnt++;
		if (cnt >= 5)
		{
			moduleReset();
			cnt = 0;
		}
	}
    index = my_getstrindex(rebuf, "+TCPACK:", relen);
    if (index < 0)
    {
        return;
    }
    rebuf += 9;
    relen -= 9;
    if (relen < 0)
    {
        return;
    }
    index = getCharIndex(rebuf, relen, '\n');
    cnt = 0;
    stringToItem(&item, rebuf, index);
    socketid = atoi(item.item_data[0]);
    sent = atoi(item.item_data[1]);
    recv = atoi(item.item_data[2]);
    moduleState.tcpAck = recv;
    moduleState.tcpNack = sent - recv;
    LogPrintf(DEBUG_ALL, "Socktid:%d,Ack:%d,NAck:%d", socketid, moduleState.tcpAck, moduleState.tcpNack);

}


/**************************************************
@bref		Ä£×é·¢Êý¾Ý´íÎó½âÎö
@param
@return
@note
+TCPSEND: SOCKET ID OPEN FAILED

**************************************************/

void tcpsendParser(uint8_t *buf, uint16_t len)
{
	if (my_strstr((char *)buf, "ERROR", len) || 
		my_strstr((char *)buf, "+TCPSEND: SOCKET ID OPEN FAILED", len))
    {
		if (moduleState.curSendId == NORMAL_LINK ||
		    moduleState.curSendId == JT808_LINK)
		{
			socketDelAll();
			changeProcess(CPIN_STATUS);
		}
    }
}

/**************************************************
@bref		+IPSTATUS	½âÎö
@param
@return
@note
+IPSTATUS: 0,CONNECT,TCP,4096
+IPSTATUS: 1,DISCONNECT

**************************************************/
void ipstatusParser(uint8_t *buf, uint16_t len)
{
	int index;
	char *rebuf;
	uint16_t relen;
	uint8_t sockId;
	char restore[20];
	rebuf = buf;
	relen = len;
	index = my_getstrindex(rebuf, "+IPSTATUS:", relen);

	while (index >= 0)
	{
		rebuf += index + 11;
		relen -= index + 11;
		index = getCharIndex(rebuf, relen, ',');
		if (index < 0 || index >= 3)
			return;
		memcpy(restore, rebuf, index);
		restore[index] = 0;
		sockId = atoi(restore);
		rebuf += index + 1;
		relen -= index + 1;
		index = getCharIndex(rebuf, relen, ',');
		if (index < 0 || index >= 20)
		{
			index = getCharIndex(rebuf, relen, '\r');
		}
		if (index < 0 || index >= 20)
			return;
		memcpy(restore, rebuf, index);
		restore[index] = 0;
		LogPrintf(DEBUG_ALL, "socket[%d]:%s", sockId, restore);
		if (my_strpach(restore, "CONNECT"))
		{
			socketSetConnState(sockId, SOCKET_CONN_SUCCESS);
		}
		else
		{
			socketSetConnState(sockId, SOCKET_CONN_ERR);
		}
		rebuf += index + 1;
		relen -= index + 1;
		index = my_getstrindex(rebuf, "+IPSTATUS:", relen);
	}
}

/**************************************************
@bref		Ë«¿¨µ¥´ýÇÐ»»
@param
@return
@note
+SIMCROSS: 1

**************************************************/
static void simcrossParser(uint8_t *buf, uint16_t len)
{
	uint8_t *rebuf;
	uint16_t relen;
	int index;
	char restore[20]={0};
	uint8_t currentSim;
	rebuf = buf;
	relen = len;
	index = my_getstrindex(rebuf, "+SIMCROSS:", relen);
	if (index < 0)
	{
		return;
	}
	while (index > 0)
	{
		rebuf += index + 11;
		relen -= index + 11;
		index = getCharIndex(rebuf, relen, '\r');
		if (index == 1)
		{
			memcpy(restore, rebuf, index);
			if (atoi(restore) == 1)
			{
				currentSim = 1;
			}
			else if (atoi(restore) == 2)
			{
				currentSim = 2;
			}
			else
			{
				currentSim = 1;
			}
		}
		else
		{
			currentSim = 1;
		}
		LogPrintf(DEBUG_ALL, "Current sim:%d", currentSim);
		if (currentSim != dynamicParam.sim)
		{
			LogPrintf(DEBUG_ALL, "Target sim is %d, current sim is %d, change and reset module", dynamicParam.sim, currentSim);
			netSetSim(dynamicParam.sim);
			startTimer(30, moduleReset, 0);
		}
		rebuf += index;
		relen -= index;
		index = my_getstrindex(rebuf, "+SIMCROSS:", relen);
	}
}

/**************************************************
@bref		µç³ØµçÑ¹½âÎö
@param
@return
@note
+READADC: 3, 3902

**************************************************/

void readAdcParser(uint8_t *buf, uint16_t len)
{
	uint8_t *rebuf;
	uint16_t relen;
	uint16_t vol;
	char restore[10]={0};
	int index;
	rebuf = buf;
	relen = len;

	index = my_getstrindex(rebuf, "+READADC:", relen);
	if (index >= 0)
	{
		rebuf += index + 10;
		relen -= index + 10;
		index = getCharIndex(rebuf, relen, ',');
		if (index >= 0 && index <= 2)
		{
			rebuf += index + 2;
			relen -= index + 2;
			memcpy(restore, rebuf, 4);
			restore[4] = 0;
			vol = atoi(restore);
			sysinfo.insidevoltage = ((float)vol / 1000) + 0.02;
			LogPrintf(DEBUG_ALL, "Bat vol:%.2f", sysinfo.insidevoltage);
		}
	}
}

static uint8_t answerPhone(void)
{
    sendModuleCmd(ATA_CMD, NULL);
}

static void clearPhone(void)
{
    moduleState.ataflag = 0;
    LogMessage(DEBUG_ALL, "clearPhone");
}

/**************************************************
@bref		À´µç½âÎö
@param
@return
@note
+CLIP: "18922219090",129,,,,0

**************************************************/

void clipParser(uint8_t *buf, uint16_t len)
{
	uint8_t *rebuf;
	uint16_t relen;
	uint8_t i;
	int index;
	char restore[50] = { 0 };
	char phoneNum[20] = { 0 };
	rebuf = buf;
	relen = len;
	index = my_getstrindex(rebuf, "+CLIP:", relen);
	if (index >= 0)
	{
		rebuf += index + 8;
		relen -= index + 8;
		index = getCharIndex(rebuf, relen, '"');
		if (index == 11)
		{
			memcpy(phoneNum, rebuf, index);
			phoneNum[index] = 0;
			LogPrintf(DEBUG_ALL, "Caller Identification:%s", phoneNum);
			for (i = 0; i < 3; i++)
			{
				if (sysparam.sosNum[i][0] != 0)
				{
					if (my_strstr((char *)phoneNum, (char *)sysparam.sosNum[i], 11))
					{
						LogPrintf(DEBUG_ALL, "SOS number %s,anaswer phone", sysparam.sosNum[i]);
						if (moduleState.ataflag == 0)
						{
                            answerPhone();
                            moduleState.ataflag = 1;
                            startTimer(100, clearPhone, 0);//ÔÚ½ÓÍ¨ºó10ÃëÇå³ý±êÖ¾
						}
						return;
					}
				}
			}
			if (i == 3)
			{
				LogPrintf(DEBUG_ALL, "Not sos number, hang up");
				sendModuleCmd(ATH_CMD, NULL);
			}
		}
	}
}

/**************************************************
@bref       ATA ½âÎö
@param
@return
@note
NO CARRIER  ¶Ô¶Ë²»½ÓÌý
NO ANSWER   ¶Ô¶Ë¹Ò¶Ï
BUSY
CONNECT
**************************************************/
void ataParser(uint8_t *buf, uint16_t len)
{
    uint8_t *rebuf;
    uint16_t relen;
    int index;
    rebuf = buf;
    relen = len;

    if (my_getstrindex((char *)rebuf, "NO CARRIER", relen) >= 0 ||
        my_getstrindex((char *)rebuf, "NO ANSWER",  relen) >= 0 ||
        my_getstrindex((char *)rebuf, "BUSY",       relen) >= 0 ||
        my_getstrindex((char *)rebuf, "CONNECT",    relen) >= 0)
    {
        clearPhone();
    }
}

/**************************************************
@bref		+FSLIST Ð´ÎÄ¼þ½âÎö
@param
@return
@note
nwy_last_imsi,15
gpssave.data,220
OK

**************************************************/

void fslistParser(uint8_t *buf, uint16_t len)
{
	uint8_t *rebuf;
	uint16_t relen, i;
	char restore[50];
	int index;
	rebuf = buf;
	relen = len;
	index = my_getstrindex(rebuf, "OK", relen);
	LogPrintf(DEBUG_ALL, "okindex:%d", index);
	if (index < 5)
	{
		moduleState.fileNum = 0;
		LogPrintf(DEBUG_ALL, "No file");
		return;
	}
	/* ¹ýÂË¿ªÍ·µÄ\r\n */
	rebuf += 2;
	relen -= 2;
	moduleState.fileNum = 0;
	LogPrintf(DEBUG_ALL, "File list>>>>>>>>>>>>");
	while (relen >= 0)
	{
		index = getCharIndex(rebuf, relen, ',');
		if (index < 0)
			return;
		if (moduleState.fileNum <= FILE_MAX_CNT)
		{
			tmos_memcpy(moduleState.file[moduleState.fileNum].fileName, rebuf, index);
			moduleState.file[moduleState.fileNum].fileName[index] = 0;
			rebuf += index + 1;
			relen -= index + 1;
			index = getCharIndex(rebuf, relen, '\r');

			if (index < 0)
				return;
			tmos_memcpy(restore, rebuf, index);
			restore[index] = 0;
			moduleState.file[moduleState.fileNum].fileSize = atoi(restore);
			LogPrintf(DEBUG_ALL, "File[%d]:%s,size:%d", moduleState.fileNum,
														moduleState.file[moduleState.fileNum].fileName, 
														moduleState.file[moduleState.fileNum].fileSize);
			moduleState.fileNum++;
		}
		rebuf += index + 2;
		relen -= index + 2;
	}
}

/**************************************************
@bref		+FSWF Ð´ÎÄ¼þ½âÎö
@param
@return
@note
**************************************************/

void fswfParser(uint8_t *buf, uint16_t len)
{
	uint8_t *rebuf;
	uint16_t relen;
	int index;
	rebuf = buf;
	relen = len;
	index = my_getstrindex(rebuf, "+FSWF:", relen);
	while (index >= 0)
	{
		rebuf += index + 8;
		relen -= index + 8;
		index = my_getstrindex(rebuf, "+FSWF:", relen);
	}
}

/**************************************************
@bref		+FSRF Ð´ÎÄ¼þ½âÎö
@param
@return
@note
+FSRF: 10,start01234
**************************************************/

void fsrfParser(uint8_t *buf, uint16_t len)
{
	uint8_t *rebuf;
	uint16_t relen;
	uint16_t readSize;
	char restore[20] = { 0 };
	int index;
	rebuf = buf;
	relen = len;
	index = my_getstrindex(rebuf, "+FSRF:", relen);
	while (index >= 0)
	{
		rebuf += index + 7;
		relen -= index + 7;
		index = getCharIndex(rebuf, relen, ',');
		if (index > 0 && index <= 5)
		{
			tmos_memcpy(restore, rebuf, index);
			restore[index] = 0;
			readSize = atoi(restore);
			if (readSize != 0)
			{
				rebuf += index + 1;
				relen -= index + 1;
				LogPrintf(DEBUG_UP, "relen:%d readSize:%d", relen, readSize);
				if (relen > readSize)
				{
					if (relen - readSize >= 10)
					{
						LogPrintf(DEBUG_UP, "¶ÁÈ¡Êý¾ÝÖØµþ");
					}
					else
					{
						ota_package_t pkg;
						pkg.data   = rebuf;
						pkg.len    = readSize;
						if (getBleOtaFsm() == BLE_OTA_FSM_READ)
						{
							if (bleDevGetOtaStatusByIndex(getBleOtaStatus()) != 0)
							{
								bleOtaFilePackage(pkg);
							}
						}
						else if (getBleOtaFsm() == BLE_OTA_FSM_VERI_READ)
						{
							if (bleDevGetOtaStatusByIndex(getBleOtaStatus()) != 0)
							{
								bleVerifyFilePackage(pkg);
							}
						}
					}
				}
				else
				{
					LogPrintf(DEBUG_UP, "¶ÁÎÄ¼þ´íÎó");
					if (getBleOtaFsm() == BLE_OTA_FSM_READ)
					{
						bleOtaFsmChange(BLE_OTA_FSM_READ);
					}
					else if (getBleOtaFsm() == BLE_OTA_FSM_VERI_READ)
					{
						bleOtaFsmChange(BLE_OTA_FSM_VERI_READ);
					}
				}
			}
		}
		index = my_getstrindex(rebuf, "+FSRF:", relen);
	}
}

/**************************************************
@bref		+FSFS Ð´ÎÄ¼þ½âÎö
@param
@return
@note
+FSFS: 44624

**************************************************/

static void fsfsParser(uint8_t *buf, uint16_t len)
{
	uint8_t *rebuf;
	uint16_t relen;
	uint16_t fileSize;
	char restore[20] = { 0 };
	int  index;
	rebuf = buf;
	relen = len;
	index = my_getstrindex(rebuf, "+FSFS:", relen);
	while (index >= 0)
	{
		rebuf += index + 7;
		relen -= index + 7;
		index = getCharIndex(rebuf, relen, '\r');
//		if (index < 0)
//		{
//			index = getCharIndex(rebuf, relen, '\r');
//		}
		if (index > 0 && index < 10)
		{
			tmos_memcpy(restore, rebuf, index);
			restore[index] = 0;
			fileSize = atoi(restore);
			LogPrintf(DEBUG_ALL, "File size:%d", fileSize);
			if (getUpgradeServerFsm() == NETWORK_VERIFY_LENGTH)
			{
				if (fileSize == getFileTotalSize())
				{
					LogPrintf(DEBUG_UP, "Verfy length success");
					upgradeServerChangeFsm(NETWORK_DOWNLOAD_END);
				}
				else
				{
					upgradeServerChangeFsm(NETWORK_VERIFY_ERROR);
				}
			}
		}
		index = my_getstrindex(rebuf, "+FSFS:", relen);
	}
}

/**************************************************
@bref		Ä£×éÒì³£¸´Î»¼ì²â
@param
@return
@note
**************************************************/
static void moduleRstDetector(uint8_t * buf, uint16_t len)
{
	int index;
	if (moduleState.powerState != 1)
	{
		return;
	}

	index = my_getstrindex((char *)buf, "+PBREADY", len);
	if (index >= 0)
	{
		if (sysinfo.moduleRstFlag == 1)
		{
			sysinfo.moduleRstFlag = 0;
			LogMessage(DEBUG_ALL, "ignore module abnormal reset");
			return;
		}

		sysinfo.moduleRstCnt++;
		LogPrintf(DEBUG_ALL, "module abnormal reset %d", sysinfo.moduleRstCnt);

	}
}


/**************************************************
@bref		Ä£×é¶ËÊý¾Ý½ÓÊÕ½âÎöÆ÷
@param
@return
@note
**************************************************/

void moduleRecvParser(uint8_t *buf, uint16_t bufsize)
{
    static uint16_t len = 0;
    static uint8_t dataRestore[MODULE_RX_BUFF_SIZE + 1];
    if (bufsize == 0)
        return;
    if (len + bufsize > MODULE_RX_BUFF_SIZE)
    {
        len = 0;
        bufsize %= MODULE_RX_BUFF_SIZE;
        LogMessage(DEBUG_ALL, "UartRecv Full!!!");
    }
    memcpy(dataRestore + len, buf, bufsize);
    len += bufsize;
    dataRestore[len] = 0;
    if (dataRestore[len - 1] != '\n')
    {
        if (dataRestore[2] != '>')
        {
            return;
        }
    }
    LogPrintf(DEBUG_ALL, "--->>>---0x%X [%d]", dataRestore, len);
    LogMessageWL(DEBUG_ALL, (char *)dataRestore, len);
    LogMessage(DEBUG_ALL, "---<<<---");
    /*****************************************/
	gprsDisconnectionParser(dataRestore, len);
    moduleRstDetector(dataRestore, len);
    moduleRspSuccess();
    cmtiParser(dataRestore, len);
    cmgrParser(dataRestore, len);
    tcpsetupRspParser(dataRestore, len);
    wifiapscanParser(dataRestore, len);
    mylacidParser(dataRestore, len);
    netdmsgParser(dataRestore, len);
    tcprecvParser(dataRestore, len);
    tcpreadParser(dataRestore, len);
	tcpcloseParser(dataRestore, len);
	clipParser(dataRestore, len);
	fsrfParser(dataRestore, len);
	ataParser(dataRestore, len);
    /*****************************************/
    switch (moduleState.cmd)
    {
        case AT_CMD:
            if (distinguishOK((char *)dataRestore))
            {
                moduleState.atResponOK = 1;
            }
            break;
        case CPIN_CMD:
            if (my_strstr((char *)dataRestore, "+CPIN: READY", len))
            {
                moduleState.cpinResponOk = 1;
            }
            else if (my_getstrindex((char *)dataRestore, "+CPIN: NO SIM", len))
            {
				if (moduleState.fsmtick >= 29)
				{
                	//ÑÓ³Ù3SÊÇµ£ÐÄ´ËÊ±Ä£×éÖ¸Áî·¢ËÍ¶ÓÁÐÌ«¶à·¢²»¹ýÀ´
                    startTimer(30, moduleReset, 0);
                    if (sysparam.simpulloutalm && dynamicParam.sim == SIM_1)
                    {
						alarmRequestSet(ALARM_SIMPULLOUT_REQUEST);
						if (sysparam.simpulloutLock)
						{
							sysparam.relayCtl = 1;
                            paramSaveAll();
                            relayAutoRequest();
                            LogPrintf(DEBUG_ALL, "simpullout==>try to relay on");
						}
                    }
                    if (sysparam.simSel == SIM_MODE_1)
                	{
						if (dynamicParam.sim == SIM_1)
						{
							netSetSim(SIM_2);
						}
						else if (dynamicParam.sim == SIM_2)
						{
							netSetSim(SIM_1);
						}
                	}
                	changeProcess(AT_STATUS);
				}
            }
            break;
        case CSQ_CMD:
            csqParser(dataRestore, len);
            break;
        case CREG_CMD:
            cgregParser(dataRestore, len);
            break;
        case CIMI_CMD:
            cimiParser(dataRestore, len);
            break;
		case CCID_CMD:
			iccidParser(dataRestore, len);
			break;
        case CGSN_CMD:
            cgsnParser(dataRestore, len);
            break;
        case CGATT_CMD:
            cgattParser(dataRestore, len);
            break;
        case XIIC_CMD:
			xiicParser(dataRestore, len);
        	break;
        case FSLIST_CMD:
			fslistParser(dataRestore, len);
        	break;
        case FSFS_CMD:
			fsfsParser(dataRestore, len);
        	break;
        case FSRF_CMD:
			
        	break;
        case SIMCROSS_CMD:
        	simcrossParser(dataRestore, len);
        	break;
       	case READADC_CMD:
			readAdcParser(dataRestore, len);
       		break;
       	case TCPACK_CMD:
			tcpackParser(dataRestore, len);
       		break;
       	case IPSTATUS_CMD:
       		ipstatusParser(dataRestore, len);
       		break;
		case TCPSEND_CMD:
			tcpsendParser(dataRestore, len);
			break;
        case TCPREAD_CMD:
        	if (my_strstr((char *)dataRestore, "ERROR", len) || my_strstr((char *)dataRestore, "+TCPREAD: SOCKET ID OPEN FAILED", len))
            {
                switch (moduleState.curQirdId)
                {
                    case NORMAL_LINK:
                        moduleState.normalLinkQird = 0;
                        break;
                    case BLE_LINK:
                        moduleState.bleLinkQird = 0;
                        break;
                    case JT808_LINK:
                        moduleState.jt808LinkQird = 0;
                        break;
                    case HIDDEN_LINK:
                        moduleState.hideLinkQird = 0;
                        break;
                    case AGPS_LINK:
                        moduleState.agpsLinkQird = 0;
                        break;
                    case UPGRADE_LINK:
						moduleState.upgradeQird  = 0;
                    	break;
                }
                LogPrintf(DEBUG_ALL, "Link[%d] recv err", moduleState.curQirdId);
            }
        	break;
        default:
            break;
    }
    moduleState.cmd = 0;
    len = 0;
}

/*--------------------------------------------------------------*/

/**************************************************
@bref		ÖØÖÃÐÅºÅËÑË÷Ê±³¤
@param
@return
@note
**************************************************/

void netResetCsqSearch(void)
{
    moduleCtrl.csqTime = 90;
}

/**************************************************
@bref		socket·¢ËÍÊý¾Ý
@param
@return
@note
**************************************************/

int socketSendData(uint8_t link, uint8_t *data, uint16_t len)
{
    int ret = 0;
    char param[10];

    if (socketGetConnStatus(link) == 0)
    {
        //Á´Â·Î´Á´½Ó
        return 0;
    }
	moduleState.curSendId = link;
    sprintf(param, "%d,%d", link, len);
    sendModuleCmd(TCPSEND_CMD, param);
    createNode((char *)data, len, TCPSEND_CMD);
    if (link == NORMAL_LINK || link == JT808_LINK)
    {
        moduleState.tcpNack = len;
    }
    return len;
}
/**************************************************
@bref		Ä£×éË¯Ãß¿ØÖÆ
@param
@return
@note
**************************************************/
void moduleSleepCtl(uint8_t onoff)
{
    char param[5];
    uint8_t mode=0;
    if (onoff == 0)
    {
        return;
    }
    else
    {
		mode = 2;
    }
    sprintf(param, "%d", mode);
    sendModuleCmd(AT_CMD, NULL);
    sendModuleCmd(ENPWRSAVE_CMD, param);
}

/**************************************************
@bref		»ñÈ¡CSQ
@param
@return
@note
**************************************************/

void moduleGetCsq(void)
{
    sendModuleCmd(CSQ_CMD, NULL);
}

/**************************************************
@bref		»ñÈ¡ÍøÂç×´Ì¬
@param
@return
@note
**************************************************/

void moduleGetNetstatus(void)
{
	sendModuleCmd(CREG_CMD, "?");
}

/**************************************************
@bref		»ñÈ¡»ùÕ¾
@param
@return
@note
**************************************************/

void moduleGetLbs(void)
{
	sendModuleCmd(NETDMSG_CMD, NULL);
	sendModuleCmd(MYLACID_CMD, NULL);
}
/**************************************************
@bref		»ñÈ¡WIFIscan
@param
@return
@note
**************************************************/

void moduleGetWifiScan(void)
{
    sendModuleCmd(AT_CMD, NULL);
    sendModuleCmd(WIFIAPSCAN_CMD, NULL);
}

/**************************************************
@bref		·¢ËÍ¶ÌÏûÏ¢
@param
@return
@note
**************************************************/

void sendMessage(uint8_t *buf, uint16_t len, char *telnum)
{
    char param[60];
    sprintf(param, "\"%s\"", telnum);
    sendModuleCmd(CMGF_CMD, "1");
    sendModuleCmd(CMGS_CMD, param);
    LogPrintf(DEBUG_ALL, "len:%d", len);
    buf[len] = 0x1A;
    createNode((char *)buf, len + 1, CMGS_CMD);
}
/**************************************************
@bref		É¾³ýËùÓÐ¶ÌÏûÏ¢
@param
@return
@note
**************************************************/


void deleteMessage(void)
{
    sendModuleCmd(CMGD_CMD, "0,4");
}

/**************************************************
@bref		²éÑ¯Êý¾ÝÊÇ·ñ·¢ËÍÍê±Ï
@param
@return
@note
**************************************************/

void querySendData(uint8_t link)
{
    char param[5];
    sprintf(param, "%d", link);
    sendModuleCmd(TCPACK_CMD, param);
}


/**************************************************
@bref		²éÑ¯Ä£×éµç³ØµçÑ¹
@param
@return
@note
**************************************************/

void queryBatVoltage(void)
{
	sendModuleCmd(READADC_CMD, "3");
}

/**************************************************
@bref		Çå¿ÕÎÄ¼þÁÐ±í
@param
@return
@note
**************************************************/

void moduleFileInfoClear(void)
{
	tmos_memset(&moduleState.file, 0, sizeof(moduleState.file));
	moduleState.fileNum = 0;
}


/**************************************************
@bref		±È½ÏÎÄ¼þÁÐ±í
@param
@return
@note
**************************************************/

uint8_t compareFile(uint8_t *file, uint8_t len)
{
	for (uint8_t i = 0; i < moduleState.fileNum; i++)
	{
		if (strncmp(moduleState.file[i].fileName, file, len) == 0)
			return 1;
	}
	return 0;
}

/**************************************************
@bref		¶ÁÈ¡ÐÅºÅÖµ
@param
@return
@note
**************************************************/

uint8_t getModuleRssi(void)
{
    return moduleState.rssi;
}

/**************************************************
@bref		¶ÁÈ¡IMSI
@param
@return
@note
**************************************************/

uint8_t *getModuleIMSI(void)
{
    return moduleState.IMSI;
}
/**************************************************
@bref		¶ÁÈ¡IMEI
@param
@return
@note
**************************************************/

uint8_t *getModuleIMEI(void)
{
    return moduleState.IMEI;
}



/**************************************************
@bref		¶ÁÈ¡ICCID
@param
@return
@note
**************************************************/

uint8_t *getModuleICCID(void)
{
    return moduleState.ICCID;
}

/**************************************************
@bref		¶ÁÈ¡MCC
@param
@return
@note
**************************************************/

uint16_t getMCC(void)
{
    return moduleState.mcc;
}

/**************************************************
@bref		¶ÁÈ¡MNC
@param
@return
@note
**************************************************/

uint8_t getMNC(void)
{
    return moduleState.mnc;
}

/**************************************************
@bref		¶ÁÈ¡LAC
@param
@return
@note
**************************************************/

uint16_t getLac(void)
{
    return moduleState.lac;
}

/**************************************************
@bref		¶ÁÈ¡CID
@param
@return
@note
**************************************************/

uint32_t getCid(void)
{
    return moduleState.cid;
}

/**************************************************
@bref		¶ÁÈ¡Î´·¢ËÍ×Ö½ÚÊý£¬ÅÐ¶ÏÊÇ·ñ·¢ËÍ³É¹¦
@param
@return
@note
**************************************************/

uint32_t getTcpNack(void)
{
    return moduleState.tcpNack;
}

/**************************************************
@bref		»ñÈ¡ÎÄ¼þÁÐ±í
@param
@return
@note
**************************************************/

fileInfo_s* getFileList(uint8_t *num)
{
	*num = moduleState.fileNum;
	return moduleState.file;
}


/**************************************************
@bref		Ä£×éÊÇ·ñ´ïµ½ÁªÍø×´Ì¬
@param
@return
@note
**************************************************/

uint8_t isModuleRunNormal(void)
{
    if (moduleState.fsmState == NORMAL_STATUS)
        return 1;
    return 0;
}

/**************************************************
@bref		Ä£×é´ïµ½Õý³£¿ª»ú
@param
@return
@note
**************************************************/

uint8_t isModulePowerOnOk(void)
{
    if (moduleState.powerOnTick > 10)
        return 1;
    return 0;
}

/**************************************************
@bref		Ä£×éÊÇ·ñ¿ª»ú
@param
@return
@note
**************************************************/

uint8_t isModulePowerOn(void)
{
	if (moduleState.powerState)
		return 1;
	return 0;
}


/**************************************************
@bref		¹Ò¶Ïµç»°
@param
@return
@note
**************************************************/

void stopCall(void)
{
    sendModuleDirect("ATH\r\n");
}
/**************************************************
@bref		²¦´òµç»°
@param
@return
@note
**************************************************/

void callPhone(char *tel)
{
    char param[50];
    sprintf(param, "ATD%s;\r\n", tel);
    LogPrintf(DEBUG_ALL, "Call %s", tel);
    sendModuleDirect(param);

}


