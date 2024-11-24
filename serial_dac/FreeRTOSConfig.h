#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#define vPortSVCHandler sv_call_handler
#define xPortPendSVHandler pend_sv_handler
#define xPortSysTickHandler sys_tick_handler

#define configUSE_PREEMPTION		1
#define configUSE_IDLE_HOOK		0
#define configUSE_TICK_HOOK		0
#define configCPU_CLOCK_HZ		( ( unsigned long ) 84000000 )
#define configSYSTICK_CLOCK_HZ		( configCPU_CLOCK_HZ / 8 )
#define configTICK_RATE_HZ		( ( TickType_t ) 1000 )
#define configMAX_PRIORITIES		( 5 )
#define configMINIMAL_STACK_SIZE	( ( unsigned short ) 128 )
#define configTOTAL_HEAP_SIZE		( ( size_t ) ( 17 * 1024 ) )
#define configMAX_TASK_NAME_LEN		( 16 )
#define configUSE_16_BIT_TICKS		0
#define configIDLE_SHOULD_YIELD		1
#define configCHECK_FOR_STACK_OVERFLOW	1

#define INCLUDE_vTaskDelay		1

#define configKERNEL_INTERRUPT_PRIORITY 	255
#define configMAX_SYSCALL_INTERRUPT_PRIORITY 	191
#define configLIBRARY_KERNEL_INTERRUPT_PRIORITY	15

#endif /* FREERTOS_CONFIG_H */
