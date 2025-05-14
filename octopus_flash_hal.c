/**
 * @file	halPeripheral.c
 * @author	chipsea
 * @brief	
 * @version	0.1
 * @date	2020-11-30
 * @copyright Copyright (c) 2020, CHIPSEA Co., Ltd.
 * @note
 */

/*********************************************************************
 * INCLUDES
 */
#include "octopus_platform.h"
#include "octopus_flash_hal.h"
#include "octopus_log.h"

void hal_flash_init(uint8_t task_id)
{	
#ifdef PLATFORM_CST_OSAL_RTOS
	///HalDMAInit();
	///HalDMAInitChannel(dma_cfg);
 #endif
	LOG_LEVEL("hal flash init\r\n");
}

void hal_flash_read_to_buff(uint32_t startaddr, uint8_t *buffer, uint8_t length)
{
     #ifdef PLATFORM_CST_OSAL_RTOS
	 HalFlashRead(addr, buf, len);
     #endif
}

/**
 * @brief  Read a data buffer from Flash.
 * @param  addr: Start address in Flash memory
 * @param  buf: Destination buffer to read into
 * @param  len: Length in bytes
 */
void hal_flash_read_buff(uint32_t startaddr, uint8_t *buffer, uint8_t length)
{
    //memcpy(buffer, (const void *)startaddr, length);
}
/**
 * @brief  Write a data buffer to Flash with auto-erasure.
 * @param  addr: Start address in Flash memory
 * @param  buf: Pointer to data buffer
 * @param  len: Length in bytes
 */
void hal_flash_write_buff(uint32_t startaddr, uint8_t *buffer, uint8_t length)
{

}

void hal_flash_write_buff_dma(uint32_t startaddr, uint8_t *buffer, uint8_t length)
{
	#ifdef PLATFORM_CST_OSAL_RTOS
	HalFlashWriteByDma(addr, buf, len);
	#endif
}

void hal_eeprom_save(uint32_t startaddr, uint8_t *buffer, uint8_t length)
{
	#ifdef USE_EEROM_FOR_DATA_SAVING
	I2C_EepromPageWrite(startaddr,buffer,length);
	#endif
}

void hal_eeprom_read(uint32_t startaddr, uint8_t *buffer, uint8_t length)
{
	#ifdef USE_EEROM_FOR_DATA_SAVING
	I2C_EepromPageWrite(startaddr,buffer,length);
	#endif
}
