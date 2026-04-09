# 架构

## 概述

UnrealRoboticsLab (URLab) 将 MuJoCo 物理引擎集成到虚幻引擎中，作为编辑器插件。`AAMjManager` 是顶层协调器参与者（Actor），但它将核心职责委托给四个 `UActorComponent` 子系统：`UMjPhysicsEngine`（物理模拟）、`UMjDebugVisualizer`（调试渲染）、`UMjNetworkManager`（ZMQ 发现）和 `UMjInputHandler`（热键）。组件系统与 MJCF 元素层级结构相对应——每个 XML 元素类型都映射到一个附加到 `AMjArticulation` 蓝图的 `UMjComponent` 子类。物理引擎运行在专用的异步线程中；游戏线程读取结果进行渲染。ZMQ 网络提供外部控制和传感器广播功能。

---

## 子系统架构

`AAMjManager` 将职责委托给四个  `UActorComponent` 子系统，这些子系统是通过构造函数中的 `CreateDefaultSubobject` 创建的：


```
AAMjManager (轻量级协调器)
├── UMjPhysicsEngine       — 模拟核心
├── UMjDebugVisualizer      — 调试渲染
├── UMjNetworkManager       — ZMQ 发现与相机
└── UMjInputHandler         — 热键处理
```

### UMjPhysicsEngine

**文件：** `Source/URLab/Public/MuJoCo/Core/MjPhysicsEngine.h`

拥有所有仿真状态：`m_spec`、`m_vfs`、`m_model`、`m_data` 和 `回调互斥量(CallbackMutex)`。处理完整的编译流程（`预编译(PreCompile)`/`编译(Compile)`/`后编译(PostCompile)`/`应用选项(ApplyOptions)`）。运行异步物理循环（`RunMujocoAsync`）。提供回调注册（`RegisterPreStepCallback`、`RegisterPostStepCallback`），以便其他子系统和 ZMQ 组件可以接入步进循环而无需直接耦合。此外，还提供 `StepSync`、`Reset` 和`快照(Snapshot)`/`恢复(Restore)`功能。


### UMjDebugVisualizer

**文件：** `Source/URLab/Public/MuJoCo/Core/MjDebugVisualizer.h`

拥有 `DebugData`（来自 `MjDebugTypes.h` 的 `FMuJoCoDebugData`）和 `DebugMutex`。`CaptureDebugData` 注册为 `UMjPhysicsEngine` 的后步骤回调函数——它将物理线程上互斥锁下的接触数据复制到该线程。`TickComponent` 在游戏线程上渲染接触力箭头/点。为每种调试可视化模式提供切换方法。


### UMjNetworkManager

**文件：** `Source/URLab/Public/MuJoCo/Net/MjNetworkManager.h`

负责 `ZmqComponents` 的发现和相机注册/流传输。在 BeginPlay 期间调用 `DiscoverZmqComponents()` 来收集管理器参与者上的所有 `UMjZmqComponent` 子组件。


### UMjInputHandler

**文件：** `Source/URLab/Public/MuJoCo/Input/MjInputHandler.h`

处理 `TickComponent` 中的热键。向 `UMjPhysicsEngine`（暂停/重置）、`UMjDebugVisualizer`（调试切换）和场景参与者（网格可见性、碰撞线框、相机）分发。


### 通讯模式

子系统通过以下方式通信：

- **回调：** `UMjPhysicsEngine` 公开了 `RegisterPreStepCallback`/`RegisterPostStepCallback`。其他子系统在 BeginPlay 期间注册 lambda 表达式。

- **`GetOwner()` 遍历:** 子系统通过 `GetOwner()->FindComponentByClass<T>()` 访问兄弟组件（例如，`UMjInputHandler` 找到 `UMjPhysicsEngine` 来调用 `Reset()`）。

- **直接访问：** 外部代码可通过 `Manager->PhysicsEngine->Options`、`Manager->DebugVisualizer->bShowDebug` 等直接访问子系统状态。`AAMjManager` 没有重复属性——它是一个纯粹的协调器。诸如 `SetPaused()`、`StepSync()` 和 `ResetSimulation()` 等蓝图可调用的便捷方法委托给 `PhysicsEngine`。

---

## 模块初始化

**文件：** `Source/URLab/Private/URLab.cpp` -- `FURLabModule::StartupModule()`

DLL 加载顺序（顺序加载，每个 DLL 必须成功加载）：

1. `mujoco.dll` (从 `third_party/install/MuJoCo/bin/`)
2. `obj_decoder.dll` (相同路径)
3. `stl_decoder.dll` (相同路径)
4. `libzmq-v143-mt-4_3_6.dll` (从 `third_party/install/libzmq/bin/`)
5. `lib_coacd.dll` (从 `third_party/install/CoACD/bin/`)

搜索路径策略：首先是 `third_party/install/{SubDir}/bin/`（编辑器/开发版本），然后是 `FPlatformProcess::GetModulesDirectory()`（DLL 与可执行文件放在一起的打包版本）。

在编辑器构建中，该模块还通过 `IAssetTools::RegisterAdvancedAssetCategory()` 注册了“MuJoCo”资产类别。


---

## 场景声明周期

### BeginPlay

**文件：** `Source/URLab/Private/MuJoCo/Core/AMjManager.cpp` -- `AAMjManager::BeginPlay()`

1. **单例模式强制执行。** 如果 `AAMjManager::Instance` 已分配给其他参与者，则此实例会记录错误并返回。每个级别仅支持一个 `AAMjManager`。
2. **子系统创建。** 四个子系统（`UMjPhysicsEngine`、`UMjDebugVisualizer``UMjNetworkManager`、`UMjInputHandler`）通过构造函数中的 `CreateDefaultSubobject` 创建。
3. **自动创建 ZMQ 组件。** 如果管理器参与者上不存在 `UMjZmqComponent` 子类，则会创建一个 `UZmqSensorBroadcaster` (tcp://*:5555) 和一个 `UZmqControlSubscriber` (tcp://127.0.0.1:5556)。 
4. **ZMQ 发现。** `UMjNetworkManager::DiscoverZmqComponents()` 收集管理器参与者上的所有 ZMQ 组件。
5. **自动创建回放管理器。** 如果关卡中不存在  `AMjReplayManager`，则会生成一个。
6. **`Compile()`** -- 委托给 `UMjPhysicsEngine` 进行规范构建和 `mj_compile()`（见下文）。
7. **回调注册。** `UMjDebugVisualizer::CaptureDebugData` 注册为 `UMjPhysicsEngine` 上的步骤后回调。
8. **`UpdateCameraStreamingState()`** -- 应用全局相机广播切换。
9. **`RunMujocoAsync()`** -- 委托给 `UMjPhysicsEngine` 来启动物理线程。
10. **自动创建 MjSimulate 小部件。** 加载 `/UnrealRoboticsLab/WBP_MjSimulate` 蓝图并将其添加到视口（由 `bAutoCreateSimulateWidget` 控制）。

### 规范构建（预编译）

**文件：** `UMjPhysicsEngine::PreCompile()`

1.通过 `mj_makeSpec()` 创建一个新的 `mjSpec`（弧度模式：`compiler.degree = false`）。 

2.通过 `mj_defaultVFS()` 初始化虚拟文件系统(Virtual File System, VFS)。

3.通过 `UGameplayStatics::GetAllActorsOfClass()` 发现关卡中的所有参与者。

4.在发现循环中按顺序处理

   - **快速转换：** 任何带有 `UMjQuickConvertComponent` 的参与者都会调用 `Setup(spec, vfs)`。
   - **关节：** 任何 `AMjArticulation` 都会调用 `Setup(spec, vfs)`。
   - **高度场：** 任何 `AMjHeightfieldActor` 都会调用 Setup(spec, vfs)。
   
5.ZMQ 组件的发现由 `UMjNetworkManager::DiscoverZmqComponents()` 在 BeginPlay 期间单独处理。


!!! 注意
   发现循环会遍历所有参与者一次。同一参与者上的快速转换组件和关节都会被处理（尽管在循环逻辑中它们并非互斥，但实际上它们存在于不同的参与者上）。


### 关节设置

**文件:** `Source/URLab/Private/MuJoCo/Core/MjArticulation.cpp` -- `AMjArticulation::Setup()`

这是规范构建中最复杂的部分。每个关节都会创建一个独立的子规范，该子规范随后会合并到根规范中。

1.**创建子规范。** 使用 `mj_makeSpec()` 进行隔离。弧度模式。通过 `ApplyToSpec()` 应用关节级 `模拟选项(SimOptions)`。

2.**创建封装器。** `FMujocoSpecWrapper` 封装子规范和虚拟文件系统(VFS)，以便于创建元素。

3.**前缀。** 设置为 `{ActorName}_` -- 此表达式中的所有元素都将在 `mjs_attach()` 之后添加前缀。

4.**处理顺序至关重要：**

   - **默认** （必须放在首位）。首先调用 `GetComponents<UMjDefault>()`，然后对每个组件调用 `m_Wrapper->AddDefault()`。其他元素引用这些默认类。

   - **遍历 WorldBody。** 找到 `UMjWorldBody` 组件，并迭代其 `GetAttachChildren()` 方法。对于每个子组件：
   `UMjBody` (非默认) -> `Setup(nullptr, nullptr, wrapper)` (递归)；
   `UMjFrame` (非默认) -> `Setup(nullptr, nullptr, wrapper)`。

   - **肌腱。** 首先调用 `GetComponents<UMjTendon>()`，然后调用 `RegisterToSpec()`。必须在实体组件之后调用，以便关节名称存在。

   - **传感器。** 首先调用 `GetComponents<UMjSensor>()`，然后调用 `RegisterToSpec()`。 

   - **执行器。** 首先调用 `GetComponents<UMjActuator>()`，然后调用 `RegisterToSpec()`。

   - **接触对。** 首先调用 `GetComponents<UMjContactPair>()`，然后调用 `RegisterToSpec()`。

   - **接触排除。** `GetComponents<UMjContactExclude>()` then `RegisterToSpec()`.

   - **相等性(Equalities)。** 先调用 `GetComponents<UMjEquality>()` 然后 `RegisterToSpec()`。

   - **关键帧。** 先调用 `GetComponents<UMjKeyframe>()`，然后调用 `RegisterToSpec()`.

5.**附着。** `mjs_attach(attachmentFrame->element, childSpec->element, prefix, "")` 通过 `mjsFrame` 将子模型合并到根世界主体(world body)中。帧的 pos/quat 值由关节参与者的世界变换（转换为 MuJoCo 坐标）设置。附加完成后，`m_ChildSpec` 被设置为 nullptr（所有权转移给根模型）。


### 刚体遍历（递归）

**文件：** `Source/URLab/Private/MuJoCo/Components/Bodies/MjBody.cpp` -- `UMjBody::Setup()`

`UMjBody::Setup()` 通过封装器创建一个 `mjsBody`，然后迭代 `GetAttachChildren()`：

- 子对象 `UMjBody` -> 递归 `Setup()`
- 子对象 `UMjFrame` -> `Frame::Setup()`
- 子对象 `UMjGeom`, `UMjJoint`, `UMjSensor`, `UMjSite` 等 -> `RegisterToSpec()`

网格准备（通过 `PrepareMeshForMuJoCo` 进行 CoACD 凸分解）在几何体配准期间进行。


### 编译

**文件** `UMjPhysicsEngine::Compile()`

1.调用 `PreCompile()`。

2.`mj_compile(m_spec, &m_vfs)` 产生 `mjModel*`。

3.**失败时：** 通过 `mjs_getError(m_spec)` 获取错误信息，记录错误日志，并显示编辑器对话框 (`FMessageDialog`)。返回，不创建数据。

4.**成功时：**

   - 如果 `bSaveDebugXml` 为 true：将 `scene_compiled.xml` 和 `scene_compiled.mjb` 保存到 `Saved/URLab/` 目录，文件路径为相对路径。
   - 通过 `mjData` 创建 `mj_makeData(m_model)`.
   - 调用 `ApplyOptions()`（将管理器级别的选项覆盖应用于 `m_model->opt`）。
   - 调用 `PostCompile()`.

### 后编译 (绑定)

**文件:** `UMjPhysicsEngine::PostCompile()`

1. 对每个 `UMjQuickConvertComponent` 调用 `PostSetup(model, data)`。
2. 构建查找时间复杂度为 O(1) 的 `m_ArticulationMap` (name -> `AMjArticulation*`) 。
3. 对每个 `AMjArticulation` 调用 `PostSetup(model, data)`。 
4. 对每个 `AMjHeightfieldActor` 调用 `PostSetup(model, data)`。

**在 `AMjArticulation::PostSetup()` 函数内部：**

每个组件都会调用 `Bind(model, data, prefix)`。`UMjComponent` 组件的 `Bind()` 方法会调用 `BindToView<T>()` 函数，该函数会执行以下操作：

1. **路径 1 (基于 ID):** 如果设置了 `m_SpecElement`，则尝试使用 `mjs_getId(m_SpecElement)`。验证 ID 是否在模型范围内（防止在 `mjs_attach` 之后使用过时的 ID）。 
2. **路径 2 (名称回退):** 构造 `{Prefix}{MjName}`（如果 MjName 为空，则构造 `{Prefix}{GetName()}`），并调用 `mj_name2id()` 函数。

此操作会创建视图结构体（`BodyView`、`GeomView`、`JointView`、`ActuatorView`、`SensorView`、`TendonView`、`SiteView`），这些结构体将原始指针缓存到 `mjModel`/`mjData` 数组中。

PostSetup 还会填充组件名称映射（`ActuatorComponentMap`、`JointComponentMap` 等）和 MuJoCo-ID 映射（`BodyIdMap`、`GeomIdMap` 等），以实现 O(1) 运行时访问。


---

## 物理循环 (异步线程)

**文件:** `Source/URLab/Private/MuJoCo/Core/MjPhysicsEngine.cpp` -- `UMjPhysicsEngine::RunMujocoAsync()`

通过 `Async(EAsyncExecution::Thread, ...)` 在专用线程上运行。存储在 `AsyncPhysicsFuture` 中。

每次迭代（在 `UMjPhysicsEngine` 持有的 `CallbackMutex` 锁下）：

1.**检查 `bPendingReset`** -> `mj_resetData()` + `mj_forward()`. 在游戏线程上广播 `OnSimulationReset`。 

2.**检查 `bPendingRestore`** -> `mj_setState()` 使用 `PendingStateVector` + `mj_forward()`。

3.**注册预步回调** (取代直接调用 ZMQ PreStep)。 ZMQ 组件通过 `RegisterPreStepCallback()` 注册。

4.对每个关节应用 **ApplyControls**（根据执行器值写入 `d->ctrl`）。 

5.**物理步骤：**

   - 如果 `bIsPaused`：跳过。

   - 如果已经绑定 `CustomStepHandler`：调用它而不是 `mj_step()`（用于回放）。

   - 否则：`mj_step(m_model, m_data)`。

6.**已注册的步骤后回调函数** （取代直接调用 ZMQ PostStep 和调试捕获函数）。ZMQ 组件和 `UMjDebugVisualizer::CaptureDebugData` 通过 `RegisterPostStepCallback()` 进行注册。

7.**`OnPostStep` 委托** （回放录制在此处捕获状态）。



**时序：** 释放互斥锁后，循环会进行自旋等待（`FPlatformProcess::YieldThread()`），直到 `TargetInterval / SpeedFactor` 的时间过去。`SimSpeedPercent` 控制速度因子。 

---

## 游戏线程 (节拍信号)

**文件:** `AAMjManager::Tick()`

1. **向后兼容的指针同步。** 从 `UMjPhysicsEngine` 复制 `m_model`/`m_data`，以便读取 `AAMjManager::m_model` 的旧版调用者仍然可以正常工作。
2. 热键处理和调试绘图分别委托给 `UMjInputHandler` 和 `UMjDebugVisualizer`（两者都通过各自的 `TickComponent` 发送节拍）。 


### UMjInputHandler::TickComponent

每帧处理热键并分派至相应的子系统：

- `1` -> `DebugVisualizer->ToggleContactForces()`
- `2` -> 切换视觉网格可见性（场景参与者）
- `3` -> 切换关节碰撞线框（场景参与者）
- `4` -> 切换关节调试轴（场景参与者）
- `5` -> 切换快速转换碰撞线框（场景参与者）
- `P` -> `PhysicsEngine->TogglePause()`
- `R` -> `PhysicsEngine->Reset()`
- `O` -> 切换轨道和关键帧相机（场景参与者）
- `F` -> 发射脉冲发射器（场景参与者）

### UMjDebugVisualizer::TickComponent

如果启用调试模式，则读取 `DebugData`（可视化器上受 `DebugMutex` 保护），并通过 `ULineBatchComponent` / `DrawDebugLine` 绘制接触力箭头和点。

**变换同步** 在 `UMjBody::TickComponent()` 中进行：

- 如果 `bDrivenByUnreal`：将 UE 世界变换写入 MuJoCo 动作捕捉模型（`d->mocap_pos`，`d->mocap_quat`）。 
- 否则：从 `BodyView` 读取 MuJoCo `xpos`/`xquat`，并通过 `SetWorldLocationAndRotation()` 设置 UE 世界变换。 

---

## 线程安全

| 互斥量/机制 | 所有者 | 保护 | 使用者 |
|---|---|---|---|
| `CallbackMutex` (`FCriticalSection`) | `UMjPhysicsEngine` | 物理步进期间的 `m_model`, `m_data` | 物理线程（主锁）, `StepSync()` |
| `DebugMutex` (`FCriticalSection`) | `UMjDebugVisualizer` | 调试数据`DebugData`（接触可视化缓冲区） | 物理线程写入（通过步骤后回调），游戏线程读取 |
| `CameraMutex` (`FCriticalSection`) | `UMjNetworkManager` | `ActiveCameras` 数组 | 在任何线程中注册/取消注册 |
| `bPendingReset` (`std::atomic<bool>`) | `UMjPhysicsEngine` | 复位信号 | 游戏线程设置，物理线程读取/清除 |
| `bPendingRestore` (`std::atomic<bool>`) | `UMjPhysicsEngine` | 恢复信号 | 游戏线程设置，物理线程读取/清除 |
| `bIsPaused`（非原子操作，但仅从游戏线程写入） | `UMjPhysicsEngine` | 暂停状态 | 由物理线程读取 |
| `bShouldStopTask` (`std::atomic<bool>`) | `UMjPhysicsEngine` | 关闭信号 | 游戏线程在 `EndPlay` 中设置，物理线程进行检查 |
| `UMjActuator::InternalValue` / `NetworkValue` | `UMjActuator` | 每个执行器控制值 | 跨线程的原子读写操作 |

---

## 组件系统

### 基类：UMjComponent

**文件：** `Source/URLab/Public/MuJoCo/Components/MjComponent.h`

继承自 `USceneComponent` 和 `IMjSpecElement`。所有 MuJoCo 元素组件均派生自此基类。

关键成员:

- `m_SpecElement` (`mjsElement*`) -- 在 `RegisterToSpec()` 期间设置，用于 `Bind()` 中的 ID 解析。
- `m_ID` (`int`) -- MuJoCo 对象 ID，在 `Bind()` 期间解析。
- `m_Model`, `m_Data` -- 缓存指针，在 `Bind()` 期间设置。
- `MjName` (`FString`) -- 原始 MJCF 名称，用于交叉引用。
- `bIsDefault` (`bool`) -- 如果为 true，则此组件为默认模板，运行时查找时会跳过。

关键方法:

- `RegisterToSpec(FMujocoSpecWrapper&, mjsBody*)` -- 创建 spec 元素。子类会重写此方法。
- `Bind(mjModel*, mjData*, Prefix)` -- 解析 ID 并缓存模型/数据指针。
- `BindToView<T>(Prefix)` -- 创建 View 结构体的模板方法。首先尝试使用 `mjs_getId()`（带有边界验证），如果失败则回退到 `mj_name2id()`。
- `ResolveDefault(mjSpec*, ClassName)` -- 通过类名查找 `mjsDefault` 的静态辅助方法。如果失败则回退到全局默认值。
- `FindEditorDefault()` -- 编辑器时解析默认值，无需 mjSpec。
- `SetSpecElementName()` -- 通过封装器去重为 spec 元素分配唯一名称。 

### 查看结构体

**文件：** `Source/URLab/Public/MuJoCo/Utils/MjBind.h`

轻量级结构体，将原始指针缓存到 `mjModel`/`mjData` 数组中，实现零开销运行时访问。每个结构体都有一个 `static constexpr mjtObj obj_type` 用于模板分发。

| 结构体 | 对象类型 | 关键指针 |
|---|---|---|
| `BodyView` | `mjOBJ_BODY` | `xpos`, `xquat`, `xfrc_applied`, `mass`, `mocap_id` |
| `GeomView` | `mjOBJ_GEOM` | `xpos`, `xmat`, `size`, `type`, `contype`, `conaffinity`, `dataid` |
| `JointView` | `mjOBJ_JOINT` | `qpos`, `qvel`, `qacc`, `xanchor`, `xaxis`, `range`, `stiffness`, `stiffnesspoly`, `damping`, `dampingpoly` |
| `ActuatorView` | `mjOBJ_ACTUATOR` | `ctrl`, `force`, `length`, `ctrlrange`, `gainprm`, `biasprm` |
| `SensorView` | `mjOBJ_SENSOR` | `data` (pointer into `sensordata`), `dim`, `adr`, `type` |
| `TendonView` | `mjOBJ_TENDON` | `length`, `velocity`, `stiffness`, `stiffnesspoly`, `damping`, `dampingpoly`, `range` |
| `SiteView` | `mjOBJ_SITE` | `xpos`, `xmat`, `size`, `type`, `body_id` |


`BodyView` 还提供了遍历方法：`Bodies()`、`Geoms()` 和 `Joints()`，用于遍历已编译的模型层次结构。

独立的模板函数 `bind<T>(model, data, name)` 提供基于名称的查找功能。

### 默认系统

`UMjDefault` 组件存储的模板属性与 MJCF 的 `<default>` 层级结构相对应。父子类链通过 `FMujocoSpecWrapper::AddDefault()` 构建。在组件配置设置期间，`ExportTo(mjsDefault*)` 会遍历子组件并将其属性写入规范默认值。在注册时，组件通过 `ResolveDefault()` 解析其默认类，该函数会调用 `mjs_findDefault()`。

---

## 坐标系统

MuJoCo使用右手坐标系，Z 轴朝上，单位为米。而虚幻引擎使用左手坐标系，Z 轴朝上，单位为厘米。

**位置转换** (`MjUtils::MjToUEPosition` / `UEToMjPosition`):
- X -> X, Y -> -Y, Z -> Z
- 比例：MuJoCo 米 * 100 = 虚幻引擎的厘米

**旋转转换** (`MjUtils::MjToUERotation` / `UEToMjRotation`):
- MuJoCo四元数 `[w, x, y, z]` -> `FQuat`，其中 X 和 Z 分量取反（以处理手性翻转）。

所有转换工具都在 `Source/URLab/Public/MuJoCo/Utils/MjUtils.h` 文件中。

---

## 导入与用于自制的关节

创建 `AMjArticulation` 有两种方法：

**1. 从MJCF XML导入（拖放式）：**
- 使用 `UMujocoImportFactory` -> `UMujocoGenerationAction`
- 所有组件均通过 XML 解析自动生成
- 默认类层次结构保留为 `UMjDefault` 组件
- `MuJoCoXMLFile` 属性用于存储源 XML 路径
- 导入到虚幻内容浏览器(Content Browser)的网格资源

**2. 用户从零开始构建（右键点击 -> 新建 MuJoCo 关节）：**
- 使用 `UMjArticulationFactory` -> `UMujocoGenerationAction::SetupEmptyArticulation()`
- 创建组织层次结构（定义、默认值、执行器、传感器等）
- 用户在蓝图编辑器中手动添加组件
- 无XML文件 — 组件直接编写

**这两种路径都会产生相同的结果：** 一个包含 `UMjComponent` 子类的 `AMjArticulation` 蓝图。在运行时，两者都会经历相同的设置(Setup)->编译(Compile)->绑定(Bind)流程。规范系统并不知晓或关心组件是如何创建的。 

### 特殊情况标志

| 标志 | On | 目的 |
|------|------|---------|
| `bIsDefault` | `UMjComponent` | 在 `<default>` 块内标记模板组件。这些组件在运行时发现（`GetRuntimeComponents`）时被排除，在遍历主体时也被排除在 `RegisterToSpec` 之外。 |
| `bIsQuickConverted` | `UMjBody` | 由 Quick Convert 设置。在时间步同步期间启用网格枢轴偏移校正（UE 网格可能具有 MuJoCo 不知道的偏心枢轴）。 |
| `bDrivenByUnreal` | `UMjBody` | 实现单向耦合：UE 变换驱动 MuJoCo 运动捕捉体。用于以虚幻引擎为准的运动学对象。 |
| `bFromToResolvedHalfLength` | `UMjGeom` | 在导入过程中，当 `fromto` 属性解析为位置/四元数/尺寸值时，设置此属性。控制 `ExportTo` 如何写入尺寸值——仅写入半长槽，半径值则来自默认链。 |

---

## 导入管线

**文件：** `Source/URLabEditor/Private/MujocoImportFactory.cpp`, `Source/URLabEditor/Private/MjArticulationFactory.cpp`

### 步骤

1.用户将一个 `.xml` 文件拖入内容浏览器(Content Browser)。

2.`UMujocoImportFactory::FactoryCreateFile()` 触发。

3.**网格预处理：** 自动运行`Scripts/clean_meshes.py`（Python子进程）。 

   - 检测 Python 安装情况，若未安装则自动安装 `trimesh`。 

   - 解析 MJCF XML 文件以查找所有网格资产。

   - 检测 GLB 文件中的 stem 冲突（例如，`link1.obj` 和 `link1.stl` 都生成了 `link1.glb`文件）。

   - 使用计数器后缀重命名冲突的文件，并更新 XML。

   - 将所有网格转换为 GLB 格式（保留 UVs，移除嵌入纹理）。

   - 生成带有更新网格引用的  `_ue.xml` 文件。

   - 每一步都有优雅的回退机制——如果 Python/trimesh 缺失，则使用原始 XML。 

4.创建一个 `AMjArticulation` 蓝图。

5.`UMujocoGenerationAction::GenerateForBlueprint()` 通过四个步骤解析 XML：

   - **第一步：** 解析资产（带缩放的网格、带文件路径的纹理、带 RGBA/纹理引用的材质）。

   - **第二步：** 解析默认设置（类层次结构 — 创建带有子几何体/关节/执行器模板的 `UMjDefault` 组件）。

   - **第三步：** 递归导入世界刚体 worldbody（刚体、几何体、关节、位点成为附加在蓝图层次结构中的 `UMjComponent` 子类）。 

   - **第四步：** 导入执行器、传感器、肌腱、equalities、关键帧、接触对/排除对。

6.网格导入使用虚幻引擎的资产工具，格式优先级为：FBX > GLB > OBJ。网格保存在`/Meshes/` 子文件夹中，以避免纹理名称冲突。 

7.以 XML 材质名称（所有引用 `"white"` 的几何体共享的 `MI_white`）为键，创建作为共享实例的材质。

8.纹理已导入到资源文件夹，并已应用于带有 `bUseTexture` 标志的材质实例。 

9.通过 `FKismetEditorUtilities` 进行最终蓝图编译。


### Import Normalization

The importer normalizes several MJCF constructs for consistency:

**FromTo resolution:** All `fromto` attributes on geoms and sites are decomposed at import time into explicit `pos`, `quat`, and `size`:
- `pos` = midpoint of the fromto segment
- `quat` = rotation aligning local +Z with the fromto direction
- `size[1]` = half-length (for capsule/cylinder), `size[2]` for box/ellipsoid
- `size[0]` (radius) left untouched — inherited from defaults
- `bFromToResolvedHalfLength` flag set so ExportTo knows to only write the half-length slot
- The component's relative transform is set for accurate editor visualization

**Default class visual transforms:** When a geom inherits a transform from a default class (e.g., `class="visual_zflip"` with `quat="0 0 0 1"`), the importer walks the default hierarchy to find the transform and applies it as a visual offset to the `UStaticMeshComponent` child — NOT to the `UMjGeom` component itself (which would double-apply via the spec default system).

**Orientation handling:** All MJCF rotations (euler, axisangle, quat, xyaxes, zaxis) are converted to UE quaternions at import time using `MjOrientationUtils`. The `<compiler>` settings (angle="radian"/"degree", eulerseq) are respected.

**Mesh name deduplication:** `MjSpecWrapper::GetUniqueName()` appends `_1`, `_2`, etc. if a mesh/body/geom name already exists in the spec.

---

## Export Pipeline (Runtime)

Each component's `RegisterToSpec()` writes to an `mjsElement` via the `FMujocoSpecWrapper`. The component's `ExportTo()` method populates properties (type, size, pos, quat, friction, etc.).

### Override Flag System

Every exportable property has a corresponding `bOverride_*` bool. During `ExportTo()`:
- If `bOverride_X == true`: the property is explicitly written to the spec element.
- If `bOverride_X == false`: the property is NOT written, allowing MuJoCo's default system to provide the value.

This is critical for round-trip fidelity: an imported geom with `class="collision"` that inherits `group=3` from its default should NOT have `group=3` written explicitly — that would bypass the default if the user later changes the default.

### FromTo Export

Geoms with `bFromToResolvedHalfLength == true`:
- If `bIsDefault`: writes raw fromto to the default's geom (MuJoCo handles size resolution for defaults).
- If instance with `bOverride_Size == false`: writes only the half-length slot, letting the default provide the radius.
- If instance with `bOverride_Size == true`: writes all size slots explicitly.

### Spec Attachment

`mjs_attach()` merges the child spec into the root with the articulation's prefix, enabling multi-articulation scenes with unique namespaces. After attachment, all element names gain the prefix (e.g., `pelvis` becomes `g1_C_1_pelvis`). Runtime name lookups use this prefix.

---

## ZMQ Networking

**Files:** `Source/URLab/Private/MuJoCo/Net/ZmqSensorBroadcaster.cpp`, `ZmqControlSubscriber.cpp`

Binary pub/sub protocol for minimal latency.

**Topics:**
- `{prefix}/joint/{name}` -- joint state
- `{prefix}/sensor/{name}` -- sensor reading
- `{prefix}/base_state/` -- root body pose
- `{prefix}/twist` -- twist command velocity
- `{prefix}/control` -- actuator control values

**Timing:**
- Sensor broadcast: `PostStep()` on the physics thread.
- Control receive: `PreStep()` on the physics thread.

**Control source:** `EControlSource` enum (`ZMQ` or `UI`). Set globally on `AAMjManager::ControlSource`, with per-articulation override via `AMjArticulation::ControlSource`.

**Info broadcast:** JSON payload with actuator names, IDs, and ranges. Sent periodically for client discovery.

---

## Quick Convert

**File:** `Source/URLab/Public/MuJoCo/Components/QuickConvert/MjQuickConvertComponent.h`

`UMjQuickConvertComponent` attaches to any Static Mesh actor for one-click physics integration.

- At `Setup()` time, creates an `MjBody` + `MjGeom` on the root world body.
- Runs CoACD convex decomposition with configurable threshold.
- Separates visual mesh (group 2) from collision mesh (group 3).
- Supports static, dynamic, or mocap body modes.

---

## Heightfield

**File:** `Source/URLab/Public/MuJoCo/Components/QuickConvert/AMjHeightfieldActor.h`

`AMjHeightfieldActor` samples Unreal landscape geometry via raycasts.

- Configurable resolution, trace channel, and actor whitelist.
- `bTraceComplex` for accurate mesh sampling.
- Registers as `mjsHField` + static geom on the world body.
- Binary cache (`Saved/URLab/`) for fast recompilation.

---

## Replay System

**File:** `Source/URLab/Public/Replay/MjReplayManager.h`, `MjReplayTypes.h`

- **Recording:** `OnPostStep` callback captures `qpos`/`qvel` per joint per frame.
- **Playback:** `CustomStepHandler` replaces `mj_step()`, applies recorded `qpos` then calls `mj_forward()`.
- **CSV import:** Supports external trajectory data with automatic joint name matching.
- **Snapshot/Restore:** Full state capture via `mj_getState()` / `mj_setState()`. Game thread writes `PendingStateVector` then sets `bPendingRestore`; physics thread applies on next iteration.
- **Thread safety:** Playback time is `std::atomic`. The `CustomStepHandler` runs inside `CallbackMutex`.

---

## Controller System

**Files:** `Source/URLab/Public/MuJoCo/Components/Controllers/`

- `UMjArticulationController` -- abstract base class. Override `ComputeAndApply()` to implement control logic.
- `UMjPDController` -- PD servo controller with per-actuator Kp/Kv/torque limits.
- `UMjPassthroughController` -- passes control values directly to `d->ctrl`.
- `UMjKeyframeController` -- cycles through preset keyframe sequences with smooth interpolation.

**Discovery:** During `PostSetup()`, the articulation calls `FindComponentByClass<UMjArticulationController>()`.

**Bind:** Maps actuator IDs to `qpos`/`qvel` addresses via `FActuatorBinding` structs for efficient access.

**ApplyControls:** `AMjArticulation::ApplyControls()` delegates to the controller's `ComputeAndApply()` if one is bound; otherwise falls back to writing actuator values directly to `d->ctrl`.

---

## Input System

**Files:** `Source/URLab/Public/MuJoCo/Input/MjInputHandler.h`, `Source/URLab/Public/MuJoCo/Input/MjTwistController.h`, `Source/URLab/Public/MuJoCo/Net/MjInputMapping.h`

### MjInputHandler

`UMjInputHandler` is an `UActorComponent` subsystem on `AAMjManager`. It processes debug/simulation hotkeys in `TickComponent` and dispatches to the appropriate subsystem or scene actors. See "Game Thread (Tick)" above for the full key mapping. It accesses sibling subsystems via `GetOwner()->FindComponentByClass<T>()`.

### MjTwistController

Added automatically to articulations on Possess. Captures WASD/gamepad input and stores twist commands (`vx`, `vy`, `yaw_rate`) for broadcasting over ZMQ.

- Uses Unreal's Enhanced Input System (`UInputAction`, `UInputMappingContext`)
- Input context added in `PossessedBy()`, removed in `UnPossessed()`
- `GetTwist()` returns the current twist vector (thread-safe, read by ZMQ broadcaster on physics thread)
- Supports 10 action keys (mapped to a bitmask) for discrete commands
- `MaxVx`, `MaxVy`, `MaxYawRate` properties clamp the output

### MjInputMapping

Configurable actuator-to-input binding system. Maps Enhanced Input actions directly to actuator SetControl calls. Each `FMjInputBinding` specifies:
- Target actuator component
- Input action reference
- Scale multiplier
- The binding fires `SetControl(value * scale)` on the Triggered event

---

## Simulation Options

**File:** `Source/URLab/Public/MuJoCo/Core/MjSimOptions.h`

`FMuJoCoOptions` wraps MuJoCo's `mjOption` fields as UPROPERTYs (timestep, gravity, integrator, solver, iterations, etc.). Used in **two contexts with different semantics**:

### On AMjArticulation (child spec options)
- ALL fields are written to the child spec's `mjOption` before `mjs_attach()`
- The `bOverride_*` toggles are **ignored** — every value applies unconditionally
- This defines the "native" physics settings for that robot model

### On AAMjManager (post-compile overrides)
- Acts as **post-compile overrides** on `m_model->opt` via `ApplyOptions()`
- Only fields with `bOverride_* = true` are applied
- Fields with `bOverride_* = false` keep whatever the compiled model has (from the articulation specs or MJCF defaults)
- Called once after `mj_compile()` succeeds

### Resolution order
1. MuJoCo's built-in defaults (from `<option>` in XML or MuJoCo hardcoded defaults)
2. Articulation's `SimOptions` overwrites the child spec options during Setup
3. After compilation, Manager's `Options` selectively overwrites `m_model->opt` fields

This means: an articulation can set `timestep = 0.002` in its SimOptions, and the manager can override it to `timestep = 0.005` if `bOverride_Timestep = true` on the manager.

---

## Keyframes

**File:** `Source/URLab/Public/MuJoCo/Components/Keyframes/MjKeyframe.h`

`UMjKeyframe` components store named simulation state snapshots from the MJCF `<keyframe>` section:
- `Time` -- simulation time
- `Qpos` -- joint positions (array of doubles, size = nq)
- `Qvel` -- joint velocities (array of doubles, size = nv)
- `Act` -- actuator activations
- `Ctrl` -- control values
- `MocapPos` / `MocapQuat` -- mocap body transforms

Registered to the spec during Setup (after bodies/joints). At runtime, `AMjArticulation::ResetToKeyframe()` teleports the robot to a named keyframe, and `HoldKeyframe()` continuously maintains the pose (via ctrl or direct qpos injection). The MjSimulate widget exposes these via a keyframe dropdown and Reset/Hold/Stop buttons.

---

## Editor Tools

- **MjSimulate widget** (`WBP_MjSimulate`): Physics options, per-actuator sliders, debug visualization toggles, replay controls, possess button. Auto-created at BeginPlay.
- **ValidateSpec:** Blueprint compile hook on `AMjArticulation`. Creates a temporary spec, runs `mj_compile()`, and reports errors without affecting the running simulation.
- **MjGeomDetailCustomization** (`Source/URLabEditor/`): Custom Details panel for geom properties in the editor.

---

## Editor Workflow

### Blueprint Compile Validation

When a user compiles an `AMjArticulation` Blueprint in the editor, `ValidateSpec()` runs automatically (hooked via `OnBlueprintCompiled`). This creates a temporary mjSpec, runs the full Setup() pipeline on it, and attempts `mj_compile()`. If compilation fails, a dialog shows the MuJoCo error message. This catches spec errors (missing joints, invalid ranges, etc.) at edit time rather than at Play.

### MjSimulate Widget

Auto-created from `WBP_MjSimulate` Blueprint asset at BeginPlay. Provides:
- Physics options: timestep, solver, integrator, iterations, sim speed
- Per-actuator sliders (locked when control source is ZMQ)
- Joint/sensor readouts
- Articulation selector dropdown
- Debug toggles (per-articulation and global)
- Record/Replay/Snapshot controls
- Possess button (attaches spring arm camera)

### MjGeomDetailCustomization

Custom Details panel for `UMjGeom` components. Adds "Decompose Mesh" and "Remove Decomposition" buttons for CoACD operations directly in the editor.

---

## Debug Visualization

**File:** `Source/URLab/Public/MuJoCo/Core/MjDebugVisualizer.h`

`UMjDebugVisualizer` is an `UActorComponent` subsystem on `AAMjManager`. It owns `DebugData` (`FMuJoCoDebugData`, defined in `MjDebugTypes.h`) and `DebugMutex`. Data capture runs on the physics thread via a registered post-step callback on `UMjPhysicsEngine`. Rendering runs on the game thread in `TickComponent`.

Hotkeys (processed by `UMjInputHandler::TickComponent()`, dispatched to `UMjDebugVisualizer`):

| Key | Toggle |
|---|---|
| `1` | Contact force arrows |
| `2` | Visual mesh visibility |
| `3` | Articulation collision wireframes |
| `4` | Joint axis/range arcs |
| `5` | Quick Convert collision wireframes |

Collision drawing logic: renders geoms in group 3 OR geoms where `contype != 0 && conaffinity != 0`.

Convex hull rendering uses `mesh_graph` data from MuJoCo (hull edges, not full mesh triangles).

Both per-articulation (`bDrawDebugCollision`, `bDrawDebugJoints`, `bDrawDebugSites`) and global toggles (`bGlobalDrawDebugCollision`, etc.) are supported.

---

## Cinematic Tools

**Files:** `Source/URLab/Public/Cinematics/`

- `AMjOrbitCameraActor` -- auto-orbiting camera that tracks detected robots. Toggled via `O` key.
- `AMjKeyframeCameraActor` -- waypoint-based camera path with smooth interpolation. Play/pause toggled via `O` key.

---

## Error Handling and Failure Modes

### Compile failure
If `mj_compile()` returns null, `mjs_getError(m_spec)` retrieves the error string. In editor builds, a dialog box shows the message. The error is also stored in `m_LastCompileError` (accessible via Blueprint). The simulation does not start — `m_model` and `m_data` remain null, and `RunMujocoAsync` checks for this before stepping.

### Mesh import failure
`ImportSingleMesh()` tries multiple paths: GLB (via Interchange), then raw OBJ/STL (via FBX factory), then FBX fallback. If all fail, the geom component is created but has no visual mesh — it still exists in the spec as a collision primitive. A warning is logged. The compile still succeeds since MuJoCo doesn't need visual meshes.

### ZMQ connection failure
ZMQ uses `bind()` not `connect()` — it listens on ports. If the port is already in use (e.g., previous PIE session didn't clean up), ZMQ will log an error but the simulation still runs. External clients simply can't connect. There is no retry logic — the user must restart PIE.

### Python mesh preprocessing failure
Every step of the auto-clean-meshes pipeline has a fallback:
- Python not found → uses raw XML
- trimesh not installed → attempts pip install, if that fails → uses raw XML
- Script errors → uses raw XML
- The import always proceeds; mesh preprocessing is best-effort

### ValidateSpec failure
Shows a dialog with the MuJoCo error. Does NOT prevent the Blueprint from being saved — it's informational. The user can fix the issue and recompile.

---

## Debug XML (Diagnostics)

When `bSaveDebugXml = true` on the AAMjManager, after successful compilation the system saves:
- `Saved/URLab/scene_compiled.xml` — the full MuJoCo spec as XML
- `Saved/URLab/scene_compiled.mjb` — the compiled binary model

The XML undergoes path relativization: absolute paths like `C:/Users/.../Saved/URLab/ConvertedMeshes/...` are stripped to `ConvertedMeshes/...` relative to the output directory. Double slashes and `../` traversals are cleaned.

**Diagnostic use:** Diff the original MJCF XML against `scene_compiled.xml` to identify import/export mismatches. Missing elements, wrong property values, or broken default inheritance will be visible in the diff. The compiled XML can also be loaded directly into native MuJoCo (`mj_loadXML`) to verify it produces the same behavior.

---

## Key File Reference

| Path | Purpose |
|---|---|
| `Source/URLab/Private/URLab.cpp` | Module startup, DLL loading |
| `Source/URLab/Public/MuJoCo/Core/AMjManager.h` | Manager actor header (thin coordinator) |
| `Source/URLab/Private/MuJoCo/Core/AMjManager.cpp` | Manager lifecycle, subsystem wiring |
| `Source/URLab/Public/MuJoCo/Core/MjPhysicsEngine.h` | Physics engine subsystem (spec, model, data, async loop) |
| `Source/URLab/Public/MuJoCo/Core/MjDebugVisualizer.h` | Debug visualization subsystem |
| `Source/URLab/Public/MuJoCo/Core/MjDebugTypes.h` | `FMuJoCoDebugData` shared struct |
| `Source/URLab/Public/MuJoCo/Net/MjNetworkManager.h` | Network/ZMQ discovery subsystem |
| `Source/URLab/Public/MuJoCo/Input/MjInputHandler.h` | Input/hotkey subsystem |
| `Source/URLab/Public/MuJoCo/Core/MjArticulation.h` | Articulation actor header |
| `Source/URLab/Private/MuJoCo/Core/MjArticulation.cpp` | Articulation setup, attach |
| `Source/URLab/Public/MuJoCo/Components/MjComponent.h` | Base component, BindToView |
| `Source/URLab/Public/MuJoCo/Utils/MjBind.h` | View structs (BodyView, GeomView, etc.) |
| `Source/URLab/Public/MuJoCo/Utils/MjUtils.h` | Coordinate conversions |
| `Source/URLab/Public/MuJoCo/Core/Spec/MjSpecWrapper.h` | Spec element creation wrapper |
| `Source/URLab/Public/MuJoCo/Components/Defaults/MjDefault.h` | Default system |
| `Source/URLabEditor/Private/MujocoImportFactory.cpp` | MJCF import pipeline |
| `Source/URLabEditor/Private/MjArticulationFactory.cpp` | Blueprint generation |
| `Source/URLab/Public/Replay/MjReplayTypes.h` | Replay data structures |
| `Source/URLab/Public/MuJoCo/Net/MjZmqComponent.h` | ZMQ base component |
