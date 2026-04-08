# 控制器框架

控制器框架允许您定义位置目标如何转化为关节扭矩。每个关节都可以有一个控制器组件，该组件以物理速率拦截控制管线。

## 控制流

```
ZMQ / 蓝图 → 在 UMjActuator 上 SetNetworkControl(target)
                         │
         AMjArticulation::ApplyControls()
                         │
              ┌──────────┴──────────┐
              │找到控制器?          │ 没有控制器
              │                     │
     Controller.ComputeAndApply()  d->ctrl[id] = target
              │                     (direct write)
         自定义控制律
     在每个物理步骤中运行
```

如果没有控制器组件，`ApplyControls` 会将原始 `NetworkValue` 写入 `d->ctrl` — 适用于 MuJoCo 在内部处理 PD 的位置执行器。


## 内置控制器

| 控制器 | 用例 |
|-----------|----------|
| **UMjPassthroughController** | 直接写入（与不使用控制器相同，但显式写入） |
| **UMjPDController** | 位置目标 → 物理速率下的PD扭矩。适用于运动执行器。 |

## UMjPDController

每个物理步计算`torque = Kp * (target - qpos) - Kv * qvel`。目标值被限制在关节活动范围内。与 MJCF 中的 `<motor>` 执行器一起使用。

**属性：**

| 属性 | 类型 | 描述 |
|----------|------|-------------|
| `Kp` | `TArray<float>` | 每个执行器一个比例增益 |
| `Kv` | `TArray<float>` | 微分增益 |
| `TorqueLimits` | `TArray<float>` | 对称扭矩范围 |
| `DefaultKp/Kv/TorqueLimit` | `float` | 未设置条目的备用方案 |

增益可以通过 Python 以 ZMQ `set_gains` 消息（名称键 JSON）的形式推送。


## UMjKeyframeController

一个基于预设的姿态序列器。与 `UMjPDController` 和 `UMjPassthroughController` 不同，这是一个 `UActorComponent`（并非派生自 `UMjArticulationController`）——它通过 `SetActuatorControl` 以游戏节拍速率写入执行器目标，而不是拦截物理速率控制管线。

将其添加到 `AMjArticulation` 动作者中。它会在 `BeginPlay` 时自动解析所有者关节。


**FMjKeyframePose 结构体：**

| 字段 | 类型 | 描述 |
|-------|------|-------------|
| `Name` | `FString` | 显示名称（例如，“Reach”） |
| `Targets` | `TArray<float>` | 执行器目标，按与关节执行器列表匹配的顺序排列 |
| `HoldTime` | `float` | 保持这个姿势几秒钟后再继续前进 |
| `BlendTime` | `float` | 从前一个姿势插值所需的秒数（缓入缓出） |

**预设系统：** 调用 `GetPresetNames()` 获取可用预设，然后调用 `LoadPreset(Name)` 填充 `Keyframes` 数组。在“详细信息(Details)”面板中，从`Preset(预设)`下拉列表中选择预设，并勾选 `bLoadPreset` 以应用预设。

**内置预设：** Franka (伸手-抓握-抬起, Wave), UR5e (拾取-放置, Sweep), Spot (FR Knee Cycle, Wave, Sit), ANYmal (坐下，重心偏移), Go2 (站立-蹲伏，伸展)。

**属性：**

| 属性 | 默认值 | 描述 |
|----------|---------|-------------|
| `bAutoPlay` | `true` | 在 BeginPlay 时候开始游戏 |
| `StartDelay` | `0.5s` | 第一个关键帧之前的延迟 |
| `bLoop` | `true` | 循环回到最后一个关键帧后的第一个关键帧 |
| `Preset` | empty | 选定的预设名称 |

**操作：** `Play()`, `Stop()`, `GoToKeyframe(Index)`, `LoadPreset(Name)`.

## 每个关节的控制源

管理器拥有一个全局 `EControlSource`（UI 或 ZMQ）。每个 `AMjArticulation` 都可以使用自己的 `ControlSource` 字段（`uint8`：0 = ZMQ，1 = UI）覆盖此全局 `EControlSource`。这允许在同一场景中实现混合控制——一些机器人由控制面板驱动，另一些则由外部策略驱动。

## 编写自定义控制器

派生自 `UMjArticulationController` 并重写 `ComputeAndApply` ：

```cpp
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class UMyController : public UMjArticulationController
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere)
    float MyGain = 50.0f;

    virtual void ComputeAndApply(mjModel* m, mjData* d, uint8 Source) override
    {
        for (int32 i = 0; i < Bindings.Num(); ++i)
        {
            const FActuatorBinding& B = Bindings[i];
            float Target = B.Component->ResolveDesiredControl(Source);
            float Pos = (float)d->qpos[B.QposAddr];
            float Vel = (float)d->qvel[B.QvelAddr];

            float Torque = MyGain * (Target - Pos);
            d->ctrl[B.ActuatorMjID] = (mjtNum)Torque;
        }
    }
};
```

`Bindings` 数组由 `Bind()` 自动填充——每个条目将执行器映射到其关节的 qpos/qvel 地址。


## 绑定

`Bind()` 函数在 MuJoCo 模型编译完成后运行一次。它通过 `m->actuator_trnid` 解析每个执行器的关节传动，并存储其 qpos/qvel 地址。自由关节和球形关节将被跳过。

绑定操作按执行器 MjID 排序，因此数组索引与 Python 发现执行器的顺序一致。


## 线程安全

`ComputeAndApply` 函数在异步物理线程上运行。通过 `ResolveDesiredControl()` 函数（原子操作）读取执行器值。增益数组在更新期间可以接受良性撕裂读取——在 x86/ARM 架构上，单个浮点数写入操作是原子的。

!!! 笔记
    撕裂读取（Torn read）是指在多线程编程中，当一个线程正在写入一个变量时，另一个线程读取该变量，可能会导致读取到不一致的数据。例如，当一个线程将一个长整型变量赋值为某个值时，另一个线程可能只读取到该值的一部分，从而导致读取结果不完整或错误。这种现象通常发生在32位操作系统中，因为长整型变量的读取需要执行两次MOV指令，无法在一个时间内完成。为了避免这种问题，开发者可以使用线程同步机制，如锁或原子操作，确保在多线程环境中对共享资源的安全访问。


## 关节的补充说明

1. 打开您的 AMjArticulation 蓝图
2. **添加组件** → 搜索您的控制器（例如，`MjPDController`）
3. 在“详细信息(Details)”面板中配置属性（或通过 ZMQ 推送）
4. 当 `bEnabled = true` 且 `IsBound()` 时，控制器会自动激活。 
