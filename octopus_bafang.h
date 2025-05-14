
#ifndef __TASK_COM_UART_PTL_BAFANG_H__
#define __TASK_COM_UART_PTL_BAFANG_H__

/*******************************************************************************
 * INCLUDES
 */
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"{
#endif

/**
 * \defgroup APP_SETTING APP:SETTING
 * @{
 */

//??-??????
typedef enum
{
    SETTING_WHEEL_16_Inch,          //?????16??
    SETTING_WHEEL_18_Inch,          //?????18??
    SETTING_WHEEL_20_Inch,          //?????20??
    SETTING_WHEEL_22_Inch,          //?????22??
    SETTING_WHEEL_24_Inch,          //?????24??
    SETTING_WHEEL_26_Inch,          //?????26??
    SETTING_WHEEL_27_Inch,          //?????27??
    SETTING_WHEEL_27_5_Inch,        //?????27.5??
    SETTING_WHEEL_28_Inch,          //?????28??
    SETTING_WHEEL_29_Inch,          //?????29??
}SETTING_WHEEL;
//??-????????
typedef enum
{
    SETTING_MAX_PAS_3_LEVEL = 3,        //???????????0-3level
    SETTING_MAX_PAS_5_LEVEL = 5,        //???????????0-5level
    SETTING_MAX_PAS_9_LEVEL = 9,        //???????????0-9level
}SETTING_MAX_PAS;
/*******************************************************************************
 * GLOBAL FUNCTIONS DECLEAR
 */

void app_bafang_ptl_init_running(void);
void app_bafang_ptl_start_running(void);
void app_bafang_ptl_assert_running(void);
void app_bafang_ptl_running(void);
void app_bafang_ptl_post_running(void);
void app_bafang_ptl_stop_running(void);

uint16_t get_top_error_code(void);
uint16_t get_wheel_radius_mm(void);
uint16_t get_wheel_radius_inch(void);
void add_error_code(uint16_t code);
void TRIP_Reset_mileage(void);  
void Factory_Setting_mileage(void); 
/**
 * end of group APP_SETTING
 * @}
 */

#ifdef __cplusplus
}
#endif


#endif
