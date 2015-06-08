/*
 * BTM.h
 *
 *  Created on: Jun 7, 2015
 *      Author: Grzegorz W�JCIK
 */

#ifndef BTM_H_
#define BTM_H_

/*** PREDEFINED SETTINGS	***/
#define BTM_PORT 			GPIOB
#define BTM_USART1_TX		GPIO_Pin_9
#define BTM_USART1_RX		GPIO_Pin_10
#define BTM_BUFFOR_LENGTH	30

/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Global variables ----------------------------------------------------------*/
volatile unsigned char GV_bufforBTM[BTM_BUFFOR_LENGTH];
volatile unsigned char GV_flag_BTMRX;
volatile unsigned char GV_flag_BTMTX;

/* Private functions ---------------------------------------------------------*/
void BTM_initRCC(void);
void BTM_initGPIO(void);
void BTM_initUART(void);
void BTM_ClearBuffor(void);
void USART_puts(USART_TypeDef* USARTx, volatile char *s);

#endif /* BTM_H_ */