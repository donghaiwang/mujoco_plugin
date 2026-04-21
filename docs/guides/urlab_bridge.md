# urlab_bridge

用于 [虚幻机器人实验室 (Unreal Robotics Lab, ULab)](https://github.com/URLab-Sim/UnrealRoboticsLab) 的 Python 中间件，即通过 ZeroMQ 将神经网络策略连接到 MuJoCo-in-Unreal 仿真。

运行预训练的运动策略、可视化关节状态和摄像头流，或将所有内容桥接到 ROS 2，即所有操作均可通过单个 Python 包完成。


## 安装

```bash
conda activate hutb_3.11
powershell -ExecutionPolicy ByPass -c "irm https://astral.sh/uv/install.ps1 | iex"
# 重新打开cmd
# 递归克隆仓库
git clone https://github.com/donghaiwang/URLab_Bridge.git --recursive

# 推荐 uv，安装方法：https://hellowac.github.io/uv-zh-cn/getting-started/installation/#__tabbed_1_2
cd URLab_Bridge
uv sync                          # 核心依赖 (ZMQ, NumPy, OpenCV, DearPyGui)

cd 
pip install -e .

# 运行策略 (可选):
uv sync --extra policy            # + PyTorch, ONNX 等
uv pip install -e ./RoboJuDo     # 策略框架 (内置的子模块)：安装模块 robojudo
```

仪表盘（关节、传感器、摄像头、执行器控制）无需策略扩展即可正常工作。只有当您需要运行神经网络策略时才需要 RoboJuDo。

需要 Python 3.11 或更高版本。


## 快速开始

```bash
# 启动仪表盘 (关节/传感器/相机查看器、执行器控制、可选策略运行器)，注意在关卡中必须放置 AMjManager，否则连接不上
uv run src/run.py --ui

# 以无头模式（不打开图形界面）运行特定策略
uv run src/run.py --policy unitree_12dof --prefix g1

# 测试 ZMQ 连接
uv run src/run.py --test --prefix g1
```

## 可用策略

| 关键字                | 机器人   | 自由度（Degrees of Freedom, DOF） | 描述                              | 需要永久人形控制（Perpetual Humanoid Control , PHC） |
|--------------------|---------|-----|------------------------------------------|:------------:|
| `unitree_12dof`    | G1      | 12  | 基本行走——WASD 扭转控制      |              |
| `unitree_wo_gait`  | G1      | 29  | 无需步态时钟即可进行全身行走     |              |
| `smooth`           | G1      | 29  | 更平稳的步行策略                  |              |
| `beyondmimic_dance`| G1      | 29  | 动作模仿——舞蹈                |      Y       |
| `h2h`             | G1      | 21  | 人体运动重定向                 |      Y       |
| `amo`             | G1      | 29  | 自适应运动优化             |      Y       |
| `twist_tracker`   | G1      | 12  | 带扭动功能的运动追踪器                |      Y       |
| `go2_wtw`         | Go2     | 12  | 沿着这些路行走的崎岖地形 |              |

标记为 **PHC** 的策略需要 RoboJuDo 内部安装 [PHC 子模块](https://github.com/ZhengyiLuo/PHC)。


## ZMQ 协议

URLab 通过 ZeroMQ PUB/SUB 套接字发布二进制打包的数据。所有主题都以划分名称为前缀（例如 `g1/`）。


| 主题模式          | 方向       | 有效载荷格式               |
|------------------------|-----------------|------------------------------|
| `{prefix}/joint/{id}`  | Unreal -> Python | `<Ifff` (ID, pos, vel, acc) |
| `{prefix}/sensor/{name}` | Unreal -> Python | `<I` ID + `<I` dim + `f`*N floats |
| `{prefix}/camera/{name}` | Unreal -> Python | Raw BGRA bytes (dedicated socket) |
| `{prefix}/control`     | Python -> Unreal | `<I` count + (`<If`)*N (ID, value) pairs |

- **状态套接字**（默认 `tcp://127.0.0.1:5555`）：关节和传感器数据，最高采样率达 1000 Hz。
- **控制套接字**（默认 `tcp://127.0.0.1:5556`）：策略发送目标位置。
- **相机套接字**（默认 `tcp://127.0.0.1:5558`）：通过单独的套接字传输高带宽图像流。

## ROS 2 桥接

`ros2_broadcaster.py` 将 ZMQ 流重新发布为标准的 ROS 2 主题（JointState、Image、Float64MultiArray）。需要已加载的 ROS 2 工作空间（Humble/Jazzy）。

```bash
source /opt/ros/humble/setup.bash
uv run src/ros2_broadcaster.py
```

## 许可证

Apache 2.0 -- see [LICENSE](LICENSE).

Copyright 2026 Jonathan Embley-Riches.

## 相关软件包

该软件包是 [Unreal Robotics Lab](https://github.com/URLab-Sim/UnrealRoboticsLab) 的 Python 配套软件包，Unreal Robotics Lab 是一个 Unreal Engine 插件，嵌入了 MuJoCo 物理引擎，用于模拟到现实的机器人研究。
