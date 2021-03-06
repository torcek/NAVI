/**
*****************************************************************************
**
**  File        : main.c
**
**  Abstract    : Quadcopter navigation board main function.
**
**  Author		: Grzegorz W�JCIK
**
**  Environment : Atollic TrueSTUDIO/STM32
**
**	MCU:		: STM32F103RBT6
*****************************************************************************
*/

/* Includes */
#include <stddef.h>
#include "stm32f10x.h"

#include "functions.h"
#include "AUTOPILOT.h"
#include "BTM.h"
#include "CONTROLLER.h"
#include "FAULTS.h"
#include "SPI.h"
#include "ff.h"
#include "diskio.h"

/* Private typedef */
/* Private define  */
/* Private macro */
/* Private variables */
/* Private function prototypes */
/* Private functions */


int main(void)
{
	/* TODO - Add your application code here */
	PLL_Configurattion();
	SYSTEM_ClockCheck();

	NVIC_SetPriorityGrouping(NVIC_PriorityGroup_2);
	NVIC_SetPriority(SysTick_IRQn, NVIC_EncodePriority(2,2,0));
	NVIC_SetPriority(USART1_IRQn, NVIC_EncodePriority(2,1,2));

	/* Wireless communication via Bluetooth with a Tablet/PC */
	BTM_initRCC();
	BTM_initGPIO();
	BTM_initUART();
	BTM_ClearBuffer();

	/* On-board communication with Autopilot Board */
	AP_initRCC();
	AP_initGPIO();
	AP_initUART();
	AP_ClearBuffer();

	/* Enter infinite loop only when clock frequencies are OK */
	if( GV_SystemReady == 1 ){
		FAULTS_initRCC();
		FAULTS_Servo_initGPIO();
		FAULTS_Servo_initTIM();		// 50 Hz PWM (together with control loop below
		FAULTS_ESC_initGPIO();

		SPI_initRCC();
		SPI_initGPIO();
		SPI_initSPI();
		SPI_initSD();

		CTRL_initNaviStruct();
		CTRL_initTIM();				// 50 Hz control loop
		CTRL_initNAVI_RCC();		// 50 Hz Autopilot PWMs
		CTRL_initNAVI_GPIO();		// 50 Hz Autopilot PWMs
		CTRL_initNAVI_PWM();		// 50 Hz Autopilot PWMs
		SysTick_Config(SystemCoreClock/1000);	// SysTick 1 kHZ (1ms interval)

		SD_initInterrupt_CardDetect();

		while (1)
		{

			if(GV_flag_BTMRX == 1 ){
				if( BTM_checkCRC(BTM_DF_CHAR, BTM_BUFFER_LENGTH) != RESET ){
						CTRL_DataProcess(CTRL_source_MobileDevice);
				}
				else{
					/*
					USART_puts(USART1, "Data frame error");
					while(USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET);
					USART_SendData(USART1, 0X0A);
					while(USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET);
					USART_SendData(USART1, 0X0D);
					while(USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET);
					 */
				}
				BTM_ClearBuffer();
				GV_flag_BTMRX = 0;
			}

			if( GV_SystemCounter % 100 == 0){		//Update log every 250ms
				if(GV_TimeStart == SET)
					SD_createLog();
			}


			if( ( GV_flag_APRX == 1 ) && ( GV_SystemCounter % 20 == 0 ) )
			{
				if( AP_checkCRC(AP_DF_CHAR, AP_BUFFER_LENGTH) != RESET ){
						CTRL_DataProcess(CTRL_source_Autopilot);
				}
				USART_puts( USART1, GV_bufferAP );
				while(USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET);
				AP_ClearBuffer();
				GV_flag_APRX = 0;
			}

		}
	}
	else
		while(1);
}



/*
 * Minimal __assert_func used by the assert() macro
 * */
void __assert_func(const char *file, int line, const char *func, const char *failedexpr)
{
  while(1)
  {}
}

/*
 * Minimal __assert() uses __assert__func()
 * */
void __assert(const char *file, int line, const char *failedexpr)
{
   __assert_func (file, line, NULL, failedexpr);
}
