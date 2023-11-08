/*
 * app_encrypt.c
 *
 *  Created on: May 8, 2022
 *      Author: idea
 */
#include "app_encrypt.h"
#include "app_port.h"
#include "app_sys.h"
/*
 * @����
 */
static void encrypt(uint8 *enc, uint8 seed, uint8 *denc)
{
    uint8 i;
    uint8 encryptData[4];
    //Դ����
    //����1
    for (i = 0; i < 4; i++)
    {
        encryptData[i] = (enc[i] + seed + i) % 0x100;
    }
    //�Ի�
    encryptData[0] = encryptData[0] ^ encryptData[3];
    encryptData[3] = encryptData[0] ^ encryptData[3];
    encryptData[0] = encryptData[0] ^ encryptData[3];
    encryptData[1] = encryptData[1] ^ encryptData[2];
    encryptData[2] = encryptData[1] ^ encryptData[2];
    encryptData[1] = encryptData[1] ^ encryptData[2];
    //����2
    for (i = 0; i < 4; i++)
    {
        encryptData[i] = encryptData[i] ^ (seed + i);
    }
    //����3
    encryptData[0] = encryptData[0] ^ encryptData[2];
    encryptData[1] = encryptData[1] ^ encryptData[3];
    memcpy(denc, encryptData, 4);
}
/*
 * @����
 */
static void decrypt(uint8 *data, uint8 seed, uint8 *denc)
{
    uint8 i;
    uint8 dencryptData[4];
    //Դ����
    data[0] = data[0] ^ data[2];
    data[1] = data[1] ^ data[3];
    for (i = 0; i < 4; i++)
    {
        dencryptData[i] = data[i] ^ (seed + i);
    }

    dencryptData[0] = dencryptData[0] ^ dencryptData[3];
    dencryptData[3] = dencryptData[0] ^ dencryptData[3];
    dencryptData[0] = dencryptData[0] ^ dencryptData[3];
    dencryptData[1] = dencryptData[1] ^ dencryptData[2];
    dencryptData[2] = dencryptData[1] ^ dencryptData[2];
    dencryptData[1] = dencryptData[1] ^ dencryptData[2];
    //�Ի�

    for (i = 0; i < 4; i++)
    {
        dencryptData[i] = (dencryptData[i] + 0x100 - (seed + i)) % 0x100;
    }
    memcpy(denc, dencryptData, 4);
}

/*
 * @��������
 * @bref    �����������Ҫ����250���ֽڣ��������ݺ���4���ֽڵģ�����0����
 * @param
 *  seed    �������
 *  data    ����������
 *  len     ���������ݳ���
 *  outData �������ݴ�ŵ�ַ
 *  outDataLen  �������ݳ���
 * @return
 *  1       �ɹ�
 *  0       ʧ��
 */
uint8 encryptStr(uint8 seed, uint8 *data, uint8 len, uint8 *outData, uint8 *outDataLen)
{
    uint8 usableSize, used, myEncDataLen;
    uint8 en4[4], den4[4];
    if (data == NULL || len == 0 || outData == NULL || outDataLen == NULL)
    {
        return  0;
    }
    myEncDataLen = 0;
    outData[myEncDataLen++] = seed;
    used = 0;
    do
    {
        usableSize = (len - used) > 4 ? 4 : (len - used);
        memset(en4, 0, 4);
        memcpy(en4, data + used, usableSize);
        encrypt(en4, seed, den4);
        memcpy(outData + myEncDataLen, den4, 4);
        myEncDataLen += 4;
        used += usableSize;
    }
    while (used < len);
    *outDataLen = myEncDataLen;
    return 1;
}
/*
 * @��������
 * @bref    ���ڽ������ݺ���4�ֽڲ�����0���䣬���Խ��ܺ�0�����ݶ���
 * @param
 *  data    ����������
 *  len     ���������ݳ���
 *  outData �������ݴ�ŵ�ַ
 *  outDataLen  �������ݳ���
 * @return
 *  1       �ɹ�
 *  0       ʧ��
 */
uint8 dencryptStr(uint8 *data, uint8 len, uint8 *outData, uint8 *outDataLen)
{
    uint8 usableSize, used, myEncDataLen;
    uint8 en4[4], den4[4], seed;
    if (data == NULL || len == 0 || outData == NULL || outDataLen == NULL)
    {
        return  0;
    }
    myEncDataLen = 0;
    seed = data[0];
    used = 1;
    do
    {
        usableSize = (len - used) > 4 ? 4 : (len - used);
        memset(en4, 0, 4);
        memcpy(en4, data + used, usableSize);
        decrypt(en4, seed, den4);
        memcpy(outData + myEncDataLen, den4, 4);
        myEncDataLen += 4;
        used += usableSize;
    }
    while (used < len);
    *outDataLen = myEncDataLen;
    return 1;
}
/********************************************************************************
 ********************************************************************************
 ********************************************************************************/
/*
 * 1�����������Ҫ��ǰ֪��ñ�ӵ�ΨһID
 * 2����������ñ�Ӻ���������һ��������
 * 3��ñ����2���ڻظ���������Я��ñ��ΨһID�Լ���
 * 4�����ΨһID��𰸴��������Ͽ���ñ�ӵ�����
 * 5��ֻ�м�Ȩ�ɹ���������ñ��֮����ܱ��ֳ�����������ͨѶ
 */
//�����˷����Ȩ
uint32 startAuthentication(uint8 seed, uint16 a, uint16 b, sendCallBack callBack)
{
    uint8 data[20], enc[20], debugStr[100];
    uint8 encLen;
    uint32 result = 0;

    result = a + b + (a % 100) * (b % 100);
    //����2������
    data[0] = a >> 8 & 0xFF;
    data[1] = a & 0xFF;
    data[2] = b >> 8 & 0xFF;
    data[3] = b & 0xFF;

    encryptStr(seed, data, 4, enc, &encLen);

    if (callBack != NULL)
    {
        callBack(enc, encLen);
    }
	//��ӡԭʼ����
    //memset(debugStr, 0, 100);
    //byteArrayToHexString(data, debugStr, 4);
    //LogPrintf(0, "ԭʼ:%s,���%#X", debugStr, result);
	//��ӡ��������
    //memset(debugStr, 0, 100);
    //byteArrayToHexString(enc, debugStr, encLen);
    //LogPrintf(0, "���ܺ�:%s", debugStr);

    //���������ӿڰ� enc and encLen ���ͳ�ȥ

    //sendAuthenticationRespon(seed, "0000AAAA", cauculateAuthResult(enc, encLen));
    return result;
}
/*
 * ñ�ӽ���
 * �����յ��ļ��ܺ�ļ�Ȩ����
 * ��������
 */
uint32 cauculateAuthResult(uint8 *encryptData, uint8 encryptDataLen)
{
    uint8 dencrypt[20], dencryptLen;
    uint16 a;
    uint16 b;
    uint32 result = 0;

    dencryptStr(encryptData, encryptDataLen, dencrypt, &dencryptLen);

    a = dencrypt[0] << 8 | dencrypt[1];
    b = dencrypt[2] << 8 | dencrypt[3];
    result = a + b + (a % 100) * (b % 100);

	//�Լ��ܵļ�Ȩ���ݽ��н��ܣ����õ����
    LogPrintf(0, "cauculateAuthResult==>%08X", result);
    return result;
}

/*
 * ñ�ӻظ�
 * DevId    :ΨһID��8���ֽڳ���
 * result   :����ļ�Ȩ��
 */
void sendAuthenticationRespon(uint8 seed, uint8 *DevId, uint32 result)
{
    uint8 data[20], i, dataLen = 0;
    uint8 encrypt[20], encryptLen;
    uint8 debugStr[100];
    for (i = 0; i < 8; i++)
    {
        data[dataLen++] = DevId[i];
    }
    data[dataLen++] = result >> 24 & 0xFF;
    data[dataLen++] = result >> 16 & 0xFF;
    data[dataLen++] = result >> 8 & 0xFF;
    data[dataLen++] = result & 0xFF;
    //���м���
    encryptStr(seed, data, dataLen, encrypt, &encryptLen);

	//��ӡԭʼ����
    memset(debugStr, 0, 100);
    byteToHexString(data, debugStr, dataLen);
    LogPrintf(0, "ԭʼ:%s", debugStr);
	//��ӡ���ܺ������
    memset(debugStr, 0, 100);
    byteToHexString(encrypt, debugStr, encryptLen);
    LogPrintf(0, "���ܺ�:%s", debugStr);

    //���������ӿڰ����ݴ��ͳ�ȥ
    //encrypt and encryptLen


}


void createEncrypt(uint8_t *mac, uint8_t *encBuff, uint8_t *encLen)
{
    uint8_t debug[50];
    uint8_t src[50];
    uint8_t srcLen;
    uint8_t dec[50];
    uint8_t decLen;
    uint8_t i;

    uint16_t year;
    uint8_t  month;
    uint8_t date;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;

    portGetRtcDateTime(&year, &month, &date, &hour, &minute, &second);

    srcLen = 0;
    //����MAC��ַ
    for (i = 0; i < 6; i++)
    {
        src[srcLen++] = mac[i];
    }
    //��ʼ��Ч�ڣ�������ʱ��
    src[srcLen++] = year % 100;
    src[srcLen++] = month;
    src[srcLen++] = date;
    src[srcLen++] = hour;
    src[srcLen++] = minute;
    //��Чʱ��������
    src[srcLen++] = 0xAA; //0xAAΪ��ʱ�����൱�ڲ��ж�ʱ��������
    i = rand();
    //������Կ
    encryptStr(i, src, srcLen, dec, &decLen);

    byteToHexString(mac, debug, 6);
    debug[12] = 0;
    LogPrintf(DEBUG_ALL, "MAC:[%s]", debug);

    byteToHexString(dec, debug, decLen);
    debug[decLen * 2] = 0;
    LogPrintf(DEBUG_ALL, "DEC:[%s],DECLEN:%d", debug, decLen);
    LogPrintf(DEBUG_ALL, "LimiteTime:[%02d/%02d/%02d %02d:%02d],Limit Minute:%d", year, month, date, hour, minute, 5);

    memcpy(encBuff, dec, decLen);
    *encLen = decLen;
}
