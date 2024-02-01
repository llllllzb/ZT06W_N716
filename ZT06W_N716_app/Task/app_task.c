#include <app_protocol.h>
#include "app_task.h"
#include "app_mir3da.h"
#include "app_atcmd.h"
#include "app_gps.h"
#include "app_instructioncmd.h"
#include "app_kernal.h"
#include "app_net.h"
#include "app_net.h"
#include "app_param.h"
#include "app_port.h"
#include "app_sys.h"
#include "app_socket.h"
#include "app_server.h"
#include "app_jt808.h"
#include "app_bleRelay.h"
#include "app_peripheral.h"
#include "app_central.h"

#define SYS_LED1_ON       LED1_ON
#define SYS_LED1_OFF      LED1_OFF


static SystemLEDInfo sysledinfo;
static motionInfo_s motionInfo;
static int8_t wifiTimeOutId = -1;
static centralPoint_s centralPoi = { 0 };

/**************************************************
@bref		bit0 置位，布防
@param
@return
@note
**************************************************/
void terminalDefense(void)
{
    sysinfo.terminalStatus |= 0x01;
}

/**************************************************
@bref		bit0 清除，撤防
@param
@return
@note
**************************************************/
void terminalDisarm(void)
{
    sysinfo.terminalStatus &= ~0x01;
}
/**************************************************
@bref		获取运动或静止状态
@param
@return
	>0		运动
	0		静止
@note
**************************************************/

uint8_t getTerminalAccState(void)
{
    return (sysinfo.terminalStatus & 0x02);

}

/**************************************************
@bref		bit1 置位，运动，accon
@param
@return
@note
**************************************************/

void terminalAccon(void)
{
    sysinfo.terminalStatus |= 0x02;
    jt808UpdateStatus(JT808_STATUS_ACC, 1);
}

/**************************************************
@bref		bit1 清除，静止，accoff
@param
@return
@note
**************************************************/
void terminalAccoff(void)
{
    sysinfo.terminalStatus &= ~0x02;
    jt808UpdateStatus(JT808_STATUS_ACC, 0);
}

/**************************************************
@bref		bit2 置位，充电
@param
@return
@note
**************************************************/

void terminalCharge(void)
{
    sysinfo.terminalStatus |= 0x04;
}
/**************************************************
@bref		bit2 清除，未充电
@param
@return
@note
**************************************************/

void terminalunCharge(void)
{
    sysinfo.terminalStatus &= ~0x04;
}

/**************************************************
@bref		获取充电状态
@param
@return
	>0		充电
	0		未充电
@note
**************************************************/

uint8_t getTerminalChargeState(void)
{
    return (sysinfo.terminalStatus & 0x04);
}

/**************************************************
@bref		bit 3~5 报警信息
@param
@return
@note
**************************************************/

void terminalAlarmSet(TERMINAL_WARNNING_TYPE alarm)
{
    sysinfo.terminalStatus &= ~(0x38);
    sysinfo.terminalStatus |= (alarm << 3);
}

/**************************************************
@bref		bit6 置位，已定位
@param
@return
@note
**************************************************/

void terminalGPSFixed(void)
{
    sysinfo.terminalStatus |= 0x40;
}

/**************************************************
@bref		bit6 清除，未定位
@param
@return
@note
**************************************************/

void terminalGPSUnFixed(void)
{
    sysinfo.terminalStatus &= ~0x40;
}

/**************************************************
@bref		LED1 运行任务
@param
@return
@note
**************************************************/

static void sysLed1Run(void)
{
    static uint8_t tick = 0;


    if (sysledinfo.sys_led1_on_time == 0)
    {
        SYS_LED1_OFF;
        return;
    }
    else if (sysledinfo.sys_led1_off_time == 0)
    {
        SYS_LED1_ON;
        return;
    }

    tick++;
    if (sysledinfo.sys_led1_onoff == 1) //on status
    {
        SYS_LED1_ON;
        if (tick >= sysledinfo.sys_led1_on_time)
        {
            tick = 0;
            sysledinfo.sys_led1_onoff = 0;
        }
    }
    else   //off status
    {
        SYS_LED1_OFF;
        if (tick >= sysledinfo.sys_led1_off_time)
        {
            tick = 0;
            sysledinfo.sys_led1_onoff = 1;
        }
    }
}

/**************************************************
@bref		设置灯的闪烁频率
@param
@return
@note
**************************************************/

static void ledSetPeriod(uint8_t ledtype, uint8_t on_time, uint8_t off_time)
{
    if (ledtype == GPSLED1)
    {
        //系统信号灯
        sysledinfo.sys_led1_on_time = on_time;
        sysledinfo.sys_led1_off_time = off_time;
    }
}

/**************************************************
@bref		更新系统灯状态
@param
@return
@note
**************************************************/

void ledStatusUpdate(uint8_t status, uint8_t onoff)
{
    if (onoff == 1)
    {
        sysinfo.sysLedState |= status;
    }
    else
    {
        sysinfo.sysLedState &= ~status;
    }
    if ((sysinfo.sysLedState & SYSTEM_LED_RUN) == SYSTEM_LED_RUN)
    {

        //慢闪
        ledSetPeriod(GPSLED1, 10, 10);
        if ((sysinfo.sysLedState & SYSTEM_LED_NETOK) == SYSTEM_LED_NETOK)
        {
            //常亮
            ledSetPeriod(GPSLED1, 1, 9);
            if ((sysinfo.sysLedState & SYSTEM_LED_GPSOK) == SYSTEM_LED_GPSOK)
            {
                //普通灯常亮
                ledSetPeriod(GPSLED1, 1, 0);
            }
        }

    }
    else
    {
        SYS_LED1_OFF;
        ledSetPeriod(GPSLED1, 0, 1);
    }
}

/**************************************************
@bref		灯控任务
@param
@return
@note
**************************************************/

static void ledTask(void)
{
	if (sysparam.ledctrl == 0)
	{
		if (sysinfo.sysTick >= 300)
		{
			SYS_LED1_OFF;
			return;
		}
	}
	else
	{
		if (sysinfo.sysTick >= 300)
		{
			if (getTerminalAccState() == 0)
			{
				SYS_LED1_OFF;
				return;
			}
		}
	}
    sysLed1Run();
}
/**************************************************
@bref		gps开启请求
@param
@return
@note
**************************************************/
void gpsRequestSet(uint32_t flag)
{
//	if (getTerminalAccState() == 0 && centralPoi.init && (flag & GPS_REQUEST_UPLOAD_ONE))
//	{
//		gpsinfo_s newgps;
//		tmos_memcpy(&newgps, &centralPoi.gpsinfo, sizeof(gpsinfo_s));
//		updateHistoryGpsTime(&newgps);
//        protocolSend(NORMAL_LINK, PROTOCOL_12, &newgps);
//	    jt808SendToServer(TERMINAL_POSITION,   &newgps);
//		return;
//	}
	if (dynamicParam.lowPowerFlag == 1)
	{
		LogPrintf(DEBUG_ALL, "gpsRequestSet==>low power");
		return;
	}
    LogPrintf(DEBUG_ALL, "gpsRequestSet==>0x%04X", flag);
    sysinfo.gpsRequest |= flag;
}

/**************************************************
@bref		gps清除请求
@param
@return
@note
**************************************************/

void gpsRequestClear(uint32_t flag)
{
    LogPrintf(DEBUG_ALL, "gpsRequestClear==>0x%04X", flag);
    sysinfo.gpsRequest &= ~flag;
}

uint32_t gpsRequestGet(uint32_t flag)
{
    return sysinfo.gpsRequest & flag;
}

/**************************************************
@bref		gps是否存在除了flag以外的其他请求
@param
@return
@note
**************************************************/
uint8_t gpsRequestOtherGet(uint32_t flag)
{
	uint32_t req;
	req = sysinfo.gpsRequest;
	req &= ~flag;
	if ((req & GPS_REQUEST_ALL))
	{
		return 1;
	}
	else
	{
		return 0;
	}	
}

/**************************************************
@bref		gps任务状态机切换
@param
@return
@note
**************************************************/

static void gpsChangeFsmState(uint8_t state)
{
    sysinfo.gpsFsm = state;
}

/**************************************************
@bref		保存上一次gps位置
@param
@return
@note
**************************************************/
void saveGpsHistory(void)
{
	gpsinfo_s *gpsinfo;
	gpsinfo = getCurrentGPSInfo();
	if (gpsinfo->fixstatus)
	{
		dynamicParam.saveLat = (uint16_t)gpsinfo->latitude;
		dynamicParam.saveLon = (uint16_t)gpsinfo->longtitude;
		dynamicParamSaveAll();
		LogPrintf(DEBUG_ALL, "saveGpsHistory==>lat:%d  lon:%d", dynamicParam.saveLat, dynamicParam.saveLon);
	}
}

/**************************************************
@bref		gps数据接收
@param
@return
@note
**************************************************/

static void gpsUartRead(uint8_t *msg, uint16_t len)
{
    static uint8_t gpsRestore[UART_RECV_BUFF_SIZE + 1];
    static uint16_t size = 0;
    uint16_t i, begin;
    if (len + size > UART_RECV_BUFF_SIZE)
    {
        size = 0;
    }
    memcpy(gpsRestore + size, msg, len);
    size += len;
    begin = 0;
    for (i = 0; i < size; i++)
    {
        if (gpsRestore[i] == '\n')
        {
            if (sysinfo.nmeaOutPutCtl)
            {
                LogWL(DEBUG_GPS, gpsRestore + begin, i - begin);
                LogPrintf(DEBUG_ALL, "gpsdatalen:%d", len);
                LogWL(DEBUG_GPS, "\r\n", 2);
            }
            nmeaParser(gpsRestore + begin, i - begin);
            begin = i + 1;
        }
    }
    if (begin != 0)
    {
        memmove(gpsRestore, gpsRestore + begin, size - begin);
        size -= begin;
    }
}

static void gpsCfg(void)
{
    char param[50];
	//关闭GSV
    //sprintf(param, "$CCMSG,GSV,1,0,*1A\r\n");
    sprintf(param, "$PCAS03,1,0,1,0,1,0,0,0,0,0,0,0,0,0*03\r\n");
    portUartSend(GPS_UART, (uint8_t *)param, strlen(param));
	sprintf(param, "$PCAS03,,,,,,,,,,,1*1F\r\n");
	portUartSend(GPS_UART, (uint8_t *)param, strlen(param));

    LogMessage(DEBUG_ALL, "gps config nmea output");
}
/**************************************************
@bref		切换中科微波特率为115200
@param
@return
@note
**************************************************/

//$PCAS03,1,0,1,1,1,0,0,0,0,0,0,0,0,0*02
static void changeGPSBaudRate(void)
{
    char param[50];
    sprintf(param, "$PCAS01,5*19\r\n");
    portUartSend(GPS_UART, (uint8_t *)param, strlen(param));
    portUartCfg(APPUSART0, 1, 115200, gpsUartRead);
    LogMessage(DEBUG_ALL, "gps config baudrate to 115200");
    startTimer(10, gpsCfg, 0);
}
/**************************************************
@bref		中科微热启动配置
@param
@return
@note
**************************************************/

static void gpsWarmStart(void)
{
	char param[50];
	//热启动
	sprintf(param, "$PCAS10,0*1C\r\n");
	portUartSend(GPS_UART, (uint8_t *)param, strlen(param));
	LogMessage(DEBUG_ALL, "Gps config warm start");
    startTimer(10, changeGPSBaudRate, 0);
}
/**************************************************
@bref		开启gps
@param
@return
@note
**************************************************/

void gpsOpen(void)
{
	GPSPWR_ON;
    GPSLNA_ON;
    portUartCfg(APPUSART0, 1, 9600, gpsUartRead);
    startTimer(10, changeGPSBaudRate, 0);
    sysinfo.gpsUpdatetick = sysinfo.sysTick;
    sysinfo.gpsOnoff = 1;
    gpsChangeFsmState(GPSWATISTATUS);
    gpsClearCurrentGPSInfo();
    ledStatusUpdate(SYSTEM_LED_GPSOK, 0);
    moduleSleepCtl(0);
    LogMessage(DEBUG_ALL, "gpsOpen");
    sysinfo.ephemerisFlag = 0;

}
/**************************************************
@bref		等待gps稳定
@param
@return
@note
**************************************************/

static void gpsWait(void)
{
    static uint8_t runTick = 0;
    if (++runTick >= 5)
    {
        runTick = 0;
        gpsChangeFsmState(GPSOPENSTATUS);
        if (sysinfo.ephemerisFlag == 0)
        {
			agpsRequestSet();
        }
    }
}

/**************************************************
@bref		关闭gps
@param
@return
@note
**************************************************/

void gpsClose(void)
{
    GPSPWR_OFF;
    GPSLNA_OFF;
    portUartCfg(APPUSART0, 0, 115200, NULL);
    sysinfo.rtcUpdate = 0;
    sysinfo.gpsOnoff = 0;
    gpsClearCurrentGPSInfo();
    terminalGPSUnFixed();
    gpsChangeFsmState(GPSCLOSESTATUS);
    ledStatusUpdate(SYSTEM_LED_GPSOK, 0);
    if (primaryServerIsReady())
    {
        moduleSleepCtl(1);
    }
    LogMessage(DEBUG_ALL, "gpsClose");
}



/**************************************************
@bref		gps控制任务
@param
@return
@note
**************************************************/

static void gpsRequestTask(void)
{
    gpsinfo_s *gpsinfo;
	static uint8_t gpsInvalidFlag = 0, gpsInvalidFlagTick = 0;
	static uint16_t gpsInvalidTick = 0;
	uint16_t gpsInvalidparam;
    switch (sysinfo.gpsFsm)
    {
        case GPSCLOSESTATUS:
            //有设备请求开关
            if (sysinfo.gpsRequest != 0)
            {
                gpsOpen();
            }
            break;
        case GPSWATISTATUS:
            gpsWait();
            break;
        case GPSOPENSTATUS:
            gpsinfo = getCurrentGPSInfo();
            if (gpsinfo->fixstatus)
            {
                ledStatusUpdate(SYSTEM_LED_GPSOK, 1);
                lbsRequestClear();
				wifiRequestClear();
				
            }
            else
            {
                ledStatusUpdate(SYSTEM_LED_GPSOK, 0);
            }
            if (sysinfo.gpsRequest == 0 || (sysinfo.sysTick - sysinfo.gpsUpdatetick) >= 20)
            {
            	if (sysinfo.gpsRequest == 0)
            	{
					saveGpsHistory();
            	}
                gpsClose();
            }
            break;
        default:
            gpsChangeFsmState(GPSCLOSESTATUS);
            break;
    }
    if (getTerminalAccState() == 0)
    {
        gpsInvalidTick = 0;
        gpsInvalidFlag = 0;
        gpsInvalidFlagTick = 0;
        return;
    }
    //如果仅关闭gps，不清除gpsInvalidTick
    if (sysinfo.gpsRequest == 0)
    {
		return;
    }
    gpsInvalidparam = (sysparam.gpsuploadgap < 60) ? 60 : sysparam.gpsuploadgap;
    //LogPrintf(DEBUG_ALL, "gpsInvalidTick:%d  gpsInvalidparam:%d", gpsInvalidTick, gpsInvalidparam);
    gpsinfo = getCurrentGPSInfo();
    if (gpsinfo->fixstatus == 0)
    {
        if (++gpsInvalidTick >= gpsInvalidparam)
        {
            gpsInvalidTick = 0;
            gpsInvalidFlag = 1;
    		wifiRequestSet(DEV_EXTEND_OF_MY);
        }
    }
    else
    {
        gpsInvalidTick = 0;
        gpsInvalidFlag = 0;
        gpsInvalidFlagTick = 0;
    }
}

/**************************************************
@bref		生成基准点
@param
@return
@note
**************************************************/

void centralPointInit(gpsinfo_s *gpsinfo)
{
	centralPoi.init = 1;
	tmos_memcpy(&centralPoi.gpsinfo, gpsinfo, sizeof(gpsinfo_s));
	LogPrintf(DEBUG_ALL, "%s==>lat:%.2f, lon:%.2f", __FUNCTION__, 
				centralPoi.gpsinfo.latitude, centralPoi.gpsinfo.longtitude);
}

/**************************************************
@bref		清除基准点
@param
@return
@note
**************************************************/

void centralPointClear(void)
{
	centralPoi.init = 0;
	tmos_memset(&centralPoi.gpsinfo, 0, sizeof(gpsinfo_s));
	LogPrintf(DEBUG_ALL, "%s==>OK", __FUNCTION__);
}


/**************************************************
@bref		上送一个gps位置
@param
@return
@note
**************************************************/

static void gpsUplodOnePointTask(void)
{
    gpsinfo_s *gpsinfo;
    static uint16_t runtick = 0;
    static uint8_t  uploadtick = 0;
    //判断是否有请求该事件
    if (sysinfo.gpsOnoff == 0)
    {
    	runtick    = 0;
    	uploadtick = 0;
        return;
    }
    if (gpsRequestGet(GPS_REQUEST_UPLOAD_ONE) == 0)
    {
        runtick    = 0;
        uploadtick = 0;
        return;
    }
    gpsinfo = getCurrentGPSInfo();
    runtick++;
    if (gpsinfo->fixstatus == 0)
    {
        uploadtick = 0;
        if (runtick >= sysinfo.gpsuploadonepositiontime)
        {
            runtick = 0;
            uploadtick = 0;
            gpsRequestClear(GPS_REQUEST_UPLOAD_ONE);
            if (getTerminalAccState())
            {
            	wifiRequestSet(DEV_EXTEND_OF_MY);
            }
        }
        return;
    }
    runtick = 0;
    uploadtick++;
    LogPrintf(DEBUG_ALL, "uploadtick:%d", uploadtick);
	if (uploadtick >= 10)
	{
		if (sysinfo.flag123)
        {
            dorequestSend123();
        }
		protocolSend(NORMAL_LINK, PROTOCOL_12, getCurrentGPSInfo());
	    jt808SendToServer(TERMINAL_POSITION, getCurrentGPSInfo());
	    gpsRequestClear(GPS_REQUEST_UPLOAD_ONE);
	    if (getTerminalAccState() == 0)
	    {
			centralPointInit(getCurrentGPSInfo());
	    }
    }
	

//    /* 静止时采用漂移点过滤算法 */
//    if (getTerminalAccState() == 0)
//    {
//		if (centralPoi.init)
//		{
//			double centerRadius;
//			centerRadius = calculateTheDistanceBetweenTwoPonits(gpsinfo->latitude, gpsinfo->longtitude, 
//																centralPoi.latitude, centralPoi.longtitude);
//			LogPrintf(DEBUG_ALL, "Center radius:%.2f", centerRadius);	
//			if (centerRadius < 6.5 || uploadtick >= 30)
//			{
//				if (sysinfo.flag123)
//		        {
//		            dorequestSend123();
//		        }
//				protocolSend(NORMAL_LINK, PROTOCOL_12, getCurrentGPSInfo());
//			    jt808SendToServer(TERMINAL_POSITION, getCurrentGPSInfo());
//			    gpsRequestClear(GPS_REQUEST_UPLOAD_ONE);
//			}
//		}
//		else
//		{
//			uint8_t total = 0;
//			for (uint8_t i = 0; i < sizeof(gpsinfo->gpsCn); i++)
//			{
//				if (gpsinfo->gpsCn[i] >= 35)
//				{
//					total++;
//				}
//			}
//			for (uint8_t j = 0; j < sizeof(gpsinfo->beidouCn); j++)
//			{
//				if (gpsinfo->beidouCn[j] >= 30)
//					total++;
//			}
//			LogPrintf(DEBUG_ALL, "Cn total:%d", total);
//			if (gpsinfo->used_star >= 10 || total >= 6 || (gpsinfo->pdop <= 6 && uploadtick >= 10))
//			{
//				if (sysinfo.flag123)
//		        {
//		            dorequestSend123();
//		        }
//				protocolSend(NORMAL_LINK, PROTOCOL_12, getCurrentGPSInfo());
//			    jt808SendToServer(TERMINAL_POSITION, getCurrentGPSInfo());
//			    gpsRequestClear(GPS_REQUEST_UPLOAD_ONE);
//			    centralPointInit(gpsinfo->latitude, gpsinfo->longtitude);
//			}
//			else if (uploadtick >= 30)
//			{
//				if (sysinfo.flag123)
//		        {
//		            dorequestSend123();
//		        }
//				protocolSend(NORMAL_LINK, PROTOCOL_12, getCurrentGPSInfo());
//			    jt808SendToServer(TERMINAL_POSITION, getCurrentGPSInfo());
//			    gpsRequestClear(GPS_REQUEST_UPLOAD_ONE);
//			    centralPointInit(gpsinfo->latitude, gpsinfo->longtitude);
//			}
//		}
//    }
//    /* 运动时 */
//    else
//    {
//    	if (uploadtick >= 10)
//    	{
//			if (sysinfo.flag123)
//	        {
//	            dorequestSend123();
//	        }
//			protocolSend(NORMAL_LINK, PROTOCOL_12, getCurrentGPSInfo());
//		    jt808SendToServer(TERMINAL_POSITION, getCurrentGPSInfo());
//		    gpsRequestClear(GPS_REQUEST_UPLOAD_ONE);
//	    }
//    }
}

/**************************************************
@bref		保存特定报警信息值
@param
@note
**************************************************/

void alarmRequestSave(uint32_t request)
{
    sysparam.alarmRequest |= request;
    paramSaveAll();
}

/**************************************************
@bref		清除保存的报警值
@param
@note
**************************************************/

void alarmRequestClearSave(void)
{
    if (sysparam.alarmRequest != 0)
    {
        LogMessage(DEBUG_ALL, "clear saved alarm request");
        sysparam.alarmRequest = 0;
        paramSaveAll();
    }
}

/**************************************************
@bref		获取保存的报警值
@param
@note
**************************************************/

void alarmRequestSaveGet(void)
{
	if (sysparam.alarmRequest != 0)
	{
		LogMessage(DEBUG_ALL, "get saved alarm request");
		sysinfo.alarmRequest = sysparam.alarmRequest;
	}
}

/**************************************************
@bref		报警上送请求
@param
@return
@note
**************************************************/
void alarmRequestSet(uint32_t request)
{
	if (dynamicParam.lowPowerFlag == 1)
	{
		LogPrintf(DEBUG_ALL, "alarmRequestSet==>low power");
		return;
	}
    LogPrintf(DEBUG_ALL, "alarmRequestSet==>0x%04X", request);
    sysinfo.alarmRequest |= request;
    alarmRequestSave(sysinfo.alarmRequest);
}
/**************************************************
@bref		清除报警上送
@param
@return
@note
**************************************************/

void alarmRequestClear(uint32_t request)
{
    LogPrintf(DEBUG_ALL, "alarmRequestClear==>0x%04X", request);
    sysinfo.alarmRequest &= ~request;
}

/**************************************************
@bref		报警任务
@param
@return
@note
**************************************************/

void alarmRequestTask(void)
{
    uint32_t alarm;
    if (primaryServerIsReady() == 0 || sysinfo.alarmRequest == 0)
    {
        return;
    }
    if (getTcpNack() != 0)
    {
        return;
    }
    //感光报警
    if (sysinfo.alarmRequest & ALARM_LIGHT_REQUEST)
    {
        alarmRequestClear(ALARM_LIGHT_REQUEST);
        LogMessage(DEBUG_ALL, "alarmRequestTask==>Light Alarm");
        terminalAlarmSet(TERMINAL_WARNNING_LIGHT);
        alarm = 0;
        protocolSend(NORMAL_LINK, PROTOCOL_16, &alarm);
    }

    //低电报警
    if (sysinfo.alarmRequest & ALARM_LOWV_REQUEST)
    {
        alarmRequestClear(ALARM_LOWV_REQUEST);
        LogMessage(DEBUG_ALL, "alarmRequestTask==>LowVoltage Alarm");
        terminalAlarmSet(TERMINAL_WARNNING_LOWV);
        alarm = 0;
        protocolSend(NORMAL_LINK, PROTOCOL_16, &alarm);
    }

    //断电报警
    if (sysinfo.alarmRequest & ALARM_LOSTV_REQUEST)
    {
        alarmRequestClear(ALARM_LOSTV_REQUEST);
        LogMessage(DEBUG_ALL, "alarmRequestTask==>lostVoltage Alarm");
        terminalAlarmSet(TERMINAL_WARNNING_LOSTV);
        alarm = 0;
        protocolSend(NORMAL_LINK, PROTOCOL_16, &alarm);
    }

    //SOS报警
    if (sysinfo.alarmRequest & ALARM_SOS_REQUEST)
    {
        alarmRequestClear(ALARM_SOS_REQUEST);
        LogMessage(DEBUG_ALL, "alarmRequestTask==>SOS Alarm");
        terminalAlarmSet(TERMINAL_WARNNING_SOS);
        alarm = 0;
        protocolSend(NORMAL_LINK, PROTOCOL_16, &alarm);
    }
    
    //蓝牙断连报警
    if (sysinfo.alarmRequest & ALARM_BLE_LOST_REQUEST)
    {
        alarmRequestClear(ALARM_BLE_LOST_REQUEST);
        LogMessage(DEBUG_ALL, "alarmUploadRequest==>BLE disconnect Alarm");
        alarm = 0x14;
        protocolSend(NORMAL_LINK, PROTOCOL_16, &alarm);
    }

    //蓝牙断连恢复报警
    if (sysinfo.alarmRequest & ALARM_BLE_RESTORE_REQUEST)
    {
        alarmRequestClear(ALARM_BLE_RESTORE_REQUEST);
        LogMessage(DEBUG_ALL, "alarmUploadRequest==>BLE restore Alarm");
        alarm = 0x1A;
        protocolSend(NORMAL_LINK, PROTOCOL_16, &alarm);
    }

    //油电恢复报警
    if (sysinfo.alarmRequest & ALARM_OIL_RESTORE_REQUEST)
    {
        alarmRequestClear(ALARM_OIL_RESTORE_REQUEST);
        LogMessage(DEBUG_ALL, "alarmUploadRequest==>oil restore Alarm");
        alarm = 0x19;
        protocolSend(NORMAL_LINK, PROTOCOL_16, &alarm);
    }
    //信号屏蔽报警
    if (sysinfo.alarmRequest & ALARM_SHIELD_REQUEST)
    {
        alarmRequestClear(ALARM_SHIELD_REQUEST);
        LogMessage(DEBUG_ALL, "alarmUploadRequest==>BLE shield Alarm");
        alarm = 0x17;
        protocolSend(NORMAL_LINK, PROTOCOL_16, &alarm);
    }
    //快速预报警
    if (sysinfo.alarmRequest & ALARM_FAST_PERSHIELD_REQUEST)
	{
		alarmRequestClear(ALARM_FAST_PERSHIELD_REQUEST);
        LogMessage(DEBUG_ALL, "alarmUploadRequest==>BLE fast shield Alarm");
        alarm = 0x38;
        protocolSend(NORMAL_LINK, PROTOCOL_16, &alarm);
        sysinfo.fastGap = sysinfo.sysTick;
	}
    //蓝牙预警报警
    if (sysinfo.alarmRequest & ALARM_PREWARN_REQUEST)
    {
        alarmRequestClear(ALARM_PREWARN_REQUEST);
        LogMessage(DEBUG_ALL, "alarmUploadRequest==>BLE warnning Alarm");
        alarm = 0x1D;
        protocolSend(NORMAL_LINK, PROTOCOL_16, &alarm);
    }
    //蓝牙锁定报警
    if (sysinfo.alarmRequest & ALARM_OIL_CUTDOWN_REQUEST)
    {
        alarmRequestClear(ALARM_OIL_CUTDOWN_REQUEST);
        LogMessage(DEBUG_ALL, "alarmUploadRequest==>BLE locked Alarm");
        alarm = 0x1E;
        protocolSend(NORMAL_LINK, PROTOCOL_16, &alarm);
    }
    //蓝牙异常报警
    if (sysinfo.alarmRequest & ALARM_BLE_ERR_REQUEST)
    {
        alarmRequestClear(ALARM_BLE_ERR_REQUEST);
        LogMessage(DEBUG_ALL, "alarmUploadRequest==>BLE err Alarm");
        alarm = 0x18;
        protocolSend(NORMAL_LINK, PROTOCOL_16, &alarm);
    }
    //拖车报警
    if (sysinfo.alarmRequest & ALARM_TRIAL_REQUEST)
    {
        alarmRequestClear(ALARM_TRIAL_REQUEST);
        LogMessage(DEBUG_ALL, "alarmUploadRequest==>拖车报警");
        alarm = 0x1F;
        protocolSend(NORMAL_LINK, PROTOCOL_16, &alarm);
    }
    //拔卡报警
    if (sysinfo.alarmRequest & ALARM_SIMPULLOUT_REQUEST)
    {
        alarmRequestClear(ALARM_SIMPULLOUT_REQUEST);
        LogMessage(DEBUG_ALL, "alarmUploadRequest==>拔卡报警");
        alarm = 0x32;
        protocolSend(NORMAL_LINK, PROTOCOL_16, &alarm);
    }
    //关机报警
    if (sysinfo.alarmRequest & ALARM_SHUTDOWN_REQUEST)
    {
        alarmRequestClear(ALARM_SHUTDOWN_REQUEST);
        LogMessage(DEBUG_ALL, "alarmUploadRequest==>关机报警");
        alarm = 0x30;
        protocolSend(NORMAL_LINK, PROTOCOL_16, &alarm);
    }
    //开盖报警
    if (sysinfo.alarmRequest & ALARM_UNCAP_REQUEST)
    {
        alarmRequestClear(ALARM_UNCAP_REQUEST);
        LogMessage(DEBUG_ALL, "alarmUploadRequest==>开盖报警");
        alarm = 0x31;
        protocolSend(NORMAL_LINK, PROTOCOL_16, &alarm);
    }
}



/**************************************************
@bref		更新运动或静止状态
@param
	src 		检测来源
	newState	新状态
@note
**************************************************/

static void motionStateUpdate(motion_src_e src, motionState_e newState)
{
    char type[20];


    if (motionInfo.motionState == newState)
    {
        return;
    }
    motionInfo.motionState = newState;
    switch (src)
    {
        case ACC_SRC:
            strcpy(type, "acc");
            break;
        case VOLTAGE_SRC:
            strcpy(type, "voltage");
            break;
        case GSENSOR_SRC:
            strcpy(type, "gsensor");
            break;
       	case SYS_SRC:
       		strcpy(type, "sys");
            break;
        default:
            return;
            break;
    }
    LogPrintf(DEBUG_ALL, "Device %s , detected by %s", newState == MOTION_MOVING ? "moving" : "static", type);

    if (newState)
    {
        netResetCsqSearch();
        if (sysparam.gpsuploadgap != 0)
        {
            gpsRequestSet(GPS_REQUEST_UPLOAD_ONE);
            if (sysparam.gpsuploadgap < GPS_UPLOAD_GAP_MAX)
            {
                gpsRequestSet(GPS_REQUEST_ACC_CTL);
            }
        }
        terminalAccon();
        //ClearLastMilePoint();
        centralPointClear();
        hiddenServerCloseClear();
    }
    else
    {
        if (sysparam.gpsuploadgap != 0)
        {
            gpsRequestSet(GPS_REQUEST_UPLOAD_ONE);
            gpsRequestClear(GPS_REQUEST_ACC_CTL);
        }
        terminalAccoff();
        updateRTCtimeRequest();
    }
    if (primaryServerIsReady())
    {
        protocolInfoResiter(getBatteryLevel(), sysinfo.outsidevoltage > 5.0 ? sysinfo.outsidevoltage : sysinfo.insidevoltage,
                            dynamicParam.startUpCnt, dynamicParam.runTime);
        protocolSend(NORMAL_LINK, PROTOCOL_13, NULL);
        jt808SendToServer(TERMINAL_POSITION, getLastFixedGPSInfo());
    }
}


/**************************************************
@bref       震动中断
@param
@note
**************************************************/

void motionOccur(void)
{
    motionInfo.tapInterrupt++;
}

/**************************************************
@bref       tapCnt 大小
@param
@note
**************************************************/

uint8_t motionGetSize(void)
{
    return sizeof(motionInfo.tapCnt);
}
/**************************************************
@bref		统计每一秒的中断次数
@param
@note
**************************************************/

static void motionCalculate(void)
{
    motionInfo.ind = (motionInfo.ind + 1) % sizeof(motionInfo.tapCnt);
    motionInfo.tapCnt[motionInfo.ind] = motionInfo.tapInterrupt;
    motionInfo.tapInterrupt = 0;
}
/**************************************************
@bref		获取这最近n秒的震动次数
@param
@note
**************************************************/

static uint16_t motionGetTotalCnt(uint8_t n)
{
    uint16_t cnt;
    uint8_t i;
    cnt = 0;
    for (i = 0; i < n; i++)
    {
        cnt += motionInfo.tapCnt[(motionInfo.ind + sizeof(motionInfo.tapCnt) - i) % sizeof(motionInfo.tapCnt)];
    }
    return cnt;
}

/**************************************************
@bref       检测单位时间内振动频率，判断是否运动
@param
@note
**************************************************/

static uint16_t motionCheckOut(uint8_t sec)
{
    uint8_t i;
    uint16_t validCnt;

    validCnt = 0;
    if (sec == 0 || sec > sizeof(motionInfo.tapCnt))
    {
        return 0;
    }
    for (i = 0; i < sec; i++)
    {
        if (motionInfo.tapCnt[(motionInfo.ind + sizeof(motionInfo.tapCnt) - i) % sizeof(motionInfo.tapCnt)] != 0)
        {
            validCnt++;
        }
    }
    return validCnt;
}

/**************************************************
@bref		获取运动状态
@param
@note
**************************************************/

motionState_e motionGetStatus(void)
{
    return motionInfo.motionState;
}


/**************************************************
@bref		运动和静止的判断
@param
@note
**************************************************/

static void motionCheckTask(void)
{
    static uint16_t gsStaticTick = 0;
    static uint16_t autoTick = 0;
    static uint8_t  accOnTick = 0;
    static uint8_t  accOffTick = 0;
    static uint8_t fixTick = 0;

    static uint8_t  volOnTick = 0;
    static uint8_t  volOffTick = 0;
    static uint8_t bfFlag = 0;
    static uint8_t bfTick = 0;
    static uint8_t lTick = 0, hTick = 0;
    static uint8_t vFlag = 0;
    static uint8_t alarmFlag = 0;
    static uint8_t detTick = 0;
    static uint8_t motionState = 0;
    gpsinfo_s *gpsinfo;

    uint16_t totalCnt, staticTime;

    motionCalculate();

    if (sysparam.MODE == MODE21 || sysparam.MODE == MODE23)
    {
        staticTime = 180;
    }
    else
    {
        staticTime = 180;
    }

    if (sysparam.MODE == MODE1 || sysparam.MODE == MODE3 || dynamicParam.lowPowerFlag == 1)
    {
        motionStateUpdate(SYS_SRC, MOTION_STATIC);
        gsStaticTick = 0;
        return ;
    }

    //保持运动状态时，如果gap大于Max，则周期性上报gps
    if (getTerminalAccState() && sysparam.gpsuploadgap >= GPS_UPLOAD_GAP_MAX)
    {
        if (++autoTick >= sysparam.gpsuploadgap)
        {
            autoTick = 0;
            gpsRequestSet(GPS_REQUEST_UPLOAD_ONE);
        }
    }
    else
    {
        autoTick = 0;
    }
    totalCnt = motionCheckOut(sysparam.gsdettime);
//        LogPrintf(DEBUG_ALL, "motionCheckOut=%d,%d,%d,%d,%d", totalCnt, sysparam.gsdettime, sysparam.gsValidCnt,
//                  sysparam.gsInvalidCnt,motionState);

    if (totalCnt >= sysparam.gsValidCnt && sysparam.gsValidCnt != 0)
    {
        motionState = 1;
    }
    else if (totalCnt <= sysparam.gsInvalidCnt)
    {
        motionState = 0;
    }
    if (sysparam.bf)
    {
        if (motionState && bfFlag == 0)
        {
            bfFlag = 1;
            alarmRequestSet(ALARM_SHUTTLE_REQUEST);
        }
        if (motionState == 0)
        {
            if (++bfTick >= 90)
            {
                bfFlag = 0;
            }
        }
        else
        {
            bfTick = 0;
        }
    }
    else
    {
        bfFlag = 0;
    }

    //拖车报警检测：运动后，如果没有ACC on，且在off下速度超过30km/h，则认为是拖车报警
    if ((sysparam.accdetmode == ACCDETMODE0 || sysparam.accdetmode == ACCDETMODE1) && getTerminalAccState() == 0)
    {
        //运动检测
        if (motionState)
        {
            if (gpsRequestGet(GPS_REQ_MOVE) == 0)
            {
                alarmFlag = 0;
                detTick = 0;
                gpsRequestSet(GPS_REQ_MOVE);
                LogMessage(DEBUG_ALL, "Device move !!!");
            }
        }
        else if (motionState == 0)
        {
            gsStaticTick++;
            if (gsStaticTick >= 90)
            {
                if (gpsRequestGet(GPS_REQ_MOVE) == GPS_REQ_MOVE)
                {
                    alarmFlag = 0;
                    detTick = 0;
                    gpsRequestClear(GPS_REQ_MOVE);
                    LogMessage(DEBUG_ALL, "Device static !!!");
                }
            }
        }
        else
        {
            gsStaticTick = 0;
        }
        //是否拖车报警检测，判断速度
        if (alarmFlag == 0 && sysinfo.gpsOnoff)
        {
            gpsinfo = getCurrentGPSInfo();
            if (gpsinfo->fixstatus && gpsinfo->speed >= 30)
            {
                detTick++;
                if (detTick >= 60)
                {
                    //拖车报警
                    alarmFlag = 1;
                    alarmRequestSet(ALARM_TRIAL_REQUEST);
                }
            }
            else
            {
                detTick = 0;
            }
        }
        else
        {
            detTick = 0;
        }

    }
    else
    {
        alarmFlag = 0;
        //其他模式不需要这个request
        if (gpsRequestGet(GPS_REQ_MOVE) == GPS_REQ_MOVE)
        {
            gpsRequestClear(GPS_REQ_MOVE);
        }
    }

    if (ACC_READ == ACC_STATE_ON)
    {
        //线永远是第一优先级
        if (++accOnTick >= 10)
        {
            accOnTick = 0;
            motionStateUpdate(ACC_SRC, MOTION_MOVING);
        }
        accOffTick = 0;
        return;
    }
    accOnTick = 0;
    if (sysparam.accdetmode == ACCDETMODE0)
    {
        //仅由acc线控制
        if (++accOffTick >= 10)
        {
            accOffTick = 0;
            motionStateUpdate(ACC_SRC, MOTION_STATIC);
        }
        return;
    }

    if (sysparam.accdetmode == ACCDETMODE1 || sysparam.accdetmode == ACCDETMODE3)
    {
        //由acc线+电压控制
        if (sysinfo.outsidevoltage >= sysparam.accOnVoltage)
        {
            if (++volOnTick >= 5)
            {
                vFlag = 1;
                volOnTick = 0;
                motionStateUpdate(VOLTAGE_SRC, MOTION_MOVING);
            }
        }
        else
        {
            volOnTick = 0;
        }

        if (sysinfo.outsidevoltage < sysparam.accOffVoltage)
        {
            if (++volOffTick >= 15)
            {
                vFlag = 0;
                volOffTick = 0;
                if (sysparam.accdetmode == ACCDETMODE1)
                {
                    motionStateUpdate(MOTION_MOVING, MOTION_STATIC);
                }
            }
        }
        else
        {
            volOffTick = 0;
        }
        if (sysparam.accdetmode == ACCDETMODE1 || vFlag != 0)
        {
            return;
        }
    }
    //剩下的，由acc线+gsensor控制

    if (motionState)
    {
        motionStateUpdate(GSENSOR_SRC, MOTION_MOVING);
    }
    if (motionState == 0)
    {
        if (sysinfo.gpsOnoff)
        {
            gpsinfo = getCurrentGPSInfo();
            if (gpsinfo->fixstatus && gpsinfo->speed >= 7)
            {
                if (++fixTick >= 5)
                {
                    gsStaticTick = 0;
                }
            }
            else
            {
                fixTick = 0;
            }
        }
        gsStaticTick++;
        if (gsStaticTick >= staticTime)
        {
            motionStateUpdate(GSENSOR_SRC, MOTION_STATIC);
        }
    }
    else
    {
        gsStaticTick = 0;
    }
}




/**************************************************
@bref		电压检测任务
@param
@return
@note
**************************************************/

static void voltageCheckTask(void)
{
    static uint16_t lowpowertick = 0;
    static uint8_t  lowwflag = 0;
    static uint32_t LostVoltageTick = 0;
    static uint8_t  LostVoltageFlag = 0;
	static uint32_t lostTick = 0;
    static uint8_t  bleCutFlag = 0;
    static uint8_t  bleCutTick = 0;
    float x;
    x = portGetAdcVol(VCARD_CHANNEL);
    sysinfo.outsidevoltage = x * sysparam.adccal;

    //低电报警
    if (sysinfo.outsidevoltage < sysinfo.lowvoltage)
    {
        lowpowertick++;
        if (lowpowertick >= 30)
        {
            if (lowwflag == 0)
            {
                lowwflag = 1;
                LogPrintf(DEBUG_ALL, "power supply too low %.2fV", sysinfo.outsidevoltage);
                //低电报警
                jt808UpdateAlarm(JT808_LOWVOLTAE_ALARM, 1);
                alarmRequestSet(ALARM_LOWV_REQUEST);

                //wifiRequestSet(DEV_EXTEND_OF_MY);
                gpsRequestSet(GPS_REQUEST_UPLOAD_ONE);
            }
        }
    }
    else
    {
        lowpowertick = 0;
    }


    if (sysinfo.outsidevoltage >= sysinfo.lowvoltage + 0.5)
    {
        lowwflag = 0;
        jt808UpdateAlarm(JT808_LOWVOLTAE_ALARM, 0);
    }

    if (sysinfo.outsidevoltage < 5.0)
    {
    	if ((lostTick % 60) == 0 && dynamicParam.lowPowerFlag == 0)
    	{
			LogPrintf(DEBUG_ALL, "检查电池");
			queryBatVoltage();
    	}
    	lostTick++;
        if (LostVoltageFlag == 0 && lostTick >= 10)
        {
            LostVoltageFlag = 1;
            LostVoltageTick = sysinfo.sysTick;
            terminalunCharge();
            jt808UpdateAlarm(JT808_LOSTVOLTAGE_ALARM, 1);
            LogMessage(DEBUG_ALL, "Lost power supply");
            alarmRequestSet(ALARM_LOSTV_REQUEST);
            gpsRequestSet(GPS_REQUEST_UPLOAD_ONE);
            if (sysparam.bleRelay != 0 && bleCutFlag != 0)
            {
				LogMessage(DEBUG_ALL, "ble relay on immediately");
				sysparam.relayCtl = 1;
				paramSaveAll();
				if (sysparam.relayFun)
				{
					RELAY_ON;
					relayAutoClear();
					bleRelayClearAllReq(BLE_EVENT_SET_DEVOFF);
                    bleRelaySetAllReq(BLE_EVENT_SET_DEVON);
                    sysinfo.bleforceCmd = bleDevGetCnt();
				}
				else
				{
					relayAutoRequest();
				}
            }
            else
            {
				LogMessage(DEBUG_ALL, "relay on was disable");
            }
        }
        if (LostVoltageFlag)
        {
			if (sysinfo.insidevoltage < 3.5)
			{
				if (dynamicParam.lowPowerFlag == 0)
				{
					dynamicParam.lowPowerFlag = 1;
					dynamicParamSaveAll();
					LogPrintf(DEBUG_ALL, "电量过低，开启低电保护");
					startTimer(30, modeTryToStop, 0);
				}
			}
        }
    }
    else if (sysinfo.outsidevoltage > 6.0)
    {
    	if (dynamicParam.lowPowerFlag)
		{
			dynamicParam.lowPowerFlag = 0;
			dynamicParamSaveAll();
			LogPrintf(DEBUG_ALL, "电量恢复，取消低电保护");
		}
    	//电压小于设置的保护电压范围时，则运行蓝牙去执行断电报警
		if (sysinfo.outsidevoltage > sysparam.bleVoltage)
		{
			if (bleCutTick++ >= 30)
			{
				bleCutFlag = 1;
			}
		}
		else
		{
			bleCutFlag = 0;
			bleCutTick = 0;
		}
    	lostTick = 0;
        terminalCharge();
        if (LostVoltageFlag == 1)
        {
            LostVoltageFlag = 0;
			sosRequestSet();
            jt808UpdateAlarm(JT808_LOSTVOLTAGE_ALARM, 0);
            if (sysinfo.sysTick - LostVoltageTick >= 10)
            {
                LogMessage(DEBUG_ALL, "power supply resume");
				if (sysparam.simSel == SIM_MODE_1 && dynamicParam.sim == SIM_2)
				{
					netSetSim(SIM_1);
				}
                startTimer(30, modulePowerOff, 0);
                startTimer(60, portSysReset, 0);
            }
        }
    }

}

/**************************************************
@bref		模式状态机切换
@param
@return
@note
**************************************************/

static void changeModeFsm(uint8_t fsm)
{
    sysinfo.runFsm = fsm;
}

/**************************************************
@bref		快速关闭
@param
@return
@note
**************************************************/

static void modeShutDownQuickly(void)
{
    static uint8_t delaytick = 0;
    if (sysinfo.gpsRequest == 0 && sysinfo.alarmRequest == 0 && sysinfo.wifiRequest == 0 && sysinfo.lbsRequest == 0)
    {
        delaytick++;
        if (delaytick >= 30)
        {
            LogMessage(DEBUG_ALL, "modeShutDownQuickly==>shutdown");
            delaytick = 0;
            changeModeFsm(MODE_STOP); //执行完毕，关机
        }
    }
    else
    {
        delaytick = 0;
    }
}

/**************************************************
@bref		模式启动
@param
@return
@note
**************************************************/

void modeTryToStop(void)
{
    sysinfo.gpsRequest = 0;
	sysinfo.wifiRequest = 0;
	sysinfo.wifiExtendEvt = 0;
	sysinfo.lbsExtendEvt = 0;
	sysinfo.lbsRequest = 0;
    changeModeFsm(MODE_STOP);
    LogPrintf(DEBUG_ALL, "modeTryToStop==>ok");
}

/**************************************************
@bref		模式启动
@param
@return
@note
**************************************************/

static void modeStart(void)
{
    uint16_t year;
    uint8_t month, date, hour, minute, second;
    portGetRtcDateTime(&year, &month, &date, &hour, &minute, &second);
    LogPrintf(DEBUG_ALL, "modeStart==>%02d/%02d/%02d %02d:%02d:%02d", year, month, date, hour, minute, second);
    sysinfo.runStartTick = sysinfo.sysTick;
    sysinfo.gpsuploadonepositiontime = 180;
    updateRTCtimeRequest();
    switch (sysparam.MODE)
    {
        case MODE1:
            portGsensorCtl(0);
            dynamicParam.startUpCnt++;
            portSetNextAlarmTime();
            dynamicParamSaveAll();
            break;
        case MODE2:
            portGsensorCtl(1);
            if (sysparam.accctlgnss == 0)
            {
                gpsRequestSet(GPS_REQUEST_GPSKEEPOPEN_CTL);
            }
            break;
        case MODE3:
            portGsensorCtl(0);
            dynamicParam.startUpCnt++;
            dynamicParamSaveAll();
            break;
        case MODE21:
            portGsensorCtl(1);
            portSetNextAlarmTime();
            break;
        case MODE23:
            portGsensorCtl(1);
            break;
        default:
            sysparam.MODE = MODE2;
            paramSaveAll();
            break;
    }
    gpsRequestSet(GPS_REQUEST_UPLOAD_ONE);
    ledStatusUpdate(SYSTEM_LED_RUN, 1);
    modulePowerOn();
    netResetCsqSearch();
    changeModeFsm(MODE_RUNING);
}

static void sysRunTimeCnt(void)
{
    static uint8_t runTick = 0;
    if (++runTick >= 180)
    {
        runTick = 0;
        dynamicParam.runTime++;
        dynamicParamSaveAll();
    }
}

/**************************************************
@bref		模式运行
@param
@return
@note
**************************************************/

static void modeRun(void)
{
	gpsMileRecord();
    static uint8_t runtick = 0;
    switch (sysparam.MODE)
    {
        case MODE1:
        case MODE3:
            //该模式下工作3分半钟
            if ((sysinfo.sysTick - sysinfo.runStartTick) >= 210)
            {
                gpsRequestClear(GPS_REQUEST_ALL);
                changeModeFsm(MODE_STOP);
            }
            modeShutDownQuickly();
            break;
        case MODE2:
            //该模式下每隔3分钟记录时长
            sysRunTimeCnt();
            gpsUploadPointToServer();
            break;
        case MODE21:
        case MODE23:
            //该模式下无gps请求时，自动关机
            sysRunTimeCnt();
            modeShutDownQuickly();
            gpsUploadPointToServer();
            break;
        default:
            LogMessage(DEBUG_ALL, "mode change unknow");
            sysparam.MODE = MODE2;
            break;
    }
}

/**************************************************
@bref		模式接收
@param
@return
@note
**************************************************/

static void modeStop(void)
{
    if (sysparam.MODE == MODE1 || sysparam.MODE == MODE3)
    {
        portGsensorCtl(0);
    }
    ledStatusUpdate(SYSTEM_LED_RUN, 0);
    modulePowerOff();
    changeModeFsm(MODE_DONE);
}

/**************************************************
@bref		等待唤醒模式
@param
@return
@note
**************************************************/

static void modeDone(void)
{
    static uint16_t runTick = 0;	
    if (sysparam.MODE == MODE2)
    {
        runTick++;
    }
    if ((sysinfo.gpsRequest || runTick >= 3600) && dynamicParam.lowPowerFlag == 0)
    {
        runTick = 0;
        changeModeFsm(MODE_START);
        LogMessage(DEBUG_ALL, "modeDone==>Change to mode start");
    }
}

/**************************************************
@bref		当前是否为运行模式
@param
@return
	1		是
	0		否
@note
**************************************************/

uint8_t isModeRun(void)
{
    if (sysinfo.runFsm == MODE_RUNING)
        return 1;
    return 0;
}

static void sysAutoReq(void)
{
    uint16_t year;
    uint8_t month, date, hour, minute, second;

    if (sysparam.MODE == MODE1 || sysparam.MODE == MODE21)
    {
        portGetRtcDateTime(&year, &month, &date, &hour, &minute, &second);
        if (date == sysinfo.alarmDate && hour == sysinfo.alarmHour && minute == sysinfo.alarmMinute && second == 0)
        {
            LogPrintf(DEBUG_ALL, "sysAutoReq==>%02d/%02d/%02d %02d:%02d:%02d", year, month, date, hour, minute, second);
            gpsRequestSet(GPS_REQUEST_UPLOAD_ONE);
        }
    }
    else
    {
        if (sysparam.gapMinutes != 0)
        {
            if (sysinfo.sysTick % (sysparam.gapMinutes * 60) == 0)
            {
                gpsRequestSet(GPS_REQUEST_UPLOAD_ONE);
                LogMessage(DEBUG_ALL, "upload period");
            }
        }
    }
}

/**************************************************
@bref		模式运行任务
@param
@return
@note
**************************************************/

static void sysModeRunTask(void)
{
    sysAutoReq();
    switch (sysinfo.runFsm)
    {
        case MODE_START:
            modeStart();
            break;
        case MODE_RUNING:
            modeRun();
            break;
        case MODE_STOP:
            modeStop();
            break;
        case MODE_DONE:
            modeDone();
            break;
    }
}

/**************************************************
@bref		基站上送请求
@param
@return
@note
**************************************************/

void lbsRequestSet(uint8_t ext)
{
	if (dynamicParam.lowPowerFlag == 1)
	{
		LogPrintf(DEBUG_ALL, "lbsRequestSet==>low power");
		return;
	}
    sysinfo.lbsRequest = 1;
    sysinfo.lbsExtendEvt |= ext;
}

/**************************************************
@bref		清除基站上送请求
@param
@return
@note
**************************************************/

void lbsRequestClear(void)
{
	sysinfo.lbsRequest = 0;
    sysinfo.lbsExtendEvt = 0;
}

static void sendLbs(void)
{
	gpsinfo_s *gpsinfo;
	gpsinfo = getCurrentGPSInfo();
	//当前已经定到位置则不发送lbs
	if (gpsinfo->fixstatus)
	{
		sysinfo.lbsExtendEvt = 0;
		return;
	}
    if (sysinfo.lbsExtendEvt & DEV_EXTEND_OF_MY)
    {
    	sysinfo.jt808Lbs = 1;
        protocolSend(NORMAL_LINK, PROTOCOL_19, NULL);
        jt808SendToServer(TERMINAL_POSITION, getCurrentGPSInfo());
    }
    if (sysinfo.lbsExtendEvt & DEV_EXTEND_OF_BLE)
    {
        protocolSend(BLE_LINK, PROTOCOL_19, NULL);
    }
    sysinfo.lbsExtendEvt = 0;
}
/**************************************************
@bref		基站上送任务
@param
@return
@note
**************************************************/

static void lbsRequestTask(void)
{
    if (sysinfo.lbsRequest == 0)
    {
        return;
    }
    if (primaryServerIsReady() == 0)
        return;
    sysinfo.lbsRequest = 0;
    moduleGetLbs();
    startTimer(70, sendLbs, 0);    
}

/**************************************************
@bref		wifi超时处理
@param
@return
@note
**************************************************/

void wifiTimeout(void)
{
	LogMessage(DEBUG_ALL, "wifiTimeout");
	lbsRequestSet(DEV_EXTEND_OF_MY);
	wifiRequestClear();
	wifiTimeOutId = -1;
}

/**************************************************
@bref		wifi应答成功
@param
@return
@note
**************************************************/

void wifiRspSuccess(void)
{
	if (wifiTimeOutId != -1)
	{
		stopTimer(wifiTimeOutId);
		wifiTimeOutId = -1;
		LogMessage(DEBUG_ALL, "wifiRspSuccess");
	}
}

/**************************************************
@bref		设置WIFI上送请求
@param
@return
@note
**************************************************/

void wifiRequestSet(uint8_t ext)
{
	if (dynamicParam.lowPowerFlag == 1)
	{
		LogPrintf(DEBUG_ALL, "wifiRequestSet==>low power");
		return;
	}
    sysinfo.wifiRequest = 1;
    sysinfo.wifiExtendEvt |= ext;
}

/**************************************************
@bref		清除WIFI上送请求
@param
@return
@note
**************************************************/

void wifiRequestClear(void)
{
	sysinfo.wifiRequest = 0;
	sysinfo.wifiExtendEvt = 0;
	wifiRspSuccess();
}

/**************************************************
@bref		WIFI上送任务
@param
@return
@note
**************************************************/

static void wifiRequestTask(void)
{
    if (sysinfo.wifiRequest == 0)
    {
        return;
    }
    if (primaryServerIsReady() == 0)
        return;
    sysinfo.wifiRequest = 0;
	if (sysinfo.agpsRequest)
	{
		startTimer(70, moduleGetWifiScan, 0);
	}
	else
	{
		startTimer(30, moduleGetWifiScan, 0);
	}
	wifiTimeOutId = startTimer(300, wifiTimeout, 0);

}

/**************************************************
@bref		唤醒设备
@param
@return
@note
**************************************************/
void wakeUpByInt(uint8_t      type, uint8_t sec)
{
    switch (type)
    {
        case 0:
            sysinfo.ringWakeUpTick = sec;
            break;
        case 1:
        	if (sec > sysinfo.cmdTick)
            	sysinfo.cmdTick = sec;
            break;
    }

    portSleepDn();
}

/**************************************************
@bref		查询是否需要休眠
@param
@return
@note
**************************************************/

static uint8_t getWakeUpState(void)
{
    //打印串口信息时，不休眠
    if (sysinfo.logLevel == DEBUG_FACTORY)
    {
        return 1;
    }
    //未联网，不休眠
    if (primaryServerIsReady() == 0 && isModeRun())
    {
        return 2;
    }
    //开gps时，不休眠
    if (sysinfo.gpsRequest != 0)
    {
        return 3;
    }
    if (sysinfo.ringWakeUpTick != 0)
    {
        return 4;
    }
    if (sysinfo.cmdTick != 0)
    {
        return 5;
    }

    //非0 时强制不休眠
    return 0;
}

/**************************************************
@bref		自动休眠
@param
@return
@note
**************************************************/

void autoSleepTask(void)
{
    static uint8_t flag = 0;
    if (sysinfo.ringWakeUpTick != 0)
    {
        sysinfo.ringWakeUpTick--;
    }
    if (sysinfo.cmdTick != 0)
    {
        sysinfo.cmdTick--;
    }
    if (getWakeUpState())
    {
        portSleepDn();
        if (flag != 0)
        {
            flag = 0;
			//portFclkChange(0);
            LogMessage(DEBUG_ALL, "disable sleep");
        }
    }
    else
    {
        portSleepEn();
        if (flag != 1)
        {
            flag = 1;
			//portFclkChange(1);
            LogMessage(DEBUG_ALL, "enable sleep");
        }
    }
}

/**************************************************
@bref		继电器状态初始化
@param
@note
**************************************************/

static void relayInit(void)
{
    if (sysparam.relayCtl)
    {
        RELAY_ON;
    }
    else
    {
        RELAY_OFF;
    }
}

/**************************************************
@bref       relayAutoRequest
@param
@note
**************************************************/

void relayAutoRequest(void)
{
    sysinfo.doRelayFlag = 1;
    sysinfo.bleforceCmd = bleDevGetCnt();
}

/**************************************************
@bref       relayAutoClear
@param
@note
**************************************************/

void relayAutoClear(void)
{
    sysinfo.doRelayFlag = 0;
}

/**************************************************
@bref       继电器自动控制
@param
@note
**************************************************/
static void doRelayOn(void)
{
    relayAutoClear();
    RELAY_ON;
    bleRelayClearAllReq(BLE_EVENT_SET_DEVOFF);
    bleRelaySetAllReq(BLE_EVENT_SET_DEVON);
    LogMessage(DEBUG_ALL, "do relay on");
}

void relayAutoCtrlTask(void)
{
    static uint8_t runTick = 0;
    gpsinfo_s *gpsinfo;
    char message[50];
    if (sysinfo.doRelayFlag == 0)
    {
        runTick = 0;
        return  ;
    }
    if (getTerminalAccState() == 0)
    {
        //设备静止了，立即断油电，本机relay控制线断，如果有蓝牙，也一块断
        doRelayOn();
        return;
    }
    if (sysparam.relaySpeed == 0)
    {
        //没有配置速度规则，那就等acc off才断
        instructionRespone("Relay on: Acc on");
        return;
    }
    if (sysinfo.gpsOnoff == 0)
    {
    	instructionRespone("Relay on: No gps");
        return;
    }
    gpsinfo = getCurrentGPSInfo();
    if (gpsinfo->fixstatus == 0)
    {
    	instructionRespone("Relay on: No gps");
        return;
    }
    if (gpsinfo->speed > sysparam.relaySpeed)
    {
    	sprintf(message, "Relay on: Overspeed %d km/h", sysparam.relaySpeed);
    	instructionRespone(message);
        runTick = 0;
        return;
    }
    if (++runTick >= 5)
    {
        runTick = 0;
        doRelayOn();
    }
}


/**************************************************
@bref		每天重启
@param
@note
**************************************************/

static void rebootEveryDay(void)
{
    sysinfo.sysTick++;
    if (sysinfo.sysTick < 86400)
        return ;
    if (sysinfo.gpsRequest != 0)
        return ;
   	if (sysparam.simSel == SIM_MODE_1 && dynamicParam.sim == SIM_2)
   	{
		netSetSim(SIM_1);
   	}
   	startTimer(30, modulePowerOff, 0);
    startTimer(60, portSysReset, 0);
}
void sosRequestSet(void)
{
    sysinfo.doSosFlag = 1;
}

static void sosRequestTask(void)
{
    static uint8_t runTick;
    static uint8_t runFsm = 0;
    static uint8_t ind;
    char  msg[80];
    uint8_t flag = 0;

    if (sysinfo.doSosFlag == 0)
    {
        runFsm = 0;
        return;
    }
    if (sysparam.sosalm == ALARM_TYPE_NONE)
    {
        sysinfo.doSosFlag = 0;
        return ;
    }
    if (isModuleRunNormal() == 0 || getTcpNack())
    {
        return;
    }
    switch (runFsm)
    {
        case 0:
            //gprs
            runFsm = 1;
            runTick = 0;
            ind = 0;
            alarmRequestSet(ALARM_SOS_REQUEST);
            if (sysparam.sosalm == ALARM_TYPE_GPRS)
            {
                runFsm = 99;
            }
            break;
        case 1:
            if (++runTick <= 3)
            {
                break;
            }
            runTick = 0;
            //sms
            flag = 0;
            for (; ind < 3;)
            {
                if (sysparam.sosNum[ind][0] != 0)
                {
                    flag = 1;
                    sprintf(msg, "Your device(%s) is sending you an SOS alert", dynamicParam.SN);
                    //LogPrintf(DEBUG_ALL, "[%s]==>%s", sysparam.sosNum[ind], msg);
                    sendMessage(msg, strlen(msg), sysparam.sosNum[ind]);
                    ind++;
                    break;
                }
                else
                {
                    ind++;
                }
            }
            if (flag == 0)
            {
                runFsm = 2;
                runTick = 60;
                ind = 0;
                if (sysparam.sosalm == ALARM_TYPE_GPRS_SMS)
                {
                    runFsm = 99;
                }
            }
            break;
        case 2:
            if (++runTick <= 60)
            {
                break;
            }
            runTick = 0;
            //tel
            flag = 0;
            for (; ind < 3;)
            {
                if (sysparam.sosNum[ind][0] != 0)
                {
                    flag = 1;

                    LogPrintf(DEBUG_ALL, "Try to call [%s]", sysparam.sosNum[ind]);
                    stopCall();
                    callPhone(sysparam.sosNum[ind]);
                    ind++;
                    break;
                }
                else
                {
                    ind++;
                }
            }
            if (flag == 0)
            {
                stopCall();
                runFsm = 99;
            }
            break;
        default:
            sysinfo.doSosFlag = 0;
            LogMessage(DEBUG_ALL, "SOS Done!!!");
            break;
    }
}

/**************************************************
@bref       gsensor检查任务
@param
@note
**************************************************/
static void gsensorRepair(void)
{
    portGsensorCtl(1);
    LogMessage(DEBUG_ALL, "repair gsensor");
}
static void gsCheckTask(void)
{
    static uint8_t tick = 0;
    static uint8_t errorcount = 0;
    if (sysinfo.gsensorOnoff == 0)
    {
        tick = 0;
        return;
    }

    tick++;
    if (tick % 60 == 0)
    {
        tick = 0;
        if (readInterruptConfig() != 0)
        {
            LogMessage(DEBUG_ALL, "gsensor error");
            portGsensorCtl(0);
            startTimer(20, gsensorRepair, 0);

        }
        else
        {
            errorcount = 0;
        }
    }
}

/**************************************************
@bref		感光检测任务
@param
@note
**************************************************/

static void lightDetectionTask(void)
{
    static uint32_t darknessTick = 0;
    static uint32_t FrontdarknessTick = 0;
    uint8_t curLdrState;
    if (dynamicParam.lowPowerFlag == 1)
    {
		darknessTick = 0;
		FrontdarknessTick = 0;
		return ;
    }
    curLdrState = LDR2_READ;

    if (curLdrState == 0)
    {
        //亮
        if (darknessTick >= 60)
        {
            if (sysparam.ldrEn != 0)
            {
                LogMessage(DEBUG_ALL, "Light alarm");
                alarmRequestSet(ALARM_LIGHT_REQUEST);
            }
        }
        darknessTick = 0;
    }
    else
    {
        //暗
        darknessTick++;
    }

    //前感光检测
    curLdrState = LDR1_READ;
    if (curLdrState == 0)
    {
        //亮
        if (FrontdarknessTick >= 60)
        {
            if (sysparam.uncapalm != 0)
            {
                LogMessage(DEBUG_ALL, "Uncap alarm");
                alarmRequestSet(ALARM_UNCAP_REQUEST);
                if (sysparam.uncapLock != 0)
                {
                    sysparam.relayCtl = 1;
                    paramSaveAll();
                    relayAutoRequest();
                    LogPrintf(DEBUG_ALL, "uncap==>try to relay on");
                }
            }
        }
        FrontdarknessTick = 0;
    }
    else
    {
        //暗
        FrontdarknessTick++;
    }
}


/**************************************************
@bref		系统关机任务
@param
@note		检测到关机引脚为高电平时关机
**************************************************/

static void autoShutDownTask(void)
{
	static uint8_t tick = 0;
	static uint8_t flag = 0;
	if (sysinfo.logLevel != 0) 
	{
		tick = 0;
		return;
	}
	if (tick++ >= 20)
	{
		portUartCfg(APPUSART2, 0, 115200, NULL);
		portAccGpioCfg();
		portSysOnoffGpioCfg();
	}
	if (usart2_ctl.init)
	{
		return;
	}
    static uint16_t shutdownTick;
    if (SYSONOFF_READ == ON_STATE)
    {
    	flag = 0;
        shutdownTick = 0;
        return;
    }
    ledStatusUpdate(SYSTEM_LED_RUN, 0);
    if (shutdownTick >= 3 && sysparam.shutdownalm != 0 && flag == 0)
    {
        alarmRequestSet(ALARM_SHUTDOWN_REQUEST);
        if (sysparam.shutdownLock != 0)
        {
            sysparam.relayCtl = 1;
            paramSaveAll();
            doRelayOn();
            LogPrintf(DEBUG_ALL, "shutdown==>try to relay on");
        }
        flag = 1;
    }
    shutdownTick++;
}

/**************************************************
@bref		1秒任务
@param
@return
@note
**************************************************/

void taskRunInSecond(void)
{
    rebootEveryDay();
    voltageCheckTask();
    netConnectTask();
    motionCheckTask();
    gsCheckTask();
    gpsRequestTask();
    gpsUplodOnePointTask();
    lbsRequestTask();
    wifiRequestTask();
    lightDetectionTask();
    sysModeRunTask();
    serverManageTask();
	autoSleepTask();
	relayAutoCtrlTask();
	autoShutDownTask();
}


/**************************************************
@bref		串口调试接收
@param
@return
@note
**************************************************/
void doDebugRecvPoll(uint8_t *msg, uint16_t len)
{
    static uint8_t gpsRestore[DEBUG_BUFF_SIZE + 1];
    static uint16_t size = 0;
    uint16_t i, begin;
    if (len + size > DEBUG_BUFF_SIZE)
    {
        size = 0;
    }
    memcpy(gpsRestore + size, msg, len);
    size += len;
    begin = 0;
    for (i = 0; i < size; i++)
    {
        if (gpsRestore[i] == '\n')
        {
            atCmdParserFunction(gpsRestore + begin, i - begin + 1);
            begin = i + 1;
        }
    }
    if (begin != 0)
    {
        memmove(gpsRestore, gpsRestore + begin, size - begin);
        size -= begin;
    }
}

/**************************************************
@bref		系统启动时配置
@param
@return
@note
**************************************************/

void myTaskPreInit(void)
{
    tmos_memset(&sysinfo, 0, sizeof(sysinfo));
	//sysinfo.logLevel = 9;

    SetSysClock(CLK_SOURCE_PLL_60MHz);
    portGpioSetDefCfg();
    portUartCfg(APPUSART2, 1, 115200, doDebugRecvPoll);
    portModuleGpioCfg();
    portGpsGpioCfg();
    portLedGpioCfg();
    portAdcCfg();
    portWdtCfg();
    portGsensorPwrCtl(1);
    portLdrGpioCfg();
    portRelayGpioCfg();
	portSleepDn();
    paramInit();
    relayInit();
    socketListInit();
    createSystemTask(outputNode, 2);
    //3秒运行一次可以过滤 3秒内一起重复的报警,例如快速预报警
    createSystemTask(alarmRequestTask, 30);
    
    sysinfo.sysTaskId = createSystemTask(taskRunInSecond, 10);
    LogMessage(DEBUG_ALL, ">>>>>>>>>>>>>>>>>>>>>");
    LogPrintf(DEBUG_ALL, "【%s】SYS_GetLastResetSta:%x", VER_LIB, SYS_GetLastResetSta());

}

/**************************************************
@bref		tmos 任务回调
@param
@return
@note
**************************************************/

static tmosEvents myTaskEventProcess(tmosTaskID taskID, tmosEvents events)
{
    if (events & SYS_EVENT_MSG)
    {
        uint8 *pMsg;
        if ((pMsg = tmos_msg_receive(sysinfo.taskId)) != NULL)
        {
            tmos_msg_deallocate(pMsg);
        }
        return (events ^ SYS_EVENT_MSG);
    }

    if (events & APP_TASK_KERNAL_EVENT)
    {
        kernalRun();
        ledTask();
        bleOtaTask();
        return events ^ APP_TASK_KERNAL_EVENT;
    }

    if (events & APP_TASK_POLLUART_EVENT)
    {
        pollUartData();
        portWdtFeed();
        
        return events ^ APP_TASK_POLLUART_EVENT;
    }
    return 0;
}

/**************************************************
@bref		任务初始化
@param
@return
@note
**************************************************/

void myTaskInit(void)
{
    sysinfo.taskId = TMOS_ProcessEventRegister(myTaskEventProcess);
    tmos_start_reload_task(sysinfo.taskId, APP_TASK_KERNAL_EVENT, MS1_TO_SYSTEM_TIME(100));
    tmos_start_reload_task(sysinfo.taskId, APP_TASK_POLLUART_EVENT, MS1_TO_SYSTEM_TIME(50));
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
}

