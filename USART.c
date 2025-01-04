#include <stm32f4xx.h>
#include <stm32f411xe.h>



#include <stdio.h>
#include <string.h>



void USART2_Init(void);		 
void CMSIS_USART_Transmit(USART_TypeDef *USART, uint32_t *data, uint16_t Size);
void USART2_IRQHandler(void);

struct USART_name {
	uint32_t tx_buffer[64]; //Буфер под выходящие данные
	uint32_t rx_buffer[64]; //Буфер под входящие данные
	uint16_t rx_counter; //Счетчик приходящих данных типа uint8_t по USART
	uint16_t rx_len; //Количество принятых байт после сработки флага IDLE
};
static struct USART_name husart2; 







/////////////////////////ОБРАБОТЧИК ПРЕРЫВАНИЯ////////////////////////////////////////////////////////////////////////
//void USART2_IRQHandler(void){
//	
//	if((USART2->SR & USART_SR_RXNE)!=0){
//	//
//		        temp1 = USART2->DR;                       //получили данные из терминала, например, записали их в переменную и тут же отправили строку(ниже)
//		       for(int i = 0; i <10000; i++){}
//         		USART2_Send_String("Byte received\r\n");
//						//for(int i = 0; i <3000000; i++){}
//						//USART2->DR = data4Int;
//					
//		        USART2->SR &= ~USART_SR_RXNE;        //очистили флаг , задержка
//						for(int i = 0; i <10000; i++){}
//						
//							if((USART2->SR &USART_SR_TC)!=0)              //если данные переданные, передаём другие , очищаем флаг
//							{
//								USART2->DR = temp1;
//								for(int i = 0; i <100000; i++){}
//								USART2_Send_String("\r\nByte sent\r\n\r\n");
//								USART2->SR &= ~USART_SR_TC;
//							}
//						}
//}





 void USART2_IRQHandler(void) {
	 
// 	if (READ_BIT(USART2->SR, USART_SR_RXNE)) {  //работает, ПРИНИМАЕТ И ТУТ ЖЕ ОТСЫЛАЕТ
//		USART2->SR &=	~USART_SR_RXNE;
//		temp = USART2->DR;
//		USART2->DR = temp;
//		//NVIC_GetPriority(TIM3_IRQn);
//		
//						

//	}
	 
	 // ЭКВИВАЛНТНАЯ ЛОГИКА ОБРАБОТЧИКА : ПРИНЯЛ - ПЕРЕДАЛ
	 
	 
	if (READ_BIT(USART2->SR, USART_SR_RXNE)) {
		//Если пришли данные по USART
		//USART2->SR &=	~USART_SR_RXNE;
		husart2.rx_buffer[husart2.rx_counter] = USART2->DR; //Считаем данные в соответствующую ячейку в rx_buffer. ВСЕ ПРИШЕДШИЕ ДАННЫЫЕ ЗАПИСЫВАЕМ В МАССИВ
		husart2.rx_counter++; //Увеличим счетчик принятых байт на 1
						

	}
	if (READ_BIT(USART2->SR, USART_SR_IDLE)) {
		//Если прилетел флаг IDLE
		//USART2->SR &= ~USART_SR_IDLE;
		USART2->DR; //Сбросим флаг IDLE
		husart2.rx_len = husart2.rx_counter; //Узнаем, сколько байт получили
		CMSIS_USART_Transmit(USART2, husart2.rx_buffer, husart2.rx_counter); //Отправим в порт то, что прилетело для проверки.
	 
		husart2.rx_counter = 0; //сбросим счетчик приходящих данных
	}
	
}

 
void USART2_Init(void){

	USART2->BRR = 0x23; ////16 MHz: 0x683 для 9600, 0x8B - 115200 baud
	USART2->CR1 = USART_CR1_RE | USART_CR1_TE | USART_CR1_UE | USART_CR1_RXNEIE | USART_CR1_IDLEIE;
	
	MODIFY_REG(GPIOA->MODER, GPIO_MODER_MODER2_0 | GPIO_MODER_MODER3_0, GPIO_MODER_MODER2_1 | GPIO_MODER_MODER3_1); //ALTERNATIVE FUNC PIN2/3 PORTA
	
	GPIOA->AFR[0]	|= (7 << GPIO_AFRL_AFSEL2_Pos) ; // That alt fn is alt 7 for PA2
	GPIOA->AFR[0] 	|= (7 << GPIO_AFRL_AFSEL3_Pos) ; // Alt fn for PA3 is same as for PA2
	
  NVIC_EnableIRQ(USART2_IRQn);
	
	
	
}

									//////////////////////// ФУНКЦИИ ПЕРЕДАЧИ СИМВОЛА/СТРОКИ /////////////////////////////

static void USART2_Send(char chr)   
{
	while(!(USART2->SR & USART_SR_TC));
	USART2->DR = chr;
}

void USART2_Send_String(char* str)
{
	uint8_t i = 0;
	while(str[i]){
		USART2_Send(str[i++]);
	}
}

////////////////////////функция передачи данных////////////////////

void CMSIS_USART_Transmit(USART_TypeDef *USART, uint32_t *data, uint16_t Size) {
	for (uint16_t i = 0; i < Size; i++) {
		while (READ_BIT(USART2->SR, USART_SR_TXE) == 0) ; //Ждем, пока линия не освободится
		USART->DR = *data++; //Кидаем данные  
	}	
}