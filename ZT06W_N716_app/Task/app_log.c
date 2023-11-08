/*
 * app_log.c
 *
 *  Created on: Oct 17, 2023
 *      Author: nimo
 */

#include "app_log.h"

#ifdef LOG_FLASH_BUFMODE_ENABLED
/* log����ģʽ�洢�� */
static char log_buf[LOG_FLASH_BUFMODE_SIZE] = { 0 };
/* log����ģʽ�洢����ǰ���� */
static uint16_t log_buf_size = 0;

#endif
/* log�洢�����ͷβָ�� */
static uint32_t log_start_addr = 0, log_end_addr = 0;


/**
 * 	��ʼ��log�洢ϵͳ
 *
 */

void log_flash_init(void)
{
#ifdef LOG_FLASH_BUFMODE_ENABLED
	log_buf_size = 0;
#endif
	/* ��ʼ��log�洢����ͷβָ�� */

	
}

/**
 * 	��Logд��flash��
 *
 * @param log,ָ���ַ�������ָ��,����ͨ��logָ���޸�������
 * @param size,log����
 */

void log_flash_write(const char *log, uint16_t size)
{
#ifdef LOG_FLASH_BUFMODE_ENABLED
	uint16_t write_size = 0, write_index = 0;
#endif

	/* ��һ����Դͬ����:���� */

#ifdef LOG_FLASH_BUFMODE_ENABLED
	while (1){
//		if (cur_buf_size + size > ELOG_FLASH_BUF_SIZE) {
//            write_size = ELOG_FLASH_BUF_SIZE - cur_buf_size;
//            elog_memcpy(log_buf + cur_buf_size, log + write_index, write_size);
//            write_index += write_size;
//            size -= write_size;
//            cur_buf_size += write_size;
//            /* unlock flash log buffer */
//            log_buf_unlock();
//            /* write all buffered log to flash, cur_buf_size will reset */
//            elog_flash_flush();
//            /* lock flash log buffer */
//            log_buf_lock();
//        } else {
//            elog_memcpy(log_buf + cur_buf_size, log + write_index, size);
//            cur_buf_size += size;
//            break;
//        }

		if (log_buf_size + size > LOG_FLASH_BUFMODE_SIZE) {
			write_size = LOG_FLASH_BUFMODE_SIZE - log_buf_size;
			tmos_memcpy(log_buf + log_buf_size, log + write_index, write_size);
			write_index += write_size;
			size -= write_size;
			log_buf_size += write_size;
			
		} else {
			
		}
        

	}
#else

#endif
}





#ifdef LOG_FLASH_BUFMODE_ENABLED
/**
 * 	����������logд��flash
 */
void log_flash_write_from_buf(void)
{
	uint16_t size = 0;
	/* ���� */	
	
	if (log_buf_size % 4 != 0) {
		size = 4 - (log_buf_size % 4);
	}
	/* ��'\r'�������ĸ��ֽ� */
	tmos_memset(log_buf + log_buf_size, '\r', size);

	/* д�� */

	log_buf_size = 0;
	/* ���� */
}

#endif


/**
 * Write log to flash.
 *
 * @param log the log which will be write to flash
 * @param size write bytes size
 *
 * @return result
 */
void log_write_to_flash(const uint32_t *log ,uint16_t size)
{
	uint32_t write_addr;
	uint16_t writable_size; // �Ѳ�������ûд�Ĵ�С

	
	if (log_start_addr != log_end_addr) {
		write_addr = log_end_addr + 4;
	} else {
		write_addr = log_start_addr;
	}
	/* ��д���Ѳ�����û��ʹ�õ����� */
	writable_size = LOG_ERASE_MIN_SIZE - ((write_addr - LOG_AREA_ADDRESS) % LOG_ERASE_MIN_SIZE);
	
}




