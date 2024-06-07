#include <app_protocol.h>
#include "app_instructioncmd.h"

#include "app_peripheral.h"
#include "app_bleRelay.h"
#include "app_gps.h"
#include "app_kernal.h"
#include "app_net.h"
#include "app_param.h"
#include "app_central.h"
#include "app_sys.h"
#include "app_task.h"
#include "app_server.h"
#include "app_jt808.h"
#include "app_mir3da.h"
#include "app_file.h"
const instruction_s insCmdTable[] =
{
    {PARAM_INS, "PARAM"},
    {STATUS_INS, "STATUS"},
    {VERSION_INS, "VERSION"},
    {SERVER_INS, "SERVER"},
    {HBT_INS, "HBT"},
    {MODE_INS, "MODE"},
    {POSITION_INS, "123"},
    {APN_INS, "APN"},
    {UPS_INS, "UPS"},
    {LOWW_INS, "LOWW"},
    {LED_INS, "LED"},
    {POITYPE_INS, "POITYPE"},
    {RESET_INS, "RESET"},
    {UTC_INS, "UTC"},
    {ALARMMODE_INS, "ALARMMODE"},
    {DEBUG_INS, "DEBUG"},
    {ACCCTLGNSS_INS, "ACCCTLGNSS"},
    {ACCDETMODE_INS, "ACCDETMODE"},
    {FENCE_INS, "FENCE"},
    {FACTORY_INS, "FACTORY"},
    {ICCID_INS, "ICCID"},
    {SETAGPS_INS, "SETAGPS"},
    {JT808SN_INS, "JT808SN"},
    {HIDESERVER_INS, "HIDESERVER"},
    {BLESERVER_INS, "BLESERVER"},
    {RELAY_INS, "RELAY"},
    {READPARAM_INS, "READPARAM"},
    {SETBLEPARAM_INS, "SETBLEPARAM"},
    {SETBLEWARNPARAM_INS, "SETBLEWARNPARAM"},
    {SETBLERFPARAM_INS, "SETBLERFPARAM"},
    {SETBLEMAC_INS, "SETBLEMAC"},
    {RELAYSPEED_INS, "RELAYSPEED"},
    {RELAYFORCE_INS, "RELAYFORCE"},
    {BF_INS, "BF"},
    {CF_INS, "CF"},
    {PROTECTVOL_INS, "PROTECTVOL"},
    {SOS_INS, "SOS"},
    {CENTER_INS, "CENTER"},
    {SOSALM_INS, "SOSALM"},
    {TIMER_INS, "TIMER"},
    {MOTIONDET_INS, "MOTIONDET"},
    {FCG_INS, "FCG"},
    {QFOTA_INS, "QFOTA"},
    {BLEEN_INS, "BLEEN"},
    {AGPSEN_INS, "AGPSEN"},
    {BLERELAYCTL_INS, "BLERELAYCTL"},
    {RELAYFUN_INS, "RELAYFUN"},
    {SETBATRATE_INS, "SETBATRATE"},
    {SETMILE_INS, "SETMILE"},
	{SHUTDOWNALM_INS, "SHUTDOWNALM"},
	{UNCAPALM_INS, "UNCAPALM"},
	{SIMPULLOUTALM_INS, "SIMPULLOUTALM"},
	{SIMSEL_INS, "SIMSEL"},
	{READVERSION_INS, "READVERSION"},
	{SETTXPOWER_INS, "SETTXPOWER"},
	{FILE_INS, "FILE"},
	{BTFLIST_INS, "BTFLIST"},
    {BTFDEL_INS, "BTFDEL"},
    {BTFDOWNLOAD_INS, "BTFDOWNLOAD"},
    {BTFUPS_INS, "BTFUPS"},
    {UART_INS, "UART"},
    {BLEDEBUG_INS, "BLEDEBUG"},
    {LOCKTIMER_INS, "LOCKTIMER"},
    {RFDETTYPE_INS, "RFDETTYPE"},
    {ADCCAL_INS, "ADCCAL"},
    {RELAYTYPE_INS, "RELAYTYPE"},
    {SN_INS, "*"},
};

static insMode_e mode123;
static insParam_s param123;
static uint8_t serverType;

/*关于指令延迟回复*/
static insMode_e lastmode;
insParam_s lastparam;
static int rspTimeOut = -1;


static void sendMsgWithMode(uint8_t *buf, uint16_t len, insMode_e mode, void *param)
{
    insParam_s *insparam;
    switch (mode)
    {
        case DEBUG_MODE:
            LogMessage(DEBUG_FACTORY, "----------Content----------");
            LogMessage(DEBUG_FACTORY, (char *)buf);
            LogMessage(DEBUG_FACTORY, "------------------------------");
            break;
        case SMS_MODE:
            if (param != NULL)
            {
                insparam = (insParam_s *)param;
                sendMessage(buf, len, insparam->telNum);
                startTimer(15, deleteMessage, 0);
            }
            break;
        case NET_MODE:
            if (param != NULL)
            {
                insparam = (insParam_s *)param;
                insparam->data = buf;
                insparam->len = len;
                protocolSend(insparam->link, PROTOCOL_21, insparam);
            }
            break;
        case BLE_MODE:
            appSendNotifyData(buf, len);
            break;
        case JT808_MODE:
            jt808MessageSend(buf, len);
            break;
    }
}

void instructionRespone(char *message)
{
	if (rspTimeOut != -1)
	{
		char buf[50] = { 0 };
		sprintf(buf, "%s", message);
		stopTimer(rspTimeOut);
		setLastInsid();

		sendMsgWithMode((uint8_t *)buf, strlen(buf), lastmode, &lastparam);
	}
	rspTimeOut = -1;
}

static void relayOnRspTimeOut(void)
{
	char buf[50] = {"Relay on fail:Time out"};
	setLastInsid();
	sendMsgWithMode((uint8_t *)buf, strlen(buf), lastmode, &lastparam);
	rspTimeOut = -1;
}

static void relayOffRspTimeOut(void)
{
	char buf[50] = {"Relay off fail:Time out"};
	setLastInsid();
	sendMsgWithMode((uint8_t *)buf, strlen(buf), lastmode, &lastparam);
	rspTimeOut = -1;
}

static void doParamInstruction(ITEM *item, char *message)
{
    uint8_t i;
    uint8_t debugMsg[15];
    if (sysparam.protocol == JT808_PROTOCOL_TYPE)
    {
        byteToHexString(dynamicParam.jt808sn, debugMsg, 6);
        debugMsg[12] = 0;
        sprintf(message + strlen(message), "JT808SN:%s;SN:%s;IP:%s:%u;", debugMsg, dynamicParam.SN, sysparam.jt808Server,
                sysparam.jt808Port);
    }
    else
    {
        sprintf(message + strlen(message), "SN:%s;IP:%s:%d;", dynamicParam.SN, sysparam.Server, sysparam.ServerPort);
    }
    sprintf(message + strlen(message), "APN:%s;", sysparam.apn);
    sprintf(message + strlen(message), "UTC:%s%d;", sysparam.utc >= 0 ? "+" : "", sysparam.utc);
    switch (sysparam.MODE)
    {
        case MODE1:
        case MODE21:
            if (sysparam.MODE == MODE1)
            {
                sprintf(message + strlen(message), "Mode1:");

            }
            else
            {
                sprintf(message + strlen(message), "Mode21:");

            }
            for (i = 0; i < 5; i++)
            {
                if (sysparam.AlarmTime[i] != 0xFFFF)
                {
                    sprintf(message + strlen(message), " %.2d:%.2d", sysparam.AlarmTime[i] / 60, sysparam.AlarmTime[i] % 60);
                }

            }
            sprintf(message + strlen(message), ", Every %d day;", sysparam.gapDay);
            break;
        case MODE2:
            if (sysparam.gpsuploadgap != 0)
            {
                if (sysparam.gapMinutes == 0)
                {
                    //检测到震动，n 秒上送
                    sprintf(message + strlen(message), "Mode2: %dS;", sysparam.gpsuploadgap);
                }
                else
                {
                    //检测到震动，n 秒上送，未震动，m分钟自动上送
                    sprintf(message + strlen(message), "Mode2: %dS,%dM;", sysparam.gpsuploadgap, sysparam.gapMinutes);

                }
            }
            else
            {
                if (sysparam.gapMinutes == 0)
                {
                    //保持在线，不上送
                    sprintf(message + strlen(message), "Mode2: online;");
                }
                else
                {
                    //保持在线，不检测震动，每隔m分钟，自动上送
                    sprintf(message + strlen(message), "Mode2: %dM;", sysparam.gapMinutes);
                }
            }
            break;
        case MODE3:
            sprintf(message + strlen(message), "Mode3: %d minutes;", sysparam.gapMinutes);
            break;
        case MODE23:
            sprintf(message + strlen(message), "Mode23: %dM %dS;", sysparam.gapMinutes, sysparam.gpsuploadgap);
            break;
    }

    sprintf(message + strlen(message), "StartUp:%u;RunTime:%u;", dynamicParam.startUpCnt, dynamicParam.runTime);

}
static void doStatusInstruction(ITEM *item, char *message)
{
    gpsinfo_s *gpsinfo;
    uint8_t i;
    moduleGetCsq();
    sprintf(message, "OUT-V=%.2fV;", sysinfo.outsidevoltage);
    sprintf(message + strlen(message), "BAT-V=%.2fV;", sysinfo.insidevoltage);
    if (sysinfo.gpsOnoff)
    {
        gpsinfo = getCurrentGPSInfo();
        sprintf(message + strlen(message), "GPS=%s;", gpsinfo->fixstatus ? "Fixed" : "Invalid");
        sprintf(message + strlen(message), "PDOP=%.2f;", gpsinfo->pdop);
    }
    else
    {
        sprintf(message + strlen(message), "GPS=Close;");
    }

    sprintf(message + strlen(message), "ACC=%s;", getTerminalAccState() > 0 ? "On" : "Off");
    sprintf(message + strlen(message), "SIGNAL=%d;", getModuleRssi());
    sprintf(message + strlen(message), "BATTERY=%s;", getTerminalChargeState() > 0 ? "Charging" : "Uncharged");
    sprintf(message + strlen(message), "LOGIN=%s;", primaryServerIsReady() > 0 ? "Yes" : "No");
    sprintf(message + strlen(message), "Sim=%d;", dynamicParam.sim);
    sprintf(message + strlen(message), "Gsensor=%s;", read_gsensor_id() == 0x13 ? "OK" : "ERR");
    sprintf(message + strlen(message), "MILE=%.2lfkm;", (sysparam.mileage * (double)(sysparam.milecal / 100.0 + 1.0)) / 1000);
}

static void doSNInstruction(ITEM *item, char *message)
{
//    char IMEI[15];
//    uint8_t sndata[30];
//    if (my_strpach(item->item_data[1], "ZTINFO") && my_strpach(item->item_data[2], "SN"))
//    {
//        changeHexStringToByteArray(sndata, (uint8_t *)item->item_data[3], strlen(item->item_data[3]) / 2);
//        decryptSN(sndata, IMEI);
//        strncpy((char *)sysparam.SN, IMEI, 15);
//        jt808CreateSn(sysparam.jt808sn, (uint8_t *)sysparam.SN + 3, 12);
//        sysparam.jt808isRegister = 0;
//        sysparam.jt808AuthLen = 0;
//        jt808RegisterLoginInfo(sysparam.jt808sn, sysparam.jt808isRegister, sysparam.jt808AuthCode, sysparam.jt808AuthLen);
//        paramSaveAll();
//        sprintf(message, "Update Sn [%s]", IMEI);
//    }
//    else
//    {
//        strcpy(message, "Update sn fail");
//    }
}

static void serverChangeCallBack(void)
{
    jt808ServerReconnect();
    privateServerReconnect();
}

static void doServerInstruction(ITEM *item, char *message)
{
    if (item->item_data[2][0] != 0 && item->item_data[3][0] != 0)
    {
        serverType = atoi(item->item_data[1]);
        if (serverType == JT808_PROTOCOL_TYPE)
        {
            strncpy((char *)sysparam.jt808Server, item->item_data[2], 50);
            stringToLowwer(sysparam.jt808Server, strlen(sysparam.jt808Server));
            sysparam.jt808Port = atoi((const char *)item->item_data[3]);
            sprintf(message, "Update jt808 domain %s:%d;", sysparam.jt808Server, sysparam.jt808Port);

        }
        else
        {
            strncpy((char *)sysparam.Server, item->item_data[2], 50);
            stringToLowwer(sysparam.Server, strlen(sysparam.Server));
            sysparam.ServerPort = atoi((const char *)item->item_data[3]);
            sprintf(message, "Update domain %s:%d;", sysparam.Server, sysparam.ServerPort);
        }
        if (serverType == JT808_PROTOCOL_TYPE)
        {
            sysparam.protocol = JT808_PROTOCOL_TYPE;
            dynamicParam.jt808isRegister = 0;
        }
        else
        {
            sysparam.protocol = ZT_PROTOCOL_TYPE;
        }
        paramSaveAll();
        dynamicParamSaveAll();
        startTimer(30, serverChangeCallBack, 0);
    }
    else
    {
        sprintf(message, "Update domain fail,please check your param");
    }

}


static void doVersionInstruction(ITEM *item, char *message)
{
    sprintf(message, "Version:%s;Compile:%s %s;", EEPROM_VERSION, __DATE__, __TIME__);

}
static void doHbtInstruction(ITEM *item, char *message)
{
    if (item->item_data[1][0] == 0 || item->item_data[1][0] == '?')
    {
        sprintf(message, "The time of the heartbeat interval is %d seconds;", sysparam.heartbeatgap);
    }
    else
    {
        sysparam.heartbeatgap = (uint8_t)atoi((const char *)item->item_data[1]);
        paramSaveAll();
        sprintf(message, "Update the time of the heartbeat interval to %d seconds;", sysparam.heartbeatgap);
    }

}
static void doModeInstruction(ITEM *item, char *message)
{
    uint8_t workmode, i, j, timecount = 0, gapday = 1;
    uint16_t mode1time[7];
    uint16_t valueofminute;
    if (item->item_data[1][0] == 0 || item->item_data[1][0] == '?')
    {
    	switch(sysparam.MODE)
    	{
			case MODE1:
				workmode = 1;
				break;
			case MODE2:
				workmode = 2;
				break;
			case MODE3:
				workmode = 3;
				break;
			case MODE21:
				workmode = 21;
				break;
			case MODE23:
				workmode = 23;
				break;
    	}
        sprintf(message, "Mode%d,%d,%d", workmode, sysparam.gpsuploadgap, sysparam.gapMinutes);
    }
    else
    {
        workmode = atoi(item->item_data[1]);
        gpsRequestClear(GPS_REQUEST_GPSKEEPOPEN_CTL);
        switch (workmode)
        {
            case 1:
            case 21:
                //内容项如果大于2，说明有时间或日期
                if (item->item_cnt > 2)
                {
                    for (i = 0; i < item->item_cnt - 2; i++)
                    {
                        if (strlen(item->item_data[2 + i]) <= 4 && strlen(item->item_data[2 + i]) >= 3)
                        {
                            valueofminute = atoi(item->item_data[2 + i]);
                            mode1time[timecount++] = valueofminute / 100 * 60 + valueofminute % 100;
                        }
                        else
                        {
                            gapday = atoi(item->item_data[2 + i]);
                        }
                    }

                    for (i = 0; i < (timecount - 1); i++)
                    {
                        for (j = 0; j < (timecount - 1 - i); j++)
                        {
                            if (mode1time[j] > mode1time[j + 1]) //相邻两个元素作比较，如果前面元素大于后面，进行交换
                            {
                                valueofminute = mode1time[j + 1];
                                mode1time[j + 1] = mode1time[j];
                                mode1time[j] = valueofminute;
                            }
                        }
                    }

                }

                for (i = 0; i < 5; i++)
                {
                    sysparam.AlarmTime[i] = 0xFFFF;
                }
                //重现写入AlarmTime
                for (i = 0; i < timecount; i++)
                {
                    sysparam.AlarmTime[i] = mode1time[i];
                }
                sysparam.gapDay = gapday;
                if (workmode == MODE1)
                {
                    terminalAccoff();
                    if (gpsRequestGet(GPS_REQUEST_ACC_CTL))
                    {
                        gpsRequestClear(GPS_REQUEST_ACC_CTL);
                    }
                    sysparam.MODE = MODE1;
                    portGsensorCtl(0);
                }
                else
                {
                    sysparam.MODE = MODE21;
                    portGsensorCtl(0);
                }
                sprintf(message, "Change to Mode%d,and work on at", workmode);
                for (i = 0; i < timecount; i++)
                {
                    sprintf(message + strlen(message), " %.2d:%.2d", sysparam.AlarmTime[i] / 60, sysparam.AlarmTime[i] % 60);
                }
                sprintf(message + strlen(message), ",every %d day;", gapday);
                portSetNextAlarmTime();
                break;
            case 2:
                portGsensorCtl(1);
                sysparam.gpsuploadgap = (uint16_t)atoi((const char *)item->item_data[2]);
                sysparam.gapMinutes = (uint16_t)atoi((const char *)item->item_data[3]);
                sysparam.MODE = MODE2;
                if (sysparam.gpsuploadgap == 0)
                {
                    gpsRequestClear(GPS_REQUEST_ACC_CTL);
                    //运动不自动传GPS
                    if (sysparam.gapMinutes == 0)
                    {

                        sprintf(message, "The device switches to mode 2 without uploading the location");
                    }
                    else
                    {
                        sprintf(message, "The device switches to mode 2 and uploads the position every %d minutes all the time",
                                sysparam.gapMinutes);
                    }
                }
                else
                {
                    if (getTerminalAccState())
                    {
                        if (sysparam.gpsuploadgap < GPS_UPLOAD_GAP_MAX)
                        {
                            gpsRequestSet(GPS_REQUEST_ACC_CTL);
                        }
                        else
                        {
                            gpsRequestClear(GPS_REQUEST_ACC_CTL);
                        }
                    }
                    else
                    {
                        gpsRequestClear(GPS_REQUEST_ACC_CTL);
                    }
                    if (sysparam.accctlgnss == 0)
                    {
                        gpsRequestSet(GPS_REQUEST_GPSKEEPOPEN_CTL);
                    }
                    if (sysparam.gapMinutes == 0)
                    {
                        sprintf(message, "The device switches to mode 2 and uploads the position every %d seconds when moving",
                                sysparam.gpsuploadgap);

                    }
                    else
                    {
                        sprintf(message,
                                "The device switches to mode 2 and uploads the position every %d seconds when moving, and every %d minutes when standing still",
                                sysparam.gpsuploadgap, sysparam.gapMinutes);
                    }

                }
                break;
            case 3:
            case 23:
                if (workmode == MODE3)
                {
                	if (atoi(item->item_data[2]) != 0)
                	{
                		sysparam.gapMinutes = (uint16_t)atoi(item->item_data[2]);
                	}
                	if (sysparam.gapMinutes < 5)
	                {
	                    sysparam.gapMinutes = 5;
	                }
	                if (sysparam.gapMinutes >= 10080)
					{
						sysparam.gapMinutes = 10080;
					}
                    terminalAccoff();
                    if (gpsRequestGet(GPS_REQUEST_ACC_CTL))
                    {
                        gpsRequestClear(GPS_REQUEST_ACC_CTL);
                    }
                    sysparam.MODE = MODE3;
                    portGsensorCtl(0);
                    sprintf(message, "Change to mode %d and update the startup interval time to %d minutes;", workmode,
                        sysparam.gapMinutes);
                }
                else
                {
                	if (atoi(item->item_data[2]) != 0)
                	{
						sysparam.gpsuploadgap = (uint16_t)atoi((const char *)item->item_data[2]);
                	}
                    if (atoi(item->item_data[3]) != 0)
                    {
                        sysparam.gapMinutes = (uint16_t)atoi((const char *)item->item_data[3]);
                    }
                    if (sysparam.gapMinutes < 5)
	                {
	                    sysparam.gapMinutes = 5;
	                }
	                if (sysparam.gapMinutes >= 10080)
					{
						sysparam.gapMinutes = 10080;
					}
                    sysparam.MODE = MODE23;
                    portGsensorCtl(1);
                    sprintf(message, "Change to mode %d and update interval time to %d s %d m;", workmode, sysparam.gpsuploadgap,
                            sysparam.gapMinutes);
                }
                break;

            default:
                strcpy(message, "Unsupport mode");
                break;
        }
        paramSaveAll();

    }
}



void dorequestSend123(void)
{
    char message[150];
    uint16_t year;
    uint8_t  month;
    uint8_t date;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    gpsinfo_s *gpsinfo;

    portGetRtcDateTime(&year, &month, &date, &hour, &minute, &second);
    sysinfo.flag123 = 0;
    gpsinfo = getCurrentGPSInfo();
    sprintf(message, "(%s)<Local Time:%.2d/%.2d/%.2d %.2d:%.2d:%.2d>http://maps.google.com/maps?q=%s%f,%s%f", dynamicParam.SN, \
            year, month, date, hour, minute, second, \
            gpsinfo->NS == 'N' ? "" : "-", gpsinfo->latitude, gpsinfo->EW == 'E' ? "" : "-", gpsinfo->longtitude);
    reCover123InstructionId();
    sendMsgWithMode((uint8_t *)message, strlen(message), mode123, &param123);
}

void do123Instruction(ITEM *item, insMode_e mode, void *param)
{
    insParam_s *insparam;
    mode123 = mode;
    if (param != NULL)
    {
        insparam = (insParam_s *)param;
        param123.telNum = insparam->telNum;
    }
    if (sysparam.poitype == 0)
    {
        lbsRequestSet(DEV_EXTEND_OF_MY);
        LogMessage(DEBUG_ALL, "Only LBS reporting");
    }
    else if (sysparam.poitype == 1)
    {
        lbsRequestSet(DEV_EXTEND_OF_MY);
        gpsRequestSet(GPS_REQUEST_123);
        LogMessage(DEBUG_ALL, "LBS and GPS reporting");
    }
    else
    {
        lbsRequestSet(DEV_EXTEND_OF_MY);
        wifiRequestSet(DEV_EXTEND_OF_MY);
        gpsRequestSet(GPS_REQUEST_123);
        LogMessage(DEBUG_ALL, "LBS ,WIFI and GPS reporting");
    }
    sysinfo.flag123 = 1;
    save123InstructionId();

}


void doAPNInstruction(ITEM *item, char *message)
{
    if (item->item_data[1][0] == 0 || item->item_data[1][0] == '?')
    {
        sprintf(message, "APN:%s,APN User:%s,APN Password:%s", sysparam.apn, sysparam.apnuser, sysparam.apnpassword);
    }
    else
    {
        sysparam.apn[0] = 0;
        sysparam.apnuser[0] = 0;
        sysparam.apnpassword[0] = 0;
        if (item->item_data[1][0] != 0 && item->item_cnt >= 2)
        {
            strcpy((char *)sysparam.apn, item->item_data[1]);
        }
        if (item->item_data[2][0] != 0 && item->item_cnt >= 3)
        {
            strcpy((char *)sysparam.apnuser, item->item_data[2]);
        }
        if (item->item_data[3][0] != 0 && item->item_cnt >= 4)
        {
            strcpy((char *)sysparam.apnpassword, item->item_data[3]);
        }

        startTimer(300, moduleReset, 0);
        paramSaveAll();
        sprintf(message, "Update APN:%s,APN User:%s,APN Password:%s", sysparam.apn, sysparam.apnuser, sysparam.apnpassword);
    }

}


void doUPSInstruction(ITEM *item, char *message)
{
	/* 文件正在传输 */
	if (sysinfo.bleUpgradeOnoff)
	{
		strcpy(message, "Device was upgrading");
		return;
	}
	/* 文件正在传输 */
	if (sysparam.updateStatus)
	{
		strcpy(message, "Device was downloading firmware");
		return;
	}

    if (item->item_cnt >= 3)
    {
        strncpy((char *)bootparam.updateServer, item->item_data[1], 50);
        bootparam.updateServer[49] = 0;
        bootparam.updatePort = atoi(item->item_data[2]);
        bootParamSaveAll();
    }
    bootParamGetAll();
    sprintf(message, "The device will download the firmware from %s:%d in 6 seconds", bootparam.updateServer,
            bootparam.updatePort);
    bootparam.updateStatus = 1;
    strncpy(bootparam.SN, dynamicParam.SN, 20);
    bootparam.SN[19] = 0;
    strncpy(bootparam.apn, sysparam.apn, 50);
    bootparam.apn[49] = 0;
    strncpy(bootparam.apnuser, sysparam.apnuser, 50);
    bootparam.apnuser[49] = 0;
    strncpy(bootparam.apnpassword, sysparam.apnpassword, 50);
    bootparam.apnpassword[49] = 0;
    strncpy(bootparam.codeVersion, EEPROM_VERSION, 50);
    bootparam.codeVersion[49] = 0;
    bootparam.alreadyUpgrade = 0;
	
    bootParamSaveAll();
    startTimer(30, modulePowerOff, 0);
    startTimer(60, portSysReset, 0);
}

void doLOWWInstruction(ITEM *item, char *message)
{
    if (item->item_data[1][0] == 0 || item->item_data[1][0] == '?')
    {
        sysinfo.lowvoltage = sysparam.lowvoltage / 10.0;
        sprintf(message, "The low voltage param is %.1fV", sysinfo.lowvoltage);

    }
    else
    {
        sysparam.lowvoltage = atoi(item->item_data[1]);
        sysinfo.lowvoltage = sysparam.lowvoltage / 10.0;
        paramSaveAll();
        sprintf(message, "When the voltage is lower than %.1fV, an alarm will be generated", sysinfo.lowvoltage);
    }
}

void doLEDInstruction(ITEM *item, char *message)
{
    if (item->item_data[1][0] == 0 || item->item_data[1][0] == '?')
    {
        sprintf(message, "LED was %s", sysparam.ledctrl ? "open" : "close");

    }
    else
    {
        sysparam.ledctrl = atoi(item->item_data[1]);
        paramSaveAll();
        sprintf(message, "%s", sysparam.ledctrl ? "LED ON" : "LED OFF");
    }
}


void doPOITYPEInstruction(ITEM *item, char *message)
{
    if (item->item_data[1][0] == 0 || item->item_data[1][0] == '?')
    {
        switch (sysparam.poitype)
        {
            case 0:
                strcpy(message, "Current poitype is only LBS reporting");
                break;
            case 1:
                strcpy(message, "Current poitype is LBS and GPS reporting");
                break;
            case 2:
                strcpy(message, "Current poitype is LBS ,WIFI and GPS reporting");
                break;
        }
    }
    else
    {
        if (strstr(item->item_data[1], "AUTO") != NULL)
        {
            sysparam.poitype = 2;
        }
        else
        {
            sysparam.poitype = atoi(item->item_data[1]);
        }
        switch (sysparam.poitype)
        {
            case 0:
                strcpy(message, "Set to only LBS reporting");
                break;
            case 1:
                strcpy(message, "Set to LBS and GPS reporting");
                break;
            case 2:
                strcpy(message, "Set to LBS ,WIFI and GPS reporting");
                break;
            default:
                sysparam.poitype = 2;
                strcpy(message, "Unknow type,default set to LBS ,WIFI and GPS reporting");
                break;
        }
        paramSaveAll();

    }
}

void doResetInstruction(ITEM *item, char *message)
{
    sprintf(message, "System will reset after 7 seconds");
    if (sysparam.simSel == SIM_MODE_1 && dynamicParam.sim == SIM_2)
   	{
		netSetSim(SIM_1);
   	}
    startTimer(40, modulePowerOff, 0);
    startTimer(70, portSysReset, 0);
}

void doUTCInstruction(ITEM *item, char *message)
{
    if (item->item_data[1][0] == 0 || item->item_data[1][0] == '?')
    {
        sprintf(message, "System time zone:UTC %s%d", sysparam.utc >= 0 ? "+" : "", sysparam.utc);
        updateRTCtimeRequest();
    }
    else
    {
        sysparam.utc = atoi(item->item_data[1]);
        updateRTCtimeRequest();
        LogPrintf(DEBUG_ALL, "utc=%d", sysparam.utc);
        if (sysparam.utc < -12 || sysparam.utc > 12)
            sysparam.utc = 8;
        paramSaveAll();
        sprintf(message, "Update the system time zone to UTC %s%d", sysparam.utc >= 0 ? "+" : "", sysparam.utc);
    }
}

static void doAlarmModeInstrucion(ITEM *item, char *message)
{
    if (item->item_data[1][0] == 0 || item->item_data[1][0] == '?')
    {
        sprintf(message, "The light-sensing alarm function was %s", sysparam.ldrEn ? "enable" : "disable");
    }
    else
    {
        if (my_strpach(item->item_data[1], "L1"))
        {
            sysparam.ldrEn = 1;
            sprintf(message, "Enables the light-sensing alarm function successfully");
        }
        else if (my_strpach(item->item_data[1], "L0"))
        {
            sysparam.ldrEn = 0;
            sprintf(message, "Disable the light-sensing alarm function successfully");

        }
        else
        {
            sysparam.ldrEn = 1;
            sprintf(message, "Unknow cmd,enable the light-sensing alarm function by default");
        }
        paramSaveAll();

    }
}


void doDebugInstrucion(ITEM *item, char *message)
{
    uint16_t year;
    uint8_t  month;
    uint8_t date;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;

    portGetRtcDateTime(&year, &month, &date, &hour, &minute, &second);

    sprintf(message, "Time:%.2d/%.2d/%.2d %.2d:%.2d:%.2d;", year, month, date, hour, minute, second);
    sprintf(message + strlen(message), "sysrun:%.2d:%.2d:%.2d;gpsRequest:%02X;gpslast:%.2d:%.2d:%.2d;",
            sysinfo.sysTick / 3600, sysinfo.sysTick % 3600 / 60, sysinfo.sysTick % 60, sysinfo.gpsRequest,
            sysinfo.gpsUpdatetick / 3600, sysinfo.gpsUpdatetick % 3600 / 60, sysinfo.gpsUpdatetick % 60);
    sprintf(message + strlen(message), "hideLogin:%s;", hiddenServerIsReady() ? "Yes" : "No");
    sprintf(message + strlen(message), "bleConnStatus[%s]:%s", sysinfo.bleConnStatus ? "CONNECTED" : "DISCONNECTED", appPeripheralParamCallback());
    sprintf(message + strlen(message), "upgrade server:%s serverfsm:%d blefsm:%d bleupgrade:%d  %d", 
    	upgradeServerIsReady() ? "Yes" : "No", getUpgradeServerFsm(), getBleOtaFsm(), sysparam.relayUpgrade[0], sysparam.relayUpgrade[1]);
    sprintf(message + strlen(message), "SYSONOFF_READ:%d light:%d uncap:%d acc:%d", SYSONOFF_READ, LDR2_READ, LDR1_READ, ACC_READ);

}

void doACCCTLGNSSInstrucion(ITEM *item, char *message)
{
    if (item->item_data[1][0] == 0 || item->item_data[1][0] == '?')
    {
        sprintf(message, "%s", sysparam.accctlgnss ? "GPS is automatically controlled by the program" :
                "The GPS is always be on");
    }
    else
    {
        sysparam.accctlgnss = (uint8_t)atoi((const char *)item->item_data[1]);
        if (sysparam.MODE == MODE2)
        {
            if (sysparam.accctlgnss == 0)
            {
                gpsRequestSet(GPS_REQUEST_GPSKEEPOPEN_CTL);
            }
            else
            {
                gpsRequestClear(GPS_REQUEST_GPSKEEPOPEN_CTL);
            }
        }
        sprintf(message, "%s", sysparam.accctlgnss ? "GPS will be automatically controlled by the program" :
                "The GPS will always be on");
        paramSaveAll();
    }

}


void doAccdetmodeInstruction(ITEM *item, char *message)
{
    if (item->item_data[1][0] == 0 || item->item_data[1][0] == '?')
    {
        switch (sysparam.accdetmode)
        {
            case ACCDETMODE0:
                sprintf(message, "The device is use acc wire to determine whether ACC is ON or OFF.");
                if (sysparam.rfDetType)
                	strcpy(message + strlen(message), "But rf detect type is wire,so only use gsensor");
                break;
            case ACCDETMODE1:
                sprintf(message, "The device is use acc wire first and voltage second to determine whether ACC is ON or OFF.");
                break;
            case ACCDETMODE2:
                sprintf(message, "The device is use acc wire first and gsensor second to determine whether ACC is ON or OFF.");
                break;
        }

    }
    else
    {
        sysparam.accdetmode = atoi(item->item_data[1]);
        switch (sysparam.accdetmode)
        {
            case ACCDETMODE0:
           	 	if (sysparam.rfDetType == 0)
                	sprintf(message, "The device is use acc wire to determine whether ACC is ON or OFF.");
                else
                	sprintf(message, "Rf detect type is wire,so use gsensor");
                	sysparam.accdetmode = 2;
                break;
            case ACCDETMODE1:
                sprintf(message, "The device is use acc wire first and voltage second to determine whether ACC is ON or OFF.");
                break;
            case ACCDETMODE2:
                sprintf(message, "The device is use acc wire first and gsensor second to determine whether ACC is ON or OFF.");
                break;
            default:
                sysparam.accdetmode = ACCDETMODE2;
                sprintf(message,
                        "Unknow mode,Using acc wire first and voltage second to determine whether ACC is ON or OFF by default");
                break;
        }
        paramSaveAll();
    }
}

void doFenceInstrucion(ITEM *item, char *message)
{
    if (item->item_data[1][0] == 0 || item->item_data[1][0] == '?')
    {
        sprintf(message, "The static drift fence is %d meters", sysparam.fence);
    }
    else
    {
        sysparam.fence = atol(item->item_data[1]);
        paramSaveAll();
        sprintf(message, "Set the static drift fence to %d meters", sysparam.fence);
    }

}

void doIccidInstrucion(ITEM *item, char *message)
{
    sendModuleCmd(CCID_CMD, NULL);
    sprintf(message, "ICCID:%s", getModuleICCID());
}

void doFactoryInstrucion(ITEM *item, char *message)
{
    if (my_strpach(item->item_data[1], "ZTINFO8888"))
    {
        paramDefaultInit(0);
        sprintf(message, "Factory all successfully");
    }
    else
    {
        paramDefaultInit(1);
        sprintf(message, "Factory Settings restored successfully");

    }

}
void doSetAgpsInstruction(ITEM *item, char *message)
{
    if (item->item_data[1][0] == 0 || item->item_data[1][0] == '?')
    {
        sprintf(message, "Agps:%s,%d,%s,%s", sysparam.agpsServer, sysparam.agpsPort, sysparam.agpsUser, sysparam.agpsPswd);
    }
    else
    {
        if (item->item_data[1][0] != 0)
        {
            strcpy((char *)sysparam.agpsServer, item->item_data[1]);
            stringToLowwer((char *)sysparam.agpsServer, strlen(sysparam.agpsServer));
        }
        if (item->item_data[2][0] != 0)
        {
            sysparam.agpsPort = atoi(item->item_data[2]);
        }
        if (item->item_data[3][0] != 0)
        {
            strcpy((char *)sysparam.agpsUser, item->item_data[3]);
            stringToLowwer((char *)sysparam.agpsUser, strlen(sysparam.agpsUser));
        }
        if (item->item_data[4][0] != 0)
        {
            strcpy((char *)sysparam.agpsPswd, item->item_data[4]);
            stringToLowwer((char *)sysparam.agpsPswd, strlen(sysparam.agpsPswd));
        }
        paramSaveAll();
        sprintf(message, "Update Agps info:%s,%d,%s,%s", sysparam.agpsServer, sysparam.agpsPort, sysparam.agpsUser,
                sysparam.agpsPswd);
    }
}

static void doJT808SNInstrucion(ITEM *item, char *message)
{
    char senddata[40];
    uint8_t snlen;
    if (item->item_data[1][0] == 0 || item->item_data[1][0] == '?')
    {
        byteToHexString(dynamicParam.jt808sn, (uint8_t *)senddata, 6);
        senddata[12] = 0;
        sprintf(message, "JT808SN:%s", senddata);
    }
    else
    {
        snlen = strlen(item->item_data[1]);
        if (snlen > 12)
        {
            sprintf(message, "SN number too long");
        }
        else
        {
            jt808CreateSn(dynamicParam.jt808sn, (uint8_t *)item->item_data[1], snlen);
            byteToHexString(dynamicParam.jt808sn, (uint8_t *)senddata, 6);
            senddata[12] = 0;
            sprintf(message, "Update SN:%s", senddata);
            dynamicParam.jt808isRegister = 0;
            dynamicParam.jt808AuthLen = 0;
            jt808RegisterLoginInfo(dynamicParam.jt808sn, dynamicParam.jt808isRegister, dynamicParam.jt808AuthCode, dynamicParam.jt808AuthLen);
            dynamicParamSaveAll();
        }
    }
}

static void doHideServerInstruction(ITEM *item, char *message)
{
    if (item->item_data[1][0] == 0 || item->item_data[1][0] == '?')
    {
        sprintf(message, "hidden server %s:%d was %s", sysparam.hiddenServer, sysparam.hiddenPort,
                sysparam.hiddenServOnoff ? "on" : "off");
    }
    else
    {
        if (item->item_data[1][0] == '1')
        {
            sysparam.hiddenServOnoff = 1;
            if (item->item_data[2][0] != 0 && item->item_data[3][0] != 0)
            {
                strncpy((char *)sysparam.hiddenServer, item->item_data[2], 50);
                stringToLowwer(sysparam.hiddenServer, strlen(sysparam.hiddenServer));
                sysparam.hiddenPort = atoi((const char *)item->item_data[3]);
                sprintf(message, "Update hidden server %s:%d and enable it", sysparam.hiddenServer, sysparam.hiddenPort);
            }
            else
            {
                strcpy(message, "please enter your param");
            }
        }
        else
        {
            sysparam.hiddenServOnoff = 0;
            strcpy(message, "Disable hidden server");
        }
        paramSaveAll();
    }
}


static void doBleServerInstruction(ITEM *item, char *message)
{
    if (item->item_data[1][0] == 0 || item->item_data[1][0] == '?')
    {
        sprintf(message, "ble server was %s:%d", sysparam.bleServer, sysparam.bleServerPort);
    }
    else
    {
        if (item->item_data[1][0] != 0 && item->item_data[2][0] != 0)
        {
            strncpy((char *)sysparam.bleServer, item->item_data[1], 50);
            stringToLowwer(sysparam.bleServer, strlen(sysparam.bleServer));
            sysparam.bleServerPort = atoi((const char *)item->item_data[2]);
            sprintf(message, "Update ble server %s:%d", sysparam.bleServer, sysparam.bleServerPort);
            paramSaveAll();
        }
        else
        {
            strcpy(message, "please enter your param");
        }

    }
}

static void doRelayInstrucion(ITEM *item, char *message, insMode_e mode, void *param)
{
    if (item->item_data[1][0] == '1')
    {
        sysparam.relayCtl = 1;
        paramSaveAll();
        relayAutoRequest();
        lastmode = mode;
        sysinfo.bleforceCmd = bleDevGetCnt();
        if (rspTimeOut == -1)
        {
			rspTimeOut = startTimer(300, relayOnRspTimeOut, 0);
        }
    }
    else if (item->item_data[1][0] == '0')
    {
        sysparam.relayCtl = 0;
        lastmode = mode;
        RELAY_OFF;
        paramSaveAll();
        bleRelaySetAllReq(BLE_EVENT_SET_DEVOFF | BLE_EVENT_CLR_CNT);
        bleRelayClearAllReq(BLE_EVENT_SET_DEVON);
        relayAutoClear();
        sysinfo.bleforceCmd = bleDevGetCnt();
        if (bleDevGetCnt() == 0 && sysparam.relayType == 1)
        {
        	strcpy(message, "relayoff success");
        	alarmRequestSet(ALARM_OIL_RESTORE_REQUEST);
			sendMsgWithMode((uint8_t *)message, strlen(message), mode, param);
        }
        else
        {
			if (sysparam.relayCloseCmd != 0)
	        {
				sysparam.relayCloseCmd = 0;
				paramSaveAll();
				LogPrintf(DEBUG_ALL, "clear relay close cmd");
	        }
	        if (rspTimeOut == -1)
	        {
	            rspTimeOut = startTimer(300, relayOffRspTimeOut, 0);
	        }
        }
    }
    else
    {
        sprintf(message, "Relay status %s", sysparam.relayCtl == 1 ? "on" : "off");
        sendMsgWithMode((uint8_t *)message, strlen(message), mode, param);
    }
    
}

static void doReadParamInstruction(ITEM *item, char *message)
{
    uint8_t i, cnt;
    bleRelayInfo_s *bleinfo;
    deviceConnInfo_s *devinfo;
    cnt = 0;
    if (getBleOtaStatus() >= 0)
    {
		sprintf(message, "Dev(%d) is not ready ", getBleOtaStatus());
		return;
    }
    bleDevReadAllRssi();
    for (i = 0; i < BLE_CONNECT_LIST_SIZE; i++)
    {
        bleinfo = bleRelayGeInfo(i);
        
        if (bleinfo != NULL)
        {
            cnt++;
            devinfo = bleDevGetInfoById(i); 
            sprintf(message + strlen(message), "[T:%ld,V:(%.2fV,%.2fV),RSSI:%d] ", sysinfo.sysTick - bleinfo->updateTick, bleinfo->rfV,
                    bleinfo->outV, devinfo->rssi);
        }
    }
    if (cnt == 0)
    {
        sprintf(message, "no ble info");
    }
}

static void doSetBleParamInstruction(ITEM *item, char *message)
{
    uint8_t i, cnt;
    bleRelayInfo_s *bleinfo;
    if (item->item_data[1][0] == 0 || item->item_data[1][0] == '?')
    {
        cnt = 0;

        for (i = 0; i < BLE_CONNECT_LIST_SIZE; i++)
        {
            bleinfo = bleRelayGeInfo(i);
            if (bleinfo != NULL)
            {
                cnt++;
                sprintf(message + strlen(message), "(%d:[%.2f,%.2f,%d]) ", i, bleinfo->rf_threshold, bleinfo->out_threshold,
                        bleinfo->disc_threshold);
            }
        }
        if (cnt == 0)
        {
            sprintf(message, "no ble info");
        }
        else
        {
            sprintf(message + strlen(message), "DISC:%dm", sysparam.bleAutoDisc);
        }
    }
    else
    {
        for (i = 0; i < BLE_CONNECT_LIST_SIZE; i++)
        {
            bleinfo = bleRelayGeInfo(i);
            if (bleinfo != NULL)
            {
                bleinfo->rf_threshold = 0;
                bleinfo->out_threshold = 0;
            }
        }
        sysparam.bleRfThreshold = atoi(item->item_data[1]);
        sysparam.bleOutThreshold = atoi(item->item_data[2]);
        sysparam.bleAutoDisc = atoi(item->item_data[3]);
        paramSaveAll();
        sprintf(message, "Update new ble param to %.2fv,%.2fv,%d", sysparam.bleRfThreshold / 100.0,
                sysparam.bleOutThreshold / 100.0, sysparam.bleAutoDisc);
        bleRelaySetAllReq(BLE_EVENT_SET_RF_THRE | BLE_EVENT_SET_OUTV_THRE | BLE_EVENT_SET_AD_THRE | BLE_EVENT_GET_RF_THRE |
                          BLE_EVENT_GET_OUT_THRE | BLE_EVENT_GET_AD_THRE);
    }
}
static void doSetBleWarnParamInstruction(ITEM *item, char *message)
{
    uint8_t i, cnt;
    bleRelayInfo_s *bleinfo;

    if (item->item_data[1][0] == 0 || item->item_data[1][0] == '?')
    {
        cnt = 0;
        for (i = 0; i < BLE_CONNECT_LIST_SIZE; i++)
        {
            bleinfo = bleRelayGeInfo(i);
            if (bleinfo != NULL)
            {
                cnt++;
                sprintf(message + strlen(message), "(%d:[%.2f,%d,%d]) ", i, bleinfo->preV_threshold / 100.0,
                        bleinfo->preDetCnt_threshold,
                        bleinfo->preHold_threshold);
            }
        }
        if (cnt == 0)
        {
            sprintf(message, "no ble info");
        }
    }
    else
    {
    	if (item->item_data[1][0] != 0)
        	sysparam.blePreShieldVoltage = atoi(item->item_data[1]);
        if (item->item_data[2][0] != 0)
        	sysparam.blePreShieldDetCnt = atoi(item->item_data[2]);
        if (item->item_data[3][0] != 0)
        	sysparam.blePreShieldHoldTime = atoi(item->item_data[3]);
//        if (sysparam.blePreShieldDetCnt >= 60)
//        {
//            sysparam.blePreShieldDetCnt = 30;
//        }
//        else if (sysparam.blePreShieldDetCnt == 0)
//        {
//            sysparam.blePreShieldDetCnt = 10;
//        }
//        if (sysparam.blePreShieldHoldTime == 0)
//        {
//            sysparam.blePreShieldHoldTime = 1;
//       }
        paramSaveAll();
        sprintf(message, "Update ble warnning param to %d,%d,%d", sysparam.blePreShieldVoltage, sysparam.blePreShieldDetCnt,
                sysparam.blePreShieldHoldTime);
        for (i = 0; i < BLE_CONNECT_LIST_SIZE; i++)
        {
            bleinfo = bleRelayGeInfo(i);
            if (bleinfo != NULL)
            {
                bleinfo->preV_threshold = 0;
                bleinfo->preDetCnt_threshold = 0;
                bleinfo->preHold_threshold = 0;
            }
        }
        bleRelaySetAllReq(BLE_EVENT_SET_PRE_PARAM | BLE_EVENT_GET_PRE_PARAM);
    }
}

static void doSetBleRfParamInstruction(ITEM *item, char *message)
{
	uint8_t i, cnt;
    bleRelayInfo_s *bleinfo;
    if (item->item_data[1][0] == 0 || item->item_data[1][0] == '?')
    {
        cnt = 0;

        for (i = 0; i < BLE_CONNECT_LIST_SIZE; i++)
        {
            bleinfo = bleRelayGeInfo(i);
            if (bleinfo != NULL)
            {
                cnt++;
                sprintf(message + strlen(message), "(%d:[%.2f,%d]) ", i, bleinfo->shieldDetectVol / 100.0, bleinfo->shieldDetectTime);
            }
        }
        if (cnt == 0)
        {
            sprintf(message, "no ble info");
        }
       
        bleRelaySetAllReq(BLE_EVENT_GET_RF_PARAM);
    }
    else
    {
        for (i = 0; i < BLE_CONNECT_LIST_SIZE; i++)
        {
            bleinfo = bleRelayGeInfo(i);
            if (bleinfo != NULL)
            {
                bleinfo->rf_threshold = 0;
                bleinfo->shieldDetTime = 0;
            }
        }
        sysparam.shieldDetectVol = atoi(item->item_data[1]);
        sysparam.shieldDetectTime = atoi(item->item_data[2]);

        paramSaveAll();
        sprintf(message, "Update new ble param to %.2fv,%d", sysparam.shieldDetectVol / 100.0, sysparam.shieldDetectTime);
        bleRelaySetAllReq(BLE_EVENT_SET_RF_PARAM | BLE_EVENT_GET_RF_PARAM);
    }
}


static void doSetBleMacInstruction(ITEM *item, char *message)
{
    uint8_t i, j, l, ind;
    char mac[20];
    char mac2[20];
    if (item->item_data[1][0] == 0 || item->item_data[1][0] == '?')
    {
        strcpy(message, "BLELIST:");
        for (i = 0; i < sysparam.bleMacCnt; i++)
        {
            byteToHexString(sysparam.bleConnMac[i], (uint8_t *)mac, 6);
            mac[12] = 0;

            l = 5;
            for (j = 0; j < 3; j++)
            {
                tmos_memcpy(mac2, &mac[j * 2], 2);
                tmos_memcpy(&mac[j * 2], &mac[l * 2], 2);
                tmos_memcpy(&mac[l * 2], mac2, 2);
                l--;
            }
            sprintf(message + strlen(message), " %s", mac);
        }
        strcat(message, ";");
    }
    else
    {
        bleRelayDeleteAll();
        tmos_memset(sysparam.bleConnMac, 0, sizeof(sysparam.bleConnMac));
        sysparam.bleMacCnt = 0;
        ind = 0;
        strcpy(message, "Enable ble function,Update MAC: ");
        for (i = 1; i < item->item_cnt; i++)
        {
            if (strlen(item->item_data[i]) != 12)
            {
                continue;
            }
            if (strncmp(item->item_data[i], "000000000000", 12) == 0)
            {
				LogPrintf(DEBUG_ALL, "filter 00");
            	continue;
            }

            l = 0;
            for (j = 0; j < 12; j += 2)
            {
                tmos_memcpy(mac + l, item->item_data[i] + j, 2);
                l += 2;
                if (j % 2 == 0 && (j + 2 < 12))
                {
                    mac[l++] = ':';
                }
            }
            mac[l] = 0;
            sprintf(message + strlen(message), " %s", mac);

            l = 5;
            for (j = 0; j < 3; j++)
            {
                tmos_memcpy(mac, &item->item_data[i][j * 2], 2);
                tmos_memcpy(&item->item_data[i][j * 2], &item->item_data[i][l * 2], 2);
                tmos_memcpy(&item->item_data[i][l * 2], mac, 2);
                l--;
            }
            if (ind < 2)
            {
                changeHexStringToByteArray(sysparam.bleConnMac[ind], item->item_data[i], 6);
                //bleRelayInsert(sysparam.bleConnMac[ind], 0);
                ind++;
            }
        }
        if (ind == 0)
        {
            strcpy(message, "Disable the ble function,and the ble mac was clear");
        }
        sysparam.bleMacCnt = ind;
        if (sysparam.bleMacCnt != 0)
        {
			sysparam.relayType = 0;
        }
        //startTimer(100, setBleMacCallback, 0);
        paramSaveAll();
        if (primaryServerIsReady() && sysparam.protocol != JT808_PROTOCOL_TYPE)
        {
			protocolSend(NORMAL_LINK, PROTOCOL_0B, NULL);
        }
    }
}


static void doRelaySpeedInstruction(ITEM *item, char *message)
{
    if (item->item_data[1][0] == 0 || item->item_data[1][0] == '?')
    {
        sprintf(message, "Current relaySpeed %d km/h", sysparam.relaySpeed);
    }
    else
    {
        sysparam.relaySpeed = atoi(item->item_data[1]);
        paramSaveAll();
        sprintf(message, "Update relaySpeed %d km/h", sysparam.relaySpeed);
    }
}


static void doRelayForceInstrucion(ITEM *item, char *message)
{
    if (item->item_data[1][0] == '1')
    {
    	RELAY_ON;
        sysparam.relayCtl = 1;
        paramSaveAll();
        relayAutoClear();
        bleRelaySetAllReq(BLE_EVENT_SET_DEVON);
        bleRelayClearAllReq(BLE_EVENT_SET_DEVOFF);
		sysinfo.bleforceCmd = bleDevGetCnt();
        strcpy(message, "Relay force on");
    }
    else if (item->item_data[1][0] == '0')
    {
        sysparam.relayCtl = 0;
        paramSaveAll();
        RELAY_OFF;
        relayAutoClear();
        bleRelaySetAllReq(BLE_EVENT_SET_DEVOFF | BLE_EVENT_CLR_CNT);
        bleRelayClearAllReq(BLE_EVENT_SET_DEVON);
        sysinfo.bleforceCmd = bleDevGetCnt();
       	if (sysparam.relayCloseCmd != 0)
        {
			sysparam.relayCloseCmd = 0;
			paramSaveAll();
			LogPrintf(DEBUG_ALL, "clear relay close cmd");
        }
        strcpy(message, "Relay force off");
    }
    else
    {
        sprintf(message, "Relay status was %s", sysparam.relayCtl == 1 ? "relay on" : "relay off");
    }
}

void doBFInstruction(ITEM *item, char *message)
{
    terminalDefense();
    sysparam.bf = 1;
    paramSaveAll();
    strcpy(message, "BF OK");
}


void doCFInstruction(ITEM *item, char *message)
{
    terminalDisarm();
    sysparam.bf = 0;
    paramSaveAll();
    strcpy(message, "CF OK");
}

void doProtectVolInstrucion(ITEM *item, char *message)
{
    if (item->item_data[1][0] == 0 || item->item_data[1][0] == '?')
    {
        sprintf(message, "Current protect voltage is %.2f V", sysparam.protectVoltage);
    }
    else
    {
        sysparam.protectVoltage = atoi(item->item_data[1]) / 10.0;
        sprintf(message, "Update protect voltage to %.2f V", sysparam.protectVoltage);
        paramSaveAll();
    }
}
void doTimerInstrucion(ITEM *item, char *message)
{
    if (item->item_data[1][0] == 0 || item->item_data[1][0] == '?')
    {
        sprintf(message, "Current timer is %d", sysparam.gpsuploadgap);
    }
    else
    {
        sysparam.gpsuploadgap = (uint16_t)atoi((const char *)item->item_data[1]);
        sprintf(message, "Update timer to %d", sysparam.gpsuploadgap);
        if (sysparam.MODE == MODE2 || sysparam.MODE == MODE21 || sysparam.MODE == MODE23)
        {
            if (sysparam.gpsuploadgap == 0)
            {
                gpsRequestClear(GPS_REQUEST_ACC_CTL);
            }
            else
            {
                if (getTerminalAccState())
                {
                    if (sysparam.gpsuploadgap < GPS_UPLOAD_GAP_MAX)
                    {
                        gpsRequestSet(GPS_REQUEST_ACC_CTL);
                    }
                    else
                    {
                        gpsRequestClear(GPS_REQUEST_ACC_CTL);
                    }
                }
                else
                {
                    gpsRequestClear(GPS_REQUEST_ACC_CTL);
                }
            }
        }
        paramSaveAll();
    }
}

void doSOSInstruction(ITEM *item, char *messages, insMode_e mode, void *param)
{
    uint8_t i, j, k;
    insParam_s *insparam;

    if (item->item_data[1][0] == 0 || item->item_data[1][0] == '?')
    {
        strcpy(messages, "Query sos number:");
        for (i = 0; i < 3; i++)
        {
            sprintf(messages + strlen(messages), " %s", sysparam.sosNum[i][0] == 0 ? "NULL" : (char *)sysparam.sosNum[i]);
        }
    }
    else
    {

//        if (mode == SMS_MODE && param != NULL)
//        {
//            insparam = (insParam_s *)param;
//            if (my_strstr(insparam->telNum, (char *)sysparam.centerNum, strlen(insparam->telNum)) == 0)
//            {
//                strcpy(messages, "Operation failure,please use center number!");
//                return ;
//            }
//        }
        if (item->item_data[1][0] == 'A' || item->item_data[1][0] == 'a')
        {
            for (i = 0; i < 3; i++)
            {
                for (j = 0; j < 19; j++)
                {
                    sysparam.sosNum[i][j] = item->item_data[2 + i][j];
                }
                sysparam.sosNum[i][19] = 0;
            }
            strcpy(messages, "Add sos number:");
            for (i = 0; i < 3; i++)
            {
                sprintf(messages + strlen(messages), " %s", sysparam.sosNum[i][0] == 0 ? "NULL" : (char *)sysparam.sosNum[i]);
            }
        }
        else if (item->item_data[1][0] == 'D' || item->item_data[1][0] == 'd')
        {
            j = strlen(item->item_data[2]);
            if (j < 20 && j > 0)
            {
                k = 0;
                for (i = 0; i < 3; i++)
                {
                    if (my_strpach((char *)sysparam.sosNum[i], item->item_data[2]))
                    {
                        sprintf(messages + strlen(messages), "Delete %s OK;", (char *)sysparam.sosNum[i]);
                        sysparam.sosNum[i][0] = 0;
                        k = 1;
                    }
                }
                if (k == 0)
                {
                    sprintf(messages, "Delete %s Fail,no this number", item->item_data[2]);
                }
            }
            else
            {
                strcpy(messages, "Invalid sos number");
            }
        }
        else
        {
            strcpy(messages, "Invalid option");
        }
        paramSaveAll();
    }
}

void doCenterInstruction(ITEM *item, char *messages, insMode_e mode, void *param)
{
    uint8_t j, k;
    insParam_s *insparam;
    if (item->item_data[1][0] == 0 || item->item_data[1][0] == '?')
    {
        strcpy(messages, "Query center number:");
        sprintf(messages + strlen(messages), " %s", sysparam.centerNum[0] == 0 ? "NULL" : (char *)sysparam.centerNum);
    }
    else
    {
        if (mode == SMS_MODE && param != NULL)
        {
            insparam = (insParam_s *)param;
            if (my_strstr(insparam->telNum, (char *)sysparam.centerNum, strlen(insparam->telNum)) == 0)
            {
                strcpy(messages, "Operation failure,please use center number!");
                return;
            }
        }
        if (item->item_data[1][0] == 'A' || item->item_data[1][0] == 'a')
        {
            for (j = 0; j < 19; j++)
            {
                sysparam.centerNum[j] = item->item_data[2][j];
            }
            sysparam.centerNum[19] = 0;
            strcpy(messages, "Add center number:");
            sprintf(messages + strlen(messages), " %s", sysparam.centerNum[0] == 0 ? "NULL" : (char *)sysparam.centerNum);
        }
        else if (item->item_data[1][0] == 'D' || item->item_data[1][0] == 'd')
        {
            j = strlen(item->item_data[2]);
            if (j < 20 && j > 0)
            {
                k = 0;
                if (my_strpach((char *)sysparam.centerNum, item->item_data[2]))
                {
                    sprintf(messages + strlen(messages), "Delete %s OK;", (char *)sysparam.centerNum);
                    sysparam.centerNum[0] = 0;
                    k = 1;
                }
                if (k == 0)
                {
                    sprintf(messages, "Delete %s Fail,no this number", item->item_data[2]);
                }
            }
            else
            {
                strcpy(messages, "Invalid center number");
            }
        }
        else
        {
            strcpy(messages, "Invalid option");
        }
        paramSaveAll();
    }
}

void doSosAlmInstruction(ITEM *item, char *message)
{
    if (item->item_data[1][0] == 0 || item->item_data[1][0] == '?')
    {
        switch (sysparam.sosalm)
        {
            case ALARM_TYPE_NONE:
                sprintf(message, "Query:%s", "SOS alarm was closed");
                break;
            case ALARM_TYPE_GPRS:
                sprintf(message, "Query:the device will %s when sos occur", "only send gprs");
                break;
            case ALARM_TYPE_GPRS_SMS:
                sprintf(message, "Query:the device will %s when sos occur", "send gprs and sms");
                break;
            case ALARM_TYPE_GPRS_SMS_TEL:
                sprintf(message, "Query:the device will %s when sos occur", "send gprs,sms and call sos number");
                break;
            default:
                strcpy(message, "Unknow");
                break;
        }
    }
    else
    {
        sysparam.sosalm = atoi(item->item_data[1]);
        switch (sysparam.sosalm)
        {
            case ALARM_TYPE_NONE:
                strcpy(message, "Close sos function");
                break;
            case ALARM_TYPE_GPRS:
                sprintf(message, "The device will %s when sos occur", "only send gprs");
                break;
            case ALARM_TYPE_GPRS_SMS:
                sprintf(message, "The device will %s when sos occur", "send gprs and sms");
                break;
            case ALARM_TYPE_GPRS_SMS_TEL:
                sprintf(message, "The device will %s when sos occur", "send gprs,sms and call sos number");
                break;
            default:
                sprintf(message, "%s", "Unknow status,change to send gprs by default");
                sysparam.sosalm = ALARM_TYPE_GPRS;
                break;
        }
        paramSaveAll();
    }
}


static void doQfotaInstruction(ITEM *item, char *message)
{
//    char param[120];
//    if (item->item_cnt < 2)
//    {
//        strcpy(message, "Please enter the upgrade url");
//        return;
//    }
//    sprintf(message, "Fota URL:%s", item->item_data[1]);
//    sprintf(param, "\"%s\",1,50,100", item->item_data[1]);
//    sendModuleCmd(QFOTADL_CMD, param);
}

static void doMotionDetInstruction(ITEM *item, char *message)
{
    if (item->item_data[1][0] == 0 || item->item_data[1][0] == '?')
    {
        sprintf(message, "Motion param %d,%d,%d", sysparam.gsdettime, sysparam.gsValidCnt, sysparam.gsInvalidCnt);
    }
    else
    {
        sysparam.gsdettime = atoi(item->item_data[1]);
        sysparam.gsValidCnt = atoi(item->item_data[2]);
        sysparam.gsInvalidCnt = atoi(item->item_data[3]);

        sysparam.gsdettime = (sysparam.gsdettime > motionGetSize() ||
                              sysparam.gsdettime == 0) ? motionGetSize() : sysparam.gsdettime;
        sysparam.gsValidCnt = (sysparam.gsValidCnt > sysparam.gsdettime ||
                               sysparam.gsValidCnt == 0) ? sysparam.gsdettime : sysparam.gsValidCnt;

        sysparam.gsInvalidCnt = sysparam.gsInvalidCnt > sysparam.gsValidCnt ? sysparam.gsValidCnt : sysparam.gsInvalidCnt;
        paramSaveAll();
        sprintf(message, "Update motion param to %d,%d,%d", sysparam.gsdettime, sysparam.gsValidCnt, sysparam.gsInvalidCnt);
    }
}
//FCG=SET1,132,128
static void doFcgInstruction(ITEM *item, char *message)
{
    if (item->item_data[1][0] == 0 || item->item_data[1][0] == '?' || item->item_cnt < 3)
    {
        sprintf(message, "ACC On %.2fV , ACC Off %.2fV", sysparam.accOnVoltage, sysparam.accOffVoltage);
    }
    else
    {
        if (item->item_cnt >= 4)
        {
            sysparam.accOnVoltage = atof(item->item_data[2]) / 10.0;
            sysparam.accOffVoltage = atof(item->item_data[3]) / 10.0;
            if (sysparam.accOffVoltage >= sysparam.accOnVoltage)
            {
                sysparam.accOnVoltage = 13.2;
                sysparam.accOffVoltage = 12.8;
            }
            paramSaveAll();
            sprintf(message, "SetUp ACC On to %.2fV , ACC Off to %.2fV", sysparam.accOnVoltage, sysparam.accOffVoltage);

        }
        else
        {
            strcpy(message, "Please use FCG=SET,<on vol>,<off vol>#");
        }
    }
}

static void doBleenInstruction(ITEM *item, char *message)
{
	if (item->item_data[1][0] == 0 || item->item_data[1][0] == '?')
	{
		sprintf(message, "Bleen is %s", sysparam.bleen ? "On" : "Off");
	}
	else
	{
		sysparam.bleen = atoi(item->item_data[1]);
		if (sysparam.bleen == 1)
		{	
			char broadCastNmae[30];
			sprintf(broadCastNmae, "%s-%s", "AUTO", dynamicParam.SN + 9);
			appPeripheralBroadcastInfoCfg(broadCastNmae);
		}
		else if (sysparam.bleen == 0)
		{
			appPeripheralCancel();
		}

		sprintf(message, "Bleen is %s", sysparam.bleen ? "On" : "Off");
		paramSaveAll();
	}
}

static void doAgpsenInstrution(ITEM *item, char *message)
{
	if (item->item_data[1][0] == 0 || item->item_data[1][0] == '?')
	{
		sprintf(message, "Agpsen is %s", sysparam.agpsen ? "On" : "Off");
	}
	else
	{
		sysparam.agpsen = atoi(item->item_data[1]);
		sprintf(message, "Agpsen is %s", sysparam.agpsen ? "On" : "Off");
		paramSaveAll();
	}
}

static void doBleRelayCtlInstruction(ITEM *item, char *message)
{
	if (item->item_data[1][0] == 0 || item->item_data[1][0] == '?')
    {
        sprintf(message, "BleRelayCtrl param is %d,%.2f", sysparam.bleRelay, sysparam.bleVoltage);
        return ;
    }
    sysparam.bleRelay = atoi(item->item_data[1]);
    sysparam.bleVoltage = atof(item->item_data[2]);
    paramSaveAll();
    sprintf(message, "set success ,%d,%.2f", sysparam.bleRelay, sysparam.bleVoltage);
	
}

static void doRelayFunInstruction(ITEM *item, char *message)
{
    if (item->item_data[1][0] == 0 || item->item_data[1][0] == '?')
    {
        sprintf(message, "Current relayfun %d", sysparam.relayFun);
    }
    else
    {
        sysparam.relayFun = atoi(item->item_data[1]);
        paramSaveAll();
        sprintf(message, "Update relayfun %d", sysparam.relayFun);
    }
}

static void doSetBatRateInstruction(ITEM *item, char *message)
{
    if (item->item_data[1][0] == 0 || item->item_data[1][0] == '?')
    {
        sprintf(message, "Bat high rate is %.2f, low rate is %.2f", sysparam.batLowLevel, sysparam.batHighLevel);
    }
    else
    {
        sysparam.batLowLevel = atof(item->item_data[1]);
        sysparam.batHighLevel = atof(item->item_data[2]);
        sprintf(message, "Update bat high rate to %.2f, low rate to %.2f", sysparam.batLowLevel, sysparam.batHighLevel);
        paramSaveAll();
    }
}

static void doSetmileInstruction(ITEM *item, char *message)
{
	if (item->item_data[1][0] == 0 || item->item_data[1][0] == '?')
    {
        sprintf(message, "Current mileage is %.2lf km, milecal is %d%%", 
				(sysparam.mileage * (double)(sysparam.milecal / 100.0 + 1.0)) / 1000, sysparam.milecal);
    }
    else 
    {
		sysparam.mileage = atof(item->item_data[1]) * 1000;
		if (item->item_data[2][0] != 0)
		{
			sysparam.milecal = atoi(item->item_data[2]);
		}
		sprintf(message, "Update mileage to %.2lf km, milecal is %d%%", sysparam.mileage / 1000, sysparam.milecal);
    }
}

static void doUncapAlmInstrucion(ITEM *item, char *message)
{
    if (item->item_data[1][0] == 0 || item->item_data[1][0] == '?')
    {
        sprintf(message, "Uncap alarm was %s", sysparam.uncapalm ? "Enable" : "Disable");
        sprintf(message + strlen(message), ", %s locking the car", sysparam.uncapLock ? "Enable" : "Disable");
    }
    else
    {
        sysparam.uncapalm = atol(item->item_data[1]);
        sysparam.uncapLock = 0;
        if (sysparam.uncapalm != 0)
        {
            sysparam.uncapLock = atol(item->item_data[2]);
        }
        paramSaveAll();
        sprintf(message, "%s the Uncap alarm", sysparam.uncapalm ? "Enable" : "Disable");
        if (sysparam.uncapLock != 0)
        {
            sprintf(message + strlen(message), ", %s locking the car", sysparam.uncapLock ? "Enable" : "Disable");
        }
    }
}

static void doShutdownAlmInstrucion(ITEM *item, char *message)
{
    if (item->item_data[1][0] == 0 || item->item_data[1][0] == '?')
    {
        sprintf(message, "Shutdown alarm was %s", sysparam.shutdownalm ? "Enable" : "Disable");
        if (sysparam.shutdownLock != 0)
        {
            sprintf(message + strlen(message), ", %s locking the car", sysparam.shutdownLock ? "Enable" : "Disable");
        }
    }
    else
    {
        sysparam.shutdownalm = atol(item->item_data[1]);
        sysparam.shutdownLock = 0;
        if (sysparam.shutdownalm != 0)
        {
            sysparam.shutdownLock = atol(item->item_data[2]);
        }
        paramSaveAll();
        sprintf(message, "%s the shutdown alarm", sysparam.shutdownalm ? "Enable" : "Disable");

        if (sysparam.shutdownLock != 0)
        {
            sprintf(message + strlen(message), ", %s locking the car", sysparam.shutdownLock ? "Enable" : "Disable");
        }
    }
}


static void doSimSelInstrucion(ITEM *item, char *message)
{
    if (item->item_data[1][0] == 0 || item->item_data[1][0] == '?')
    {
        sprintf(message, "Current SimSel is %d , SimID is %d", sysparam.simSel, dynamicParam.sim);
    }
    else
    {
        sysparam.simSel = atoi(item->item_data[1]) > 0 ? SIM_MODE_2 : SIM_MODE_1;
        if (sysparam.simSel == SIM_MODE_2)
        {
			netSetSim(SIM_2);
        }
        else
        {
			netSetSim(SIM_1);
        }
        paramSaveAll();
        startTimer(30, moduleReset, 0);
        sprintf(message, "Set sim sel alarm mode(%d) successfully,module will reboot within 3 sec", sysparam.simSel);
    }
}

static void doSimPullOutAlmInstrucion(ITEM *item, char *message)
{
    if (item->item_data[1][0] == 0 || item->item_data[1][0] == '?')
    {
        sprintf(message, "Sim pull out alarm is %s", sysparam.simpulloutalm ? "Enable" : "Disable");
        sprintf(message + strlen(message), ", locking the car function is %s", sysparam.simpulloutLock ? "Enable" : "Disable");
    }
    else
    {
        sysparam.simpulloutalm = atoi(item->item_data[1]);
        sysparam.simpulloutLock = 0;
        if (sysparam.simpulloutalm != 0)
        {
            sysparam.simpulloutLock = atoi(item->item_data[2]);
        }
        paramSaveAll();
        sprintf(message, "%s the sim pull out alarm", sysparam.simpulloutalm ? "Enable" : "Disable");
        if (sysparam.simpulloutLock != 0)
        {
            sprintf(message + strlen(message), ", %s locking the car", sysparam.simpulloutLock ? "Enable" : "Disable");
        }
    }
}



static void doReadVersionInstruction(ITEM *item, char *message)
{
	uint8_t i, cnt;
    bleRelayInfo_s *bleinfo;
    cnt = 0;
    bleRelaySetAllReq(BLE_EVENT_VERSION);
    for (i = 0; i < BLE_CONNECT_LIST_SIZE; i++)
    {
        bleinfo = bleRelayGeInfo(i);
        if (bleinfo != NULL)
        {
            cnt++;
            
            sprintf(message + strlen(message), "Dev(%d) version:%s; ", i, bleinfo->version);
        }
    }
    if (cnt == 0)
    {
        sprintf(message, "no ble info");
    }
}

static void doSetTxPowerInstruction(ITEM *item, char *message)
{
	uint8_t db;
	if (item->item_data[1][0] == 0 || item->item_data[1][0] == '?')
	{
		switch (sysparam.txPower)
        {
            case LL_TX_POWEER_0_DBM:
                db = APP_TX_POWEER_0_DBM;
                break;
            case LL_TX_POWEER_1_DBM:
                db = APP_TX_POWEER_1_DBM;
                break;
            case LL_TX_POWEER_2_DBM:
                db = APP_TX_POWEER_2_DBM;
                break;
            case LL_TX_POWEER_3_DBM:
                db = APP_TX_POWEER_3_DBM;
                break;
            case LL_TX_POWEER_4_DBM:
                db = APP_TX_POWEER_4_DBM;
                break;
            case LL_TX_POWEER_5_DBM:
                db = APP_TX_POWEER_5_DBM;
                break;
            case LL_TX_POWEER_6_DBM:
            	db = APP_TX_POWEER_6_DBM;
            	break;
            default:
               	db = APP_TX_POWEER_6_DBM;
                break;
        }
        sprintf(message, "Ble tx power is %d DB[0x%02x]", db, sysparam.txPower);
    }
    else
    {
		db = atoi(item->item_data[1]);
		switch (db)
        {
            case APP_TX_POWEER_0_DBM:
                sysparam.txPower = LL_TX_POWEER_0_DBM;
                break;
            case APP_TX_POWEER_1_DBM:
                sysparam.txPower = LL_TX_POWEER_1_DBM;
                break;
            case APP_TX_POWEER_2_DBM:
                sysparam.txPower = LL_TX_POWEER_2_DBM;
                break;
            case APP_TX_POWEER_3_DBM:
                sysparam.txPower = LL_TX_POWEER_3_DBM;
                break;
            case APP_TX_POWEER_4_DBM:
                sysparam.txPower = LL_TX_POWEER_4_DBM;
                break;
            case APP_TX_POWEER_5_DBM:
                sysparam.txPower = LL_TX_POWEER_5_DBM;
                break;
            case APP_TX_POWEER_6_DBM:
            	sysparam.txPower = LL_TX_POWEER_6_DBM;
            	break;
            default:
               	sysparam.txPower = LL_TX_POWEER_5_DBM;
                break;
        }
        bleRelaySetAllReq(BLE_EVENT_SET_TXPOWER);
        sprintf(message, "Update ble tx power to %d DB[0x%02x]", db, sysparam.txPower);
        paramSaveAll();
    }
	
}


/**************************************************
@bref       升级文件下载
@param
@return
@note
**************************************************/

static void doFileInstruction(ITEM *item, char *message)
{
	fileInfo_s *file;
	uint8_t i;
	uint8_t num;
	uint8_t index = 0;
	file = getFileList(&num);
	fileQueryList();
	if (item->item_data[1][0] == 0 || item->item_data[1][0] == '?')
	{
		if (sysparam.updateStatus)
		{
			sprintf(message, ">>>>>>>>>> Download [%s]completed progress %.1f%% <<<<<<<<<<", getNewCodeVersion(),
                  ((float)getRxfileOffset() / getFileTotalSize()) * 100);
		}
		else
		{

			for (i = 0; i < num; i++)
			{
				sprintf(message + strlen(message), "[file(%d):%s, size:%d bytes]\n", i, file[i].fileName, file[i].fileSize);
			}
		}
	}
	else
	{
		if (strncmp(item->item_data[1], "A", strlen(item->item_data[1])) == 0 ||
			strncmp(item->item_data[1], "a", strlen(item->item_data[1])) == 0)
		{
			if (sysparam.updateStatus)
			{
				strcpy(message, "Forbidden operation, file is downloading");
				return;
			}
			if (sysinfo.bleUpgradeOnoff)
			{
				strcpy(message, "Ble was upgrading");
				return;
			}
			if (item->item_data[2][0] != 0)
			{
				strncpy(sysparam.upgradeServer, item->item_data[2], 50);
				sysparam.upgradeServer[49] = 0;
			}
			if (item->item_data[3][0] != 0)
			{
				sysparam.upgradePort = atoi(item->item_data[3]);
			}
			sprintf(message, "Device will download the firmware from %s:%d in 5 seconds", sysparam.upgradeServer,
			sysparam.upgradePort);
			upgradeServerInit(0);
			paramSaveAll();

		}
		else if (strncmp(item->item_data[1], "D", strlen(item->item_data[1])) == 0 ||
			     strncmp(item->item_data[1], "d", strlen(item->item_data[1])) == 0)
		{
			if (sysparam.updateStatus)
			{
				strcpy(message, "Download server is already open");
				return;
			}
			if (strlen(item->item_data[2]) == 0)
			{
				strcpy(message, "Please enter file param which you want to delete");
				return;
			}
			if (strlen(item->item_data[2]) <= 2 && strlen(item->item_data[2]) > 0)
			{
				index = atoi(item->item_data[2]);
				if (index >= num)
				{
					strcpy(message, "file number is error");
					return;
				}
				fileDelete(file[index].fileName);
				sprintf(message, "Delete file[%s] success", file[index].fileName);
			}
			else
			{
				for (i = 0; i < num; i++)
				{
					if (strncmp(file[i].fileName, item->item_data[2], strlen(item->item_data[2])) == 0)
						break;
				}
				if (num == i)
				{
					strcpy(message, "file name is error");
					return;
				}
				else
				{
					fileDelete(item->item_data[2]);
					sprintf(message, "Delete file[%s] success", item->item_data[2]);
				}
			}
			
			fileQueryList();
		}
		else if (strncmp(item->item_data[1], "S", strlen(item->item_data[1])) == 0 ||
			     strncmp(item->item_data[1], "s", strlen(item->item_data[1])) == 0)
		{
			upgradeServerCancel();
			strcpy(message, "File download cancel");
		}
		else
		{
			strcpy(message, "Invalid option");
		}

	}
}

/**************************************************
@bref       升级文件列表
@param
@return
@note
**************************************************/

static void doBtfListInstruction(ITEM *item, char *message)
{
	fileInfo_s *file;
	uint8_t i;
	uint8_t num;
	/* 文件正在下载 */
	if (sysparam.updateStatus)
	{
		sprintf(message, "Download [%s]completed progress: %.1f%%", 
						  getNewCodeVersion(),
                          ((float)getRxfileOffset() / getFileTotalSize()) * 100);
		return;
	}
	file = getFileList(&num);
	fileQueryList();
	strcpy(message, "Bt firmware list:\r\n");
	for (i = 0; i < num; i++)
	{
		sprintf(message + strlen(message), "[file(%d):%s, size:%d bytes]\r\n", 
											 i, 
											 file[i].fileName,
											 file[i].fileSize);
	}
}

/**************************************************
@bref       升级文件全部删除
@param
@return
@note
**************************************************/

static void doBtfDelInstruction(ITEM *item, char *message)
{
	if (sysparam.updateStatus)
	{
		strcpy(message, "Forbidden operation,device was downloading bt firmware");
		return;
	}

	fileInfo_s *file;
	uint8_t i;
	uint8_t num;
	uint8_t index;
	file = getFileList(&num);
	if (item->item_data[1][0] == 0 || item->item_data[1][0] == '?')
	{
        for (i = 0; i < num; i++)
        {
            fileDelete(file[i].fileName);
        }
        fileQueryList();
        moduleFileInfoClear();
        strcpy(message, "Delete all bt firmware success");
	}
	else
	{
	    index = atoi(item->item_data[1]);
	    fileDelete(file[index].fileName);
	    fileQueryList();
	    sprintf(message, "Delete (%d)[%s] bt firmware success", index, file[index].fileName);
    }
}

/**************************************************
@bref       升级文件下载
@param
@return
@note
**************************************************/

static void doBtfDownloadInstruction(ITEM *item, char *message)
{
	fileRxInfo *info = getOtaInfo();

	/* 强行关闭文件下载服务器 */
	if (atoi(item->item_data[1]) == 99)
	{
		/* 文件下载配置 */
		if (item->item_data[2][0] != 0)
		{
			strncpy(sysparam.upgradeServer, item->item_data[2], 50);
			sysparam.upgradeServer[49] = 0;
		}
		if (item->item_data[3][0] != 0)
		{
			sysparam.upgradePort = atoi(item->item_data[3]);
		}
		sprintf(message, "Device will download bt firmware from %s:%d after 5 seconds", sysparam.upgradeServer,
																						sysparam.upgradePort);
		upgradeServerInit(0);
		return;
	}
	if (item->item_data[1][0] == '0')
	{
		upgradeServerCancel();
		strcpy(message, "File download cancel");
		return;
	}
	/* 文件正在下载 */
	if (sysparam.updateStatus)
	{
		sprintf(message, "Download [%s]completed progress: %.1f%%", 
						  getNewCodeVersion(),
                          ((float)getRxfileOffset() / getFileTotalSize()) * 100);
		return;
	}
	/* 文件正在传输 */
	if (sysinfo.bleUpgradeOnoff)
	{
		strcpy(message, "Dev was upgrading");
		return;
	}
	uint8_t num;
	fileInfo_s *file = getFileList(&num);
	if (num >= FILE_MAX_CNT)
	{
	    strcpy(message, "File list is full");
	    return;
	}
	/* 文件下载配置 */
	if (item->item_data[1][0] != 0)
	{
		strncpy(sysparam.upgradeServer, item->item_data[1], 50);
		sysparam.upgradeServer[49] = 0;
	}
	if (item->item_data[2][0] != 0)
	{
		sysparam.upgradePort = atoi(item->item_data[2]);
	}
	sprintf(message, "Device will download bt firmware from %s:%d after 5 seconds", sysparam.upgradeServer,
																				    sysparam.upgradePort);
	upgradeServerInit(0);
}

/**************************************************
@bref       升级文件传入蓝牙继电器
@param
@return
@note
**************************************************/

static void doBtfUpsInstruction(ITEM *item, char *message)
{
	uint8_t i, l;
	int j;
	bleRelayInfo_s *bleinfo;
	fileInfo_s *file;
	uint8_t num;
	uint8_t ind;
	int8_t index; 
	fileRxInfo *info = getOtaInfo();
	if (item->item_data[1][0] == 0 || item->item_data[1][0] == '?')
	{
		if (sysinfo.bleUpgradeOnoff)
		{
			sprintf(message, "Dev(%d)[%s] programming completed progress: %.1f%%; verify completed progress: %.1f%%",
					getBleOtaStatus(), info->fileName,
					((float)info->rxoffset / info->totalsize) * 100,
					((float)info->veroffset / info->totalsize) * 100);
		}
		else
		{
			char mac[20];
    		char mac2[20];
    		for (i = 0; i < sysparam.bleMacCnt; i++)
    		{
			    byteToHexString(sysparam.bleConnMac[i], (uint8_t *)mac, 6);
	            mac[12] = 0;
	            l = 5;
	            for (j = 0; j < 3; j++)
	            {
	                tmos_memcpy(mac2, &mac[j * 2], 2);
	                tmos_memcpy(&mac[j * 2], &mac[l * 2], 2);
	                tmos_memcpy(&mac[l * 2], mac2, 2);
	                l--;
	            }
				bleinfo = bleRelayGeInfo(i);
				if (bleinfo != NULL)
				{
					sprintf(message + strlen(message), "Dev(%d)[mac:%s version:%s];\n", i, mac, bleinfo->version);
				}
				
			}
		}
	}
	else
	{
		/* 特殊指令 */
		if (atoi(item->item_data[1]) == 99)
		{
			for (i = 0; i < DEVICE_MAX_CONNECT_COUNT; i++)
			{
				if (sysparam.relayUpgrade[i] == BLE_UPGRADE_FLAG)
				{
					bleRelayDeleteAll();
					sysparam.relayUpgrade[i] = 0;
				}
			}
			if (item->item_data[2][0] != 0)
			{
				index = atoi(item->item_data[2]);
				file = getFileList(&num);
				ind  = atoi(item->item_data[3]);
				j = my_getstrindex(file[ind].fileName, "BR", 2);
				if (j < 0)
				{
					sprintf(message, "BTFUPS error,firmware is error [%s]", file[ind].fileName);
					return;
				}
				if (file[i].fileSize >= 102400)
				{
					sprintf(message, "BTFUPS error,firmware[%s] is too big", file[i].fileName, file[i].fileSize);
					return;
				}
				strncpy(sysparam.file.fileName, file[ind].fileName, 20);
				sysparam.file.fileName[strlen(file[ind].fileName)] = 0;
				sysparam.file.fileSize = file[ind].fileSize;
				paramSaveAll();
				otaRxInfoInit(1, file[ind].fileName, file[ind].fileSize);
				sprintf(message, "Dev(%d) will download the %s after 5 seconds",
								  index, file[ind].fileName);
				bleinfo = bleRelayGeInfo(index);
				if (bleinfo != NULL)
				{
					bleRelayClearAllReq(BLE_EVENT_ALL);
					bleRelaySetReq(index, BLE_EVENT_OTA);
				}
			}
			else
			{
				otaRxInfoInit(0, NULL, 0);
				tmos_memset(&sysparam.file, 0, sizeof(fileInfo_s));
				paramSaveAll();
				strcpy(message, "Ble ups cancel");
			}
		}
		/* 常规指令 */
		else
		{
			index = atoi(item->item_data[1]);
			if (sysparam.updateStatus)
			{
				strcpy(message, "Device was downloading firmware");
				return;
			}
			bleinfo = bleRelayGeInfo(index);
			if (bleinfo == NULL)
			{
				strcpy(message, "BTFUPS error,can not get this device infomation");
				return;
			}
			if (strlen(bleinfo->version) <= 2)
			{
				strcpy(message, "BTFUPS error,can not get device version infomation");
				return;
			}
			if (index >= DEVICE_MAX_CONNECT_COUNT)
			{
				strcpy(message, "BTFUPS error,dev number is error");
				return;
			}
			if (getBleOtaStatus() >= 0)
			{
				sprintf(message, "Dev(%d) is download the firmware now,please try it later", getBleOtaStatus());
				return;
			}
			file = getFileList(&num);
			if (strlen(item->item_data[2]) <= 2 && strlen(item->item_data[2]) > 0)
			{
				ind  = atoi(item->item_data[2]);
				j = my_strstr(file[ind].fileName, "BR", 2);
				if (j == 0)
				{
					sprintf(message, "BTFUPS error,firmware is error [%s]", file[ind].fileName);
					return;
				}
				if (file[i].fileSize >= 102400)
				{
					sprintf(message, "BTFUPS error,firmware[%s] is too big", file[i].fileName, file[i].fileSize);
					return;
				}
				//如果是BR06N
				if (my_strstr(file[ind].fileName, "BR06_CH592", 10))
				{
					if (my_strstr(bleinfo->version, "BR06_CH592", strlen(bleinfo->version)) == 0)
					{
						sprintf(message, "BTFUPS error,dev version is %s and upgrade firmware is %s", 
											bleinfo->version, file[ind].fileName);
						return;
					}
					else if (my_strstr(bleinfo->version, file[ind].fileName, strlen(bleinfo->version)))
					{
						sprintf(message, "Dev version[%s] is already firmware[%s]", 
											bleinfo->version, file[ind].fileName);
						return;
					}
				}
				else
				{
					//BR04或者BR06
					if (my_strstr(file[ind].fileName, "BR06", 4))
					{
						if (my_strstr(bleinfo->version, "BR06", strlen(bleinfo->version)) == 0)
						{
							sprintf(message, "BTFUPS error,dev version is %s and upgrade firmware is %s", 
											bleinfo->version, file[ind].fileName);
						}
					}
					else if (my_strstr(file[ind].fileName, "BR04", 4))
					{
						if (my_strstr(bleinfo->version, "BR04", strlen(bleinfo->version)) == 0)
						{
							sprintf(message, "BTFUPS error,dev version is %s and upgrade firmware is %s", 
											bleinfo->version, file[ind].fileName);
						}
					}
					else
					{
						//一般不会进这里
						strcpy(message, "BTFUPS unknow error");
					}
				}
				strncpy(sysparam.file.fileName, file[ind].fileName, 20);
				sysparam.file.fileName[strlen(file[ind].fileName)] = 0;
				sysparam.file.fileSize = file[ind].fileSize;
				paramSaveAll();
				otaRxInfoInit(1, file[ind].fileName, file[ind].fileSize);
				sprintf(message, "Dev(%d) will download the %s after 5 seconds",
								  index, file[ind].fileName);
				bleinfo = bleRelayGeInfo(index);
				if (bleinfo != NULL)
				{
					bleRelayClearAllReq(BLE_EVENT_ALL);
					bleRelaySetReq(index, BLE_EVENT_OTA);
				}
			}
			else
			{
				for (i = 0; i < num; i++)
				{
					if (strncmp(file[i].fileName, item->item_data[2], strlen(item->item_data[2])) == 0)
						break;
				}
				if (num == i)
				{
					strcpy(message, "BTFUPS error,firmware name is error");
					return;
				}
				else
				{
					j = my_getstrindex(file[ind].fileName, "BR", 2);
					if (j < 0)
					{
						sprintf(message, "BTFUPS error,firmware[%s]is error", file[i].fileName);
						return;
					}
					if (file[i].fileSize >= 102400)
					{
						sprintf(message, "BTFUPS error,firmware[%s] is too big", file[i].fileName, file[i].fileSize);
						return;
					}
					otaRxInfoInit(1, file[i].fileName, file[i].fileSize);
					sprintf(message, "Dev(%d) will download the %s after 5 seconds",
								      index, file[i].fileName);
					strncpy(sysparam.file.fileName, file[ind].fileName, 20);
					sysparam.file.fileName[strlen(file[ind].fileName)] = 0;
					sysparam.file.fileSize = file[ind].fileSize;
					paramSaveAll();
					bleinfo = bleRelayGeInfo(index);
					if (bleinfo != NULL)
					{
						bleRelayClearAllReq(BLE_EVENT_ALL);
						bleRelaySetReq(index, BLE_EVENT_OTA);
					}
				}
			}
		}
		paramSaveAll();
	}
}

static void doUartInstruction(ITEM *item, char *message)
{
	uint8_t i;
	if (item->item_data[1][0] == 0 || item->item_data[1][0] == '?')
    {
        sprintf(message, "Uart is %s, loglvl is %d", usart2_ctl.init ? "Open" : "Close", sysinfo.logLevel);
    }
    else
    {
    	if (item->item_data[1][0] != 0)
    	{
			i = atoi(item->item_data[1]);
    	}
    	if (item->item_data[2][0] != 0)
    	{
			sysinfo.logLevel = atoi(item->item_data[2]);
    	}
    	if (i)
    	{
			portUartCfg(APPUSART2, 1, 115200, doDebugRecvPoll);
    	}
    	else 
    	{
			portUartCfg(APPUSART2, 0, 115200, NULL);
			portAccGpioCfg();
			portSysOnoffGpioCfg();
    	}
    	sprintf(message, "Update Uart to %s, loglvl to %d", usart2_ctl.init ? "Open" : "Close", sysinfo.logLevel);
    }
}

static void doBleDebugInstruction(ITEM *item, char *message)
{
#ifdef BLE_CENTRAL_DEBUG
	bleScheduleInfo_s *scheduleInfo = getBleScheduleInfo();
	deviceConnInfo_s *devInfo = getDevListInfo();
	bleDebug_s *debuginfo = getBleDebugInfo();
	sprintf(message, "sch fsm:%d, ind:%d, tick:%d;", scheduleInfo->fsm, debuginfo->ind, scheduleInfo->runTick);
	sprintf(message + strlen(message), "ble0: use:%d, connpermit:%d, connret:%d, discState:%d ,conntick:%d;", devInfo[0].use, 
																	devInfo[0].connPermit, devInfo[0].Connret, devInfo[0].discState,
																	devInfo[0].connTick);
	sprintf(message + strlen(message), "ble1: use:%d, connpermit:%d, connret:%d, discState:%d ,conntick:%d;", devInfo[1].use, 
																	devInfo[1].connPermit, devInfo[1].Connret, devInfo[1].discState,
																	devInfo[1].connTick);
	sprintf(message + strlen(message), "gatt: handle:0x%02x, method:0x%02x, proStatus:0x%02x;", debuginfo->connHandle,
																				debuginfo->msgMethod, 
																			 	debuginfo->msgStatus);
	sprintf(message + strlen(message), "relayoncmd:%d, bleforceCmd:%d;", sysparam.relayCloseCmd, sysinfo.bleforceCmd);
	
#else
	strcpy(message, "Ble debug function is close");

#endif

	if (atoi(item->item_data[1]) == 1)
	{
	    GAPRole_CentralInit();
	    bleCentralInit();
	    sprintf(message + strlen(message), "restart central");
	}
	else if (atoi(item->item_data[1]) == 2)
	{
		GAPRole_PeripheralInit();
	    appPeripheralInit();
	    sprintf(message + strlen(message), "restart peripheral");
	}
	else if (atoi(item->item_data[1]) == 3)
	{
	    GAPRole_PeripheralInit();
		GAPRole_CentralInit();
	    appPeripheralInit();
		bleCentralInit();
		sprintf(message + strlen(message), "restart central and peripheral");
	}
}

static void doLockTimerInstruction(ITEM *item, char *message)
{
	uint8_t i, cnt;
    bleRelayInfo_s *bleinfo;
    
	if (item->item_data[1][0] == 0 || item->item_data[1][0] == '?')
	{
		cnt = 0;
		strcpy(message, "Ble LockTimer:");
        for (i = 0; i < BLE_CONNECT_LIST_SIZE; i++)
        {
            bleinfo = bleRelayGeInfo(i);
            if (bleinfo != NULL)
            {
                cnt++;
                sprintf(message + strlen(message), "(%d:[%d,%d]) ", i, bleinfo->shieldDetTime, bleinfo->netConnDetTime);
            }
        }
        if (cnt == 0)
        {
            sprintf(message, "no ble info");
        }
	}
	else
	{
		for (i = 0; i < BLE_CONNECT_LIST_SIZE; i++)
        {
            bleinfo = bleRelayGeInfo(i);
            if (bleinfo != NULL)
            {
                bleinfo->shieldDetTime  = 0;
                bleinfo->netConnDetTime = 0;
            }
        }
        sysparam.shieldDetTime = atoi(item->item_data[1]);
        sysparam.netConnDetTime = atoi(item->item_data[2]);
        if (sysparam.shieldDetTime < 30)
        	sysparam.shieldDetTime = 30;
        if (sysparam.netConnDetTime < 30)
        	sysparam.netConnDetTime = 30;
        paramSaveAll();
        sprintf(message, "Update ble lock timer to %ds,%ds", sysparam.shieldDetTime, sysparam.netConnDetTime);
        bleRelaySetAllReq(BLE_EVENT_SET_LOCK_TIME | BLE_EVENT_GET_LOCK_TIME);
	}
		
}

static void doRfDetTypeInstruction(ITEM *item, char *message)
{
	if (item->item_data[1][0] == 0 || item->item_data[1][0] == '?')
	{
		sprintf(message, "Rf detect type is %s", sysparam.rfDetType ? "wire" : "ble");
	}
	else
	{
		if (atoi(item->item_data[1]) == 0)
		{
			sysparam.rfDetType = 0;
		}
		else
		{
			sysparam.rfDetType = 1;
		}
		paramSaveAll();
		sprintf(message, "Update rf detect type to %s", sysparam.rfDetType ? "wire" : "ble");
		if (sysparam.rfDetType)
			strcpy(message + strlen(message), ",and disable the acc wire function");
	}
}

static void doAdccalInstrucion(ITEM *item, char *message)
{
    float vol;
    uint8_t type;
    if (item->item_data[1][0] == 0 || item->item_data[1][0] == '?')
    {
		sprintf(message, "ADC cal param: %f", sysparam.adccal);
    }
    else
    {
        vol = atof(item->item_data[1]);
        type = atoi(item->item_data[2]);
        if (type == 0)
        {
        	float x;
            x = portGetAdcVol(VCARD_CHANNEL);
            sysparam.adccal = vol / x;
        }
        else
        {
            sysparam.adccal = vol;
        }
        paramSaveAll();
		sprintf(message, "Update ADC calibration parameter to %f", sysparam.adccal);
    }
}

static void doRelayTypeInstruction(ITEM *item, char *message)
{
	if (item->item_data[1][0] == 0 || item->item_data[1][0] == '?')
    {
		sprintf(message, "Relay type is %s", sysparam.relayType ? "Wire" : "Ble");
    }
    else
    {
		if (atoi(item->item_data[1]) == 0)
		{
			sysparam.relayType = 0;
		}
		else
		{
			sysparam.relayType = 1;
		}
		paramSaveAll();
		sprintf(message, "Update relay type to %s", sysparam.relayType ? "Wire" : "Ble");
    }
}


/*--------------------------------------------------------------------------------------*/
static void doinstruction(int16_t cmdid, ITEM *item, insMode_e mode, void *param)
{
    char message[512] = { 0 };
    message[0] = 0;
    insParam_s *debugparam;
    switch (cmdid)
    {
        case PARAM_INS:
            doParamInstruction(item, message);
            break;
        case STATUS_INS:
            doStatusInstruction(item, message);
            break;
        case VERSION_INS:
            doVersionInstruction(item, message);
            break;
        case SN_INS:
            doSNInstruction(item, message);
            break;
        case SERVER_INS:
            doServerInstruction(item, message);
            break;
        case HBT_INS:
            doHbtInstruction(item, message);
            break;
        case MODE_INS:
            doModeInstruction(item, message);
            break;
        case POSITION_INS:
            do123Instruction(item, mode, param);
            break;
        case APN_INS:
            doAPNInstruction(item, message);
            break;
        case UPS_INS:
            doUPSInstruction(item, message);
            break;
        case LOWW_INS:
            doLOWWInstruction(item, message);
            break;
        case LED_INS:
            doLEDInstruction(item, message);
            break;
        case POITYPE_INS:
            doPOITYPEInstruction(item, message);
            break;
        case RESET_INS:
            doResetInstruction(item, message);
            break;
        case UTC_INS:
            doUTCInstruction(item, message);
            break;
        case ALARMMODE_INS:
            doAlarmModeInstrucion(item, message);
            break;
        case DEBUG_INS:
            doDebugInstrucion(item, message);
            break;
        case ACCCTLGNSS_INS:
            doACCCTLGNSSInstrucion(item, message);
            break;
        case ACCDETMODE_INS:
            doAccdetmodeInstruction(item, message);
            break;
        case FENCE_INS:
            doFenceInstrucion(item, message);
            break;
        case FACTORY_INS:
            doFactoryInstrucion(item, message);
            break;
        case ICCID_INS:
            doIccidInstrucion(item, message);
            break;
        case SETAGPS_INS:
            doSetAgpsInstruction(item, message);
            break;
        case JT808SN_INS:
            doJT808SNInstrucion(item, message);
            break;
        case HIDESERVER_INS:
            doHideServerInstruction(item, message);
            break;
        case BLESERVER_INS:
            doBleServerInstruction(item, message);
            break;
        case RELAY_INS:
        	if (param != NULL)
        	{
	           	debugparam = (insParam_s *)param;
	           	lastparam.link = debugparam->link;
           	}
           	getLastInsid();
           	doRelayInstrucion(item, message, mode, param);
           	break;
        case READPARAM_INS:
           	doReadParamInstruction(item, message);
           	break;
        case SETBLEPARAM_INS:
           	doSetBleParamInstruction(item, message);
           	break;
        case SETBLERFPARAM_INS:
			doSetBleRfParamInstruction(item, message);
        	break;
        case SETBLEWARNPARAM_INS:
           	doSetBleWarnParamInstruction(item, message);
           	break;
        case SETBLEMAC_INS:
           	doSetBleMacInstruction(item, message);
           	break;
        case RELAYSPEED_INS:
           	doRelaySpeedInstruction(item, message);
           	break;
        case RELAYFORCE_INS:
           	doRelayForceInstrucion(item, message);
           	break;
        case BF_INS:
           	doBFInstruction(item, message);
           	break;
        case CF_INS:
           	doCFInstruction(item, message);
           	break;
        case PROTECTVOL_INS:
           	doProtectVolInstrucion(item, message);
           	break;
        case TIMER_INS:
            doTimerInstrucion(item, message);
            break;
        case SOS_INS:
            doSOSInstruction(item, message, mode, param);
            break;
        case CENTER_INS:
            doCenterInstruction(item, message, mode, param);
            break;
        case SOSALM_INS:
            doSosAlmInstruction(item, message);
            break;
        case QFOTA_INS:
            doQfotaInstruction(item, message);
            break;
        case MOTIONDET_INS:
            doMotionDetInstruction(item, message);
            break;
        case FCG_INS:
            doFcgInstruction(item, message);
            break;
        case BLEEN_INS:
        	doBleenInstruction(item, message);
        	break;
       	case AGPSEN_INS:
       		doAgpsenInstrution(item, message);
       		break;
       	case BLERELAYCTL_INS:
       		doBleRelayCtlInstruction(item, message);
       		break;
       	case RELAYFUN_INS:
       		doRelayFunInstruction(item, message);
       		break;
        case SETBATRATE_INS:
            doSetBatRateInstruction(item, message);
            break;
        case SETMILE_INS:
			doSetmileInstruction(item, message);
        	break;
        case SIMSEL_INS:
        	doSimSelInstrucion(item, message);
        	break;
       	case SIMPULLOUTALM_INS:
			doSimPullOutAlmInstrucion(item, message);
			break;
		case UNCAPALM_INS:
			doUncapAlmInstrucion(item, message);
			break;
		case SHUTDOWNALM_INS:
			doShutdownAlmInstrucion(item, message);
			break;
		case READVERSION_INS:
			doReadVersionInstruction(item, message);
			break;
		case SETTXPOWER_INS:
			doSetTxPowerInstruction(item, message);
			break;
		case FILE_INS:
			doFileInstruction(item, message);
			break;
		case BTFLIST_INS:
			doBtfListInstruction(item, message);
			break;
		case BTFDEL_INS:
			doBtfDelInstruction(item, message);
			break;
		case BTFDOWNLOAD_INS:
			doBtfDownloadInstruction(item, message);
			break;
		case BTFUPS_INS:
			doBtfUpsInstruction(item, message);
			break;
		case UART_INS:
			doUartInstruction(item, message);
			break;
		case BLEDEBUG_INS:
			doBleDebugInstruction(item, message);
			break;
		case LOCKTIMER_INS:
			doLockTimerInstruction(item, message);
			break;
		case RFDETTYPE_INS:
			doRfDetTypeInstruction(item, message);
			break;
		case ADCCAL_INS:
        	doAdccalInstrucion(item, message);
        	break;
        case RELAYTYPE_INS:
			doRelayTypeInstruction(item, message);
        	break;
        default:
            if (mode == SMS_MODE)
            {
                deleteMessage();
                return ;
            }
            snprintf(message, 50, "Unsupport CMD:%s;", item->item_data[0]);
            break;
    }
	if (cmdid != RELAY_INS)
	{
    	sendMsgWithMode((uint8_t *)message, strlen(message), mode, param);
    }
}

static int16_t getInstructionid(uint8_t *cmdstr)
{
    uint16_t i = 0;
    for (i = 0; i < sizeof(insCmdTable) / sizeof(insCmdTable[0]); i++)
    {
        if (my_strpach((char *)insCmdTable[i].cmdstr, (const char *)cmdstr))
            return insCmdTable[i].cmdid;
    }
    return -1;
}

void instructionParser(uint8_t *str, uint16_t len, insMode_e mode, void *param)
{
    ITEM item;
    int16_t cmdid;
    stringToItem(&item, str, len);
    strToUppper(item.item_data[0], strlen(item.item_data[0]));
    cmdid = getInstructionid((uint8_t *)item.item_data[0]);
    doinstruction(cmdid, &item, mode, param);
}

