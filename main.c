/********************************************************************
        ПРОГРАММА, ДЕМОНСТРИРУЮЩАЯ РАБОТУ НЕЗАВИСИМОГО СТОРОЖЕВОГО ТАЙМЕРА IWDG.
				НАСТРОЙКИ ТАЙМЕРА: 
													ЧАСТОТА LSI, НА КОТОРОЙ РАБТАЕТ IWDG = 32КГц.
													УСТАНАВЛИВАЕМ ПРЕДДЕЛИТЕЛЬ В ЗНАЧЕНИЕ 64, ДЛЯ ЭТОГО, ПРЕДВАРИТЕЛЬНО ЗАПИСАВ КЛЮЧЕВОЕ СЛОВО ДЛЯ ВОЗМОЖНОСТИ ИЗМЕНЕНИЯ
													НАСТРОЕК В РЕКИСТР IWDG_KR(0х5555), ЗАПИСЫВАЕМ В IWDG_PSC - 100. ПОЛУЧАЕМ ЧАСТОТУ РАБОТЫ IWDG = 32000/64 = 500Гц.
													УСТАНАВЛИВАЕМ ЗНАЧЕНИЕ В РЕГИСТР СЧЁТА ТАЙМЕРА IWDG_RLR, В НАШЕМ СЛУЧАЕ ЭТО 500. ТАКИМ ОБРАЗОМ, С УЧЁТОМ ЗНАЧЕНИЯ ЧАСТОТЫ
													В 500Гц, ТАЙМЕР ДОСЧИТАЕТ ОТ 500 ДО 0 ЗА 1 СЕКУНДУ, ЗА ЧЕМ ПОСЛЕДУЕТ RESET.
													В ЦИКЛЕ while ОТПРАВЛЯЕМ В ТЕРМИНАЛЬНУЮ ПРОГРАММУ ПО USART ЧИСЛА ОТ 0 ДО 100 С ИНТЕРВАЛОМ В 500 МИЛЛИСЕКУНД. 
													ЕСЛИ НЕ СДЕЛАТЬ REFRESH(ВОССТАНОВЛЕНИЕ НАЧАЛЬНОГО ЗНАЧЕНИЯ СЧЁТА ТАЙМЕРА В РЕГИСТРЕ RLR) ВОВРЕМЯ, ТО ЕСТЬ В ПРОМЕЖУТКЕ
													0 ДО 1 СЕКУНДЫ(ПОКА ТАЙМЕР НЕ ДОСЧИТАЕТ ОТ 500 ДО 0 ЗА 1 СЕКУНДУ), ТО ПРОИЗОЙДЁТ RESET. ПОЭТОМУ ДЕЛАЕМ REFRESH КАЖДЫЕ 0,5 СЕК.
													И СЧИТАЕМ ДО 100. 
													ТАКЖЕ В ТЕРМИНАЛЕ МОЖНО ОТПРАВИТЬ В ЧТО-ТО В ЧИП, И ПОЛУЧИТЬ "ЭХО-ПЕРЕДАЧУ" ОБРАТНО.

*/


#include <stm32f4xx.h>
#include <stm32f411xe.h>
#include "USART.c"
 

 
 int RCC_Init(void);
 void GPIO_Init(void);
 void TIM3_Init(void);
 void delay_Ms(int mS);
 void TIM3_IRQHandler(void);
 void IWDG_Init(void);

static __IO int StartUpCounter;
static int myTicks = 0;





void TIM3_IRQHandler(void)
   {
		 if(READ_BIT(TIM3->SR, TIM_SR_UIF) )    //проверяем флаг прерывания в Status Reg
		 {
             myTicks++;
		    
				CLEAR_BIT(TIM3->SR, TIM_SR_UIF);				//очистка бита uif статусного регистра для выхода из прерывания после обработки
			} }
		

 int main(void){
	 
	 RCC_Init();
	 GPIO_Init();
	 TIM3_Init();
   IWDG_Init();
	 USART2_Init();
	 __enable_irq();
	 

	 USART2_Send_String("start\r");

	 while(1){
		 for(uint8_t i = 0; i<=100; i++){
		 USART2->DR = i;                      
		 delay_Ms(500);
		 IWDG->KR = 0xAAAA;      							//REFRESH КАЖДЫЕ 0,5 СЕК. IWDG СЧИТАЕТ ОТ 500 ДО 0 ЗА 1 СЕКУ, ТАК КАК ЧАСТОТА IWDG = 500 Hz,
		 }                       							//ЕСЛИ ОН ДОСЧИТАЕТ ДО 0 - БУДЕТ RESET. ДОСЧИТАТЬ НЕ УСПЕВАЕТ, А ОБНОВЛЯЕТСЯ КАЖДЫЕ 0,5 СЕКИ

		 }

	 }
 

  int RCC_Init(void){
		
	   RCC->CR |= RCC_CR_HSION;
	   while(READ_BIT(RCC->CR, RCC_CR_HSIRDY) == 0){}
		 
	   CLEAR_BIT(RCC->CR, RCC_CR_HSEBYP);
			 
		 SET_BIT(RCC->CR, RCC_CR_HSEON);
					//ожидание флага готовности НЕ по-простому, на случай выхода из строя КВАРЦА HSE
				  //Ждем успешного запуска или окончания тайм-аута
				for(StartUpCounter=0; ; StartUpCounter++)
				{
			     //Если успешно запустилось, то 
			     //выходим из цикла
					if(RCC->CR & (1<<RCC_CR_HSERDY_Pos))
						break;
		        //Если не запустилось, то отключаем все, что включили и возвращаем ошибку
					if(StartUpCounter > 0x1000)
					{
						RCC->CR &= ~RCC_CR_HSEON; //Останавливаем HSE
						return 1;
					}
				}
				 
				//ожидание флага готовности по-простому
	      //while(READ_BIT(RCC->CR, RCC_CR_HSERDY) == 0){}
			 
	
        SET_BIT(RCC->CR, RCC_CR_CSSON); //Включим CSS 
				MODIFY_REG(RCC->CFGR, RCC_CFGR_SW, RCC_CFGR_SW_HSE);	
				MODIFY_REG(RCC->CFGR, RCC_CFGR_HPRE, RCC_CFGR_HPRE_DIV1); //AHB Prescaler /1/ НА AHB ТАКАЯ ЖЕ ЧАСТОТА КАК НА ЯДРЕ
				MODIFY_REG(RCC->CFGR, RCC_CFGR_PPRE1, RCC_CFGR_PPRE1_DIV2); //APB1 Prescaler /2, т.к. PCLK1 max 50MHz
				MODIFY_REG(RCC->CFGR, RCC_CFGR_PPRE2, RCC_CFGR_PPRE2_DIV1); //APB2 Prescaler /1. Тут нас ничего не ограничивает. 100MHz max 
				MODIFY_REG(FLASH->ACR, FLASH_ACR_LATENCY, FLASH_ACR_LATENCY_2WS);   //ВАЖНО! These bits represent the ratio of the CPU clock period to the Flash memory access time. 2wS = 2 wait states
	
						RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOCEN;
						RCC->APB1ENR |= RCC_APB1ENR_TIM3EN | RCC_APB1ENR_USART2EN;
						RCC->CSR |= RCC_CSR_LSION;
						
						return 0;
		}
	
  void GPIO_Init(void){
		GPIOA->MODER |= GPIO_MODER_MODE5_0;                 //LED
		GPIOA->MODER &= ~GPIO_MODER_MODE5_1;
		}
	
		
	void TIM3_Init(void)
 {
	
	 WRITE_REG(TIM3->PSC, 79);				//800-ПРИ HSE = 8mHZ; устанавливаем делитель на 1599, фоормула расчета частоты таймера х = F(частота проца, в нашем случае это 16Мгц)/значение . записанное в регистр предделителя TIMx_PSC(в нашем случае это 1599)+1 = 16000000/1600 = 10000 тактов в секунду
																			// WRITE_REG - это макрос для записи числа в регистр, он прописан в файле для ядра stm32f4xx.h(если это серия f4) б или аналогичном для других ерий мк
	 WRITE_REG(TIM3->ARR, 99);       //чтобы получить время счета таймера в 1 секунду записываем в регистр AUTO RELOAD REG число 10000. 1 секунда получается так: частота счета таймера равна 10000 тактов в секунду
																			//потому как частота МК равна 16МГцб, а Prescaler равен 1599+1, и в итоге частота равна 10000. Когда мы записываем в ARR 10000, то таймер считает до 10000 с частотой 10000 Гц. время счета будет 1 секунда, ЕСЛИ ЗАПИСЫВАЕМ 10, ТО ВРЕМЯ СЧЕТА 1МС
	 TIM3->DIER |= TIM_DIER_UIE;
	 
	 TIM3->CR1 |= TIM_CR1_URS;         //Only counter overflow/underflow generates an update interrupt or DMA request if enabled.
	 TIM3->EGR |= TIM_EGR_UG;          //ВАЖНО! РЕИНИЦИАЛИЗАЦИЯ ТАЙМЕРА И ОБНОВЛЕНИЕ РЕГИСТРОВ. ПОСЛЕ АКТИВАЦИИ ТАМЕРА СЧЁТЧИК НАЧИНАЕТСЧИТАТЬ С НУЛЯ! С ЭТИМ БИТОМ ВСЁ РАБОТАЕТ КОРРЕКТНО ТОЛЬКО В ПАРЕ С БИТОМ CR1_URS
   //TIM3->CR1 |= TIM_CR1_CEN;
	 NVIC_EnableIRQ (TIM3_IRQn);        //разрешение прерывания для таймера 3 , IRQ - Interrupt request
	 
 }
	

 void delay_Ms(int mS)  //милисек
 {
	 TIM3->CR1 |= TIM_CR1_CEN;
	 myTicks = 0;
	 while(myTicks < mS);
	 TIM3->CR1 &= ~TIM_CR1_CEN;
 }
 
  void IWDG_Init(void)
  {
   IWDG->KR = 0x5555;
	 IWDG->RLR = 500; //IWDG_RLR_RL&1000;
	 MODIFY_REG(IWDG->PR, IWDG_PR_PR, IWDG_PR_PR_2);
	// IWDG->PR = 4;               //PRESCALER = 64 => F(IWDG) = 40000/64 = 625Hz
	
	 IWDG->KR = 0xCCCC;            //ЗАПУСК ТАЙМЕРА
	 IWDG->KR = 0xAAAA;            //ОБЯЗАТЕЛЬНО ПЕРВЫЙ РЕФРЕШ ТАЙМЕРА
  }
