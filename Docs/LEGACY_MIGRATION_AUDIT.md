# 去年代码迁移审计

| 去年功能 | 旧文件/函数 | 当前模块 | 迁移方式 | 硬件依赖 | 风险 | 验证方法 |
|---|---|---|---|---|---|---|
| 电磁 5 路读取 | `ld/Inductor.c::Inductance_Original` | `BSP/bsp_emag.c`, `Control/ctrl_line.c` | 迁移 L1/L2/M/R1/R2 结构和 ADC5/4/3/0/1 作为待验证默认映射 | ADC 通道与 5LC 接线 | 通道错序导致方向反 | TEST_MODE 显示每路原始值 |
| 电磁归一化 | `ADC_Unify` min/max 表 | `Control/ctrl_signal.c`, `ctrl_line.c` | 迁移限幅和 1-100 归一化思想，参数集中到 `app_config.h/app_params` | 标定数据 | min/max 过时 | 背景和中心线标定 |
| 差比和差 | `Cha_bi_he_Cha` | `Control/ctrl_line.c` | 迁移动态水平/竖直权重思想，去除全局变量耦合 | 5LC 布局 | 分母过小/异常放大 | 离线数据回放 |
| 速度/转向双环 | `ld/motor.c::Dir_Control`, `CalculateDifferentialDrive`, `PID_Realize` | `Control/ctrl_speed.c`, `ctrl_steering.c`, `ctrl_vehicle.c` | 保留双环和差速思想，增加安全限幅和表面状态速度上限 | 编码器、IMU、推进 | 打滑时编码器不等于车速 | 空载/地面低速测试 |
| 环岛/直角 | `ld/speed.c::Roundabout_Update`, `zhijiao` | `Track/track_features.c`, `track_state_machine.c` | 迁移为赛道特征，不直接控制电机 | 电磁+Yaw | 立体赛道误判 | 离线序列和低速地面验证 |
| 实时节拍 | `ld/ld.c::Timer0_ISR`, `TIM_Inits(Timer0,2)` | `App/app_scheduler.c`, `BSP/bsp_timebase.c` | 保留 2 ms 控制节拍目标，移出 ISR 复杂逻辑 | 定时器 | 周期漂移 | 任务超期遥测 |
| 参数保存 | `Param_Save`, `Param_Load` | `App/app_params.c` | 先静态参数表，EEPROM 持久化待硬件验证后接入 | EEPROM | 写坏参数 | TEST_MODE 参数读写 |
| 启动使能 | `g_enable`, 按键翻转 | `App/app_state_machine.c`, `app_safety.c` | 改为人工授权状态机；上电不自动 RUNNING | KEY/UI | 误启动 | UI 去抖和状态显示 |
| 丢线处理 | `Check_Track_Loss_Comprehensive` | `App/app_safety.c` | 地面丢线停车；墙面/未知姿态进入保压等待 | 负压可用性 | 墙面断吸 | 工装模拟丢线 |
| 负压/无刷 | 去年未发现独立负压控制闭环 | `BSP/bsp_suction.c`, `Control/ctrl_adhesion.c` | 新增安全关键抽象，不从旧代码假设 | ESC/P23/风机 | 误输出 | 示波和固定架 |
