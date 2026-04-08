# 使用蓝图和 C++ 进行脚本编写

Unreal Robotics Lab 中的每个 MuJoCo 组件都是普通的虚幻引擎组件。关节是 Actor。您可以像操作其他 Actor 或组件一样与它们交互：获取引用、调用函数、绑定事件。除非您需要，否则无需直接操作 MuJoCo。

有关完整的类 API 列表，请参阅 [自动生成的 API 参考](../api/index.md) 。

---

## 获取参考

`AMjArticulation` 本质上就是一个动作者。获取引用的方式与通常相同：

**蓝图：** 获取所有类的参与者(Actor)、从命中结果进行类型转换，或存储引用变量。

**C++:**
```cpp
AAMjManager* Manager = AAMjManager::GetManager();
AMjArticulation* Robot = Manager->GetArticulation("MyRobot");
```

从管理器中，您可以按名称查找关节，或获取所有关节：

**蓝图：** **获取管理器** → **获取关节** (名称) / **获取所有关节**

**C++:**
```cpp
TArray<AMjArticulation*> All = Manager->GetAllArticulations();
```

---

## 使用组件

一旦你创建了一个关节，它的 MuJoCo 组件就只是子组件。你可以按名称或数组访问它们：

**蓝图：** **获取执行器** (名称), **获取关节** (名称), **获取传感器** (名称) — 或者对于数组 **获取执行器**, **获取关节**, **获取传感器**, **获取刚体**。你也可以直接从蓝图中的组件树拖拽组件。

**C++:**
```cpp
UMjActuator* Act = Robot->GetActuator("shoulder");
TArray<UMjJoint*> Joints = Robot->GetJoints();
UMjBody* Body = Robot->GetBodyByMjId(3);  // 按编译 ID
```

---

## 控制执行器

**蓝图：** 在关节上**设置执行器控制** (名称, 值)。 使用 Wire **获取游戏时间（以秒为单位）** → **正弦** → 将**执行器控制**设置为事件节拍，即可得到一个简单的正弦波。 

使用**获取执行器范围(Get Actuator Range)**获取 `Vector2D`（最小值，最大值）以限制范围。

**C++:**
```cpp
Robot->SetActuatorControl("shoulder", 1.57f);
FVector2D Range = Robot->GetActuatorRange("shoulder");
```

---

## 读取传感器

**蓝图：**

- **获取传感器标量(Get Sensor Scalar)** (名称) → Float — 适用于 1D 传感器 (触摸, 关节位置, 时钟)
- **获取传感器读数** (名称) → Float 数组 — 适用于向量传感器（力、加速度）
- **获取关节角度** (名称) → Float — 关节位置的快捷方式

**C++:**
```cpp
float Touch = Robot->GetSensorScalar("fingertip_touch");
TArray<float> Force = Robot->GetSensorReading("wrist_force");
float Angle = Robot->GetJointAngle("elbow");
```

---

## 碰撞响应

`AMjArticulation` 具有 **碰撞时(On Collision)**事件分发器。触发条件包括：自身几何体`SelfGeom` (UMjGeom*)、其他几何体`OtherGeom` (UMjGeom*) 和接触位置`ContactPos` (FVector)。

**蓝图：** 选择关节参考 → **分配碰撞时事件(Assign On Collision)** → 连接到您的逻辑。

**C++:**
```cpp
Robot->OnCollision.AddDynamic(this, &AMyActor::HandleCollision);

void AMyActor::HandleCollision(UMjGeom* SelfGeom, UMjGeom* OtherGeom, FVector ContactPos)
{
    // 停止 gripper、播放效果等。
}
```

---

## 仿真生命周期

一切都归因于管理器

| 节点 | 作用 |
|------|-------------|
| **Set Paused** (bool) | 暂停/恢复物理线程 |
| **Reset Simulation** | 零位置、速度、时间 |
| **Step Sync** (N) | 同步推进 N 步（强化学习式的循环） |
| **Get Sim Time** | 当前模拟时钟 |

**C++:**
```cpp
Manager->SetPaused(true);
Manager->StepSync(10);
Manager->ResetSimulation();
float Time = Manager->GetSimTime();
```

---

## 快照

保存并恢复完整的模拟状态：

**蓝图：** **捕获快照** → 存储在变量中 → 稍后**恢复快照**。

**C++:**
```cpp
UMjSimulationState* Snap = Manager->CaptureSnapshot();
// ... try something ...
Manager->RestoreSnapshot(Snap);
```

保存多个快照，用于 A/B 测试、检查点或撤销。

---

## 关键帧 API

在 `AMjArticulation`:

| 节点 | 作用 |
|------|-------------|
| `ResetToKeyframe(Name)` | 传送至指定的关键帧（设置 qpos/qvel/ctrl） |
| `HoldKeyframe(Name)` | 持续保持关键帧姿态 |
| `StopHoldKeyframe()` | 释放已保持的关键帧 |
| `GetKeyframeNames()` | 返回此关节上所有关键帧的名称 |
| `IsHoldingKeyframe()` | 如果当前保持某个姿势，则返回 true |

MjSimulate 小部件提供关键帧下拉菜单和重置/保持/停止按钮，以便进行交互式使用。

---

## 记录和回放

在关卡中使用回放管理器`AMjReplayManager`

**蓝图：** **开始记录(Start Recording)** / **停止记录(Stop Recording)** / **开始回放(Start Replay)** / **停止回放(Stop Replay)** / **将记录内容保存到文件(Save Recording to File)** / **从文件加载记录内容(Load Recording from File)**

**C++:**
```cpp
Replay->StartRecording();
// ... simulation runs ...
Replay->StopRecording();
Replay->SaveRecordingToFile("C:/data/experiment.dat");
Replay->StartReplay();
```

---

## 切换控制源

在仪表盘和外部 ZMQ 控制之间切换：

**蓝图：** **获取管理器(Get Manager)** → **设置控制源(Set Control Source)** → `UI` 或者 `ZMQ`

**C++:**
```cpp
Manager->SetControlSource(EControlSource::ZMQ);
```

`AMjArticulation::ControlSource` 上提供了每个关节的覆盖设置。

---

## MjKeyframeController

在关节上循环切换已命名的姿势，并实现流畅的过渡效果。

| 节点 | 描述 |
|------|-------------|
| **LoadPreset** (名称) | 加载内置姿态序列 |
| **Play** / **Stop** | 开始或暂停播放 |
| **GoToKeyframe** (索引) | 跳转到特定关键帧 |
| **GetPresetNames** | 返回所有可用的预设名称 |

有关关键帧控制器、预设和 FMjKeyframePose 结构体的完整详细信息，请参阅 [控制器框架](controller_framework.md) 。

---

## MjKeyframeCameraActor

`AMjKeyframeCameraActor` 是一款影视级相机，它能够平滑地通过一系列路径点（`FMjCameraWaypoint`：位置、旋转、时间）进行插值。它使用 `UCineCameraComponent`，并在编辑器中显示路径的样条曲线预览。

**主要功能：**

| 节点 | 描述 |
|------|-------------|
| **Play** | 开始航点回放 |
| **Pause** | 冻结在当前位置 |
| **TogglePlayPause** | 切换（也绑定到**O**键） |
| **Reset** | 返回第一个航点 |
| **CaptureCurrentView** | （仅限编辑）将视口相机快照为新的航点 |

**属性：** `bAutoPlay`, `bAutoActivate` (设置为玩家视图目标), `bLoop`, `StartDelay`, `bSmoothInterp` (立方体 vs 线性).

---

## MjImpulseLauncher

`AMjImpulseLauncher` 会对 `MjBody` 施加基于速度的脉冲。有两种模式：

- **直接**模式 — 沿 Actor 的前向向量（或 `DirectionOverride`）发射。 
- **目标**模式 — 将 `LaunchTarget` 设置为某个动作者，它会计算指向该动作者的弹道弧线。`ArcHeight` 控制抛射高度。

| 节点 | 描述 |
|------|-------------|
| **FireImpulse** | 施加一次脉冲 |
| **ResetAndFire** | 将弹丸传送回发射器位置并发射（也绑定到 **F** 键） |

**属性：** `TargetActor`, `TargetBodyName` （可选，默认为第一个 MjBody）, `LaunchSpeed` (米/秒), `LaunchTarget`, `ArcHeight`, `bAutoFire`, `AutoFireDelay`。

---

## 快捷键

由 `AAMjManager::Tick` 处理。在 PIE 期间处于活动状态：

| 键 | 动作 |
|-----|--------|
| **1** | 切换调试联系人 |
| **2** | 切换视觉网格 |
| **3** | 切换关节碰撞线框 |
| **4** | 切换调试关节 |
| **5** | 切换快速转换碰撞线框 |
| **P** | 暂停/恢复模拟 |
| **R** | 重置模拟 |
| **O** | 切换轨道相机轨道 + 关键帧相机播放/暂停 |
| **F** | 重置并发射所有脉冲启动器 |

---

## 每个关节的控制源

每个 `AMjArticulation` 都有一个 `ControlSource` 字段（`0` = ZMQ，`1` = UI），该字段会覆盖管理器级别的 `EControlSource` 设置。这样，您就可以在同一场景中通过控制面板滑块控制部分机器人，同时接收其他机器人的外部 ZMQ 命令。

**蓝图：** 在“详细信息(Details)”中或通过“设置(Set)”节点设置关节参考的`ControlSource`。 

**C++:**
```cpp
Robot->ControlSource = 1; // 该机器人的用户界面控制
```

---

## 高级：直接访问 MuJoCo

对于需要底层访问权限的用户，每个 `UMjComponent` 都公开了 **Get Mj ID**（编译后的整数 ID）、**Get Mj Name**（带前缀的名称）和 **Is Bound**（编译状态）三个方法。您可以使用这些方法直接从 C++ 访问 `mjData`。对于大多数工作流程，上述 API 已足够。

