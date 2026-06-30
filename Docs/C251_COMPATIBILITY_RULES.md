# C251 兼容规则

本工程真实目标为 Keil C251。为避免 C251 前端和后端兼容性问题，项目自有源码遵守以下规则：

1. 不在项目自有源码中包含旧路径 `#include "AI8051U.h"`；需要官方寄存器或向量定义时使用工程内官方头文件路径和已验证的 `AI8051U_Timer.h`、`AI8051U_NVIC.h`。
2. 不使用 Timer11 的普通 C `interrupt` ISR。Timer11 是扩展中断，本工程当前不引入扩展 ISR/汇编跳转机制。
3. 普通 C ISR 的 `interrupt` 向量值必须在 C251 普通范围内；Timer0/Timer1 分别使用 `TMR0_VECTOR`、`TMR1_VECTOR`。
4. Timer1 只做 1000 Hz 单通道电磁 ADC tick；Timer0 只做 1000 Hz 毫秒时基，并按 `TIMEBASE_TICK_HZ / CONTROL_PID_HZ` 分频运行 200 Hz 控制 tick。
5. 真实 C251 分支的局部变量声明放在函数或嵌套代码块开头，不放在可执行语句之后。
6. 不把 `bit` 用作普通变量名；`bit` 是 C51/C251 保留类型关键字。
7. `BSP/bsp_emag.c` 不按值返回大结构体样本；`bsp_emag_read()`、`bsp_emag_sample_from_frame()`、`bsp_emag_last_sample()` 都使用输出参数。
8. 电磁三缓冲必须保留 `front/write/reader` 三个索引；临界区只保护 index 锁定，不长时间关闭 Timer1 中断。
9. 不修改 `BSP/bsp_imu.c` 中的用户未提交差异，除非用户明确要求。
10. 不通过关闭 warning、空函数或伪实现绕过真实 C251 编译问题。
