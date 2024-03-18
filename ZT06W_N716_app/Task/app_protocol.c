#include <app_protocol.h>
#include <math.h>
#include "aes.h"
#include "app_atcmd.h"
#include "app_gps.h"
#include "app_instructioncmd.h"
#include "app_kernal.h"
#include "app_net.h"
#include "app_net.h"
#include "app_param.h"
#include "app_sys.h"
#include "app_task.h"
#include "app_server.h"
#include "app_peripheral.h"
#include "app_db.h"
#include "app_jt808.h"
#include "app_central.h"
#include "app_file.h"

static uint8_t instructionid[4];
static uint8_t bleinstructionid[4];
static uint8_t lastinstructionid[4];


static uint8_t instructionid123[4];
static uint16_t instructionserier = 0;
static gpsRestore_s gpsres;
static protocolInfo_s protocolInfo;
static UndateInfoStruct uis;


unsigned short createProtocolSerial(void);

/**************************************************
@bref		7878Э��ͷ
@param
@return
@note
**************************************************/

static int createProtocolHead(char *dest, unsigned char Protocol_type)
{
    int pdu_len;
    pdu_len = 0;
    if (dest == NULL)
    {
        return 0;
    }
    dest[pdu_len++] = 0x78;
    dest[pdu_len++] = 0x78;
    dest[pdu_len++] = 0;
    dest[pdu_len++] = Protocol_type;
    return pdu_len;
}

/**************************************************
@bref       7979Э��ͷ
@param
@return
@note
**************************************************/

static int createProtocolHead_7979(char *dest, unsigned char Protocol_type)
{
    int pdu_len = 0;

    if(dest == 0)
        return 0;
    dest[pdu_len++] = 0x79;
    dest[pdu_len++] = 0x79;
    dest[pdu_len++] = 0;
    dest[pdu_len++] = 0;
    dest[pdu_len++] = Protocol_type;
    return pdu_len;
}

/**************************************************
@bref		7878Э��β
@param
@return
@note
**************************************************/

static int createProtocolTail(char *dest, int h_b_len, int serial_no)
{
    int pdu_len;
    unsigned short crc_16;

    if (dest == NULL)
    {
        return 0;
    }
    pdu_len = h_b_len;
    dest[pdu_len++] = (serial_no >> 8) & 0xff;
    dest[pdu_len++] = serial_no & 0xff;
    dest[2] = pdu_len - 1;
    /* Caculate Crc */
    crc_16 = GetCrc16(dest + 2, pdu_len - 2);
    dest[pdu_len++] = (crc_16 >> 8) & 0xff;
    dest[pdu_len++] = (crc_16 & 0xff);
    dest[pdu_len++] = 0x0d;
    dest[pdu_len++] = 0x0a;

    return pdu_len;
}

/**************************************************
@bref		7979Э��β
@param
@return
@note
**************************************************/

static int createProtocolTail_7979(char *dest, int h_b_len, int serial_no)
{
    int pdu_len;
    unsigned short crc_16;

    if (dest == NULL)
    {
        return -1;
    }
    pdu_len = h_b_len;
    dest[pdu_len++] = (serial_no >> 8) & 0xff;
    dest[pdu_len++] = serial_no & 0xff;
    dest[2] = ((pdu_len - 2) >> 8) & 0xff;
    dest[3] = ((pdu_len - 2) & 0xff);
    crc_16 = GetCrc16(dest + 2, pdu_len - 2);
    dest[pdu_len++] = (crc_16 >> 8) & 0xff;
    dest[pdu_len++] = (crc_16 & 0xff);
    dest[pdu_len++] = 0x0d;
    dest[pdu_len++] = 0x0a;
    return pdu_len;
}


/**************************************************
@bref		���imei
@param
@return
@note
**************************************************/

static int packIMEI(char *IMEI, char *dest)
{
    int imei_len;
    int i;
    if (IMEI == NULL || dest == NULL)
    {
        return -1;
    }
    imei_len = strlen(IMEI);
    if (imei_len == 0)
    {
        return -2;
    }
    if (imei_len % 2 == 0)
    {
        for (i = 0; i < imei_len / 2; i++)
        {
            dest[i] = ((IMEI[i * 2] - '0') << 4) | (IMEI[i * 2 + 1] - '0');
        }
    }
    else
    {
        for (i = 0; i < imei_len / 2; i++)
        {
            dest[i + 1] = ((IMEI[i * 2 + 1] - '0') << 4) | (IMEI[i * 2 + 2] - '0');
        }
        dest[0] = (IMEI[0] - '0');
    }
    return (imei_len + 1) / 2;
}


/**************************************************
@bref		���GPS��Ϣ
@param
@return
@note
**************************************************/


static int protocolGPSpack(gpsinfo_s *gpsinfo, char *dest, int protocol, gpsRestore_s *gpsres)
{
    int pdu_len;
    unsigned long la;
    unsigned long lo;
    double f_la, f_lo;
    unsigned char speed, gps_viewstar, beidou_viewstar;
    int direc;
    uint16_t year;
    uint8_t  month;
    uint8_t date;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;

    portGetRtcDateTime(&year, &month, &date, &hour, &minute, &second);

    datetime_s datetimenew;
    pdu_len = 0;
    la = lo = 0;
    /* UTC���ڣ�DDMMYY��ʽ */
    if (protocol == PROTOCOL_16)
    {
        dest[pdu_len++] = year % 100;
        dest[pdu_len++] = month;
        dest[pdu_len++] = date;
        dest[pdu_len++] = hour;
        dest[pdu_len++] = minute;
        dest[pdu_len++] = second;
    }
    else
    {
        datetimenew = changeUTCTimeToLocalTime(gpsinfo->datetime, sysparam.utc);

        dest[pdu_len++] = datetimenew.year % 100 ;
        dest[pdu_len++] = datetimenew.month;
        dest[pdu_len++] = datetimenew.day;
        dest[pdu_len++] = datetimenew.hour;
        dest[pdu_len++] = datetimenew.minute;
        dest[pdu_len++] = datetimenew.second;
    }

    gps_viewstar = (unsigned char)gpsinfo->used_star;
    beidou_viewstar = 0;
    if (gps_viewstar > 15)
    {
        gps_viewstar = 15;
    }
    if (beidou_viewstar > 15)
    {
        beidou_viewstar = 15;
    }
    if (protocol == PROTOCOL_12)
    {
        dest[pdu_len++] = (beidou_viewstar & 0x0f) << 4 | (gps_viewstar & 0x0f);
    }
    else
    {
        /* ǰ 4BitΪ GPS��Ϣ���ȣ��� 4BitΪ���붨λ�������� */
        if (gpsinfo->used_star   > 0)
        {
            dest[pdu_len++] = (12 << 4) | (gpsinfo->used_star & 0x0f);
        }
        else
        {
            /* ǰ 4BitΪ GPS��Ϣ���ȣ��� 4BitΪ���붨λ�������� */
            dest[pdu_len++] = (12 << 4) | (gpsinfo->gpsviewstart & 0x0f);
        }
    }
    /*
    γ��: ռ��4���ֽڣ���ʾ��λ���ݵ�γ��ֵ����ֵ��Χ0��162000000����ʾ0�ȵ�90�ȵķ�Χ����λ��1/500�룬ת���������£�
    ��GPSģ������ľ�γ��ֵת�����Է�Ϊ��λ��С����Ȼ���ٰ�ת�����С������30000������˵Ľ��ת����16���������ɡ�
    �� �� ��Ȼ��ת����ʮ��������Ϊ0x02 0x6B 0x3F 0x3E��
     */
    f_la  = gpsinfo->latitude * 60 * 30000;
    if (f_la < 0)
    {
        f_la = f_la * (-1);
    }
    la = (unsigned long)f_la;

    dest[pdu_len++] = (la >> 24) & 0xff;
    dest[pdu_len++] = (la >> 16) & 0xff;
    dest[pdu_len++] = (la >> 8) & 0xff;
    dest[pdu_len++] = (la) & 0xff;
    /*
    ����:ռ��4���ֽڣ���ʾ��λ���ݵľ���ֵ����ֵ��Χ0��324000000����ʾ0�ȵ�180�ȵķ�Χ����λ��1/500�룬ת������
    ��γ�ȵ�ת������һ�¡�
     */

    f_lo  = gpsinfo->longtitude * 60 * 30000;
    if (f_lo < 0)
    {
        f_lo = f_lo * (-1);
    }
    lo = (unsigned long)f_lo;
    dest[pdu_len++] = (lo >> 24) & 0xff;
    dest[pdu_len++] = (lo >> 16) & 0xff;
    dest[pdu_len++] = (lo >> 8) & 0xff;
    dest[pdu_len++] = (lo) & 0xff;


    /*
    �ٶ�:ռ��1���ֽڣ���ʾGPS�������ٶȣ�ֵ��ΧΪ0x00��0xFF��ʾ��Χ0��255����/Сʱ��
     */
    speed = (unsigned char)(gpsinfo->speed);
    dest[pdu_len++] = speed;
    /*
    ����:ռ��2���ֽڣ���ʾGPS�����з��򣬱�ʾ��Χ0��360����λ���ȣ�������Ϊ0�ȣ�˳ʱ�롣
     */
    if (gpsres != NULL)
    {

        gpsres->year = datetimenew.year % 100 ;
        gpsres->month = datetimenew.month;
        gpsres->day = datetimenew.day;
        gpsres->hour = datetimenew.hour;
        gpsres->minute = datetimenew.minute;
        gpsres->second = datetimenew.second;

        gpsres->latititude[0] = (la >> 24) & 0xff;
        gpsres->latititude[1] = (la >> 16) & 0xff;
        gpsres->latititude[2] = (la >> 8) & 0xff;
        gpsres->latititude[3] = (la) & 0xff;


        gpsres->longtitude[0] = (lo >> 24) & 0xff;
        gpsres->longtitude[1] = (lo >> 16) & 0xff;
        gpsres->longtitude[2] = (lo >> 8) & 0xff;
        gpsres->longtitude[3] = (lo) & 0xff;

        gpsres->speed = speed;
    }

    direc = (int)gpsinfo->course;
    dest[pdu_len] = (direc >> 8) & 0x03;
    //GPS FIXED:
    dest[pdu_len] |= 0x10 ; //0000 1000
    /*0����γ 1����γ */
    if (gpsinfo->NS == 'N')
    {
        dest[pdu_len] |= 0x04 ; //0000 0100
    }
    /*0������ 1������*/
    if (gpsinfo->EW == 'W')
    {
        dest[pdu_len] |= 0x08 ; //0000 1000
    }
    if (gpsres != NULL)
    {
        gpsres->coordinate[0] = dest[pdu_len];
    }
    pdu_len++;
    dest[pdu_len] = (direc) & 0xff;
    if (gpsres != NULL)
    {
        gpsres->coordinate[1] = dest[pdu_len];
    }
    pdu_len++;
    return pdu_len;
}

/**************************************************
@bref		�����վ��Ϣ
@param
@return
@note
**************************************************/

static int protocolLBSPack(char *dest, gpsinfo_s *gpsinfo)
{
    int pdu_len;
    uint32_t hightvalue;
    uint8_t fixAccuracy;
    if (dest == NULL)
    {
        return -1;
    }
    pdu_len = 0;

    dest[pdu_len++] = 0;
    dest[pdu_len++] = 0;

    hightvalue = fabs(gpsinfo->hight * 10);
    hightvalue &= ~(0xF00000);
    if (gpsinfo->hight < 0)
    {
        hightvalue |= 0x100000;
    }
    fixAccuracy = gpsinfo->fixAccuracy;
    hightvalue |= (fixAccuracy & 0x07) << 21;
    dest[pdu_len++] = (hightvalue >> 16) & 0xff; //(lai.MCC) & 0xff;
    dest[pdu_len++] = (hightvalue >> 8) & 0xff; ///(lai.MNC) & 0xff;
    dest[pdu_len++] = (hightvalue) & 0xff; //(lai.LAC>> 8) & 0xff;
    dest[pdu_len++] = 0; //(lai.LAC) & 0xff;
    dest[pdu_len++] = 0;
    dest[pdu_len++] = 0;
    return pdu_len;
}

/**************************************************
@bref		����debug
@param
@return
@note
**************************************************/

static void sendTcpDataDebugShow(uint8_t link, char *txdata, int txlen)
{
    int debuglen;
    char srclen;
    char senddata[300];
    debuglen = txlen > 128 ? 128 : txlen;
    sprintf(senddata, "Socket[%d] Send:", link);
    srclen = tmos_strlen(senddata);
    byteToHexString((uint8_t *)txdata, (uint8_t *)senddata + srclen, (uint16_t) debuglen);
    senddata[srclen + debuglen * 2] = 0;
    LogMessage(DEBUG_ALL, senddata);
}


/**************************************************
@bref		����01Э��
@param
@return
@note
**************************************************/

int createProtocol01(char *IMEI, unsigned short Serial, char *DestBuf)
{
    int pdu_len;
    int ret;
    pdu_len = createProtocolHead(DestBuf, 0x01);
    ret = packIMEI(IMEI, DestBuf + pdu_len);
    if (ret < 0)
    {
        return -1;
    }
    pdu_len += ret;
    ret = createProtocolTail(DestBuf, pdu_len,  Serial);
    if (ret < 0)
    {
        return -2;
    }
    pdu_len = ret;
    return pdu_len;
}

/**************************************************
@bref		����12Э��
@param
@return
@note
**************************************************/

int createProtocol12(gpsinfo_s *gpsinfo,  unsigned short Serial, char *DestBuf)
{
    int pdu_len;
    int ret;
    unsigned char gsm_level_value;

    if (gpsinfo == NULL)
        return -1;
    pdu_len = createProtocolHead(DestBuf, 0x12);
    /* Pack GPS */
    ret = protocolGPSpack(gpsinfo,  DestBuf + pdu_len, PROTOCOL_12, &gpsres);
    if (ret < 0)
    {
        return -1;
    }
    pdu_len += ret;
    /* Pack LBS */
    ret = protocolLBSPack(DestBuf + pdu_len, gpsinfo);
    if (ret < 0)
    {
        return -2;
    }
    pdu_len += ret;
    /* Add Language Reserved */
    gsm_level_value = getModuleRssi();
    gsm_level_value |= 0x80;
    DestBuf[pdu_len++] = gsm_level_value;
    DestBuf[pdu_len++] = 0;
    /* Pack Tail */
    ret = createProtocolTail(DestBuf, pdu_len,  Serial);
    if (ret <=  0)
    {
        return -3;
    }
    gpsinfo->hadupload = 1;
    pdu_len = ret;
    return pdu_len;
}

/**************************************************
@bref		����12Э�飬���ڲ���
@param
@return
@note
**************************************************/

void gpsRestoreDataSend(gpsRestore_s *grs, char *dest	, uint16_t *len)
{
    int pdu_len;
    pdu_len = createProtocolHead(dest, 0x12);
    //ʱ���
    dest[pdu_len++] = grs->year ;
    dest[pdu_len++] = grs->month;
    dest[pdu_len++] = grs->day;
    dest[pdu_len++] = grs->hour;
    dest[pdu_len++] = grs->minute;
    dest[pdu_len++] = grs->second;


    //����
    dest[pdu_len++] = 0;
    //ά��
    dest[pdu_len++] = grs->latititude[0];
    dest[pdu_len++] = grs->latititude[1];
    dest[pdu_len++] = grs->latititude[2];
    dest[pdu_len++] = grs->latititude[3];
    //����
    dest[pdu_len++] = grs->longtitude[0];
    dest[pdu_len++] = grs->longtitude[1];
    dest[pdu_len++] = grs->longtitude[2];
    dest[pdu_len++] = grs->longtitude[3];
    //�ٶ�
    dest[pdu_len++] = grs->speed;
    //��λ��Ϣ
    dest[pdu_len++] = grs->coordinate[0];
    dest[pdu_len++] = grs->coordinate[1];
    //mmc
    dest[pdu_len++] = 0;
    dest[pdu_len++] = 0;
    //mnc
    dest[pdu_len++] = 0;
    //lac
    dest[pdu_len++] = 0;
    dest[pdu_len++] = 0;
    //cellid
    dest[pdu_len++] = 0;
    dest[pdu_len++] = 0;
    dest[pdu_len++] = 0;
    //signal
    dest[pdu_len++] = 0;
    //����λ
    dest[pdu_len++] = 1;
    pdu_len = createProtocolTail(dest, pdu_len,	createProtocolSerial());
    *len = pdu_len;
    sendTcpDataDebugShow(NORMAL_LINK, dest, pdu_len);
}

/**************************************************
@bref		�����ص���
@param
@return
@note
**************************************************/

uint8_t getBatteryLevel(void)
{
    uint8_t level = 0;
    if (sysparam.batHighLevel == 0 || sysparam.batLowLevel == 0)
    {
        if (sysinfo.insidevoltage > 4.1)
        {
            level = 100;
        }
        else if (sysinfo.insidevoltage < 3.3)
        {
            level = 0;
        }
        else
        {
            level = (uint8_t)((sysinfo.insidevoltage - 3.3) / 0.8 * 100);
        }
        return level;
    }
    else
    {
        if (sysinfo.outsidevoltage > sysparam.batHighLevel)
        {
            level = 100;
        }
        else if (sysinfo.outsidevoltage < sysparam.batLowLevel)
        {
            level = 1;
        }
        else
        {
            level = (uint8_t)((sysinfo.outsidevoltage - sysparam.batLowLevel) / (sysparam.batHighLevel - sysparam.batLowLevel) * 100);
        }
        return level;
    }
}

/**************************************************
@bref		����13Э��
@param
@return
@note
**************************************************/

int createProtocol13(unsigned short Serial, char *DestBuf)
{
    int pdu_len;
    int ret;
    uint16_t value;
    gpsinfo_s *gpsinfo;
    uint8_t gpsvewstar, beidouviewstar;
    pdu_len = createProtocolHead(DestBuf, 0x13);
    DestBuf[pdu_len++] = sysinfo.terminalStatus;
    gpsinfo = getCurrentGPSInfo();

    value  = 0;
    gpsvewstar = gpsinfo->used_star;
    beidouviewstar = 0;
    if (sysinfo.gpsOnoff == 0)
    {
        gpsvewstar = 0;
        beidouviewstar = 0;
    }
    value |= ((beidouviewstar & 0x1F) << 10);
    value |= ((gpsvewstar & 0x1F) << 5);
    value |= ((getModuleRssi() & 0x1F));
    value |= 0x8000;

    DestBuf[pdu_len++] = (value >> 8) & 0xff; //������(BDGPV>>8)&0XFF;
    DestBuf[pdu_len++] = value & 0xff; // BDGPV&0XFF;
    DestBuf[pdu_len++ ] = 0;
    DestBuf[pdu_len++ ] = 0;//language
    DestBuf[pdu_len++ ] = protocolInfo.batLevel;//����
    DestBuf[pdu_len ] = 2 | (0x01 << 6); //ģʽ
    pdu_len++;
    value = (uint16_t)(protocolInfo.Voltage * 100);
    DestBuf[pdu_len++ ] = (value >> 8) & 0xff; //��ѹ(vol >>8 ) & 0xff;
    DestBuf[pdu_len++ ] = value & 0xff; //vol & 0xff;
    DestBuf[pdu_len++ ] = 0;//�й�
    DestBuf[pdu_len++ ] = (protocolInfo.startUpcnt >> 8) & 0xff; //ģʽһ����
    DestBuf[pdu_len++ ] = protocolInfo.startUpcnt & 0xff;
    DestBuf[pdu_len++ ] = (protocolInfo.runCnt >> 8) & 0xff; //ģʽ������
    DestBuf[pdu_len++ ] = protocolInfo.runCnt & 0xff;
    ret = createProtocolTail(DestBuf, pdu_len,  Serial);
    if (ret < 0)
    {
        return -1;
    }
    pdu_len = ret;
    return pdu_len;

}
/**************************************************
@bref		����21Э��
@param
@return
@note
**************************************************/

int createProtocol21(char *DestBuf, char *data, uint16_t datalen)
{
    int pdu_len = 0, i;
    DestBuf[pdu_len++] = 0x79; //Э��ͷ
    DestBuf[pdu_len++] = 0x79;
    DestBuf[pdu_len++] = 0x00; //ָ��ȴ���
    DestBuf[pdu_len++] = 0x00;
    DestBuf[pdu_len++] = 0x21; //Э���
    DestBuf[pdu_len++] = instructionid[0]; //ָ��ID
    DestBuf[pdu_len++] = instructionid[1];
    DestBuf[pdu_len++] = instructionid[2];
    DestBuf[pdu_len++] = instructionid[3];
    DestBuf[pdu_len++] = 1; //���ݱ���
    for (i = 0; i < datalen; i++) //��������
    {
        DestBuf[pdu_len++] = data[i];
    }
    pdu_len = createProtocolTail_7979(DestBuf, pdu_len, instructionserier);
    return pdu_len;
}
/**************************************************
@bref		����61Э��
@param
@return
@note
**************************************************/


uint16_t createProtocol61(char *dest, char *datetime, uint32_t totalsize, uint8_t filetye, uint16_t packsize)
{
    uint16_t pdu_len;
    uint16_t packnum;
    pdu_len = createProtocolHead(dest, 0x61);
    changeHexStringToByteArray_10in((uint8_t *)dest + 4, (uint8_t *)datetime, 6);
    pdu_len += 6;
    dest[pdu_len++] = filetye;
    packnum = totalsize / packsize;
    if (totalsize % packsize != 0)
    {
        packnum += 1;
    }
    dest[pdu_len++] = (packnum >> 8) & 0xff;
    dest[pdu_len++] = packnum & 0xff;
    dest[pdu_len++] = (totalsize >> 24) & 0xff;
    dest[pdu_len++] = (totalsize >> 26) & 0xff;
    dest[pdu_len++] = (totalsize >> 8) & 0xff;
    dest[pdu_len++] = totalsize & 0xff;
    pdu_len = createProtocolTail(dest, pdu_len,  createProtocolSerial());
    return pdu_len;
}

/**************************************************
@bref		����62Э��
@param
@return
@note
**************************************************/

uint16_t createProtocol62(char *dest, char *datetime, uint16_t packnum, uint8_t *recdata, uint16_t reclen)
{
    uint16_t pdu_len, i;
    pdu_len = 0;
    dest[pdu_len++] = 0x79; //Э��ͷ
    dest[pdu_len++] = 0x79;
    dest[pdu_len++] = 0x00; //ָ��ȴ���
    dest[pdu_len++] = 0x00;
    dest[pdu_len++] = 0x62; //Э���
    changeHexStringToByteArray_10in((uint8_t *)dest + 5, (uint8_t *)datetime, 6);
    pdu_len += 6;
    dest[pdu_len++] = (packnum >> 8) & 0xff;
    dest[pdu_len++] = packnum & 0xff;
    for (i = 0; i < reclen; i++)
    {
        dest[pdu_len++] = recdata[i];
    }
    pdu_len = createProtocolTail_7979(dest, pdu_len,  createProtocolSerial());
    return pdu_len;
}

/**************************************************
@bref		����F3Э��
@param
@return
@note
**************************************************/

uint16_t createProtocolF3(char *dest, WIFIINFO *wap)
{
    uint8_t i, j;
    uint16_t year;
    uint8_t  month;
    uint8_t date;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;

    portGetRtcDateTime(&year, &month, &date, &hour, &minute, &second);
    uint16_t pdu_len;
    pdu_len = createProtocolHead(dest, 0xF3);
    dest[pdu_len++] = year % 100;
    dest[pdu_len++] = month;
    dest[pdu_len++] = date;
    dest[pdu_len++] = hour;
    dest[pdu_len++] = minute;
    dest[pdu_len++] = second;
    dest[pdu_len++] = wap->apcount;
    for (i = 0; i < wap->apcount; i++)
    {
        dest[pdu_len++] = 0;
        dest[pdu_len++] = 0;
        for (j = 0; j < 6; j++)
        {
            dest[pdu_len++] = wap->ap[i].ssid[j];
        }
    }
    pdu_len = createProtocolTail(dest, pdu_len,  createProtocolSerial());
    return pdu_len;
}

/**************************************************
@bref		����F1Э��
@param
@return
@note
**************************************************/

int createProtocolF1(unsigned short Serial, char *DestBuf)
{
    int pdu_len;
    pdu_len = createProtocolHead(DestBuf, 0xF1);
    sprintf(DestBuf + pdu_len, "%s&&%s&&%s", dynamicParam.SN, getModuleIMSI(), getModuleICCID());
    pdu_len += strlen(DestBuf + pdu_len);
    pdu_len = createProtocolTail(DestBuf, pdu_len,  createProtocolSerial());
    return pdu_len;
}

/**************************************************
@bref		����16Э��
@param
@return
@note
**************************************************/

int createProtocol16(unsigned short Serial, char *DestBuf, uint8_t event)
{
    int pdu_len, ret, i;
    gpsinfo_s *gpsinfo;
    pdu_len = createProtocolHead(DestBuf, 0x16);
    gpsinfo = getLastFixedGPSInfo();
    ret = protocolGPSpack(gpsinfo, DestBuf + pdu_len, PROTOCOL_16, NULL);
    pdu_len += ret;

    /**********************/

    DestBuf[pdu_len++] = 0xFF;
    DestBuf[pdu_len++] = (getMCC() >> 8) & 0xff;
    DestBuf[pdu_len++] = getMCC() & 0xff;
    DestBuf[pdu_len++] = getMNC();
    DestBuf[pdu_len++] = (getLac() >> 8) & 0xff;
    DestBuf[pdu_len++] = getLac() & 0xff;
    DestBuf[pdu_len++] = (getCid() >> 24) & 0xff;
    DestBuf[pdu_len++] = (getCid() >> 16) & 0xff;
    DestBuf[pdu_len++] = (getCid() >> 8) & 0xff;
    DestBuf[pdu_len++] = getCid() & 0xff;
    for (i = 0; i < 36; i++)
        DestBuf[pdu_len++] = 0;

    /**********************/


    DestBuf[pdu_len++] = sysinfo.terminalStatus;
    sysinfo.terminalStatus &= ~0x38;
    DestBuf[pdu_len++] = 0;
    DestBuf[pdu_len++] = 0;
    DestBuf[pdu_len++] = event;
    if (event == 0)
    {
        DestBuf[pdu_len++] = 0x01;
    }
    else
    {
        DestBuf[pdu_len++] = 0x81;
    }
    pdu_len = createProtocolTail(DestBuf, pdu_len,	Serial);
    return pdu_len;

}

/**************************************************
@bref		����19Э��
@param
@return
@note
**************************************************/

int createProtocol19(unsigned short Serial, char *DestBuf)
{
    int pdu_len;
    uint16_t year;
    uint8_t  month;
    uint8_t date;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;

    portGetRtcDateTime(&year, &month, &date, &hour, &minute, &second);
    pdu_len = createProtocolHead(DestBuf, 0x19);
    DestBuf[pdu_len++] = year % 100;
    DestBuf[pdu_len++] = month;
    DestBuf[pdu_len++] = date;
    DestBuf[pdu_len++] = hour;
    DestBuf[pdu_len++] = minute;
    DestBuf[pdu_len++] = second;
    DestBuf[pdu_len++] = getMCC() >> 8;
    DestBuf[pdu_len++] = getMCC();
    DestBuf[pdu_len++] = getMNC();
    DestBuf[pdu_len++] = 1;
    DestBuf[pdu_len++] = getLac() >> 8;
    DestBuf[pdu_len++] = getLac();
    DestBuf[pdu_len++] = getCid() >> 24;
    DestBuf[pdu_len++] = getCid() >> 16;
    DestBuf[pdu_len++] = getCid() >> 8;
    DestBuf[pdu_len++] = getCid();
    DestBuf[pdu_len++] = 0;
    DestBuf[pdu_len++] = 0;
    pdu_len = createProtocolTail(DestBuf, pdu_len,  Serial);
    return pdu_len;
}

/**************************************************
@bref		����8AЭ��
@param
@return
@note
**************************************************/

int createProtocol8A(unsigned short Serial, char *DestBuf)
{
    int pdu_len;
    int ret;
    pdu_len = createProtocolHead(DestBuf, 0x8A);
    ret = createProtocolTail(DestBuf, pdu_len,  Serial);
    if (ret < 0)
    {
        return -2;
    }
    pdu_len = ret;
    return pdu_len;
}

/**************************************************
@bref		����A0Э��
@param
@return
@note
**************************************************/

void createProtocolA0(char *DestBuf, uint16_t *len)
{
	int pdu_len;
    int ret;
    int16_t lat, lon;
    pdu_len = createProtocolHead(DestBuf, 0xA0);
	ret = packIMEI(dynamicParam.SN, DestBuf + pdu_len);
    if (ret < 0)
    {
        return ;
    }
    pdu_len += ret;
    DestBuf[pdu_len++] = getMCC() >> 8;
    DestBuf[pdu_len++] = getMCC();
    DestBuf[pdu_len++] = getMNC();
    DestBuf[pdu_len++] = getLac() >> 8;
    DestBuf[pdu_len++] = getLac();
    DestBuf[pdu_len++] = getCid() >> 24;
    DestBuf[pdu_len++] = getCid() >> 16;
    DestBuf[pdu_len++] = getCid() >> 8;
    DestBuf[pdu_len++] = getCid();
    lat = dynamicParam.saveLat;
    lon = dynamicParam.saveLon;  
    LogPrintf(DEBUG_ALL, "Lat %d  Lon %d", dynamicParam.saveLat, dynamicParam.saveLon);
    if (dynamicParam.saveLat < 0)
    {
		DestBuf[pdu_len] |= 0x80;
    }
	DestBuf[pdu_len] |= (lat >> 8) & 0xEF;
	pdu_len++;
	DestBuf[pdu_len++] = lat & 0xFF;
	if (dynamicParam.saveLon < 0)
	{
		DestBuf[pdu_len] |= 0x80;
	}
	DestBuf[pdu_len] |= (lon >> 8) & 0xEF;
	pdu_len++;
	DestBuf[pdu_len++] = lon & 0xFF;
    pdu_len = createProtocolTail(DestBuf, pdu_len,  createProtocolSerial());
    sendTcpDataDebugShow(AGPS_LINK, DestBuf, pdu_len);
    *len = pdu_len;
}



/**************************************************
@bref		����0BЭ��
@param
@return
@note
**************************************************/

int createProtocol0B(unsigned short Serial, char *DestBuf)
{
	int pdu_len;
    int ret;
    char mac[2][6];
    uint8_t l, i, j;
	pdu_len = createProtocolHead_7979(DestBuf, 0x0B);
	/* �̵���MAC��ַ */
	DestBuf[pdu_len++] = 0x60;
	DestBuf[pdu_len++] = sysparam.bleMacCnt * 6;
	l = 5;
	for (i = 0; i < 2; i++)
	{
	    for (j = 0; j < 3; j++)
	    {
	        tmos_memcpy(&mac[i][l], &sysparam.bleConnMac[i][j], 1);
	        tmos_memcpy(&mac[i][j], &sysparam.bleConnMac[i][l], 1);
	        l--;
	    }
	    l = 5;
    }
	for (uint8_t i = 0; i < sysparam.bleMacCnt; i++)
	{
		for (uint8_t j = 0; j < 6; j++)
		{
			DestBuf[pdu_len++] = mac[i][j];
		}
	}
    pdu_len = createProtocolTail_7979(DestBuf, pdu_len,  Serial);
    return pdu_len;
}

/**************************************************
@bref		Զ��������ʼ��
@param
@return
@note
**************************************************/

void updateUISInit(uint8_t object)
{
    memset(&uis, 0, sizeof(UndateInfoStruct));
    object = object > 0 ? 1 : 0;
    if (object)
    {
    	uis.updateFirst = 0;
        uis.file_len = 512;
        LogPrintf(DEBUG_UP, "Preparing to start the upgrade relay..");
    }
    else
    {
        tmos_memset(&uis, 0, sizeof(UndateInfoStruct));
        LogPrintf(DEBUG_UP, "Uis clear");
    }
}

/**************************************************
@bref		���µ�ǰ�汾
@param
@return
@note
**************************************************/

void updateUISVersion(uint8_t *version)
{
	strncpy(uis.curCODEVERSION, version, 50);
	LogPrintf(DEBUG_UP, "updateUISVersion==>Current version[%s]", uis.curCODEVERSION);
}

/**************************************************
@bref		����Զ������Э��
@param
@return
@note
**************************************************/

static int createUpdateProtocol(char *IMEI, unsigned short Serial, char *dest, uint8_t cmd)
{
    int pdu_len = 0;
    uint32_t readfilelen;
    pdu_len = createProtocolHead_7979(dest, 0xF3);
    //����
    dest[pdu_len++] = cmd;
    if (cmd == 0x01)
    {
        //SN�ų���
        dest[pdu_len++] = strlen(IMEI);
        //����SN��
        memcpy(dest + pdu_len, IMEI, strlen(IMEI));
        pdu_len += strlen(IMEI);
        //�汾�ų���
        dest[pdu_len++] = strlen(uis.curCODEVERSION);
        //�����汾��
        memcpy(dest + pdu_len, uis.curCODEVERSION, strlen(uis.curCODEVERSION));
        pdu_len += strlen(uis.curCODEVERSION);
        
    }
    else if (cmd == 0x02)
    {
        dest[pdu_len++] = (uis.file_id >> 24) & 0xFF;
        dest[pdu_len++] = (uis.file_id >> 16) & 0xFF;
        dest[pdu_len++] = (uis.file_id >> 8)  & 0xFF;
        dest[pdu_len++] = (uis.file_id) & 0xFF;

        //�ļ�ƫ��λ��
        dest[pdu_len++] = (uis.rxfileOffset >> 24) & 0xFF;
        dest[pdu_len++] = (uis.rxfileOffset >> 16) & 0xFF;
        dest[pdu_len++] = (uis.rxfileOffset >> 8)  & 0xFF;
        dest[pdu_len++] = (uis.rxfileOffset) & 0xFF;

        readfilelen = uis.file_totalsize - uis.rxfileOffset; //�õ�ʣ��δ���մ�С
        if (readfilelen > uis.file_len)
        {
            readfilelen = uis.file_len;
        }
        //�ļ���ȡ����
        dest[pdu_len++] = (readfilelen >> 24) & 0xFF;
        dest[pdu_len++] = (readfilelen >> 16) & 0xFF;
        dest[pdu_len++] = (readfilelen >> 8)  & 0xFF;
        dest[pdu_len++] = (readfilelen) & 0xFF;
    }
    else if (cmd == 0x03)
    {
        dest[pdu_len++] = uis.updateOK;
        //SN�ų���
        dest[pdu_len++] = strlen(IMEI);
        //����SN��
        memcpy(dest + pdu_len, IMEI, strlen(IMEI));
        pdu_len += strlen(IMEI);

        dest[pdu_len++] = (uis.file_id >> 24) & 0xFF;
        dest[pdu_len++] = (uis.file_id >> 16) & 0xFF;
        dest[pdu_len++] = (uis.file_id >> 8) & 0xFF;
        dest[pdu_len++] = (uis.file_id) & 0xFF;

        //�汾�ų���
        dest[pdu_len++] = strlen(uis.newCODEVERSION);
        //����SN��
        memcpy(dest + pdu_len, uis.newCODEVERSION, strlen(uis.newCODEVERSION));
        pdu_len += strlen(uis.newCODEVERSION);
    }
    else
    {
        return 0;
    }
    pdu_len = createProtocolTail_7979(dest, pdu_len, Serial);
    return pdu_len;
}

/**************************************************
@bref		�������к�
@param
@return
@note
**************************************************/

unsigned short createProtocolSerial(void)
{
    return protocolInfo.serialNum++;
}

/**************************************************
@bref		����GPS����
@param
@return
@note
**************************************************/

void gpsRestoreSave(gpsRestore_s *gpsres)
{
    uint8_t restore[60], lens;
    uint8_t *data;
    data = (uint8_t *)gpsres;
    strcpy(restore, "Save:");
    lens = strlen(restore);
    byteToHexString(data, restore + lens, 20);
    restore[lens + 40] = 0;
    LogMessage(DEBUG_ALL, (char *)restore);
    dbSave(data, 20);
}


/**************************************************
@bref		��flash�ж�ȡgps����
@param
@return
@note
**************************************************/

void gpsRestoreUpload(void)
{
    uint16 readlen, destlen, datalen;
    uint8 readBuff[400], gpscount, i;
    char dest[1024];
    gpsRestore_s *gpsinfo;
    readlen = dbCircularRead(readBuff, 400);
    if (readlen == 0)
        return;
    LogPrintf(DEBUG_ALL, "dbread:%d", readlen);
    gpscount = readlen / sizeof(gpsRestore_s);
    LogPrintf(DEBUG_ALL, "count:%d", gpscount);

    destlen = 0;
    for (i = 0; i < gpscount; i++)
    {
        gpsinfo = (gpsRestore_s *)(readBuff + (sizeof(gpsRestore_s) * i));
        if (sysparam.protocol == JT808_PROTOCOL_TYPE)
        {
            datalen = jt808gpsRestoreDataSend((uint8_t *)dest + destlen, gpsinfo);
        }
        else
        {
            gpsRestoreDataSend(gpsinfo, dest + destlen, &datalen);
        }
        destlen += datalen;
    }
    if (sysparam.protocol == JT808_PROTOCOL_TYPE)
    {
        jt808TcpSend((uint8_t *)dest, destlen);
    }
    else
    {
        socketSendData(NORMAL_LINK, (uint8_t *)dest, destlen);
    }
}

/**************************************************
@bref		�޸Ķ�λ��
@param
@return
@note
����ʷ�ѷ��ͳɹ��Ķ�λ���޸ĳ����ڵ�ʱ��Ķ�λ��
**************************************************/

void updateHistoryGpsTime(gpsinfo_s *gpsinfo)
{
	uint16_t year;
	uint8_t  month;
	uint8_t day;
	uint8_t hour;
	uint8_t minute;
	uint8_t second;
	datetime_s datetimenew;
	portGetRtcDateTime(&year, &month, &day, &hour, &minute, &second);
	gpsinfo->datetime.year   = year % 100;
	gpsinfo->datetime.month  = month;
	gpsinfo->datetime.day    = day;
	gpsinfo->datetime.hour   = hour;
	gpsinfo->datetime.minute = minute;
	gpsinfo->datetime.second = second;
	gpsinfo->hadupload       = 0;
	datetimenew = changeUTCTimeToLocalTime(gpsinfo->datetime, -sysparam.utc);
	gpsinfo->datetime.year   = datetimenew.year % 100;
	gpsinfo->datetime.month  = datetimenew.month;
	gpsinfo->datetime.day    = datetimenew.day;
	gpsinfo->datetime.hour   = datetimenew.hour;
	gpsinfo->datetime.minute = datetimenew.minute;
	gpsinfo->datetime.second = datetimenew.second;
	LogPrintf(DEBUG_ALL, "gpsRequestSet==>Upload central poi [%02d/%02d/%02d-%02d/%02d/%02d]:lat:%.2f  lon:%.2f", 
														    year % 100, 
														    month,
														    day,
														    hour,
														    minute,
														    second,
														    gpsinfo->latitude,
														    gpsinfo->longtitude);
}

/**************************************************
@bref		Э�鷢��
@param
@return
@note
**************************************************/

void protocolSend(uint8_t link, PROTOCOLTYPE protocol, void *param)
{
    gpsinfo_s *gpsinfo;
    insParam_s *insParam;
    recordUploadInfo_s *recInfo = NULL;
    char txdata[400]={0};
    int txlen = 0;
    char *debugP;

    if (sysparam.protocol != ZT_PROTOCOL_TYPE)
    {
        if (link == NORMAL_LINK)
        {
            return;
        }
    }
    debugP = txdata;
    switch (protocol)
    {
        case PROTOCOL_01:
            txlen = createProtocol01(protocolInfo.IMEI, createProtocolSerial(), txdata);
            break;
        case PROTOCOL_12:
            gpsinfo = (gpsinfo_s *)param;

            if (link == NORMAL_LINK && gpsinfo->hadupload == 1)
            {
                return;
            }
            if (gpsinfo->fixstatus == 0)
            {
                return;
            }
            txlen = createProtocol12((gpsinfo_s *)param, createProtocolSerial(), txdata);
            break;
        case PROTOCOL_13:
            txlen = createProtocol13(createProtocolSerial(), txdata);
            break;
        case PROTOCOL_16:
            txlen = createProtocol16(createProtocolSerial(), txdata, *(uint8_t *)param);
            break;
        case PROTOCOL_19:
            txlen = createProtocol19(createProtocolSerial(), txdata);
            break;
        case PROTOCOL_21:
            insParam = (insParam_s *)param;
            txlen = createProtocol21(txdata, insParam->data, insParam->len);
            break;
        case PROTOCOL_F1:
            txlen = createProtocolF1(createProtocolSerial(), txdata);
            break;
        case PROTOCOL_8A:
            txlen = createProtocol8A(createProtocolSerial(), txdata);
            break;
        case PROTOCOL_F3:
            txlen = createProtocolF3(txdata, (WIFIINFO *)param);
            break;
        case PROTOCOL_0B:
			txlen = createProtocol0B(createProtocolSerial(), txdata);
        	break;
        case PROTOCOL_61:
            recInfo = (recordUploadInfo_s *)param;
            txlen = createProtocol61(txdata, recInfo->dateTime, recInfo->totalSize, recInfo->fileType, recInfo->packSize);
            break;
        case PROTOCOL_62:
            recInfo = (recordUploadInfo_s *)param;
            if (recInfo == NULL ||  recInfo->dest == NULL)
            {
                LogMessage(DEBUG_ALL, "recInfo dest was null");
                return;
            }
            debugP = recInfo->dest;
            txlen = createProtocol62(recInfo->dest, recInfo->dateTime, recInfo->packNum, recInfo->recData, recInfo->recLen);
            break;
       	case PROTOCOL_UP:
			txlen = createUpdateProtocol((char *)dynamicParam.SN, createProtocolSerial(), txdata, *(uint8_t *)param);
       		break;
        default:
            return;
            break;
    }
    sendTcpDataDebugShow(link, debugP, txlen);
    switch (protocol)
    {
        case PROTOCOL_12:
            if (link == NORMAL_LINK)
            {
                if (primaryServerIsReady() && getTcpNack() == 0)
                {
                    socketSendData(link, (uint8_t *)debugP, txlen);
                }
                else
                {
                    gpsRestoreSave(&gpsres);
                    LogMessage(DEBUG_ALL, "save gps");
                }
            }
            else
            {
                socketSendData(link, (uint8_t *)debugP, txlen);
            }
            break;
        default:
            socketSendData(link, (uint8_t *)debugP, txlen);
            break;
    }

}


/**************************************************
@bref		01 Э�����
@param
@return
@note
78 78 05 01 00 00 C8 55 0D 0A
**************************************************/

static void protoclparase01(uint8_t link, char *protocol, int size)
{
    LogMessage(DEBUG_ALL, "Login success");
    if (link == NORMAL_LINK)
    {
        privateServerLoginSuccess();
    }
    else if (link == BLE_LINK)
    {
        bleSerLoginReady();
    }
    else if (link == HIDDEN_LINK)
    {
        hiddenServerLoginSuccess();
    }
    else if (link == UPGRADE_LINK)
    {
		upgradeServerLoginSuccess();
    }
}

/**************************************************
@bref		13 Э�����
@param
@return
@note
78 78 05 13 00 01 E9 F1 0D 0A
**************************************************/

static void protoclparase13(uint8_t link, char *protocol, int size)
{
    LogMessage(DEBUG_ALL, "heartbeat respon");
    if (link == NORMAL_LINK)
    {
        hbtRspSuccess();
        chkNetRspSuccess();
    }
}

/**************************************************
@bref		80 Э�����
@param
@return
@note
78 78 13 80 0B 00 1D D9 E6 53 54 41 54 55 53 23 00 00 00 00 A7 79 0D 0A
**************************************************/

static void protoclparase80(uint8_t link, char *protocol, int size)
{
    insParam_s insparam;
    uint8_t *instruction;
    uint8_t instructionlen;
    char encrypt[256];
    unsigned char encryptLen;
    instructionid[0] = protocol[5];
    instructionid[1] = protocol[6];
    instructionid[2] = protocol[7];
    instructionid[3] = protocol[8];
    instructionlen = protocol[4] - 4;
    instruction = protocol + 9;
    instructionserier = (protocol[instructionlen + 11] << 8) | (protocol[instructionlen + 12]);
    insparam.link = link;

    if (link == NORMAL_LINK || link == HIDDEN_LINK)
    {
        instructionParser(instruction, instructionlen, NET_MODE, &insparam);
    }
    else if (link == BLE_LINK)
    {
        getInsid();
        sprintf(encrypt, "CMD[01020304]:%s", instruction);
        instructionlen += 14;
        encryptData(encrypt, &encryptLen, encrypt, instructionlen);
        appSendNotifyData(encrypt, encryptLen); 

    }
}

/**************************************************
@bref		utc0ʱ�����
@param
@return
@note
**************************************************/

static void protoclParser8A(char *protocol, int size)
{
    datetime_s datetime;
    if (sysinfo.rtcUpdate == 1)
        return ;
    sysinfo.rtcUpdate = 1;
    datetime.year = protocol[4];
    datetime.month = protocol[5];
    datetime.day = protocol[6];
    datetime.hour = protocol[7];
    datetime.minute = protocol[8];
    datetime.second = protocol[9];
    updateLocalRTCTime(&datetime);
}

/**************************************************
@bref		��������Э��
@param
@return
@note
**************************************************/
void protoclParser16(uint8_t link, char *protocol, int size)
{
	uint16_t serial;
    LogMessage(DEBUG_ALL, "alarm respon");
    alarmRequestClearSave();
    serial = protocol[4] << 8 | protocol[5];
}

/**************************************************
@bref		��ȡ������Ϣ
@param
@return
@note
**************************************************/

uint32_t getFileTotalSize(void)
{
	return uis.file_totalsize;
}

uint32_t getRxfileOffset(void)
{
	return uis.rxfileOffset;
}

uint8_t *getNewCodeVersion(void)
{
	return uis.newCODEVERSION;
}


/**************************************************
@bref		����Э��
@param
@return
@note
**************************************************/

static void protocolParserUpdate(uint8_t link, uint8_t *protocol, int size)
{
	int8_t ret;
	uint8_t cmd, snlen, myversionlen, newversionlen;
	uint16_t index, filecrc, calculatecrc;
	uint32_t rxfileoffset, rxfilelen;
	uint8_t *codedata;
	cmd = protocol[5];
	if (cmd == 0x01)
	{
		//�ж��Ƿ��и����ļ�
		if (protocol[6] == 0x01)
		{
			uis.file_id        = (protocol[7]  << 24 | protocol[8]  << 16 | protocol[9]  << 8 | protocol[10]);
			uis.file_totalsize = (protocol[11] << 24 | protocol[12] << 16 | protocol[13] << 8 | protocol[14]);

			snlen = protocol[15];
			index = 16;
			if (snlen > (sizeof(uis.rxsn) - 1))
			{
				LogPrintf(DEBUG_UP, "Sn too long %d", snlen);
				return ;
			}
			strncpy(uis.rxsn, (char *)&protocol[index], snlen);
			uis.rxsn[snlen] = 0;
			index = 16 + snlen;
			myversionlen = protocol[index];
			index += 1;
			if (myversionlen > (sizeof(uis.rxcurCODEVERSION) - 1))
			{
				LogPrintf(DEBUG_UP, "myversion too long %d", myversionlen);
				return ;
			}
			strncpy(uis.rxcurCODEVERSION, (char *)&protocol[index], myversionlen);
			uis.rxcurCODEVERSION[myversionlen] = 0;
			index += myversionlen;
			newversionlen = protocol[index];
			index += 1;
			if (newversionlen > (sizeof(uis.newCODEVERSION) - 1))
			{
				LogPrintf(DEBUG_ALL, "newversion too long %d", newversionlen);
				return ;
			}
			strncpy(uis.newCODEVERSION, (char *)&protocol[index], newversionlen);
			uis.newCODEVERSION[newversionlen] = 0;
			LogPrintf(DEBUG_UP, "File %08X,Total size=%d Bytes", uis.file_id, uis.file_totalsize);
			LogPrintf(DEBUG_UP, "My   SN:[%s]", uis.rxsn);
			LogPrintf(DEBUG_UP, "My  Ver:[%s]", uis.rxcurCODEVERSION);
			LogPrintf(DEBUG_UP, "New Ver:[%s]", uis.newCODEVERSION);
//			if (compareFile(uis.newCODEVERSION, newversionlen))
//			{
//				LogMessage(DEBUG_UP, "Update firmware same");
//				upgradeServerCancel();
//			}
//			else
//			{
				upgradeServerChangeFsm(NETWORK_DOWNLOAD_DOING);
				if (uis.rxfileOffset == 0)
				{
					
				}
				else
				{
					LogMessage(DEBUG_UP, "Update firmware continute");
				}
//			}
		}
		else
		{
			LogMessage(DEBUG_UP, "No update file");
			upgradeServerChangeFsm(NETWORK_DOWNLOAD_END);
		}
	}
	else if (cmd == 0x02)
	{
		if (protocol[6] == 1)
		{
			rxfileoffset = (protocol[7]  << 24 | protocol[8]  << 16 | protocol[9]  << 8 | protocol[10]); //�ļ�ƫ��
			rxfilelen    = (protocol[11] << 24 | protocol[12] << 16 | protocol[13] << 8 | protocol[14]); //�ļ���С
			calculatecrc = GetCrc16(protocol + 2, size - 6);										 //�ļ�У��
			filecrc      = (*(protocol + 15 + rxfilelen + 2) << 8) | (*(protocol + 15 + rxfilelen + 2 + 1));
			if (rxfileoffset < uis.rxfileOffset)
			{
				LogMessage(DEBUG_UP, "Receive the same firmware");
				upgradeServerChangeFsm(NETWORK_DOWNLOAD_DOING);
				return ;
			}
			if (calculatecrc == filecrc)
			{
				LogMessage(DEBUG_UP, "Data validation OK,Writting...");
				codedata = protocol + 15;
				/*
				 * �ļ�д��
				 */
				fileWriteData(uis.newCODEVERSION, codedata, rxfilelen, uis.updateFirst);
				if (uis.updateFirst == 0)
				{
					uis.updateFirst = 1;
				}
//				ret = 1;
//				if (ret == 1)
//				{
					uis.rxfileOffset = rxfileoffset + rxfilelen;
					LogPrintf(DEBUG_UP, ">>>>>>>>>> Completed progress %.2f%% <<<<<<<<<",
							  (uis.rxfileOffset * 100.0 / uis.file_totalsize));
					if (uis.rxfileOffset == uis.file_totalsize)
					{
						uis.updateOK = 1;
						upgradeServerChangeFsm(NETWORK_DOWNLOAD_DONE);
					}
					else if (uis.rxfileOffset > uis.file_totalsize)
					{
						LogMessage(DEBUG_UP, "Recevie complete ,but total size is different,retry again");
						uis.rxfileOffset = 0;
						upgradeServerChangeFsm(NETWORK_LOGIN);
					}
					else
					{
						upgradeServerChangeFsm(NETWORK_DOWNLOAD_DOING);
					}
//				}
//				else
//				{
//					//LogPrintf(DEBUG_UP, "Writing firmware error at 0x%X", APPLICATION_ADDRESS + uis.rxfileOffset);
//					upgradeServerChangeFsm(NETWORK_DOWNLOAD_ERROR);
//				}
			}
			else
			{
				LogMessage(DEBUG_UP, "Data validation Fail");
				upgradeServerChangeFsm(NETWORK_DOWNLOAD_DOING);
			}
		}
		else
		{
			LogMessage(DEBUG_UP, "δ֪\n");
		}
	}
}



/**************************************************
@bref		Э�������
@param
@return
@note
**************************************************/

void protocolRxParser(uint8_t link, char *protocol, uint16_t size)
{
    if (protocol[0] == 0X78 && protocol[1] == 0X78)
    {
        switch ((uint8_t)protocol[3])
        {
            case 0x01:
                protoclparase01(link, protocol, size);
                break;
            case 0x13:
                protoclparase13(link, protocol, size);
                break;
            case 0x80:
                protoclparase80(link, protocol, size);
                break;
            case 0x8A:
                protoclParser8A(protocol, size);
                break;
            case 0x16:
            	protoclParser16(link, protocol, size);
            	break;
        }
    }
    else if (protocol[0] == 0X79 && protocol[1] == 0X79)
    {
		switch ((uint8_t)protocol[4])
		{
		    case 0xF3:
		        protocolParserUpdate(link, protocol, size);
		    break;
		}
    }
}

/**************************************************
@bref		����123ָ��id
@param
@return
@note
**************************************************/

void save123InstructionId(void)
{
    instructionid123[0] = instructionid[0];
    instructionid123[1] = instructionid[1];
    instructionid123[2] = instructionid[2];
    instructionid123[3] = instructionid[3];
}

/**************************************************
@bref		�ָ�123ָ��ID
@param
@return
@note
**************************************************/

void reCover123InstructionId(void)
{
    instructionid[0] = instructionid123[0];
    instructionid[1] = instructionid123[1];
    instructionid[2] = instructionid123[2];
    instructionid[3] = instructionid123[3];
}

/**************************************************
@bref		����ָ��ID
@param
@return
@note
**************************************************/

void getInsid(void)
{
    bleinstructionid[0] = instructionid[0];
    bleinstructionid[1] = instructionid[1];
    bleinstructionid[2] = instructionid[2];
    bleinstructionid[3] = instructionid[3];
}

/**************************************************
@bref		����ָ��ID
@param
@return
@note
**************************************************/

void setInsId(void)
{
    instructionid[0] = bleinstructionid[0];
    instructionid[1] = bleinstructionid[1];
    instructionid[2] = bleinstructionid[2];
    instructionid[3] = bleinstructionid[3];
}

/**************************************************
@bref		�����ϴ�ָ��ID
@param
@return
@note
**************************************************/

void getLastInsid(void)
{
	lastinstructionid[0] = instructionid[0];
	lastinstructionid[1] = instructionid[1];
	lastinstructionid[2] = instructionid[2];
	lastinstructionid[3] = instructionid[3];
}

/**************************************************
@bref		�����ϴ�ָ��ID
@param
@return
@note
**************************************************/

void setLastInsid(void)
{
	instructionid[0] = lastinstructionid[0];
	instructionid[1] = lastinstructionid[1];
	instructionid[2] = lastinstructionid[2];
	instructionid[3] = lastinstructionid[3];
}

/**************************************************
@bref		ע��sn�ţ����ڵ�¼
@param
@return
@note
**************************************************/

void protocolSnRegister(char *sn)
{
    strncpy(protocolInfo.IMEI, sn, 15);
    protocolInfo.IMEI[15] = 0;
    LogPrintf(DEBUG_ALL, "Register SN:%s", protocolInfo.IMEI);
}

/**************************************************
@bref		ע���豸��Ϣ����������
@param
@return
@note
**************************************************/

void protocolInfoResiter(uint8_t batLevel, float vol, uint16_t sCnt, uint16_t runCnt)
{
    protocolInfo.batLevel = batLevel;
    protocolInfo.Voltage = vol;
    protocolInfo.startUpcnt = sCnt;
    protocolInfo.runCnt = runCnt;
}
