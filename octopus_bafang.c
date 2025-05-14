
/*******************************************************************************
 * INCLUDES
 */
#include "octopus_platform.h" // Include platform-specific header for hardware platform details
#include "octopus_tickcounter.h"
#include "octopus_msgqueue.h"
#include "octopus_bafang.h"
#include "octopus_uart_hal.h"
#include "octopus_uart_ptl_2.h"    // Include UART protocol header
#include "octopus_carinfor.h"
/*******************************************************************************
 * DEBUG SWITCH MACROS
 */

/*******************************************************************************
 * MACROS
 */

/*******************************************************************************
 * TYPEDEFS
 */

typedef enum
{
    PTL_BAFANG_CMD_SYMBOL_IDLE = 0x0000,
    PTL_BAFANG_CMD_SYMBOL_SYSTEM_STATE = 0x1108,
    PTL_BAFANG_CMD_SYMBOL_CURRENT = 0x110A,
    PTL_BAFANG_CMD_SYMBOL_SPEED = 0x1120,
    PTL_BAFANG_CMD_SYMBOL_SOC = 0x1111,
    PTL_BAFANG_CMD_SYMBOL_WORKING_STATE = 0x1131,
    PTL_BAFANG_CMD_SYMBOL_BATTERY = 0x1160,
    PTL_BAFANG_CMD_SYMBOL_CELL = 0x1161,
    PTL_BAFANG_CMD_SYMBOL_LAMP = 0x161A,
    PTL_BAFANG_CMD_SYMBOL_SPEED_LIMIT = 0x161F,
    PTL_BAFANG_CMD_SYMBOL_GEAR = 0x160B,
}PTL_BAFANG_CMD_SYMBOL;

typedef enum
{
    PTL_BAFANG_PROCESS_STATE_INIT,
    PTL_BAFANG_PROCESS_STATE_SEND_CMD,
    PTL_BAFANG_PROCESS_STATE_WAIT_TIME,
    PTL_BAFANG_PROCESS_STATE_CHECK_RESPONSE,
    PTL_BAFANG_PROCESS_STATE_NEXT_CMD,
    PTL_BAFANG_PROCESS_STATE_DEINIT,
}PTL_BAFANG_PROCESS_STATE;

typedef void (*UartSendPtlBafangCmdFun_f)(void);

typedef struct UartSendProtocolCmdCtrl
{
    uint16_t                    cmdSymbol;        //命令表示
    UartSendPtlBafangCmdFun_f   pfnSendFun;       //需要调用的发送命令函数
    uint16_t                    delayTime;        //延时时间，单位ms
}UartSendPtlBafangCmdCtrl_t;


/*******************************************************************************
 * CONSTANTS
 */

/*******************************************************************************
 * LOCAL FUNCTIONS DECLEAR
 */
static void com_uart_ptl_bafang_tx_process(void);

//static bool bafang_send_handler(ptl_frame_type_t frame_type, uint16_t param1, uint16_t param2, ptl_proc_buff_t *buff);
static bool bafang_receive_handler(ptl_2_proc_buff_t* ptl_2_proc_buff);

//static bool com_uart_ptl_bafang_receive_handler(ptl_frame_type_t frame_type, uint16_t param1, uint16_t param2, ptl_proc_buff_t *buff);
//static bool com_uart_ptl_bafang_send_handler(ptl_frame_payload_t *payload, ptl_proc_buff_t *ackbuff);

//该标志符号是否需要等待响应数据
static bool uart_protocol_symbol_need_response(uint16_t symbol);
//发送读取系统状态命令
static void uart_send_protocol_cmd_system_state(void);
//发送读取工作状态命令
static void uart_send_protocol_cmd_working_state(void);
//发送读取瞬时电流命令
static void uart_send_protocol_cmd_current(void);
//发送读取电池电量命令
static void uart_send_protocol_cmd_soc(void);
//发送读取速度命令
static void uart_send_protocol_cmd_speed(void);
//发送写入限速命令
static void uart_send_protocol_cmd_speed_limit(void);
//发送写入档位命令
static void uart_send_protocol_cmd_gear(void);
//发送写入大灯命令
static void uart_send_protocol_cmd_lamp(void);
//发送读取电池命令
static void uart_send_protocol_cmd_battery_info(void);
//发送读取电芯命令
static void uart_send_protocol_cmd_cell_info(void);


static void bike_Uart_Send(unsigned char data);

static void Bike_pas_level_send_depend_max_9_level(void);
static void Bike_pas_level_send_depend_max_5_level(void);
static void Bike_pas_level_send_depend_max_3_level(void);

//系统状态协议帧处理
static bool proc_protocol_frame_system_state(uint8_t* buff, int count);
//工作状态协议帧处理
static bool proc_protocol_frame_working_state(uint8_t* buff, int count);
//电池电量协议帧处理
static bool proc_protocol_frame_soc(uint8_t* buff, int count);
//速度协议帧处理
static bool proc_protocol_frame_speed(uint8_t* buff, int count);
//瞬时电流协议帧处理
static bool proc_protocol_frame_instantaneous_current(uint8_t* buff, int count);
//电池信息协议帧处理
static bool proc_protocol_frame_battery_info(uint8_t* buff, int count);
//电芯信息协议帧处理
static bool proc_protocol_frame_cell_info(uint8_t* buff, int count);

/*******************************************************************************
 * GLOBAL VARIABLES
 */


//METER_INFO_T        theMeterInfo;       
//INDICATOR_FLAG_T    theIndicatorFlag;   
//SETTING_INFO_T      theSettingInfo;     
//ERROR_INFO_T        theErrorInfo;       
//BATTERY_INFO_T      theBatteryInfo;     
//ENV_INFO_T          theEnvInfo;         
/*******************************************************************************
 * STATIC VARIABLES
 */
static uint32_t lt_timer;

//骑行辅助等级
//档位命令：
static const uint8_t protocol_cmd_pas_gear_00[4] = { 0x16, 0x0B, 0x00, 0x21 }; //gear 0
static const uint8_t protocol_cmd_pas_gear_01[4] = { 0x16, 0x0B, 0x01, 0x22 }; //gear 1
static const uint8_t protocol_cmd_pas_gear_02[4] = { 0x16, 0x0B, 0x02, 0x23 }; //gear 2
static const uint8_t protocol_cmd_pas_gear_03[4] = { 0x16, 0x0B, 0x03, 0x24 }; //gear 3
static const uint8_t protocol_cmd_pas_gear_06[4] = { 0x16, 0x0B, 0x06, 0x27 }; //gear 6
//static const uint8_t protocol_cmd_pas_gear_10[4] = { 0x16, 0x0B, 0x0A, 0x2B }; //gear 10
static const uint8_t protocol_cmd_pas_gear_11[4] = { 0x16, 0x0B, 0x0B, 0x2C }; //gear 11
static const uint8_t protocol_cmd_pas_gear_12[4] = { 0x16, 0x0B, 0x0C, 0x2D }; //gear 12
static const uint8_t protocol_cmd_pas_gear_13[4] = { 0x16, 0x0B, 0x0D, 0x2E }; //gear 13
static const uint8_t protocol_cmd_pas_gear_21[4] = { 0x16, 0x0B, 0x15, 0x36 }; //gear 21
static const uint8_t protocol_cmd_pas_gear_22[4] = { 0x16, 0x0B, 0x16, 0x37 }; //gear 22
static const uint8_t protocol_cmd_pas_gear_23[4] = { 0x16, 0x0B, 0x17, 0x38 }; //gear 23


//大灯指示灯
static const uint8_t protocol_cmd_lamp_on[3] = { 0x16, 0x1A, 0xF1 };   //开灯
static const uint8_t protocol_cmd_lamp_off[3] = { 0x16, 0x1A, 0xF0 };  //关灯

//电池信息
static const uint8_t protocol_cmd_battery_info[3] = { 0x11, 0x60, 0x71 };   //电池信息
static const uint8_t protocol_cmd_cell_info[3] = { 0x11, 0x61, 0x72 };      //电芯信息


static uint32_t protocolResponseLose = 0;                                       //协议通讯异常次数
static PTL_BAFANG_CMD_SYMBOL protocolCmdSymbol = PTL_BAFANG_CMD_SYMBOL_IDLE;    //发送命令的标识符
static PTL_BAFANG_CMD_SYMBOL protocolResponse = PTL_BAFANG_CMD_SYMBOL_IDLE;     //接收命令的标识符

static size_t protocolProcessCurrentIndex = 0;                                  //当前命令的序号
static uint32_t protocolProcessTimer = 0;                                      //当前命令发送时间

static PTL_BAFANG_PROCESS_STATE protocolProcessState = PTL_BAFANG_PROCESS_STATE_INIT;
static const UartSendPtlBafangCmdCtrl_t protocolProcessCmdTable[] =
{
    {PTL_BAFANG_CMD_SYMBOL_SYSTEM_STATE,  uart_send_protocol_cmd_system_state,   100},  //11 08 
    {PTL_BAFANG_CMD_SYMBOL_CURRENT,       uart_send_protocol_cmd_current,        100},  //11 0A
    {PTL_BAFANG_CMD_SYMBOL_SPEED,         uart_send_protocol_cmd_speed,          100},  //11 20
    {PTL_BAFANG_CMD_SYMBOL_SOC,           uart_send_protocol_cmd_soc,            100},  //11 11
    {PTL_BAFANG_CMD_SYMBOL_WORKING_STATE, uart_send_protocol_cmd_working_state,  100},  //11 31
    {PTL_BAFANG_CMD_SYMBOL_LAMP,          uart_send_protocol_cmd_lamp,           100},  //16 1A

    {PTL_BAFANG_CMD_SYMBOL_SYSTEM_STATE,  uart_send_protocol_cmd_system_state,   100},  //11 08 
    {PTL_BAFANG_CMD_SYMBOL_CURRENT,       uart_send_protocol_cmd_current,        100},  //11 0A
    {PTL_BAFANG_CMD_SYMBOL_SPEED,         uart_send_protocol_cmd_speed,          100},  //11 20
    {PTL_BAFANG_CMD_SYMBOL_SOC,           uart_send_protocol_cmd_soc,            100},  //11 11
    {PTL_BAFANG_CMD_SYMBOL_WORKING_STATE, uart_send_protocol_cmd_working_state,  100},  //11 31
    {PTL_BAFANG_CMD_SYMBOL_SPEED_LIMIT,   uart_send_protocol_cmd_speed_limit,    100},  //16 1F

    {PTL_BAFANG_CMD_SYMBOL_SYSTEM_STATE,  uart_send_protocol_cmd_system_state,   100},  //11 08 
    {PTL_BAFANG_CMD_SYMBOL_CURRENT,       uart_send_protocol_cmd_current,        100},  //11 0A
    {PTL_BAFANG_CMD_SYMBOL_SPEED,         uart_send_protocol_cmd_speed,          100},  //11 20
    {PTL_BAFANG_CMD_SYMBOL_SOC,           uart_send_protocol_cmd_soc,            100},  //11 11
    {PTL_BAFANG_CMD_SYMBOL_WORKING_STATE, uart_send_protocol_cmd_working_state,  100},  //11 31
    {PTL_BAFANG_CMD_SYMBOL_GEAR,          uart_send_protocol_cmd_gear,           100},  //16 0B

    {PTL_BAFANG_CMD_SYMBOL_SYSTEM_STATE,  uart_send_protocol_cmd_system_state,   100},  //11 08 
    {PTL_BAFANG_CMD_SYMBOL_CURRENT,       uart_send_protocol_cmd_current,        100},  //11 0A
    {PTL_BAFANG_CMD_SYMBOL_SPEED,         uart_send_protocol_cmd_speed,          100},  //11 20
    {PTL_BAFANG_CMD_SYMBOL_SOC,           uart_send_protocol_cmd_soc,            100},  //11 11
    {PTL_BAFANG_CMD_SYMBOL_WORKING_STATE, uart_send_protocol_cmd_working_state,  100},  //11 31

	  {PTL_BAFANG_CMD_SYMBOL_BATTERY,       uart_send_protocol_cmd_battery_info,   100},  //11 60 71
	  {PTL_BAFANG_CMD_SYMBOL_CELL,          uart_send_protocol_cmd_cell_info,      100},  //11 60 72

};

/*******************************************************************************
 * EXTERNAL VARIABLES
 */


/*******************************************************************************
 *  GLOBAL FUNCTIONS IMPLEMENTATION
 */
void app_bafang_ptl_init_running(void)
{
    OTMS(TASK_ID_PTL_BAFANG, OTMS_S_INVALID);
	  LOG_LEVEL("app_bafang_ptl_init_running\r\n");
}

void app_bafang_ptl_start_running(void)
{
	  LOG_LEVEL("app_bafang_ptl_start_running\r\n");
    //只有是八方协议的情况下才运行该状态机
    //if (SETTING_PTL_BAFANG == theEnvInfo.ptl) 
		{
      ///ptl_register_module(SETTING_PTL_BAFANG, bafang_send_handler,bafang_receive_handler);
			ptl_2_register_module(SETTING_PTL_BAFANG, bafang_receive_handler);
      OTMS(TASK_ID_PTL_BAFANG, OTMS_S_ASSERT_RUN);
    }
}

void app_bafang_ptl_assert_running(void)
{
    StartTickCounter(&lt_timer);
    OTMS(TASK_ID_PTL_BAFANG, OTMS_S_RUNNING);
}

void app_bafang_ptl_running(void)
{
    if (GetTickCounter(&lt_timer) < 10)
    {
        return;
    }
    StartTickCounter(&lt_timer);

    //if(true == system_get_power_off_req())
    {
        OTMS(TASK_ID_PTL_BAFANG, OTMS_S_POST_RUN);
    }

     //lt_indicator.walk_assist = theMeterInfo.walk_assist;
     com_uart_ptl_bafang_tx_process();
}

void app_bafang_ptl_post_running(void)
{
    //printf("%s\n", __FUNCTION__);
    //if(true != system_get_power_off_req())
    {
        OTMS(TASK_ID_PTL_BAFANG, OTMS_S_ASSERT_RUN);
    }
}


void app_bafang_ptl_stop_running(void)
{
    printf("%s\n", __FUNCTION__);
    OTMS(TASK_ID_PTL_BAFANG, OTMS_S_INVALID);
}

/*******************************************************************************
 * LOCAL FUNCTIONS IMPLEMENTATION
 */


void com_uart_ptl_bafang_tx_process(void)
{
    size_t tableLen = (sizeof(protocolProcessCmdTable) / sizeof(UartSendPtlBafangCmdCtrl_t));
    ///assert((protocolProcessCurrentIndex >= 0) && (protocolProcessCurrentIndex < tableLen));

    switch (protocolProcessState)
    {
    case PTL_BAFANG_PROCESS_STATE_INIT:
    {
        ///if (power_is_power_on())
        {
            protocolProcessState = PTL_BAFANG_PROCESS_STATE_SEND_CMD;
            StartTickCounter(&protocolProcessTimer);
        }
        break;
    }
    case PTL_BAFANG_PROCESS_STATE_SEND_CMD:
    {
        ////rec_buff_offset = 0;
        ////com_uart_ptl_clear_revice_buff();
        protocolProcessCmdTable[protocolProcessCurrentIndex].pfnSendFun();
        protocolCmdSymbol =(PTL_BAFANG_CMD_SYMBOL)protocolProcessCmdTable[protocolProcessCurrentIndex].cmdSymbol;
        StartTickCounter(&protocolProcessTimer);

        protocolProcessState = PTL_BAFANG_PROCESS_STATE_WAIT_TIME;
        break;
    }
    case PTL_BAFANG_PROCESS_STATE_WAIT_TIME:
    {
        uint32_t diff = GetTickCounter(&protocolProcessTimer);
        uint32_t wait = protocolProcessCmdTable[protocolProcessCurrentIndex].delayTime;
        if (diff >= wait)
        {
            protocolProcessState = PTL_BAFANG_PROCESS_STATE_CHECK_RESPONSE;
        }
        break;
    }
    case PTL_BAFANG_PROCESS_STATE_CHECK_RESPONSE:
    {
        if (uart_protocol_symbol_need_response(protocolProcessCmdTable[protocolProcessCurrentIndex].cmdSymbol))
        {
            if (protocolResponse == protocolProcessCmdTable[protocolProcessCurrentIndex].cmdSymbol)
            {
                protocolResponse = PTL_BAFANG_CMD_SYMBOL_IDLE;
                protocolProcessState = PTL_BAFANG_PROCESS_STATE_NEXT_CMD;
            }
            else
            {
                uint32_t diff = GetTickCounter(&protocolProcessTimer);
                uint32_t wait = protocolProcessCmdTable[protocolProcessCurrentIndex].delayTime * 30;
                if (diff >= wait)
                {
                    protocolResponseLose++;
                    if (protocolResponseLose == 3)
                    {
                        //TODO
                        //add_error_code;
                        ///printf("add_error_code\n");
                        ///add_error_code(ERROR_CODE_COMMUNICATION_ABNORMALITY);

                    }
                    protocolProcessState = PTL_BAFANG_PROCESS_STATE_NEXT_CMD;
                }
            }
        }
        else
        {
            protocolProcessState = PTL_BAFANG_PROCESS_STATE_NEXT_CMD;
        }
        break;
    }
    case PTL_BAFANG_PROCESS_STATE_NEXT_CMD:
    {
        protocolProcessCurrentIndex++;
        if (protocolProcessCurrentIndex >= tableLen)
        {
            protocolProcessCurrentIndex = 0;
        }
        ///if (power_is_power_on())
        {
            protocolProcessState = PTL_BAFANG_PROCESS_STATE_SEND_CMD;
        }
        ///else
        ///{
        ///    protocolProcessState = PTL_BAFANG_PROCESS_STATE_DEINIT;
        ///}
        break;
    }
    case PTL_BAFANG_PROCESS_STATE_DEINIT:
    {
        break;
    }
    default:
        break;
    }
}

static bool bafang_receive_handler(ptl_2_proc_buff_t* ptl_2_proc_buff)
{
    bool res = false;
    if (res == false)
    {
        //系统状态协议帧处理
        res = proc_protocol_frame_system_state(ptl_2_proc_buff->buffer, ptl_2_proc_buff->size);
    }
    if (res == false)
    {
        //工作状态协议帧处理
        res = proc_protocol_frame_working_state(ptl_2_proc_buff->buffer, ptl_2_proc_buff->size);
    }
    if (res == false)
    {
        //电池电量协议帧处理
        res = proc_protocol_frame_soc(ptl_2_proc_buff->buffer, ptl_2_proc_buff->size);
    }
    if (res == false)
    {
        //速度协议帧处理
        res = proc_protocol_frame_speed(ptl_2_proc_buff->buffer, ptl_2_proc_buff->size);
    }
    if (res == false)
    {
        //瞬时电流协议帧处理
        res = proc_protocol_frame_instantaneous_current(ptl_2_proc_buff->buffer, ptl_2_proc_buff->size);
    }
    if (res == false)
    {
        //电池信息协议帧处理
        res = proc_protocol_frame_battery_info(ptl_2_proc_buff->buffer, ptl_2_proc_buff->size);
    }
    if (res == false)
    {
        //电芯信息协议帧处理
        res = proc_protocol_frame_cell_info(ptl_2_proc_buff->buffer, ptl_2_proc_buff->size);
    }

    if (res)
    {
        protocolResponse = protocolCmdSymbol;
        protocolCmdSymbol = PTL_BAFANG_CMD_SYMBOL_IDLE;
    }

    return res;
}

bool uart_protocol_symbol_need_response(uint16_t symbol)
{
    return ((symbol & 0xFF00) == 0x1100);
}

//发送读取系统状态命令
void uart_send_protocol_cmd_system_state(void)
{
    bike_Uart_Send(0x08);
}

//发送读取工作状态命令
void uart_send_protocol_cmd_working_state(void)
{
    bike_Uart_Send(0x31);
}

//发送读取瞬时电流命令
void uart_send_protocol_cmd_current(void)
{
    bike_Uart_Send(0x0A);
}

//发送读取电池电量命令
void uart_send_protocol_cmd_soc(void)
{
    bike_Uart_Send(0x11);
}

//发送读取速度命令
void uart_send_protocol_cmd_speed(void)
{
    bike_Uart_Send(0x20);
}

//发送写入限速命令
void uart_send_protocol_cmd_speed_limit(void)
{
    static uint16_t speed_limit_rpm = 241;
    static uint32_t speed_limit = 0;

    if (speed_limit != lt_carinfo_meter.speed_limit)
    {
        speed_limit = lt_carinfo_meter.speed_limit;

        double radius = get_wheel_radius_mm() / 1000.0; //轮毂半径，单位：米
        if (radius)
        {
            double kph = speed_limit / 10.0;                //限速,单位km/h
            double v = kph * 1000.0 / 3600.0;               //线速度,单位：米/秒
            double w = v / radius;                          //转换角速度，单位：弧度/秒
            double rpm = w * 60.0 / 2.0 / PI_FLOAT;         //转速rpm
            speed_limit_rpm = (uint16_t)rpm;
        }
    }

    uint8_t word_h = ((speed_limit_rpm >> 8) & 0xFF);
    uint8_t word_l = (speed_limit_rpm & 0xFF);

    uint8_t send_data[5];
    send_data[0] = 0x16;
    send_data[1] = 0x1F;
    send_data[2] = word_h;
    send_data[3] = word_l;
    send_data[4] = send_data[0] + send_data[1] + send_data[2] + send_data[3];
    hal_com_uart_send_buffer_2(send_data, 5);
}

//发送写入档位命令
void uart_send_protocol_cmd_gear(void)
{
    if (lt_carinfo_indicator.walk_assist)
    {
        hal_com_uart_send_buffer_2(protocol_cmd_pas_gear_06, 4);
    }
    else if (lt_carinfo_meter.max_gear_level == SETTING_MAX_PAS_3_LEVEL)
    {
        Bike_pas_level_send_depend_max_3_level();
    }
    else if (lt_carinfo_meter.max_gear_level == SETTING_MAX_PAS_5_LEVEL)
    {
        Bike_pas_level_send_depend_max_5_level();
    }
    else if (lt_carinfo_meter.max_gear_level == SETTING_MAX_PAS_9_LEVEL)
    {
        Bike_pas_level_send_depend_max_9_level();
    }
		else
		{
			 Bike_pas_level_send_depend_max_5_level();
		}
}

//发送写入大灯命令
void uart_send_protocol_cmd_lamp(void)
{
    //if (theIndicatorFlag.lamp)
	  if(lt_carinfo_indicator.highBeam > 0 )
    {
        hal_com_uart_send_buffer_2(protocol_cmd_lamp_on, 3);
    }
    else
    {
        hal_com_uart_send_buffer_2(protocol_cmd_lamp_off, 3);
    }
}

//发送读取电池命令
void uart_send_protocol_cmd_battery_info(void)
{
    hal_com_uart_send_buffer_2(protocol_cmd_battery_info, 3);
}

//发送读取电芯命令
void uart_send_protocol_cmd_cell_info(void)
{
    hal_com_uart_send_buffer_2(protocol_cmd_cell_info, 3);
}

//0x11 x? 
void bike_Uart_Send(unsigned char data)
{
    uint8_t send_data[2];

    send_data[0] = 0x11;
    send_data[1] = data;
    hal_com_uart_send_buffer_2(send_data, 2);

    if (data == 0x20)
    {
        //printf("bike_Uart_Send data: %02x %02x tick:%u\n", send_data[0], send_data[1], SDL_GetTicks());
    }
}

void Bike_pas_level_send_depend_max_9_level(void)
{
    switch (lt_carinfo_meter.gear)
    {
    case 0:  hal_com_uart_send_buffer_2(protocol_cmd_pas_gear_00, 4); return;
    case 1:  hal_com_uart_send_buffer_2(protocol_cmd_pas_gear_01, 4); return;
    case 2:  hal_com_uart_send_buffer_2(protocol_cmd_pas_gear_11, 4); return;
    case 3:  hal_com_uart_send_buffer_2(protocol_cmd_pas_gear_12, 4); return;
    case 4:  hal_com_uart_send_buffer_2(protocol_cmd_pas_gear_13, 4); return;
    case 5:  hal_com_uart_send_buffer_2(protocol_cmd_pas_gear_02, 4); return;
    case 6:  hal_com_uart_send_buffer_2(protocol_cmd_pas_gear_21, 4); return;
    case 7:  hal_com_uart_send_buffer_2(protocol_cmd_pas_gear_22, 4); return;
    case 8:  hal_com_uart_send_buffer_2(protocol_cmd_pas_gear_23, 4); return;
    case 9:  hal_com_uart_send_buffer_2(protocol_cmd_pas_gear_03, 4); return;
    default: hal_com_uart_send_buffer_2(protocol_cmd_pas_gear_01, 4); return;
    }
}

void Bike_pas_level_send_depend_max_5_level(void)
{
    switch (lt_carinfo_meter.gear)
    {
    case 0:  hal_com_uart_send_buffer_2(protocol_cmd_pas_gear_00, 4); return;
    case 1:  hal_com_uart_send_buffer_2(protocol_cmd_pas_gear_11, 4); return;
    case 2:  hal_com_uart_send_buffer_2(protocol_cmd_pas_gear_13, 4); return;
    case 3:  hal_com_uart_send_buffer_2(protocol_cmd_pas_gear_21, 4); return;
    case 4:  hal_com_uart_send_buffer_2(protocol_cmd_pas_gear_23, 4); return;
    case 5:  hal_com_uart_send_buffer_2(protocol_cmd_pas_gear_03, 4); return;
    default: hal_com_uart_send_buffer_2(protocol_cmd_pas_gear_11, 4); return;
    }
}

void Bike_pas_level_send_depend_max_3_level(void)
{
   switch (lt_carinfo_meter.gear)
    {
    case 0:  hal_com_uart_send_buffer_2(protocol_cmd_pas_gear_00, 4); return;
    case 1:  hal_com_uart_send_buffer_2(protocol_cmd_pas_gear_12, 4); return;
    case 2:  hal_com_uart_send_buffer_2(protocol_cmd_pas_gear_02, 4); return;
    case 3:  hal_com_uart_send_buffer_2(protocol_cmd_pas_gear_03, 4); return;
    default: hal_com_uart_send_buffer_2(protocol_cmd_pas_gear_12, 4); return;
    }
}

//系统状态协议帧处理
bool proc_protocol_frame_system_state(uint8_t* buff, int count)
{
    if (protocolCmdSymbol == PTL_BAFANG_CMD_SYMBOL_SYSTEM_STATE)//??
    {
        if (count == 1)
        {
            ERROR_CODE code = (ERROR_CODE)buff[0];
					   lt_carinfo_indicator.brake = false;
            if (code == ERROR_CODE_BRAKE)
            {
                lt_carinfo_indicator.brake = true;
							  send_message(TASK_ID_CAR_INFOR, MCU_TO_SOC_MOD_CARINFOR, CMD_MOD_CARINFOR_INDICATOR, CMD_MOD_CARINFOR_INDICATOR);//CMD_MOD_CARINFOR_INDICATOR	
            }
            else if (code >= ERROR_CODE_BEGIN && code <= ERROR_CODE_END)
            {
                app_carinfo_add_error_code(code);
							  send_message(TASK_ID_CAR_INFOR, MCU_TO_SOC_MOD_CARINFOR, CMD_MOD_CARINFOR_INDICATOR, CMD_MOD_CARINFOR_INDICATOR);	
							  send_message(TASK_ID_CAR_INFOR, MCU_TO_SOC_MOD_CARINFOR, CMD_MOD_CARINFOR_ERROR, CMD_MOD_CARINFOR_ERROR);	//CMD_MOD_CARINFOR_ERROR
            }
						else
						{
								//send_message(TASK_ID_CAR_INFOR, MCU_TO_SOC_MOD_CARINFOR, CMD_MOD_CARINFOR_INDICATOR, CMD_MOD_CARINFOR_INDICATOR);	
						}
					
            return true;
        }
    }
    return false;
}

//工作状态协议帧处理
bool proc_protocol_frame_working_state(uint8_t* buff, int count)
{
    if (protocolCmdSymbol == PTL_BAFANG_CMD_SYMBOL_WORKING_STATE)
    {
        if (count == 2)
        {
            return true;
        }
    }
    return false;
}
//速度协议帧处理
bool proc_protocol_frame_speed(uint8_t* buff, int count)
{
    if (protocolCmdSymbol == PTL_BAFANG_CMD_SYMBOL_SPEED)//speed	 速度
    {
        if (count == 3)
        {
            //printf("proc_protocol_frame_speed count:%d  buff: %02x %02x %02x %02x  tick:%u\n", count, buff[0], buff[1], buff[2], buff[3], SDL_GetTicks());
            uint8_t checksum = buff[0] + buff[1] + 0x20;
            if (checksum == buff[2])
            {
							//角速度计算公式，物理量字母ω 单位 rad/s (弧度/秒)
							//ω = rpm * 2π / 60
							//线速度，物理量字母V 单位 m/s
							//v（线速度） = ω* r

							uint16_t rpm = MK_WORD(buff[0], buff[1]);
							double radius = get_wheel_radius_mm() / 1000.0; //轮毂半径，单位：米
							double w = rpm * (2.0 * PI_FLOAT / 60.0);//转换角速度，单位：弧度/秒
							double v = w * radius; //线速度,单位：米/秒
							double kph = v * 3600.0 / 1000.0 * 10;

							//theMeterInfo.rpm = (uint32_t)rpm;
							//theMeterInfo.speed = (uint32_t)kph;
							lt_carinfo_meter.rpm = (uint16_t)rpm;
							lt_carinfo_meter.speed_actual = (uint16_t)kph;
							lt_carinfo_meter.speed = (uint16_t)kph; 
							//LOG_LEVEL("lt_meter.speed=%d\r\n",lt_meter.speed);
							send_message(TASK_ID_CAR_INFOR, MCU_TO_SOC_MOD_CARINFOR, CMD_MOD_CARINFOR_METER, 0);	 
            }
            return true;
        }
    }
    return false;
}
//电池电量协议帧处理
bool proc_protocol_frame_soc(uint8_t* buff, int count)
{
    if (protocolCmdSymbol == PTL_BAFANG_CMD_SYMBOL_SOC)//battery电池容量
    {
        if (count == 2)
        {
            uint8_t checksum = buff[0];
            if (checksum == buff[1])
            {
                uint8_t soc = buff[0];

                lt_carinfo_battery.soc = soc;
                lt_carinfo_battery.range = (lt_carinfo_battery.max_range * soc) / 100;
							  send_message(TASK_ID_CAR_INFOR, MCU_TO_SOC_MOD_CARINFOR,CMD_MOD_CARINFOR_BATTERY, 0);
            }
            return true;
        }
    }
    return false;
}

//瞬时电流协议帧处理
bool proc_protocol_frame_instantaneous_current(uint8_t* buff, int count)
{
    if (protocolCmdSymbol == PTL_BAFANG_CMD_SYMBOL_CURRENT)//瞬时电流
    {
        if (count == 2)
        {
            uint8_t checksum = buff[0];
            if (checksum == buff[1])
            {
                uint8_t current_val = buff[0];
                lt_carinfo_battery.current = current_val * 5;

                lt_carinfo_battery.power = lt_carinfo_battery.voltage * lt_carinfo_battery.current / 100;
								send_message(TASK_ID_CAR_INFOR, MCU_TO_SOC_MOD_CARINFOR, CMD_MOD_CARINFOR_BATTERY, 0);	
            }
            return true;
        }
    }
    return false;
}

//电池信息协议帧处理
bool proc_protocol_frame_battery_info(uint8_t* buff, int count)
{
    if (protocolCmdSymbol == PTL_BAFANG_CMD_SYMBOL_BATTERY)//读取电池信息
    {
        if (count == 27)
        {
            uint8_t checkSum = 0;
            for (int i = 0; i < 26; i++) //
            {
                checkSum += buff[i];
            }

            if (checkSum == buff[26])
            {
                ///uint16_t temp = MK_WORD(buff[1], buff[0]);
                ///uint16_t maxTemp = MK_WORD(buff[3], buff[2]);
                ///uint16_t minTempt = MK_WORD(buff[5], buff[4]);

                uint16_t volt = MK_WORD(buff[7], buff[6]);

                uint16_t cur = MK_WORD(buff[9], buff[8]);
                ///uint16_t avgCur = MK_WORD(buff[11], buff[10]);

                ///uint16_t resCap = MK_WORD(buff[13], buff[12]);
                ///uint16_t fulCap = MK_WORD(buff[15], buff[14]);

                uint8_t relateChargeState = buff[16];
                uint8_t absoluteChargeState = buff[17];

                ///uint16_t cycle = MK_WORD(buff[19], buff[18]);

                ///uint16_t maxUnchargeHour = MK_WORD(buff[21], buff[20]);
                ///uint8_t maxUnchargeMintues = buff[22];
                ///uint16_t lastUnchargeHour = MK_WORD(buff[24], buff[23]);
                ///uint8_t lastUnchargeMintues = buff[25];

                ///lt_carinfo_battery.temp = MK_SIG_WORD(temp);
                ///lt_carinfo_battery.max_temp = MK_SIG_WORD(maxTemp);
                ///lt_carinfo_battery.min_temp = MK_SIG_WORD(minTempt);


                lt_carinfo_battery.voltage = volt * 25 / 10;

                lt_carinfo_battery.current = MK_SIG_WORD(cur) * 2;
                ///lt_carinfo_battery.avg_current = MK_SIG_WORD(avgCur) * 2;

                ///lt_carinfo_battery.res_cap = resCap * 2;
                ///lt_carinfo_battery.full_cap = fulCap * 2;

                lt_carinfo_battery.rel_charge_state = relateChargeState;
                lt_carinfo_battery.abs_charge_state = absoluteChargeState;

                ///lt_carinfo_battery.cycle_times = cycle;

                ///lt_carinfo_battery.max_uncharge_time = (int32_t)maxUnchargeHour * 60 + (int32_t)maxUnchargeMintues;
                ///lt_carinfo_battery.last_uncharge_time = (int32_t)lastUnchargeHour * 60 + (int32_t)lastUnchargeMintues;
								send_message(TASK_ID_CAR_INFOR, MCU_TO_SOC_MOD_CARINFOR, CMD_MOD_CARINFOR_BATTERY, 0);	
            }
            return true;
        }
    }
    return false;
}

//电芯信息协议帧处理
bool proc_protocol_frame_cell_info(uint8_t* buff, int count)
{
    if (protocolCmdSymbol == PTL_BAFANG_CMD_SYMBOL_CELL)//读取电芯信息
    {
        uint8_t cellNo = 0;
        uint8_t frameLen = 0xFF;
        if (count > 0)
        {
            cellNo = buff[0];
            frameLen = (cellNo + 1) * 2;
        }
        if (count == frameLen)
        {
            uint8_t checkSum = 0;
            for (int i = 0; i < frameLen - 1; i++) //
            {
                checkSum += buff[i];
            }

            ///lt_carinfo_battery.total_cell = buff[0];
            for (int i = 0; i < cellNo; i++)
            {
                ///lt_carinfo_battery.cell_voltage[i] = MK_WORD(buff[i * 2 + 2], buff[i * 2 + 1]);
            }
						send_message(TASK_ID_CAR_INFOR, MCU_TO_SOC_MOD_CARINFOR, CMD_MOD_CARINFOR_BATTERY, 0);
            return true;
        }
    }
    return false;
}

uint16_t get_wheel_radius_mm(void)
{
	  lt_carinfo_meter.wheel_diameter = SETTING_WHEEL_27_Inch;
    switch (lt_carinfo_meter.wheel_diameter)
    {
    case SETTING_WHEEL_16_Inch:     return 203;
    case SETTING_WHEEL_18_Inch:     return 229;
    case SETTING_WHEEL_20_Inch:     return 254;
    case SETTING_WHEEL_22_Inch:     return 279;
    case SETTING_WHEEL_24_Inch:     return 305;
    case SETTING_WHEEL_26_Inch:     return 330;
    case SETTING_WHEEL_27_Inch:     return 343;
    case SETTING_WHEEL_27_5_Inch:   return 349;
    case SETTING_WHEEL_28_Inch:     return 356;
    case SETTING_WHEEL_29_Inch:     return 368;
    }
    return 330;
}

uint16_t get_wheel_radius_inch(void)
{
	  lt_carinfo_meter.wheel_diameter = SETTING_WHEEL_27_Inch;
    switch (lt_carinfo_meter.wheel_diameter)
    {
    case SETTING_WHEEL_16_Inch:     return 160;
    case SETTING_WHEEL_18_Inch:     return 180;
    case SETTING_WHEEL_20_Inch:     return 200;
    case SETTING_WHEEL_22_Inch:     return 220;
    case SETTING_WHEEL_24_Inch:     return 240;
    case SETTING_WHEEL_26_Inch:     return 260;
    case SETTING_WHEEL_27_Inch:     return 270;
    case SETTING_WHEEL_27_5_Inch:   return 275;
    case SETTING_WHEEL_28_Inch:     return 280;
    case SETTING_WHEEL_29_Inch:     return 290;
    }
    return 260;
}
///////////////////////////////////////////////////////////////////////////////////
void bafang_lamp_on_off(bool on_off)
{
	if(on_off)
	{
		  lt_carinfo_indicator.highBeam = 1;
			hal_com_uart_send_buffer_2(protocol_cmd_lamp_on,sizeof(protocol_cmd_lamp_on));
	}
	else
	{
		  lt_carinfo_indicator.highBeam = 0;
			hal_com_uart_send_buffer_2(protocol_cmd_lamp_off,sizeof(protocol_cmd_lamp_off));
	}
}

void bafang_set_gear(uint8_t level)
{
	  lt_carinfo_meter.gear= level;
	  if (lt_carinfo_indicator.walk_assist)
    {
			 LOG_LEVEL("lt_indicator.walk_assist=%d\r\n",lt_carinfo_indicator.walk_assist);
       hal_com_uart_send_buffer_2(protocol_cmd_pas_gear_06, 4);
    }
    else if (lt_carinfo_meter.max_gear_level == SETTING_MAX_PAS_3_LEVEL)
    {
			 LOG_LEVEL("SETTING_MAX_PAS_3_LEVEL:%d\r\n",lt_carinfo_meter.gear);
       Bike_pas_level_send_depend_max_3_level();
    }
    else if (lt_carinfo_meter.max_gear_level == SETTING_MAX_PAS_5_LEVEL)
    {
			 LOG_LEVEL("SETTING_MAX_PAS_5_LEVEL:%d\r\n",lt_carinfo_meter.gear);
       Bike_pas_level_send_depend_max_5_level();
    }
    else if (lt_carinfo_meter.max_gear_level == SETTING_MAX_PAS_9_LEVEL)
    {
			 LOG_LEVEL("SETTING_MAX_PAS_9_LEVEL:%d\r\n",lt_carinfo_meter.gear);
       Bike_pas_level_send_depend_max_9_level();
    }
		else
		{
			 LOG_LEVEL("SETTING_MAX_PAS_5_LEVEL:%d\r\n",lt_carinfo_meter.gear);
			 Bike_pas_level_send_depend_max_5_level();
		}
}

