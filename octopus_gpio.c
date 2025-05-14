/******************************************************************************
	* @file    gpio_implementation.c
	* @brief   GPIO management implementation for hardware platform.
	*          This module handles GPIO initialization, status polling, and key
	*          event processing, as well as interaction with the task manager.
	* @version 1.0
	* @date    2025-05-10
	*
	* @note    Platform-specific functions are defined in octopus_platform.h
	*          and GPIO configuration is handled for status and key polling.
	*          Interrupt-based handling can be added for real-time event capture.
	*          This implementation uses polling for simplicity and clarity.
	*
	*          Key Features:
	*          - GPIO Initialization
	*          - Polling for GPIO status
	*          - Polling for Key Press status
	*          - Event processing for key press and release
	*          - Task management state transitions
	*
	* @author  [ak47]
	******************************************************************************/

/******************************************************************************
 * INCLUDES
 ******************************************************************************/
#include "octopus_platform.h"	  // Include platform-specific header for hardware platform details
#include "octopus_log.h"		  // Include logging functions for debugging
#include "octopus_task_manager.h" // Include task manager for scheduling tasks
#include "octopus_gpio.h"		  // Include GPIO control and configuration
#include "octopus_tickcounter.h"  // Include tick counter for timing operations
#include "octopus_msgqueue.h"	  // Include message queue for inter-task communication
#include "octopus_flash.h"		  // Include flash memory access functions
#include "octopus_key.h"		  // Include key status and event handling

#ifdef TASK_MANAGER_STATE_MACHINE_GPIO
	/*******************************************************************************
	 * DEBUG SWITCH MACROS
	 */

	/*******************************************************************************
	 * LOCAL FUNCTIONS DECLEAR
	 */

void GPIOInit(void);
void PollingGPIOStatus(GPIO_GROUP *gpiox,uint16_t pin, GPIO_STATUS *gpio_status);
void PollingGPIOKeyStatus(GPIO_GROUP *gpiox,uint16_t pin,GPIO_KEY_STATUS *key_status);
void ProcessKeyEvent(GPIO_KEY_STATUS *key_status);

static uint32_t           l_t_msg_wait_50_timer;
 
 /*******************************************************************************
 * GLOBAL VARIABLES
 */

GPIO_STATUS acc_status = {false,true,0,0};
GPIO_STATUS ddd_status = {false,true,0,0};
GPIO_STATUS zzd_status = {false,true,0,0};
GPIO_STATUS yzd_status = {false,true,0,0};
GPIO_STATUS skd_status = {false,true,0,0};

GPIO_KEY_STATUS key_status_power = {OCTOPUS_KEY_POWER,0};
GPIO_KEY_STATUS *gpio_key_array[]={&key_status_power};
//static bool module_send_handler(ptl_frame_type_t frame_type, ptl_frame_cmd_t cmd, uint16_t param, ptl_proc_buff_t *buff);
//static bool module_receive_handler(ptl_frame_payload_t *payload, ptl_proc_buff_t *ackbuff);

/*******************************************************************************
 *  GLOBAL FUNCTIONS IMPLEMENTATION
 */
void app_gpio_init_running(void)
{
    LOG_LEVEL("app_gpio_init_running\r\n");
		GPIOInit();
    //com_uart_ptl_register_module(MSGMODULE_SYSTEM, module_send_handler, module_receive_handler);
    OTMS(TASK_ID_GPIO, OTMS_S_INVALID);
}

void app_gpio_start_running(void)
{
    LOG_LEVEL("app_gpio_start_running\r\n");
    OTMS(TASK_ID_GPIO, OTMS_S_ASSERT_RUN);
}

void app_gpio_assert_running(void)
{
    #ifdef TASK_MANAGER_STATE_MACHINE_MCU
    StartTickCounter(&l_t_msg_wait_50_timer);
    OTMS(TASK_ID_GPIO, OTMS_S_RUNNING);
    #endif
}

void app_gpio_running(void)
{
   if(GetTickCounter(&l_t_msg_wait_50_timer) >= GPIO_POLLING_PERIOD_MS)
	 {
			//PollingGPIOStatus(GPIO_ACC_PIN,&acc_status);
			//PollingGPIOStatus(GPIO_DDD_PIN,&ddd_status);
			//PollingGPIOStatus(GPIO_ZZD_PIN,&zzd_status);
			//PollingGPIOStatus(GPIO_YZD_PIN,&yzd_status);
			//PollingGPIOStatus(GPIO_SKD_PIN,&skd_status);
			PollingGPIOKeyStatus(GPIO_POWER_GROUP,GPIO_POWER_PIN,&key_status_power);
		 
      ProcessKeyEvent(&key_status_power);
		  //////////////////////////////////////////////////////////////////////////////////////////////////////
		  //////////////////////////////////////////////////////////////////////////////////////////////////////
		  //////////////////////////////////////////////////////////////////////////////////////////////////////
		  #if 0
			if(acc_status.changed)
			{
				LOG_LEVEL("get acc status=%d\r\n",acc_status.offon);
				send_message(TASK_ID_BLE, MSG_DEVICE_ACC_EVENT, GPIO_ACC_PIN, acc_status.offon);
				if(acc_status.offon)
				send_message(TASK_ID_SYSTEM, MSG_DEVICE_POWER_EVENT, CMD_MODSYSTEM_POWER_ON, acc_status.offon);
				else
				send_message(TASK_ID_SYSTEM, MSG_DEVICE_POWER_EVENT, CMD_MODSYSTEM_POWER_OFF, acc_status.offon);
				
				acc_status.changed=false;
			}	
			
	  	if(ddd_status.changed)
			{
				LOG_LEVEL("get ddd status=%d\r\n",ddd_status.offon);
				send_message(TASK_ID_CAR_INFOR, MSG_DEVICE_GPIO_EVENT, GPIO_DDD_PIN, ddd_status.offon);
				ddd_status.changed=false;
			}	
			
			if(zzd_status.changed)
			{
				LOG_LEVEL("get zzd status=%d\r\n",zzd_status.offon);
				send_message(TASK_ID_CAR_INFOR, MSG_DEVICE_GPIO_EVENT, GPIO_ZZD_PIN, zzd_status.offon);
				zzd_status.changed=false;
			}	
			
			if(yzd_status.changed)
			{
				LOG_LEVEL("get yzd status=%d\r\n",yzd_status.offon);
				send_message(TASK_ID_CAR_INFOR, MSG_DEVICE_GPIO_EVENT, GPIO_YZD_PIN, yzd_status.offon);
				yzd_status.changed=false;
			}	
			
			if(skd_status.changed)
			{
				LOG_LEVEL("get skd status=%d\r\n",skd_status.offon);
				send_message(TASK_ID_CAR_INFOR, MSG_DEVICE_GPIO_EVENT, GPIO_SKD_PIN, skd_status.offon);
				skd_status.changed=false;
			}
			#endif
			
		StartTickCounter(&l_t_msg_wait_50_timer); 
	 }
}

void app_gpio_post_running(void)
{

}

void app_gpio_stop_running(void)
{
    OTMS(TASK_ID_GPIO, OTMS_S_INVALID);
}
/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
void GPIOInit(void)
{

}
 
void PollingGPIOStatus(GPIO_GROUP *gpiox,uint16_t pin, GPIO_STATUS *gpio_status)
{
	if(GPIO_PIN_READ(gpiox,pin))
	{
		gpio_status->count1++;
		gpio_status->count2=0;
		if(gpio_status->count1 > GPIO_STATUS_REDUNDANCY)
		{
			if(!gpio_status->offon)
		  	gpio_status->changed=true;
			else
				gpio_status->changed=false;
			gpio_status->offon=true;
			gpio_status->count1=0;
		}
	}
	else
	{
		gpio_status->count1 = 0;
		gpio_status->count2++;
		if(gpio_status->count2 > GPIO_STATUS_REDUNDANCY)
		{
			if(gpio_status->offon)
		  	gpio_status->changed=true;
			else
				gpio_status->changed=false;
		  gpio_status->offon=false;
			gpio_status->count2=0;
		}
	}
}

void PollingGPIOKeyStatus(GPIO_GROUP *gpiox,uint16_t pin,GPIO_KEY_STATUS *key_status)
{
	uint8_t g_status = GPIO_PIN_READ(gpiox,pin);
	if(!g_status)
	{		
		if(key_status->press_count < GPIO_KEY_STATUS_MAX_REDUNDANCY)
		{
		   key_status->press_count++;
			 key_status->long_press_duration =key_status->press_count;
		}
													
	  if(key_status->press_count > GPIO_KEY_STATUS_PRESS_PERIOD && !key_status->pressed)
		{
			//key_status->key = hal_get_gpio_key_mask_code(pin);//key_status->key | GetGPIOKeyMask(pin);
			key_status->pressed=true;
			key_status->release=false;
			key_status->event_dispatched=false;
			key_status->state=KEY_STATE_PRESSED;
		}	
		
		if(key_status->press_count > GPIO_KEY_STATUS_LONG_PRESS_PERIOD && key_status->pressed)
		{
			//key_status->key = hal_get_gpio_key_mask_code(pin);//key_status->key | GetGPIOKeyMask(pin);
			key_status->pressed=true;
			key_status->release=false;
			key_status->event_dispatched=false;
			key_status->state=KEY_STATE_LONG_PRESSED;
		}	
		if(key_status->press_count > GPIO_KEY_STATUS_LONG_LONG_PRESS_PERIOD && key_status->pressed)
		{
			//key_status->key = hal_get_gpio_key_mask_code(pin);//key_status->key | GetGPIOKeyMask(pin);
			key_status->pressed=true;
			key_status->release=false;
			key_status->event_dispatched=false;
			key_status->state=KEY_STATE_LONG_LONG_PRESSED;
		}	
	}
	else
	{
		if(key_status->pressed)
		{
			key_status->release=true;
			key_status->event_dispatched=false;
		}
		key_status->pressed=false;
		key_status->press_count = 0;
		///key_status->key = key_status->key & (~GetGPIOKeyMask(pin));
	}
} 

void ProcessKeyEvent(GPIO_KEY_STATUS *key_status)
{
	if(!key_status->event_dispatched)	
	{	
		///LOG_LEVEL("press status key=%d %d\r\n",key_status->key,key_status->state);
		if(key_status->pressed)
		{
   			send_message(TASK_ID_KEY, MSG_DEVICE_KEY_DOWN_EVENT , key_status->key, KEY_STATE_PRESSED );
        key_status->event_dispatched=true; 										
		}
    else if(key_status->release)
		{
		    send_message(TASK_ID_KEY, MSG_DEVICE_KEY_UP_EVENT , key_status->key, KEY_STATE_RELEASED );
        key_status->event_dispatched=true;	
		}
	}	
}

GPIO_KEY_STATUS* get_key_status_by_key(uint8_t key) 
{
	size_t array_size = sizeof(gpio_key_array) / sizeof(gpio_key_array[0]); 
  for (size_t i = 0; i < array_size; i++) 
	{
     if (gpio_key_array[i]->key == key) {
        return gpio_key_array[i]; 
     }
  }
  return NULL; 
}

bool IsAccOn(void)
{
	return GPIO_PIN_READ_ACC();
}

bool IsPowerOn(void)
{
	return GPIO_PIN_READ(GPIO_POWER_F113_GROUP,GPIO_POWER_F113_PIN);
}

void power_on_off(bool onoff)
{
	if(onoff)
		GPIO_PIN_WRITE(GPIO_POWER_F113_GROUP,GPIO_POWER_F113_PIN,Bit_SET);
	else
		GPIO_PIN_WRITE(GPIO_POWER_F113_GROUP,GPIO_POWER_F113_PIN,Bit_RESET);
}

/*
bool module_send_handler(ptl_frame_type_t frame_type, ptl_frame_cmd_t cmd, uint16_t param, ptl_proc_buff_t *buff)
{
	return true;
}


bool module_receive_handler(ptl_frame_payload_t *payload, ptl_proc_buff_t *ackbuff)
{
  return true;	
}*/

#endif
