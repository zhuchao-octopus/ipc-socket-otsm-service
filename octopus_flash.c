

#include "octopus_platform.h"  			// Include platform-specific header for hardware platform details
#include "octopus_log.h"       			// Include logging functions for debugging
#include "octopus_task_manager.h" 	// Include task manager for scheduling tasks
#include "octopus_flash.h"


/*******************************************************************************
 * LOCAL FUNCTIONS DECLEAR
 */
 
 /**
* @fn void printfFuncHex(const char *fun, int line, char *str, uint8_t *dat, int len)
* @brief print input data in hex
* @param fun: print function name
* @param line: Print line number 
* @param str: print data in string
* @param dat: print data in hex
* @param len: length of data
* @return NONE.
*/

//PrintfBuffHex(__func__, __LINE__, "READ After Write By Dma", tempStr, osal_strlen(str));
void PrintfBuffHex(const char *fun, int line, char *str, uint8_t *dat, int len)
{
	///LOG_("%s(%d):%s:", fun, line, str);
	///for (int ii = 0; ii < len; ii++)
	///{
	///	LOG_("%02x ", dat[ii]);
	///}
	///LOG_("\r\n");
}

void FlashReadToBuff(uint32_t addr, uint8_t *buf, uint32_t length)
{
	 hal_flash_read_to_buff(addr, buf, length);
}

void FlashWriteBuffTo(uint32_t addr, uint8_t *buf, uint32_t length)
{
	hal_flash_write_buff(addr,buf,length);
}

void E2ROMReadToBuff(uint32_t addr, uint8_t *buf, uint32_t length)
{
	 hal_eeprom_read(addr, buf, length);
}

void E2ROMWriteBuffTo(uint32_t addr, uint8_t *buf, uint32_t length)
{
	hal_eeprom_save(addr,buf,length);
}
