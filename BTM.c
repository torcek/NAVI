/*
 * BTM.c
 *
 *  Created on: Jun 7, 2015
 *      Author: Grzegorz WÓJCIK
 */

#include "BTM.h"

/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Global variables ----------------------------------------------------------*/


/* Private functions ---------------------------------------------------------*/
void BTM_initRCC(void){
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO | RCC_APB2Periph_USART1, ENABLE);
}

void BTM_initGPIO(void){
	/* STM32F103RBT6
	 * USART1:
	 * TX: PA9
	 * RX: PA10 */
	/*	PA0 & PA1 are run through pins for BTM TX & RX lines	*/
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
}

void BTM_initUART(void){

	USART_InitTypeDef USART_InitStructure;
	USART_InitStructure.USART_BaudRate = 115200;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;

	USART_Init(USART1, &USART_InitStructure);
	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE); 			// Enable the USART1 receive and transmit interrupt
	//USART_ITConfig(USART1, USART_IT_TXE, ENABLE); 			// Enable the USART1 receive and transmit interrupt
	USART_Cmd(USART1, ENABLE);

	NVIC_InitTypeDef NVIC_InitStruct;
	NVIC_InitStruct.NVIC_IRQChannel = USART1_IRQn;
	NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
	NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 2;
	NVIC_InitStruct.NVIC_IRQChannelSubPriority = 2;
	NVIC_Init(&NVIC_InitStruct);

	//USART_ClearFlag(USART1, USART_FLAG_RXNE);
	//USART_ClearFlag(USART1, USART_FLAG_TXE);
	GV_flag_BTMRX = 0;
	GV_flag_BTMTX = 0;
}

/* Clearing BTM buffor */
void BTM_ClearBuffer(){
	uint8_t tmp = 0;
	for (tmp = 0; tmp < BTM_BUFFER_LENGTH; tmp++){
		GV_bufferBTM[tmp] = 0;
	}
}


/*
* @brief Function Name  : Checksum function
* @brief Description    : This function generates CRC of the GV_bufferBTM string. After detecting 'StartChar' it starts
* 							generating the CRC and stops when '*' is detected. The CRC does not consider
* 							'StartCHar', nor '*' sign.
* @param StartChar		: Char, after which CRC generation is started
* @param Length			: String length
* @return				: uint8_t CRC
*
* @example				: CRC of the: "#,2,0,0,1,600,* is 53 (decimal)"
* @INFO						: this function uses global variable GV_bufferBTM[]
*/
uint8_t BTM_calculateCRC(char StartChar, uint8_t Length){
	uint8_t XOR = 0;	// set CRC to 0
	uint8_t i = 0;

	for( XOR = 0, i = 0; i < Length; i++ ){
		if(GV_bufferBTM[i] == '*')
			break;
		if(GV_bufferBTM[i] != StartChar)
			XOR ^= GV_bufferBTM[i];
	}

	return XOR;
}

/*
* @brief Function Name  	: Checksum check
* @brief Description    	: This function compares CRC included in received data frame with the generated one.
* @param GeneratedChecksum	: Checksum generated for example by BTM_calculateCRC
* @return 					: 0 = CRCs do not match, 1 = CRCs match
*
* @INFO						: this function uses global variable GV_bufferBTM[]
*/

FlagStatus BTM_checkCRC(char StartChar, uint8_t Length){
	static uint8_t XOR = 0;					// calculated checksum
	static uint8_t i = 0, j = 0, tmp = 0;
	char checksum[4] = {0};					// received checksum

	for( XOR = 0, i = 0, j = 0, tmp = 0; i < Length; i++ ){
		if( tmp == 1 ){
			checksum[j] = GV_bufferBTM[i];
			j++;
			if( j >= 3 )
				break;
		}
		if(GV_bufferBTM[i] == '*')
			tmp = 1;
		if( (GV_bufferBTM[i] != StartChar) && (tmp != 1) )		// XOR is calculated for chars from StartChar up to '*'
			XOR ^= GV_bufferBTM[i];
	}

	/* 0 - CRCs do not match, 1 - CRCs match */
	if(atoi(checksum) == XOR)
		return SET;
	else
		return RESET;
}

void USART1_IRQHandler(void){

	if( USART_GetFlagStatus(USART1, USART_FLAG_RXNE ) != RESET ){
		static uint8_t cnt = 0;
		char t = USART1->DR; 		// Received character from USART1 data register is saved in

		if( GV_flag_BTMRX == 0 ){
			if( t == BTM_DF_CHAR ){			// Data frame first sign
				cnt = 0;
				GV_bufferBTM[cnt] = t;
				cnt++;
			}
			if( (t != BTM_DF_CHAR) && (GV_bufferBTM[0] == BTM_DF_CHAR) ){
				GV_bufferBTM[cnt] = t;
				cnt++;

				if( (t == '\n') || ( cnt >= BTM_BUFFER_LENGTH-1 ) ){
					cnt = 0;
					GV_flag_BTMRX = 1;
					USART_ClearFlag(USART1, USART_FLAG_ORE | USART_FLAG_PE | USART_FLAG_NE | USART_FLAG_FE | USART_FLAG_TXE);
				}
			}
		}
	}
}


