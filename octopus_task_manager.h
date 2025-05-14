/*******************************************************************************
 * @file    octopus_task_manager.h
 * @brief   Task manager header file for the Octopus platform.
 *          Provides declarations for task state machine management, including
 *          initialization, state transitions, and task-specific configurations.
 *
 * @details This file defines macros, data structures, and function prototypes
 *          for managing tasks within the Octopus system. Each task operates as
 *          a finite state machine with well-defined states and transitions.
 *
 * @version  1.0.0
 * @date     2024-12-09
 * @author   Octopus Team
 *
 * @note    Ensure that task-specific state functions are implemented and linked
 *          correctly in the corresponding source files.
 ******************************************************************************/

 #ifndef __OCTOPUS_TASK_MANAGER_H__
 #define __OCTOPUS_TASK_MANAGER_H__
 
 /*******************************************************************************
  * INCLUDES
  ******************************************************************************/
 #include "octopus_platform.h"
 //#include "octopus_system.h"
 //#include "octopus_gpio.h"
 //#include "octopus_carinfor.h"
 //#include "octopus_ble.h"
 //#include "octopus_key.h"
 //#include "octopus_update_soc.h"
 //#include "octopus_update_mcu.h"
 //#include "octopus_ipc.h"
 
 /*******************************************************************************
  * MACROS
  ******************************************************************************/
 
 /** Task states for Octopus Task Manager State Machine (OTMS). */
 #define OTMS_S_INIT        (0x00U) /**< Initial state. */
 #define OTMS_S_START       (0x01U) /**< Start state. */
 #define OTMS_S_ASSERT_RUN  (0x02U) /**< Assert run pre-check state. */
 #define OTMS_S_RUNNING     (0x03U) /**< Actively running state. */
 #define OTMS_S_POST_RUN    (0x04U) /**< Cleanup or post-run transition state. */
 #define OTMS_S_STOP        (0x05U) /**< Task stopped state. */
 #define OTMS_S_INVALID     (0xFFU) /**< Invalid/uninitialized state. */
 
 #define OTMS_S_COUNT       (6U)    /**< Total number of defined states. */
 
 /** Macro to set a task's state. */
 #define OTMS(task_id, state)        otms_set_state(task_id, state)
 
 /** Macro to get a task's current state. */
 #define GET_OTMS_STATE(task_id)     otms_get_state(task_id)
 
 /*******************************************************************************
  * TYPEDEFS
  ******************************************************************************/
 
 /** Task ID type. */
 typedef int32_t otms_id_t;
 
 /** Task state type. */
 typedef uint8_t otms_state_t;
 
 /** State handler function type. */
 typedef void (*otms_state_func_t)(void);
 
 /**
  * @brief Task state machine definition.
  * Contains the maximum state and function pointer array for each state.
  */
 typedef struct {
     otms_state_t state_limit;                   /**< Max valid state value for task. */
     const otms_state_func_t func[OTMS_S_COUNT]; /**< State handler functions. */
 } otms_t;
 
 /**
  * @brief Unique task module identifiers.
  */
 typedef enum {
		TASK_ID_PTL_1 = 0, /**< Protocol handling task. */
		TASK_ID_PTL_2, /**< Protocol handling task. */
		TASK_ID_SYSTEM,        /**< System task. */
		TASK_ID_GPIO,          /**< GPIO task. */
		TASK_ID_CAR_INFOR,     /**< Car information. */
		TASK_ID_BLE,           /**< BLE communication. */
		TASK_ID_KEY,           /**< Key input. */
		TASK_ID_UPDATE_MCU,    /**< MCU firmware update. */
		TASK_ID_UPDATE_SOC,    /**< SOC firmware update. */
		TASK_ID_CAN,
		TASK_ID_PTL_BAFANG,
		TASK_ID_IPC_SOCKET,    /**< IPC socket service. */

		TASK_ID_MAX_NUM        /**< Total number of tasks. */
 } TaskModule_t;
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /*******************************************************************************
  * FUNCTION DECLARATIONS
  ******************************************************************************/
 
 /**
  * @brief Initialize the task manager, setting all tasks to INIT.
  */
 void task_manager_init(void);
 
 /**
  * @brief Start all tasks by transitioning them to the START state.
  */
 void task_manager_start(void);
 
 /**
  * @brief Stop all tasks by setting them to the STOP state.
  */
 void task_manager_stop(void);
 
 /**
  * @brief Execute the current state handler for all registered tasks.
  */
 void task_manager_run(void);
 
 /**
  * @brief Transition all tasks into the RUNNING state.
  */
 void otms_on_enter_run(void);
 
 /**
  * @brief Transition all tasks out of the POST_RUN state.
  */
 void otms_on_exit_post_run(void);
 
 /**
  * @brief Set the current state for a specific task.
  * @param task_id Task ID to modify.
  * @param state   New state to apply.
  */
 void otms_set_state(otms_id_t task_id, otms_state_t state);
 
 /**
  * @brief Get the current state of a specific task.
  * @param task_id Task ID to query.
  * @return Current state.
  */
 otms_state_t otms_get_state(otms_id_t task_id);
 
 /**
  * @brief Get the global task state configuration table.
  * @return Pointer to array of task state machines.
  */
 const otms_t *otms_get_config(void);
 
 #ifdef __cplusplus
 }
 #endif
 
#endif /* __OCTOPUS_TASK_MANAGER_H__ */

