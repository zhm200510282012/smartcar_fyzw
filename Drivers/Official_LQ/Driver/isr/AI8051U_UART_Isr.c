/*---------------------------------------------------------------------*/
/* ------------------- Web: www.STCAI.com -----------------------------*/
/*---------------------------------------------------------------------*/

#include "AI8051U_UART.h"
#include "include.h"
bit B_ULinRX1_Flag;
bit B_ULinRX2_Flag;

//========================================================================
// 函数: UART1_ISR_Handler
// 描述: UART1中断函数.
// 参数: none.
// 返回: none.
// 版本: V1.0, 2020-09-23
//========================================================================
#ifdef UART1
void UART1_ISR_Handler (void) interrupt UART1_VECTOR
{
	u8 Status;

	if(RI)
	{
		RI = 0;

		//--------USART LIN---------------
		Status = USARTCR5;
		if(Status & 0x02)     //if LIN header is detected
		{
			B_ULinRX1_Flag = 1;
		}

		if(Status & 0xc0)     //if LIN break is detected / LIN header error is detected
		{
			COM1.RX_Cnt = 0;
		}
		USARTCR5 &= ~0xcb;    //Clear flag
		//--------------------------------

        if(COM1.RX_Cnt >= COM_RX1_Lenth)	COM1.RX_Cnt = 0;
        RX1_Buffer[COM1.RX_Cnt++] = SBUF;
        COM1.RX_TimeOut = TimeOutSet1;
	}

	if(TI)
	{
		TI = 0;

        #if(UART_QUEUE_MODE == 1)   //判断是否使用队列模式
		if(COM1.TX_send != COM1.TX_write)
		{
		 	SBUF = TX1_Buffer[COM1.TX_send];
			if(++COM1.TX_send >= COM_TX1_Lenth)		COM1.TX_send = 0;
		}
		else	COM1.B_TX_busy = 0;
        #else
        COM1.B_TX_busy = 0;     //使用阻塞方式发送直接清除繁忙标志
        #endif
	}
}
#endif

//========================================================================
// 函数: UART2_ISR_Handler
// 描述: UART2中断函数.
// 参数: none.
// 返回: none.
// 版本: V1.0, 2020-09-23
//========================================================================
#ifdef UART2
void UART2_ISR_Handler (void) interrupt UART2_VECTOR
{
	u8 Status;

	if(S2RI)
	{
		CLR_RI2();

		//--------USART LIN---------------
		Status = USART2CR5;
		if(Status & 0x02)     //if LIN header is detected
		{
			B_ULinRX2_Flag = 1;
		}

		if(Status & 0xc0)     //if LIN break is detected / LIN header error is detected
		{
			COM2.RX_Cnt = 0;
		}
		USART2CR5 &= ~0xcb;   //Clear flag
		//--------------------------------

        if(COM2.RX_Cnt >= COM_RX2_Lenth)	COM2.RX_Cnt = 0;
        RX2_Buffer[COM2.RX_Cnt++] = S2BUF;
        COM2.RX_TimeOut = TimeOutSet2;
	}

	if(S2TI)
	{
		CLR_TI2();

        #if(UART_QUEUE_MODE == 1)   //判断是否使用队列模式
		if(COM2.TX_send != COM2.TX_write)
		{
		 	S2BUF = TX2_Buffer[COM2.TX_send];
			if(++COM2.TX_send >= COM_TX2_Lenth)		COM2.TX_send = 0;
		}
		else	COM2.B_TX_busy = 0;
        #else
        COM2.B_TX_busy = 0;     //使用阻塞方式发送直接清除繁忙标志
        #endif
	}
}
#endif

//========================================================================
// 函数: UART3_ISR_Handler
// 描述: UART3中断函数.
// 参数: none.
// 返回: none.
// 版本: V1.0, 2020-09-23
//========================================================================
#ifdef UART3

#if (1 == ENABLE_SBUS)

#include "LQ_SBUS.h"
unsigned char sbuff;  //SBUS缓冲
unsigned char num3;
extern unsigned char SReadBuff[64];
#endif
void UART3_ISR_Handler (void) interrupt UART3_VECTOR
{
	if(S3RI)
	{
		CLR_RI3();

        if(COM3.RX_Cnt >= COM_RX3_Lenth)	COM3.RX_Cnt = 0;
//        RX3_Buffer[COM3.RX_Cnt++] = S3BUF;
#if (1 == ENABLE_SBUS)
        /* SBUS START 用户代码 */
        sbuff = (u8)S3BUF;
        if(sbus_Rec_Fini_Flag == 1)       //可以接收
        {
            if (num3 == 0 && sbuff == 0x0F)   //帧头
            {
                SReadBuff[num3] = sbuff;
                num3++;
            }
            else if(num3 != 0)
            {
//                LED_Ctrl(LED0, RVS);        // LED闪烁 指示程序运行状态
                SReadBuff[num3] = sbuff;
                num3++;
                if (num3 > 24)              //接收数据超过24，表示一帧接收完成
                {
                    sbus_Rec_Fini_Flag = 0;   //停止接收

                    if (SReadBuff[0] == 0x0F && SReadBuff[24] == 0x00 && num3 == 25)    //接受完一帧数据
                    {
                        UpDate_SBUS();      //数据解析
//                        LED_Ctrl(LED0, RVS);
                    }
                    num3 = 0;               //清空
                }

            }
        }
        /* SBUS END*/
	#endif
        COM3.RX_TimeOut = TimeOutSet3;
	}

	if(S3TI)
	{
		CLR_TI3();

        #if(UART_QUEUE_MODE == 1)   //判断是否使用队列模式
		if(COM3.TX_send != COM3.TX_write)
		{
		 	S3BUF = TX3_Buffer[COM3.TX_send];
			if(++COM3.TX_send >= COM_TX3_Lenth)		COM3.TX_send = 0;
		}
		else	COM3.B_TX_busy = 0;
        #else
        COM3.B_TX_busy = 0;     //使用阻塞方式发送直接清除繁忙标志
        #endif
	}
}
#endif

//========================================================================
// 函数: UART4_ISR_Handler
// 描述: UART4中断函数.
// 参数: none.
// 返回: none.
// 版本: V1.0, 2020-09-23
//========================================================================
#ifdef UART4

#if (1 == ENABLE_GPS)
#include "LQ_BDGPS.h"       //北斗数据接收，简单处理 Save_Data在 LQ_BDGPS.c开头定义
unsigned char GPS_buf;    //接收数据缓字符缓存
unsigned char GPS_ReadBuff[128];
#endif
void UART4_ISR_Handler (void) interrupt UART4_VECTOR
{
	if(S4RI)
	{
        static u8 num1;
		CLR_RI4();

        if(COM4.RX_Cnt >= COM_RX4_Lenth)	COM4.RX_Cnt = 0;
//        RX4_Buffer[COM4.RX_Cnt++] = S4BUF;
#if (1 == ENABLE_GPS)
        /* GPS_BD串口接收部分start */

        GPS_buf = (u8)S4BUF;
        if(GPS_buf == '\n' || GPS_buf == '\r')   //完成一帧数据接收
        {
            strcpy((char*)Save_Data.GPS_Buffer,(const char*)GPS_ReadBuff);
            Save_Data.isGetData=1;
            num1 = 0;
//            LED_Ctrl(LED1,RVS);// LED翻转闪烁
        }
        else
        {
//            LED_Ctrl(LED0,RVS);// LED翻转闪烁
            GPS_ReadBuff[num1++] = GPS_buf;
        }
        /* GPS_BD串口接收部分end */
#endif
        COM4.RX_TimeOut = TimeOutSet4;
	}

	if(S4TI)
	{
		CLR_TI4();

        #if(UART_QUEUE_MODE == 1)   //判断是否使用队列模式
		if(COM4.TX_send != COM4.TX_write)
		{
		 	S4BUF = TX4_Buffer[COM4.TX_send];
			if(++COM4.TX_send >= COM_TX4_Lenth)		COM4.TX_send = 0;
		}
		else	COM4.B_TX_busy = 0;
        #else
        COM4.B_TX_busy = 0;     //使用阻塞方式发送直接清除繁忙标志
        #endif
	}
}
#endif
