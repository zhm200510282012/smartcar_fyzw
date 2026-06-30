# Active Element Count Burst 检测

右直角、环岛、十字等共享入口不再靠单纯误差突变猜测，而是先检测“五路电磁有效元素数量突增”：

```text
active channel: norm[i] >= ELEMENT_ACTIVE_ENTER_NORM
release channel: norm[i] <= ELEMENT_ACTIVE_EXIT_NORM
burst: active_count >= ELEMENT_BURST_MIN_ACTIVE
       && active_count - baseline_count >= ELEMENT_BURST_MIN_RISE
       && line_quality >= ELEMENT_BURST_MIN_QUALITY
       && confirm_ms >= ELEMENT_BURST_CONFIRM_MS
```

`baseline_count` 只在普通线状态下更新，默认 `ELEMENT_BURST_BASELINE_MAX_ACTIVE=2`。burst 释放后进入 `ELEMENT_BURST_COOLDOWN_MS` 冷却，避免同一个元素被重复触发。

## 默认安全边界

当前 `ELEMENT_SPECIAL_DIRECTION_CONFIGURED=0`，因此软件只输出：

```text
TRACK_SPECIAL_ELEMENT_CANDIDATE
```

不会在未确认实物方向前把共享入口冒认成右直角、环岛或十字。确认五路通道方向、赛道元素方向和实车表现后，才允许把方向配置打开并细分候选。

## 调车观察项

- 手持车体横扫电磁线，确认 A/B/C/D/E 能量按左到右顺序变化。
- 普通直线通常只有 1 到 2 个 active channel。
- 特殊元素入口应出现 active channel 数量短时上升到 3 个或更多。
- 如果普通线也频繁 burst，只调 `ELEMENT_ACTIVE_ENTER_NORM`、`ELEMENT_ACTIVE_EXIT_NORM`、`ELEMENT_BURST_MIN_QUALITY`，不要改状态机。
