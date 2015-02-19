/**
 ******************************************************************************
 * @file    main.c
 * @author  MCD Application Team
 * @version V4.0.0
 * @date    21-January-2013
 * @brief   Virtual Com Port Demo main file
* Fixes by:
* Company: Tachcard Limited
* Website: http://www.tachcard.com/
* Email: rio@tachcard.com
* Authors: Ilya Ratovsky, Bogdanov Maksim
* Date: 04-05-2014
 */

/* Includes ------------------------------------------------------------------*/
#include "hw_config.h"
#include "usb_lib.h"
#include "usb_desc.h"
#include "usb_pwr.h"

#include <string.h>

extern __IO uint8_t Receive_Buffer[64];
extern __IO uint32_t Receive_length;
extern __IO uint32_t length;
uint8_t Send_Buffer[64];
uint32_t packet_sent = 1;
uint32_t packet_receive = 1;

void print(char * str) {
    while (packet_sent == 0);
    CDC_Send_DATA((unsigned char*) str, strlen(str));
}

void nvic_init(void);
void timer_init(void);
void gpio_init(void);

#define FLASH_PAGE_SIZE    ((uint16_t)0x400)
#define BANK1_WRITE_START_ADDR  ((uint32_t)0x08008000)

volatile FLASH_Status FLASHStatus = FLASH_COMPLETE;

void FLASH_Program(int page, int data) {
    FLASH_UnlockBank1();
    FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);
    FLASHStatus = FLASH_ErasePage(
            BANK1_WRITE_START_ADDR + (FLASH_PAGE_SIZE * page));
    FLASHStatus = FLASH_ProgramWord(
            BANK1_WRITE_START_ADDR + page * FLASH_PAGE_SIZE, data);
    FLASH_LockBank1();
}

void timer_init(void) {
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
    TIM_TimeBaseStructInit(&TIM_TimeBaseStructure);
    TIM_TimeBaseStructure.TIM_Period = 7200;
    TIM_TimeBaseStructure.TIM_Prescaler = 10000;
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV4;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);
    TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);
    TIM_Cmd(TIM3, ENABLE);
}

void nvic_init(void) {
    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    NVIC_EnableIRQ(TIM3_IRQn);
}

void gpio_init(void) {
    GPIO_InitTypeDef GPIO_InitStructure;

    GPIO_StructInit(&GPIO_InitStructure);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_7 | GPIO_Pin_9 | GPIO_Pin_12;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1 | GPIO_Pin_10;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    GPIO_WriteBit(GPIOB, GPIO_Pin_4, Bit_RESET);
    GPIO_WriteBit(GPIOA, GPIO_Pin_10, Bit_SET);

    GPIO_WriteBit(GPIOA, GPIO_Pin_1, Bit_SET);
    GPIO_WriteBit(GPIOB, GPIO_Pin_12, Bit_SET);

}
int count1;
int count2;

int main(void) {

    // alternative func enable
    RCC->APB2ENR |= RCC_APB2ENR_AFIOEN;
    AFIO->MAPR |= AFIO_MAPR_SWJ_CFG_JTAGDISABLE; // disable JTAG

    Set_System();
    Set_USBClock();
    USB_Interrupts_Config();
    USB_Init();

    gpio_init();
    nvic_init();
    timer_init();

    count1 = *((int *) 0x08008000);
    count2 = *((int *) 0x08008400);

    int i;
    while (1) {
        if (bDeviceState == CONFIGURED) {
            CDC_Receive_DATA();
            if ((Receive_length != 0)
                    && ((Receive_Buffer[Receive_length - 2] == '\r')
                            || (Receive_Buffer[Receive_length - 1] == '\r'))) {
                if (strstr((char *) Receive_Buffer, "help") != NULL) {
                    print("available Commands\r\n");
                    print("LED1 (LED1 ON; LED1 OFF)\r\n");
                    print("RELAY (RELAY ON; RELAY OFF) default - ON\r\n");
                    print("KEY1 (KEY1 ON; KEY1 OFF) default - OFF\r\n");
                    print("KEY2 (KEY2 ON; KEY2 OFF) default - OFF\r\n");
                    print("USB (USB ON; USB OFF) default - ON\r\n");
                    print("C1 (C1 RES; C1 SET 60; C1 SET 0 ) save in flash \r\n");
                    print("C2 (C2 RES; C2 SET 60; C2 SET 0 ) save in flash \r\n");

                } else if (strstr((char *) Receive_Buffer, "LED1 ON") != NULL) {
                    GPIO_WriteBit(GPIOB, GPIO_Pin_4, Bit_SET);
                    print("OK\r\n");
                } else if (strstr((char *) Receive_Buffer, "LED1 OFF") != NULL) {
                    GPIO_WriteBit(GPIOB, GPIO_Pin_4, Bit_RESET);
                    print("OK\r\n");
                } else if (strstr((char *) Receive_Buffer, "RELAY ON") != NULL) {
                    GPIO_WriteBit(GPIOB, GPIO_Pin_7, Bit_SET);
                    print("OK\r\n");
                } else if (strstr((char *) Receive_Buffer, "RELAY OFF") != NULL) {
                    GPIO_WriteBit(GPIOB, GPIO_Pin_7, Bit_RESET);
                    print("OK\r\n");
                } else if (strstr((char *) Receive_Buffer, "KEY1 ON") != NULL) {
                    GPIO_WriteBit(GPIOB, GPIO_Pin_12, Bit_SET);
                    print("OK\r\n");
                } else if (strstr((char *) Receive_Buffer, "KEY1 OFF") != NULL) {
                    GPIO_WriteBit(GPIOB, GPIO_Pin_12, Bit_RESET);
                    print("OK\r\n");
                } else if (strstr((char *) Receive_Buffer, "KEY2 ON") != NULL) {
                    GPIO_WriteBit(GPIOA, GPIO_Pin_1, Bit_SET);
                    print("OK\r\n");
                } else if (strstr((char *) Receive_Buffer, "KEY2 OFF") != NULL) {
                    GPIO_WriteBit(GPIOA, GPIO_Pin_1, Bit_RESET);
                    print("OK\r\n");
                } else if (strstr((char *) Receive_Buffer, "USB ON") != NULL) {
                    GPIO_WriteBit(GPIOB, GPIO_Pin_9, Bit_RESET);
                    print("OK\r\n");
                } else if (strstr((char *) Receive_Buffer, "USB OFF") != NULL) {
                    GPIO_WriteBit(GPIOB, GPIO_Pin_9, Bit_SET);
                    print("OK\r\n");
                } else if (strstr((char *) Receive_Buffer, "C1 RES") != NULL) {
                    count1 = *((int *) 0x08008000);
                    print("OK\r\n");
                } else if (strstr((char *) Receive_Buffer, "C2 RES") != NULL) {
                    count2 = *((int *) 0x08008400);
                    print("OK\r\n");
                } else if (strstr((char *) Receive_Buffer, "C1 SET") != NULL) {
                    i = 7;
                    count1 = 0;
                    while (Receive_Buffer[i] >= '0' && Receive_Buffer[i] <= '9') {
                        count1 = count1 * 10;
                        count1 += Receive_Buffer[i] - '0';
                        i++;
                    }
                    if (count1 == 0)
                        count1 = -1;
                    FLASH_Program(0, count1);
                    print("OK\r\n");
                } else if (strstr((char *) Receive_Buffer, "C2 SET") != NULL) {
                    i = 7;
                    count2 = 0;
                    while (Receive_Buffer[i] >= '0' && Receive_Buffer[i] <= '9') {
                        count2 = count2 * 10;
                        count2 += Receive_Buffer[i] - '0';
                        i++;
                    }
                    if (count2 == 0)
                        count2 = -1;
                    FLASH_Program(1, count2);
                    print("OK\r\n");
                } else {
                    print("unknown command \"");
                    print((char *) Receive_Buffer);
                    print("\" \r\nuse \"help\"\r\n");
                }
                Receive_length = 0;

            }

        }
    }
}

#ifdef USE_FULL_ASSERT
/*******************************************************************************
 * Function Name  : assert_failed
 * Description    : Reports the name of the source file and the source line number
 *                  where the assert_param error has occurred.
 * Input          : - file: pointer to the source file name
 *                  - line: assert_param error line source number
 * Output         : None
 * Return         : None
 *******************************************************************************/
void assert_failed(uint8_t* file, uint32_t line)
{
    /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

    /* Infinite loop */
    while (1)
    {}
}
#endif

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
