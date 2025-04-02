/*******************************************************************************
 * FILE: octopus_car_controller.c
 *
 * DESCRIPTION:
 * This file contains the implementation of the car controller application
 * for the Octopus platform. It includes functions for handling communication
 * between various car system modules, such as the meter, indicator, and
 * drivetrain information. It also includes battery voltage retrieval and
 * system information frame (SIF) updates.
 *
 * MODULES:
 * - Meter module
 * - Indicator module
 * - Drivetrain info module
 * - Car controller message processing
 * - Battery voltage retrieval
 *
 * NOTE: This file is part of the Octopus car control system and interfaces
 * with the Octopus platform to manage the car's state and communication with
 * external modules.
 *
 * File Name: octopus_task_manager_platform.h
 * @version  1.0.0
 * @date     2024-12-11
 * @author   Octopus Team
 *
 ******************************************************************************/

/*******************************************************************************
 * INCLUDES
 */
#include "octopus_platform.h"     // Include platform-specific header for hardware platform details
#include "octopus_log.h"          // Include logging functions for debugging
#include "octopus_task_manager.h" // Include task manager for scheduling tasks
#include "octopus_tickcounter.h"
#include "octopus_carinfor.h"
#include "octopus_msgqueue.h"
#include "octopus_sif.h"

/*******************************************************************************
 * DEBUG SWITCH MACROS
 */
#define CARINFOR_PTL_NO_ACK
/// #define TEST_LOG_DEBUG_SIF  // Uncomment to enable debug logging for SIF module

/*******************************************************************************
 * MACROS
 */
#define CELL_VOL_20 (1058) // Voltage corresponding to 20% battery charge
#define CELL_VOL_30 (1076) // Voltage corresponding to 30% battery charge
#define CELL_VOL_40 (1100) // Voltage corresponding to 40% battery charge
#define CELL_VOL_50 (1120) // Voltage corresponding to 50% battery charge
#define CELL_VOL_60 (1142) // Voltage corresponding to 60% battery charge
#define CELL_VOL_70 (1164) // Voltage corresponding to 70% battery charge
#define CELL_VOL_80 (1184) // Voltage corresponding to 80% battery charge
#define CELL_VOL_90 (1206) // Voltage corresponding to 90% battery charge

/*******************************************************************************
 * TYPEDEFS
 */

/*******************************************************************************
 * CONSTANTS
 */

/*******************************************************************************
 * LOCAL FUNCTIONS DECLARATION
 */
static bool meter_module_send_handler(ptl_frame_type_t frame_type, uint16_t param1, uint16_t param2, ptl_proc_buff_t *buff);
static bool meter_module_receive_handler(ptl_frame_payload_t *payload, ptl_proc_buff_t *ackbuff);

static bool indicator_module_send_handler(ptl_frame_type_t frame_type, uint16_t param1, uint16_t param2, ptl_proc_buff_t *buff);
static bool indicator_module_receive_handler(ptl_frame_payload_t *payload, ptl_proc_buff_t *ackbuff);

static bool drivinfo_module_send_handler(ptl_frame_type_t frame_type, uint16_t param1, uint16_t param2, ptl_proc_buff_t *buff);
static bool drivinfo_module_receive_handler(ptl_frame_payload_t *payload, ptl_proc_buff_t *ackbuff);

static void app_car_controller_msg_handler(void); // Process messages related to car controller
#ifdef TASK_MANAGER_STATE_MACHINE_SIF
static void app_car_controller_sif_updating(void); // Update the SIF (System Information Frame)
#endif
#ifdef BATTERY_MANAGER
static void get_battery_voltage(void); // Retrieve the current battery voltage
#endif
#ifdef TEST_LOG_DEBUG_SIF
static void log_sif_data(uint8_t *data, uint8_t maxlen); // Log SIF data for debugging purposes
#endif

/*******************************************************************************
 * GLOBAL VARIABLES
 */

/*******************************************************************************
 * STATIC VARIABLES
 */
#ifdef TASK_MANAGER_STATE_MACHINE_SIF
static uint8_t sif_buff[12] = {0}; // Buffer for storing SIF data
#endif
static carinfo_sif_t lt_sif = {0};             // Local SIF data structure
static carinfo_meter_t lt_meter = {0};         // Local meter data structure
static carinfo_indicator_t lt_indicator = {0}; // Local indicator data structure
static carinfo_drivinfo_t lt_drivinfo = {0};   // Local drivetrain information

// Timer variables
static uint32_t l_t_msg_wait_meter_timer; // Timer for 10 ms message wait (not used currently)
static uint32_t l_t_msg_wait_50_timer;    // Timer for 50 ms message wait
static uint32_t l_t_msg_wait_100_timer;   // Timer for 100 ms message wait

static uint32_t l_t_soc_timer; // Timer for state of charge monitoring

// static bool l_t_speed_changed = false;
// static bool l_t_gear_changed = false;
/*******************************************************************************
 * EXTERNAL VARIABLES
 */

/*******************************************************************************
 *  GLOBAL FUNCTIONS IMPLEMENTATION
 */
void app_carinfo_init_running(void)
{
    LOG_LEVEL("app_carinfo_init_running\r\n");
    ptl_register_module(M2A_MOD_METER, meter_module_send_handler, meter_module_receive_handler);
    ptl_register_module(M2A_MOD_INDICATOR, indicator_module_send_handler, indicator_module_receive_handler);
    ptl_register_module(M2A_MOD_DRIV_INFO, drivinfo_module_send_handler, drivinfo_module_receive_handler);

    OTMS(TASK_ID_CAR_INFOR, OTMS_S_INVALID);
}

void app_carinfo_start_running(void)
{
    LOG_LEVEL("app_carinfo_start_running\r\n");
    OTMS(TASK_ID_CAR_INFOR, OTMS_S_ASSERT_RUN);
}

void app_carinfo_assert_running(void)
{
    ptl_reqest_running(M2A_MOD_METER);
    ptl_reqest_running(M2A_MOD_INDICATOR);
    ptl_reqest_running(M2A_MOD_DRIV_INFO);
    StartTickCounter(&l_t_msg_wait_meter_timer);
    StartTickCounter(&l_t_msg_wait_50_timer);
    StartTickCounter(&l_t_msg_wait_100_timer);
    StartTickCounter(&l_t_soc_timer);
    OTMS(TASK_ID_CAR_INFOR, OTMS_S_RUNNING);
}

void app_carinfo_running(void)
{
    /// if(MB_ST_OFF == app_power_state_get_mb_state())
    ///{
    /// OTMS(CAR_INFOR_ID, OTMS_S_POST_RUN);
    ///}

#ifdef TASK_MANAGER_STATE_MACHINE_SIF
    app_car_controller_sif_updating();
#endif

    if (GetTickCounter(&l_t_msg_wait_50_timer) < 10)
        return;
    StartTickCounter(&l_t_msg_wait_50_timer);

#ifdef TASK_MANAGER_STATE_MACHINE_MCU
    app_car_controller_msg_handler();
#endif
}

void app_carinfo_post_running(void)
{
    ptl_release_running(M2A_MOD_METER);
    ptl_release_running(M2A_MOD_INDICATOR);
    ptl_release_running(M2A_MOD_DRIV_INFO);

    /// if(MB_ST_OFF != app_power_state_get_mb_state())
    ///{
    /// OTMS(CAR_INFOR_ID, OTMS_S_ASSERT_RUN);
    /// }
    /// goto app_carinfo_running;
}

void app_carinfo_stop_running(void)
{
    OTMS(TASK_ID_CAR_INFOR, OTMS_S_INVALID);
}

void app_carinfo_on_enter_run(void)
{
    /// if (KCS(AppSetting) > OTMS_S_POST_RUN)
    ///{
    ///     OTMS(CAR_INFOR_ID, OTMS_S_START);
    /// }
}

void app_carinfo_on_exit_post_run(void)
{
    OTMS(TASK_ID_CAR_INFOR, OTMS_S_STOP);
}

uint16_t app_carinfo_getSpeed(void)
{
    return lt_meter.speed_real;
}

carinfo_indicator_t *app_carinfo_get_indicator_info(void)
{
    lt_indicator.leftTurn = 0x03;
    lt_indicator.wifi = 0x0f;
    return &lt_indicator;
}

carinfo_meter_t *app_carinfo_get_meter_info(void)
{
    lt_meter.soc=1;
    lt_meter.speed_real=30;
    return &lt_meter;
}

carinfo_drivinfo_t *app_carinfo_get_drivinfo_info(void)
{
    lt_drivinfo.gear = (carinfo_drivinfo_gear_t)1;
    return &lt_drivinfo;
}

/*******************************************************************************
 * LOCAL FUNCTIONS IMPLEMENTATION
 */
bool meter_module_send_handler(ptl_frame_type_t frame_type, uint16_t param1, uint16_t param2, ptl_proc_buff_t *buff)
{
    assert(buff);
    uint8_t tmp[16] = {0};
    if (M2A_MOD_METER == frame_type)
    {
        switch (param1)
        {
        case CMD_MODMETER_RPM_SPEED:
            tmp[0] = MSB_WORD(lt_meter.speed_real);
            tmp[1] = LSB_WORD(lt_meter.speed_real);
            tmp[2] = MSB_WORD(lt_meter.rpm);
            tmp[3] = LSB_WORD(lt_meter.rpm);
            ptl_build_frame(M2A_MOD_METER, CMD_MODMETER_RPM_SPEED, tmp, 4, buff);
            return true;
        case CMD_MODMETER_SOC:
            // ACK, no thing to do
            tmp[0] = lt_meter.soc;
            tmp[1] = MSB_WORD(lt_meter.voltage);
            tmp[2] = LSB_WORD(lt_meter.voltage);
            tmp[3] = MSB_WORD(lt_meter.current);
            tmp[4] = LSB_WORD(lt_meter.current);
            tmp[5] = lt_meter.voltageSystem;
            tmp[6] = 0;
            // PRINT("SOC %d  V %d C %d adc %d\r\n",lt_meter.soc,lt_meter.voltage,lt_meter.current,SensorAdc_Get_BatVal());
            ptl_build_frame(M2A_MOD_METER, CMD_MODMETER_SOC, tmp, 7, buff);
            return true;
        default:
            break;
        }
    }
    else if (A2M_MOD_METER == frame_type)
    {
        switch (param1)
        {
        case CMD_MODMETER_RPM_SPEED:
            return false;
        case CMD_MODMETER_SOC:
            return false;
        default:
            break;
        }
    }
    return false;
}

bool meter_module_receive_handler(ptl_frame_payload_t *payload, ptl_proc_buff_t *ackbuff)
{
    assert(payload);
    assert(ackbuff);
#ifndef CARINFOR_PTL_NO_ACK // no ack
    uint8_t tmp = 0;
#endif
    if (A2M_MOD_METER == payload->frame_type)
    {
        switch (payload->cmd)
        {
        case CMD_MODMETER_RPM_SPEED:
            // ACK, no thing to do
            return false;
        case CMD_MODMETER_SOC:
            // ACK, no thing to do
            return false;
        default:
            break;
        }
    }
    else if (M2A_MOD_METER == payload->frame_type)
    {
        switch (payload->cmd)
        {
        case CMD_MODMETER_RPM_SPEED:
            // LOGIC
            lt_meter.speed_real = MK_WORD(payload->data[0], payload->data[1]);
            lt_meter.rpm = MK_WORD(payload->data[2], payload->data[3]);
            lt_meter.speed = lt_meter.speed_real * 11 / 10;
            // ACK
#ifndef CARINFOR_PTL_NO_ACK // no ack
            tmp = 0x01;
            ptl_build_frame(A2M_MOD_METER, CMD_MODMETER_RPM_SPEED, &tmp, 1, ackbuff);
#endif
            return true;
        case CMD_MODMETER_SOC:
            // LOGIC
            lt_meter.soc = payload->data[0];
            lt_meter.voltage = MK_WORD(payload->data[1], payload->data[2]);
            lt_meter.current = MK_WORD(payload->data[3], payload->data[4]);
            lt_meter.voltageSystem = payload->data[5];
            // ACK
#ifndef CARINFOR_PTL_NO_ACK // no ack
            tmp = 0x01;
            ptl_build_frame(A2M_MOD_METER, CMD_MODMETER_SOC, &tmp, 1, ackbuff);
#endif
            return true;
        default:
            break;
        }
    }
    return false;
}

bool indicator_module_send_handler(ptl_frame_type_t frame_type, uint16_t param1, uint16_t param2, ptl_proc_buff_t *buff)
{
    assert(buff);
    uint8_t tmp[16] = {0};
    if (M2A_MOD_INDICATOR == frame_type)
    {
        switch (param1)
        {
        case CMD_MODINDICATOR_INDICATOR:
            lt_indicator.highBeam ? SetBit(tmp[0], 0) : ClrBit(tmp[0], 0);    // Զ���
            lt_indicator.lowBeam ? SetBit(tmp[0], 1) : ClrBit(tmp[0], 1);     // �����
            lt_indicator.position ? SetBit(tmp[0], 2) : ClrBit(tmp[0], 2);    // λ�õ�
            lt_indicator.frontFog ? SetBit(tmp[0], 3) : ClrBit(tmp[0], 3);    // ǰ����
            lt_indicator.rearFog ? SetBit(tmp[0], 4) : ClrBit(tmp[0], 4);     // ������
            lt_indicator.leftTurn ? SetBit(tmp[0], 5) : ClrBit(tmp[0], 5);    // ��ת��
            lt_indicator.rightTurn ? SetBit(tmp[0], 6) : ClrBit(tmp[0], 6);   // ��ת��
            lt_indicator.ready ? SetBit(tmp[0], 7) : ClrBit(tmp[0], 7);       // Ready��
            lt_indicator.charge ? SetBit(tmp[1], 0) : ClrBit(tmp[1], 0);      // ��س�ŵ��
            lt_indicator.parking ? SetBit(tmp[1], 1) : ClrBit(tmp[1], 1);     // פ����
            lt_indicator.ecuFault ? SetBit(tmp[1], 2) : ClrBit(tmp[1], 2);    // ECU���ϵ�
            lt_indicator.sensorFault ? SetBit(tmp[1], 3) : ClrBit(tmp[1], 3); // ���������ϵ�
            lt_indicator.motorFault ? SetBit(tmp[1], 4) : ClrBit(tmp[1], 4);  // ������ϵ�
            ptl_build_frame(M2A_MOD_INDICATOR, CMD_MODINDICATOR_INDICATOR, tmp, 5, buff);
            return true;
        case CMD_MODINDICATOR_ERROR_INFO:
            // TODO
            ptl_build_frame(M2A_MOD_INDICATOR, CMD_MODINDICATOR_ERROR_INFO, tmp, 5, buff);
            return true;
        default:
            break;
        }
    }
    else if (A2M_MOD_INDICATOR == frame_type)
    {
        switch (param1)
        {
        case CMD_MODINDICATOR_INDICATOR:
            return false;
        case CMD_MODINDICATOR_ERROR_INFO:
            return false;
        default:
            break;
        }
    }
    return false;
}

bool indicator_module_receive_handler(ptl_frame_payload_t *payload, ptl_proc_buff_t *ackbuff)
{
    assert(payload);
    assert(ackbuff);
#ifndef CARINFOR_PTL_NO_ACK // no ack
    uint8_t tmp = 0;
#endif
    if (A2M_MOD_INDICATOR == payload->frame_type)
    {
        switch (payload->cmd)
        {
        case CMD_MODINDICATOR_INDICATOR:
            // ACK, no thing to do
            return false;
        case CMD_MODINDICATOR_ERROR_INFO:
            // ACK, no thing to do
            return false;
        default:
            break;
        }
    }
    else if (M2A_MOD_INDICATOR == payload->frame_type)
    {
        switch (payload->cmd)
        {
        case CMD_MODINDICATOR_INDICATOR:
            // LOGIC
            lt_indicator.highBeam = GetBit(payload->data[0], 0);    // Զ���
            lt_indicator.lowBeam = GetBit(payload->data[0], 1);     // �����
            lt_indicator.position = GetBit(payload->data[0], 2);    // λ�õ�
            lt_indicator.frontFog = GetBit(payload->data[0], 3);    // ǰ����
            lt_indicator.rearFog = GetBit(payload->data[0], 4);     // ������
            lt_indicator.leftTurn = GetBit(payload->data[0], 5);    // ��ת��
            lt_indicator.rightTurn = GetBit(payload->data[0], 6);   // ��ת��
            lt_indicator.ready = GetBit(payload->data[0], 7);       // Ready��
            lt_indicator.charge = GetBit(payload->data[1], 0);      // ��س�ŵ��
            lt_indicator.parking = GetBit(payload->data[1], 1);     // פ����
            lt_indicator.ecuFault = GetBit(payload->data[1], 2);    // ECU���ϵ�
            lt_indicator.sensorFault = GetBit(payload->data[1], 3); // ���������ϵ�
            lt_indicator.motorFault = GetBit(payload->data[1], 4);  // ������ϵ�
#ifndef CARINFOR_PTL_NO_ACK                                         // no ack
            tmp = 0x01;
            ptl_build_frame(A2M_MOD_INDICATOR, CMD_MODINDICATOR_INDICATOR, &tmp, 1, ackbuff);
#endif
            return true;
        case CMD_MODINDICATOR_ERROR_INFO:
#ifndef CARINFOR_PTL_NO_ACK // no ack
            tmp = 0x01;
            ptl_build_frame(A2M_MOD_INDICATOR, CMD_MODINDICATOR_ERROR_INFO, &tmp, 1, ackbuff);
#endif
            return true;
        default:
            break;
        }
    }
    return false;
}

bool drivinfo_module_send_handler(ptl_frame_type_t frame_type, uint16_t param1, uint16_t param2, ptl_proc_buff_t *buff)
{
    assert(buff);
    uint8_t tmp[16] = {0};
    if (M2A_MOD_DRIV_INFO == frame_type)
    {
        switch (param1)
        {
        case CMD_MODDRIVINFO_GEAR:
            tmp[0] = lt_drivinfo.gear;
            tmp[1] = lt_drivinfo.driveMode;
            ptl_build_frame(M2A_MOD_DRIV_INFO, CMD_MODDRIVINFO_GEAR, tmp, 2, buff);
            return true;
        default:
            break;
        }
    }
    else if (A2M_MOD_DRIV_INFO == frame_type)
    {
        switch (param1)
        {
        case CMD_MODDRIVINFO_GEAR:
            return false;
        default:
            break;
        }
    }
    return false;
}

bool drivinfo_module_receive_handler(ptl_frame_payload_t *payload, ptl_proc_buff_t *ackbuff)
{
    assert(payload);
    assert(ackbuff);

    if (A2M_MOD_DRIV_INFO == payload->frame_type)
    {
        switch (payload->cmd)
        {
        case CMD_MODDRIVINFO_GEAR:
            // ACK, no thing to do
            return false;
        default:
            break;
        }
    }
    else if (M2A_MOD_DRIV_INFO == payload->frame_type)
    {
        switch (payload->cmd)
        {
        case CMD_MODDRIVINFO_GEAR:
            // LOGIC
            lt_drivinfo.gear = (carinfo_drivinfo_gear_t)payload->data[0];
            lt_drivinfo.driveMode = (carinfo_drivinfo_drivemode_t)payload->data[1];
            // ACK
#ifndef CARINFOR_PTL_NO_ACK // no ack
            uint8_t tmp = 0;
            tmp = 0x01;
            ptl_build_frame(A2M_MOD_DRIV_INFO, CMD_MODDRIVINFO_GEAR, &tmp, 1, ackbuff);
#endif
            return true;
        default:
            break;
        }
    }
    return false;
}

#ifdef TASK_MANAGER_STATE_MACHINE_SIF
void app_car_controller_sif_updating(void)
{
    uint8_t res = SIF_ReadData(sif_buff, sizeof(sif_buff));
    // uint8_t lt_meter_current_gear = 0;
    uint16_t lt_meter_current_speed = 0;

#ifdef TEST_LOG_DEBUG_SIF
    if (res)
        log_sif_data(sif_buff, sizeof(sif_buff));
#endif
    if (res && sif_buff[0] == 0x08 && sif_buff[1] == 0x61)
    {
        lt_sif.sideStand = ((sif_buff[2] & 0x08) ? 1 : 0);                // ���Ŷϵ���  0:��������     1:���ŷ���
        lt_sif.bootGuard = ((sif_buff[2] & 0x02) ? 1 : 0);                // ��������            0:�Ǳ���          1:������
        lt_sif.hallFault = ((sif_buff[3] & 0x40) ? 1 : 0);                // ��������(���)0:����            1:����
        lt_sif.throttleFault = ((sif_buff[3] & 0x20) ? 1 : 0);            // ת�ѹ���
        lt_sif.controllerFault = ((sif_buff[3] & 0x10) ? 1 : 0);          // ����������
        lt_sif.lowVoltageProtection = ((sif_buff[3] & 0x08) ? 1 : 0);     // Ƿѹ����
        lt_sif.cruise = ((sif_buff[3] & 0x04) ? 1 : 0);                   // Ѳ��ָʾ��
        lt_sif.assist = ((sif_buff[3] & 0x02) ? 1 : 0);                   // ����ָʾ��
        lt_sif.motorFault = ((sif_buff[3] & 0x01) ? 1 : 0);               // �������
        lt_sif.gear = ((sif_buff[4] & 0x80) >> 5) | (sif_buff[4] & 0x03); // ��λ//0~7
        lt_sif.motorRunning = ((sif_buff[4] & 0x40) ? 1 : 0);             // ������� 1����
        lt_sif.brake = ((sif_buff[4] & 0x20) ? 1 : 0);                    // ɲ��
        lt_sif.controllerProtection = ((sif_buff[4] & 0x10) ? 1 : 0);     // ����������
        lt_sif.coastCharging = ((sif_buff[4] & 0x08) ? 1 : 0);            // ���г��
        lt_sif.antiSpeedProtection = ((sif_buff[4] & 0x04) ? 1 : 0);      // ���ɳ�����
        lt_sif.seventyPercentCurrent = ((sif_buff[5] & 0x80) ? 1 : 0);    // 70%����
        lt_sif.pushToTalk = ((sif_buff[5] & 0x40) ? 1 : 0);               // ����һ��ͨ
        lt_sif.ekkBackupPower = ((sif_buff[5] & 0x20) ? 1 : 0);           // ����EKK���õ�Դ
        lt_sif.overCurrentProtection = ((sif_buff[5] & 0x10) ? 1 : 0);    // ��������
        lt_sif.motorShaftLockProtection = ((sif_buff[5] & 0x08) ? 1 : 0); // ��ת����
        lt_sif.reverse = ((sif_buff[5] & 0x04) ? 1 : 0);                  // ����
        lt_sif.electronicBrake = ((sif_buff[5] & 0x02) ? 1 : 0);          // ����ɲ��
        lt_sif.speedLimit = ((sif_buff[5] & 0x01) ? 1 : 0);               // ����
        lt_sif.current = ((sif_buff[6] & 0xFF));                          // ���� ��λ��1A
        lt_sif.hallCounter = MK_WORD(sif_buff[7], sif_buff[8]);           // 0.5s�����������仯�ĸ���
        lt_sif.soc = ((sif_buff[9] & 0xFF));                              // ����/���� 0-100% 5��ָʾΪ 90,70,50,30,20���ٷֱȣ������Ӧ�ĵ�ѹ����Ϊ 47V��46V,44.5V,43V,41V)��4 ��ָʾΪ 90,70,50,30
        lt_sif.voltageSystem = ((sif_buff[10] & 0xFF));                   // ��ѹϵͳ  0x01:36V  0x02:48V  0x04:60V  0x08:64V  0x10:72V  0x20:80V  0x40:84V   0x80:96V

        double rpm = lt_sif.hallCounter * (2.0 * 60 / 100.0);
        double radius = 0.254 / 2.0;                      // ��̥�뾶
        double w = rpm * (2.0 * 3.14159265358979 / 60.0); // ת�����ٶȣ���λ������/��
        double v = w * radius;                            // ���ٶȣ���λ:��/��

        lt_meter.rpm = rpm + 20000; // offset:-20000
        lt_meter.speed = v * (10.0 * 3600.0 / 1000.0) * 1.1;

        lt_meter_current_speed = v * (10.0 * 3600.0 / 1000.0);

        lt_meter.voltageSystem = lt_sif.voltageSystem;
        // lt_meter.soc = lt_sif.soc;
        lt_meter.current = lt_sif.current * 10; // test

        if (lt_sif.gear != lt_drivinfo.gear)
        {
            // l_t_gear_changed = true;
            LOG_LEVEL("SIF DATA:lt_drivinfo.gear changed\r\n");
            send_message(TASK_ID_PTL, M2A_MOD_DRIV_INFO, CMD_MODDRIVINFO_GEAR, 0);
        }
        lt_drivinfo.gear = (carinfo_drivinfo_gear_t)lt_sif.gear;
        if (lt_meter.speed_real != lt_meter_current_speed)
        {
            // l_t_speed_changed=true;
            LOG_LEVEL("SIF DATA:lt_drivinfo.speed_real changed\r\n");
            send_message(TASK_ID_PTL, M2A_MOD_METER, CMD_MODMETER_RPM_SPEED, 0);
        }
        lt_meter.speed_real = lt_meter_current_speed;
    }
}
#endif

void app_car_controller_msg_handler(void)
{

    lt_indicator.position = GPIO_PIN_READ_SKD() ? 0 : 1;  /* ʾ���� */
    lt_indicator.highBeam = GPIO_PIN_READ_DDD() ? 0 : 1;  /* ���   */
    lt_indicator.leftTurn = GPIO_PIN_READ_ZZD() ? 0 : 1;  /* ��ת�� */
    lt_indicator.rightTurn = GPIO_PIN_READ_YZD() ? 0 : 1; /* ��ת�� */

    lt_indicator.ready = !lt_sif.bootGuard;

    lt_indicator.ecuFault = lt_sif.controllerFault;
    lt_indicator.sensorFault = lt_sif.throttleFault;
    lt_indicator.motorFault = lt_sif.motorFault | lt_sif.hallFault;

    lt_indicator.parking = lt_sif.brake;
#ifdef BATTERY_MANAGER
    get_battery_voltage();
#endif

    Msg_t *msg = get_message(TASK_ID_CAR_INFOR);
    if (msg->id != NO_MSG && (MsgId_t)msg->id == MSG_DEVICE_GPIO_EVENT)
    {
        send_message(TASK_ID_PTL, M2A_MOD_INDICATOR, CMD_MODINDICATOR_INDICATOR, 0);
    }

#if 0
    if(l_t_speed_changed)
		{
			send_message(TASK_ID_PTL, M2A_MOD_METER, CMD_MODMETER_RPM_SPEED, 0);	
			l_t_speed_changed=false;			
		}
		if(l_t_gear_changed)
		{
			send_message(TASK_ID_PTL, M2A_MOD_DRIV_INFO, CMD_MODDRIVINFO_GEAR, 0);
			l_t_gear_changed = false;
		}
#endif
#if 0
	static uint8_t l_u8_op_step = 0;  // Operational step variable
    if(GetTickCounter(&l_t_msg_wait_100_timer) >= 300)
    {
        switch(l_u8_op_step)
        {
        case 0:
            send_message(TASK_ID_PTL, M2A_MOD_METER, CMD_MODMETER_SOC, 0);
            break;
        case 1:
            send_message(TASK_ID_PTL, M2A_MOD_METER, CMD_MODMETER_RPM_SPEED, 0);		
            break;
        case 2:
            send_message(TASK_ID_PTL, M2A_MOD_DRIV_INFO, CMD_MODDRIVINFO_GEAR, 0);
            break;
				case 3:
            send_message(TASK_ID_PTL, M2A_MOD_INDICATOR, CMD_MODINDICATOR_ERROR_INFO, 0);
            break;
        default:
            l_u8_op_step = (uint8_t)-1;
            break;
        }
				
        l_u8_op_step++;
				if(l_u8_op_step >=4)
					l_u8_op_step = 0;
        StartTickCounter(&l_t_msg_wait_100_timer);
    }
#endif
}

#ifdef TEST_LOG_DEBUG_SIF
void log_sif_data(uint8_t *data, uint8_t maxlen)
{
    LOG_LEVEL("SIF DATA:");
    for (int i = 0; i < maxlen; i++)
    {
        LOG_("0x%02x ", data[i]);
    }
    LOG_("\r\n");
}
#endif

#ifdef BATTERY_MANAGER
void get_battery_voltage(void)
{
    if (GetTickCounter(&l_t_soc_timer) < 1000)
    {
        return;
    }
    StartTickCounter(&l_t_soc_timer);
    uint8_t cellcount = 4; // Ĭ��4�ŵ��
    uint16_t vol = 0;      //(uint32_t)SensorAdc_Get_BatVal() * 274 / 1000;

    bool rise = vol > lt_meter.voltage;
    lt_meter.voltage = vol;
    uint8_t soc = 100;

    if (rise)
    {
        // ���ݿͻ��ĵ�ѹ�������
        switch (lt_sif.voltageSystem)
        {
        case 0x00:
        case 0x02: // 48V
        {
            {
                if (lt_meter.voltage >= 480)
                    soc = 100;
                else if (lt_meter.voltage >= 465)
                    soc = 80;
                else if (lt_meter.voltage >= 445)
                    soc = 40;
                else if (lt_meter.voltage >= 415)
                    soc = 20;
                else
                    soc = 10;
            }

            // if(soc < lt_meter.soc)
            {
                lt_meter.soc = soc;
            }

            return;
        }
        case 0x04: // 60V
        {
            if (lt_meter.voltage >= 600)
                soc = 100;
            else if (lt_meter.voltage >= 574)
                soc = 80;
            else if (lt_meter.voltage >= 550)
                soc = 40;
            else if (lt_meter.voltage >= 526)
                soc = 20;
            else
                soc = 10;
        }

            // if(soc < lt_meter.soc)
            {
                lt_meter.soc = soc;
            }
            return;
        case 0x10: // 72V
        {
            if (lt_meter.voltage >= 719)
                soc = 100;
            else if (lt_meter.voltage >= 690)
                soc = 80;
            else if (lt_meter.voltage >= 660)
                soc = 40;
            else if (lt_meter.voltage >= 630)
                soc = 20;
            else
                soc = 10;
        }

            // if(soc < lt_meter.soc)
            {
                lt_meter.soc = soc;
            }
            return;
        }
    }
    else
    {
        // ���ݿͻ��ĵ�ѹ�������
        switch (lt_sif.voltageSystem)
        {
        case 0x00:
        case 0x02: // 48V
        {
            {
                if (lt_meter.voltage >= 470)
                    soc = 100;
                else if (lt_meter.voltage >= 455)
                    soc = 80;
                else if (lt_meter.voltage >= 435)
                    soc = 40;
                else if (lt_meter.voltage >= 405)
                    soc = 20;
                else
                    soc = 10;
            }

            // if(soc < lt_meter.soc)
            {
                lt_meter.soc = soc;
            }

            return;
        }
        case 0x04: // 60V
        {
            if (lt_meter.voltage >= 590)
                soc = 100;
            else if (lt_meter.voltage >= 564)
                soc = 80;
            else if (lt_meter.voltage >= 540)
                soc = 40;
            else if (lt_meter.voltage >= 516)
                soc = 20;
            else
                soc = 10;
        }

            // if(soc < lt_meter.soc)
            {
                lt_meter.soc = soc;
            }
            return;
        case 0x10: // 72V
        {
            if (lt_meter.voltage >= 709)
                soc = 100;
            else if (lt_meter.voltage >= 680)
                soc = 80;
            else if (lt_meter.voltage >= 650)
                soc = 40;
            else if (lt_meter.voltage >= 620)
                soc = 20;
            else
                soc = 10;
        }

            // if(soc < lt_meter.soc)
            {
                lt_meter.soc = soc;
            }
            return;
        }
    }

    // ͨ�ü���
    switch (lt_sif.voltageSystem)
    {
    case 0x01: // 36V
        cellcount = 3;
        break;
    case 0x02: // 48V
        cellcount = 4;
        break;
    case 0x04: // 60V
        cellcount = 5;
        break;
    case 0x08:         // 64V
        cellcount = 5; // ��12V��ѹ��أ���ʱ����
        break;
    case 0x10: // 72V
        cellcount = 6;
        break;
    case 0x20:         // 80V
        cellcount = 6; // ��12V��ѹ��أ���ʱ����
        break;
    case 0x40: // 84V
        cellcount = 7;
        break;
    case 0x80: // 96V
        cellcount = 8;
        break;
    default:
        break;
    }

    if (lt_meter.voltage > (CELL_VOL_90 * cellcount / 10))
    {
        soc = 90;
    }
    else if (lt_meter.voltage > (CELL_VOL_80 * cellcount / 10))
    {
        soc = 80;
    }
    else if (lt_meter.voltage > (CELL_VOL_70 * cellcount / 10))
    {
        soc = 70;
    }
    else if (lt_meter.voltage > (CELL_VOL_60 * cellcount / 10))
    {
        soc = 60;
    }
    else if (lt_meter.voltage > (CELL_VOL_50 * cellcount / 10))
    {
        soc = 50;
    }
    else if (lt_meter.voltage > (CELL_VOL_40 * cellcount / 10))
    {
        soc = 40;
    }
    else if (lt_meter.voltage > (CELL_VOL_30 * cellcount / 10))
    {
        soc = 30;
    }
    else if (lt_meter.voltage > (CELL_VOL_20 * cellcount / 10))
    {
        soc = 20;
    }
    else
    {
        soc = 0;
    }

    // if(soc < lt_meter.soc)
    {
        lt_meter.soc = soc;
    }
}
#endif
