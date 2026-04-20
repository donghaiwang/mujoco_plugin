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
uv pip install -e ./RoboJuDo     # 策略框架 (内置的子模块)
```

仪表盘（关节、传感器、摄像头、执行器控制）无需策略扩展即可正常工作。只有当您需要运行神经网络策略时才需要 RoboJuDo。

需要 Python 3.11 或更高版本。


## 快速开始

```bash
# 启动仪表盘 (关节/传感器/相机 viewer, actuator control, optional policy runner)
uv run src/run.py --ui

# Run a specific policy headless
uv run src/run.py --policy unitree_12dof --prefix g1

# Test ZMQ connection
uv run src/run.py --test --prefix g1

```

## Available Policies

| Key                | Robot   | DOF | Description                              | Requires PHC |
|--------------------|---------|-----|------------------------------------------|:------------:|
| `unitree_12dof`    | G1      | 12  | Basic walking -- WASD twist control      |              |
| `unitree_wo_gait`  | G1      | 29  | Full body walking without gait clock     |              |
| `smooth`           | G1      | 29  | Smoother walking policy                  |              |
| `beyondmimic_dance`| G1      | 29  | Motion imitation -- dance                |      Y       |
| `h2h`             | G1      | 21  | Human motion retargeting                 |      Y       |
| `amo`             | G1      | 29  | Adaptive motion optimization             |      Y       |
| `twist_tracker`   | G1      | 12  | Motion tracker with twist                |      Y       |
| `go2_wtw`         | Go2     | 12  | Walk-These-Ways rough-terrain locomotion |              |

Policies marked **PHC** require the [PHC submodule](https://github.com/ZhengyiLuo/PHC) installed inside RoboJuDo.

## ZMQ Protocol

URLab publishes binary-packed data over ZeroMQ PUB/SUB sockets. All topics are prefixed with the articulation name (e.g. `g1/`).

| Topic Pattern          | Direction       | Payload Format               |
|------------------------|-----------------|------------------------------|
| `{prefix}/joint/{id}`  | Unreal -> Python | `<Ifff` (ID, pos, vel, acc) |
| `{prefix}/sensor/{name}` | Unreal -> Python | `<I` ID + `<I` dim + `f`*N floats |
| `{prefix}/camera/{name}` | Unreal -> Python | Raw BGRA bytes (dedicated socket) |
| `{prefix}/control`     | Python -> Unreal | `<I` count + (`<If`)*N (ID, value) pairs |

- **State socket** (default `tcp://127.0.0.1:5555`): joints + sensors at up to 1000 Hz.
- **Control socket** (default `tcp://127.0.0.1:5556`): policy sends target positions.
- **Camera socket** (default `tcp://127.0.0.1:5558`): high-bandwidth image stream on a separate socket.

## ROS 2 Bridge

`ros2_broadcaster.py` republishes ZMQ streams as standard ROS 2 topics (JointState, Image, Float64MultiArray). Requires a sourced ROS 2 workspace (Humble/Jazzy).

```bash
source /opt/ros/humble/setup.bash
uv run src/ros2_broadcaster.py
```

## License

Apache 2.0 -- see [LICENSE](LICENSE).

Copyright 2026 Jonathan Embley-Riches.

## Related

This package is the Python companion to [Unreal Robotics Lab](https://github.com/URLab-Sim/UnrealRoboticsLab), an Unreal Engine plugin embedding MuJoCo physics for sim-to-real robotics research.