# 特性

## 核心架构

- 可用完整的 MuJoCo C API (`mjModel*`, `mjData*`) -- 用户可以直接调用任何 MuJoCo 函数。
- 具有可配置时间步长的异步物理线程
- 基于组件的设计与 MJCF 元素层次结构相匹配
- `MjManager` 单例负责协调编译、步进和状态

## MJCF 导入

支持将 XML 文件拖放到内容浏览器(Content Browser)中。自动生成包含完整组件层级结构、网格转换、材质导入和默认类继承的虚幻引擎蓝图。

详情请参阅 [MuJoCo 导入](guides/mujoco_import.md) 。

## 快速转换

- 一键将任何 Unreal 静态网格体转换为 MuJoCo 物理实体
- 具有可调阈值（0.01-1.0）的 CoACD 凸分解
- 视觉/碰撞网格分离（第 2 组视觉，第 3 组碰撞）
- 静态、动态或动作捕捉驱动模式
- 摩擦力，求解器参数可按组件配置

## 高度场地形

- 将虚幻场景转换为 MuJoCo 高度场碰撞
- 可配置的采样分辨率（2-512）
- 用于精确网格采样的复杂轨迹模式
- 动作者白名单过滤
- 用于快速重新编译的二进制缓存
- 编辑器网格预览可视化

## 物理模拟

- 求解器: PGS, CG, 牛顿法
- 积分器：欧拉积分器、RK4积分器、隐式积分器、快速隐式积分器
- 模拟速度控制 (5-100%)
- 暂停、恢复、重置、单步
- 快照捕获和恢复（完整状态）
- 关键帧重置并保持
- 提供个性化睡眠支持
- 多连续碰撞检测 (Continuous Collision Detection,CCD) 
- 重力、风、密度、粘度

## 执行器（8 种类型）

运动、位置、速度、IntVelocity、阻尼器、Cylinder、肌肉、粘附力

- 所有传动类型：关节(joint)、肌腱(tendon)、位点(site)、刚体(body)、滑块曲柄(slider-crank)
- 通过 `SetControl()` (BlueprintCallable)进行运行时控制
- 增益/偏置参数数组
- 控制/力/激活范围限制

## 传感器（40多种类型）

位点附属: 触摸传感器、加速度计、速度计、陀螺仪、力传感器、扭矩传感器、磁力计、测距仪(RangeFinder)、摄像头投影(CamProjection)

关节/肌腱/执行器: 每个关节/肌腱/执行器的位置、速度和力矩。

球形关节: Quat, AngVel

坐标系: Pos, Quat, XYZ 轴, LinVel, AngVel, LinAcc, AngAcc

子树: COM, LinVel, AngMom

Geometric: InsideSite, GeomDist, GeomNormal, GeomFromTo

全局: 接触(Contact), EPotential, EKinetic, Clock

- 所有内容均可通过 `GetSensorReading()` / `GetSensorScalar()` 在连接处读取（BlueprintCallable）

## 控制器

[比例微分](https://baike.baidu.com/item/%E6%AF%94%E4%BE%8B%E5%BE%AE%E5%88%86%E6%8E%A7%E5%88%B6%E5%99%A8/5459579) (proportional plus derivative, PD)控制器、直通(passthrough)、关键帧和扭转控制器用于控制关节运动。关键帧控制器内置 Franka、UR5e、Spot、ANYmal 和 Go2 的预设。

详情请参阅 [控制器框架](guides/controller_framework.md) 。

## 调试可视化


在编辑器运行(PlayInEditor, PIE) 期间，可通过快捷键切换接触力、碰撞线框、关节轴和限制弧、可视网格可见性、约束岛(constraint-island)着色、带睡眠调制(sleep modulation)功能的实例和语义分割着色器，以及具有激活驱动颜色和半径的平滑肌/肌腱管。

有关所有快捷键，请参阅蓝图参考。有关完整的快捷键表和各模式的详细信息，请参阅调试可视化。

请参阅 [蓝图参考文档](guides/blueprint_reference.md) 以获取所有快捷键。

有关完整的热键表和每种模式的详细信息，请参阅 [调试可视化](guides/debug_visualization.md) 。


## 相机系统

- **MjCamera**: 场景捕获功能，连接到 MuJoCo 位点，通过 ZMQ 进行流传输，并遵循后期处理体积(volume)。
- **MjOrbitCamera**: 具备目标检测、高度摆动和可配置焦距的自动旋转影视级摄像机
- **MjKeyframeCamera**: 基于路径点的相机路径，具有平滑插值、`O` 关键帧播放/暂停功能

有关相机 API 的详细信息，请参阅 [蓝图参考文档](guides/blueprint_reference.md#mjkeyframecameraactor) 。

## 网络 (Zero Message Queue、ZMQ)

采用二进制发布/订阅协议广播关节状态、传感器和摄像头图像。接收执行器目标值和PD增益。针对多关节场景采用基于前缀的滤波。

详情请参阅 [ZMQ 网络文档](guides/zmq_networking.md) 。


## Python 集成 (urlab_bridge)

- **策略 GUI**: DearPyGui 控制面板，用于策略选择、执行器目标定位、力扭转覆盖、运动/步态预设选择。
- **UnrealEnv**: 基于 ZMQ 的 RoboJuDo 兼容环境，具备自动检测、正向运动学和出生点对齐功能
- 预集成策略：Unitree G1 移动（12/29 自由度）、BeyondMimic（舞蹈/小提琴/华尔兹/跳跃）、Walk-These-Ways Go2、AMO、H2H、Smooth、ASAP
- **ROS 2 桥接器**: 将 ZMQ 流重新发布到标准 ROS 2 主题

## 重放系统

记录和重放完整的仿真状态。支持 CSV 轨迹导入、快照捕获/恢复和多会话管理。

有关记录/回放 API 的信息，请参阅 [蓝图参考文档](guides/blueprint_reference.md) 。

## 蓝图 API

40 多个 BlueprintCallable 函数，用于模拟控制、状态查询、执行器命令、快照和重放。

有关完整的 API 参考，请参阅 [使用蓝图编写脚本](guides/blueprint_reference.md) 。

## MjSimulate 小部件

- 编辑器内仪表板复制了 MuJoCo 的模拟用户界面
- 物理选项：时间步长、求解器、积分器、迭代次数、模拟速度
- 每个执行器控制滑块
- 关节状态读数
- 传感器值显示
- 摄像头画面缩略图
- 执行器选择下拉菜单
- 记录/回放/快照的控制
- 拥有用于弹簧臂相机跟随的按钮

## 力

- **MjImpulseLauncher**: 基于速度的对象启动（与 MuJoCo 的 object_launcher_plugin 方法相匹配）
- 直接模式（箭头方向+速度）或目标模式（弹道弧线指向目标参与者）
- 在 PIE 期间进行迭代测试的重置和触发
- 按 `F` 键触发场景中的所有启动器

## 模拟快捷键

请参阅 [快捷键](guides/blueprint_reference.md#hotkeys) 以获取完整的键盘快捷键列表。

## 网格制备工具

- `Scripts/clean_meshes.py`: 支持 XML 的网状转换器
- 解析 MJCF，检测 GLB stem 冲突，自动重命名
- 将 OBJ/STL 转换为 GLB 并保留 UV 值
- 移除嵌入纹理以防止 Interchange 导入冲突
- 输出已更新，`_ue.xml` 可进行拖放导入
