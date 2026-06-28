/*LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL
 【平    台】北京龙邱智能科技STC32位核心板
 【编    写】龙邱科技
 【E-mail  】chiusir@163.com
 【软件版本】V1.0 版权所有，单位使用请先联系授权
 【相关信息参考下列地址】
 【网    站】http://www.lqist.cn
 【淘宝店铺】http://longqiu.taobao.com
 --------------------------------------------------------------------------------
 【  IDE  】 keil C251 V5.60
 【Target 】 STC32G/STC8051U/AI8051U 32位模式
 【SYS CLK】 40 MHz使用内部晶振
QQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQ*/

#include "include.h"
#include "LQ_ADC.h"

/***********************************************************************************
 * 函 数 名：void ADC_Init(void)
 * 功    能：ADC初始化，管脚初始化和中断模式、使能等设置
 * 参    数：无
 * 返 回 值：无
 * 说    明：    //ADC 端口IO配置，详见ADC_CHx_Pin,  用哪个初始化哪个
 **********************************************************************************/
void ADC_Init(void)
{
    ADC_InitTypeDef ADC_InitStructure; // 结构定义

    ADC_GPIO_Init(ADC_CH9_P01); // ADC 端口IO配置，要初始化的ADC通道和管脚，详见ADC_CHx_Pin
    ADC_GPIO_Init(ADC_CH0_P10);
    ADC_GPIO_Init(ADC_CH1_P11);
    ADC_GPIO_Init(ADC_CH2_P12);
    ADC_GPIO_Init(ADC_CH3_P13);
    ADC_GPIO_Init(ADC_CH4_P14);
    ADC_GPIO_Init(ADC_CH5_P15);

    // ADC配置
    ADC_InitStructure.ADC_SMPduty = 31;                    // ADC 模拟信号采样时间控制, 0~31（注意： SMPDUTY 一定不能设置小于 10）
    ADC_InitStructure.ADC_CsSetup = 0;                     // ADC 通道选择时间控制 0(默认),1
    ADC_InitStructure.ADC_CsHold = 1;                      // ADC 通道选择保持时间控制 0,1(默认),2,3
    ADC_InitStructure.ADC_Speed = ADC_SPEED_2X16T;         // 设置 ADC 工作时钟频率	ADC_SPEED_2X1T~ADC_SPEED_2X16T
    ADC_InitStructure.ADC_AdjResult = ADC_RIGHT_JUSTIFIED; // ADC结果d对齐方式,	ADC_LEFT_JUSTIFIED,ADC_RIGHT_JUSTIFIED
    ADC_Inilize(&ADC_InitStructure);                       // 初始化
    ADC_PowerControl(ENABLE);                              // ADC电源开关, ENABLE或DISABLE
    NVIC_ADC_Init(DISABLE, Priority_0);                    // 断使能, ENABLE/DISABLE; 优先级(低到高)--> Priority_0~Priority_3
}

