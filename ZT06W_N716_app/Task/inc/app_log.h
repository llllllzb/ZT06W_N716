/*
 * app_log.h
 *
 *  Created on: Oct 17, 2023
 *      Author: nimo
 */

#ifndef TASK_INC_APP_LOG_H_
#define TASK_INC_APP_LOG_H_

#include "config.h"

#define LOG_ERASE_MIN_SIZE				1024

#define LOG_AREA_ADDRESS				0x64000
#define LOG_AREA_SIZE					(48 * LOG_ERASE_MIN_SIZE)		// 48K

/* Æô¶¯log»º´æÄ£Ê½ */
#define LOG_FLASH_BUFMODE_ENABLED
#define LOG_FLASH_BUFMODE_SIZE			1024


#endif /* TASK_INC_APP_LOG_H_ */
