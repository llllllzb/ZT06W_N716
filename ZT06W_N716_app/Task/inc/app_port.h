#ifndef APP_PORT
#define APP_PORT

#include <stdint.h>
#include "config.h"
#define APPUSART0_BUFF_SIZE 512
#define APPUSART1_BUFF_SIZE 10
#define APPUSART2_BUFF_SIZE 128
#define APPUSART3_BUFF_SIZE 768

#define DEBUG_UART		&usart2_ctl
#define MODULE_UART		&usart3_ctl
#define GPS_UART		&usart0_ctl


//4G模组涉及IO
#define SUPPLY_PIN      GPIO_Pin_10
#define PORT_SUPPLY_ON  GPIOB_SetBits(SUPPLY_PIN)
#define PORT_SUPPLY_OFF GPIOB_ResetBits(SUPPLY_PIN)

#define POWER_PIN    	GPIO_Pin_11	//PB11
#define PORT_PWRKEY_H   GPIOB_ResetBits(POWER_PIN)
#define PORT_PWRKEY_L   GPIOB_SetBits(POWER_PIN)

#define RST_PIN			
#define PORT_RSTKEY_H	
#define PORT_RSTKEY_L	


#define RING_PIN		GPIO_Pin_14	//PA14

#define DTR_PIN			GPIO_Pin_15	//PA15
#define DTR_HIGH		GPIOA_SetBits(DTR_PIN)
#define DTR_LOW			GPIOA_ResetBits(DTR_PIN)

//DA217涉及IO
#define GSPWR_PIN       GPIO_Pin_13	//PB13
#define GSPWR_ON        GPIOB_SetBits(GSPWR_PIN)
#define GSPWR_OFF       GPIOB_ResetBits(GSPWR_PIN)
#define GSINT_PIN       GPIO_Pin_8	//PA8
//IIC 涉及IO
#define SCL_PIN         GPIO_Pin_14	//PB14
#define SDA_PIN         GPIO_Pin_15	//PB15

//LED 涉及IO
#define LED1_PIN        GPIO_Pin_13	//PA13
#define LED1_ON         GPIOA_SetBits(LED1_PIN)
#define LED1_OFF        GPIOA_ResetBits(LED1_PIN)
#define LED2_PIN
#define LED2_ON
#define LED2_OFF

//GPS 涉及IO
#define GPSPWR_PIN		GPIO_Pin_12 //PB12
#define GPSPWR_ON		GPIOB_SetBits(GPSPWR_PIN)
#define GPSPWR_OFF		GPIOB_ResetBits(GPSPWR_PIN)

#define GPSLNA_PIN       
#define GPSLNA_ON        
#define GPSLNA_OFF       

//前感光 涉及IO
#define LDR1_PIN		GPIO_Pin_11 //PA11
#define LDR1_READ		(GPIOA_ReadPortPin(LDR1_PIN) ? 1 : 0)
//后感光 涉及IO
#define LDR2_PIN		GPIO_Pin_12 //PA12
#define LDR2_READ		(GPIOA_ReadPortPin(LDR2_PIN) ? 1 : 0)

//ACC 涉及IO
#define ACC_PIN			GPIO_Pin_22	//PB22
#define ACC_PORT_READ	(GPIOB_ReadPortPin(ACC_PIN) ? 1 : 0)
//RELAY	涉及IO
#define RELAY_PIN		GPIO_Pin_10 //PA10
#define RELAY_ON		GPIOA_SetBits(RELAY_PIN)
#define RELAY_OFF		GPIOA_ResetBits(RELAY_PIN)

#define VCARD_ADCPIN	GPIO_Pin_9
#define VCARD_CHANNEL	CH_EXTIN_13

// (MAXCALCTICKS * 5) + (max remainder) must be <= (uint16 max),
// so: (13105 * 5) + 7 <= 65535
#define MAXCALCTICKS  ((uint16_t)(13105))
 
//#define	BEGYEAR	        2000UL     // UTC started at 00:00:00 January 1, 2020
 
#define	DAY             86400UL  // 24 hours * 60 minutes * 60 seconds
 
typedef enum
{
    SIM_1 = 1,
    SIM_2,
} SIM_USE;

typedef enum
{
	SIM_MODE_1 = 0,
	SIM_MODE_2,
} SIM_MODE;


typedef enum
{
    APPUSART0,
    APPUSART1,
    APPUSART2,
    APPUSART3,
} UARTTYPE;

typedef struct
{
    uint8_t *rxbuf;
    uint8_t init;
    uint16_t rxbufsize;
    __IO uint16_t rxbegin;
    __IO uint16_t rxend;

    void (*rxhandlefun)(uint8_t *, uint16_t len);
    void (*txhandlefun)(uint8_t *, uint16_t len);

} UART_RXTX_CTL;

typedef struct{
	uint16_t year;
	uint8_t month;
	uint8_t date;
	uint8_t hour;
	uint8_t minute;
	uint8_t second;
}system_date_s;

extern UART_RXTX_CTL usart0_ctl;
extern UART_RXTX_CTL usart1_ctl;
extern UART_RXTX_CTL usart2_ctl;
extern UART_RXTX_CTL usart3_ctl;

void pollUartData(void);
void portUartCfg(UARTTYPE type, uint8_t onoff, uint32_t baudrate, void (*rxhandlefun)(uint8_t *, uint16_t len));
void portUartSend(UART_RXTX_CTL *uartctl, uint8_t *buf, uint16_t len);

void portModuleGpioCfg(void);
void portLedGpioCfg(void);
void portGpsGpioCfg(void);
void portGpioSetDefCfg(void);
void portLdrGpioCfg(void);
void portAccGpioCfg(void);

void portSysReset(void);

uint8_t iicReadData(uint8_t addr, uint8_t regaddr, uint8_t *data, uint8_t len);
uint8_t iicWriteData(uint8_t addr, uint8_t reg, uint8_t data);
void portGsensorPwrCtl(uint8 onoff);
void portIICCfg(void);
void portGsensorCtl(uint8_t onoff);

void portRtcCfg(void);
void portGetRtcDateTime(uint16_t *year, uint8_t *month, uint8_t *date, uint8_t *hour, uint8_t *minute, uint8_t *second);
void portUpdateRtcDateTime(uint8_t year, uint8_t month, uint8_t date, uint8_t hour, uint8_t minute, uint8_t second);
int portSetNextAlarmTime(void);
void portSetNextWakeUpTime(void);
void portUpdateRtcOffset(uint8_t year, uint8_t month, uint8_t date, uint8_t hour, uint8_t minute, uint8_t second);

void portLowPowerCfg(void);


void portAdcCfg(void);
float portGetAdcVol(ADC_SingleChannelTypeDef channel);

void portSleepEn(void);
void portSleepDn(void);
void portGetSleepState(void);

void portWdtCfg(void);
void portWdtFeed(void);
void portFclkChange(uint8_t type);

#endif
