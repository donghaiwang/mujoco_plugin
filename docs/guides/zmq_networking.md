# ZMQ 网络与 ROS 2


Unreal Robotics Lab 使用 [ZeroMQ](https://zeromq.org) 进行所有外部通信。其设计理念是：保持虚幻引擎端快速、二进制且无依赖——然后由一个独立的桥接器负责将其转换为用户偏好的任何框架。


---

## 为什么选择 ZMQ？

- **速度快。** 采用二进制发布/订阅模式，无序列化开销。控制循环以物理线程频率（通常为 500–1000 Hz）运行，不会因消息编码而出现瓶颈。
- **依赖项极少。** libzmq是插件内部唯一的网络依赖项。无需 ROS、中间件或构建系统集成。
- **用户选择。** 该插件不强制要求使用机器人框架。ZMQ 是原生传输协议；如果您需要 ROS 2，则可以使用单独的 urlab_bridge 进行转换。如果您需要纯 Python 代码，只需使用 `pyzmq` 即可。如果您需要其他格式，二进制格式也很容易解析。

---

## 插件端组件

三个组件负责网络连接，如果 `AAMjManager` 在 BeginPlay 时不存在，则所有组件都会自动创建：

| 组件 | 套接字 | 默认端点 | 目的 |
|-----------|--------|-------------------|---------|
| `UZmqSensorBroadcaster` | PUB | `tcp://*:5555` | 发布传感器数据、关节状态、扭转、动作 |
| `UZmqControlSubscriber` | SUB + PUB | `tcp://*:5556` (SUB), `tcp://*:5557` (info PUB) | 接收控制向量和增益更新；发布执行器信息 JSON |
| `UMjCamera` (ZMQ mode) | PUB | per-camera endpoint | 在专用线程上流式传输渲染的相机帧（使用 `SCS_FinalToneCurveHDR` 捕获源，因此会考虑后处理体积） |

所有虚幻引擎端的套接字都使用 `zmq_bind()` 函数。外部客户端使用 `zmq_connect()` 函数。

### Timing

ZMQ callbacks run directly on the physics thread:

1. **PreStep** — `UZmqControlSubscriber` reads incoming commands → writes to `mjData.ctrl`
2. **mj_step()** — physics advances
3. **PostStep** — `UZmqSensorBroadcaster` reads `mjData` → publishes

One-timestep latency between command and response. Camera frames run on their own background thread so they never stall the physics loop.

### Topics (Sensor Broadcast)

All messages are multipart: topic string + binary payload. Topics are prefixed with the articulation actor name:

| Topic Pattern | Payload | Source |
|---------------|---------|--------|
| `{Name}/joint/{JointName}` | `int32 id, float pos, float vel, float acc` (16 bytes) | `UMjJoint::BuildBinaryPayload` |
| `{Name}/sensor/{SensorName}` | `int32 id, float[] values` (4 + 4*dim bytes) | `UMjSensor::BuildBinaryPayload` |
| `{Name}/base_state/{JointName}` | `7 x float64` (pos xyz + quat wxyz) | `UMjFreeJoint::BuildBinaryPayload` |
| `{Name}/twist` | `3 x float32`: vx, vy, yaw_rate | TwistController |
| `{Name}/actions` | `int32` bitmask | TwistController (only sent when non-zero) |

### Topics (Control Receive)

The control subscriber filters on `{Name}/control` and `{Name}/set_gains`:

| Topic Pattern | Payload |
|---------------|---------|
| `{Name}/control` | `int32 count`, then `count` x `(int32 actuator_id, float value)` |
| `{Name}/set_gains` | JSON: `{"joint_name": {"kp": float, "kv": float, "torque_limit": float}, ...}` |

### Info Broadcast (Port 5557)

The control subscriber periodically publishes a JSON discovery message on the info endpoint containing actuator names, IDs, ranges, and camera endpoints. This is sent frequently at startup (every 50 steps for the first 5s), then every 500 steps (~1s).

> **Multi-articulation filtering:** In multi-robot scenes, subscribe with the articulation name prefix (e.g., `sub.setsockopt_string(zmq.SUBSCRIBE, "Robot_A/")`) to receive only that robot's data.

### Control Source

`EControlSource` determines whether actuators respond to ZMQ or dashboard input. Set it globally on the manager or per-articulation:

```
Manager->SetControlSource(EControlSource::ZMQ);
```

---

## urlab_bridge (ROS 2)

The **urlab_bridge** (separate companion repository, same GitHub organization) is the Python-side middleware. It sits between the plugin's ZMQ streams and any external system (ROS 2, RL policies, custom scripts):

```
Unreal (ZMQ binary) → urlab_bridge → ROS 2 topics
```

It subscribes to the ZMQ sensor and camera endpoints, unpacks the binary payloads, and publishes to standard ROS 2 topics (`/joint_states`, `/sensor_data`, `/camera/image_raw`). Multi-robot namespacing is handled automatically.

**Why a separate bridge?**

- Keeps the Unreal plugin free of ROS build dependencies (ament, colcon, etc.)
- The bridge is pure Python with `pyzmq` + `rclpy` — easy to install, easy to modify
- Users who do not need ROS never have to think about it
- Users who do need ROS get standard topic interfaces without any plugin changes

### Quick test (no ROS needed)

```bash
uv run src/zmq_visualizer.py \
    --main_endpoint="tcp://127.0.0.1:5555" \
    --camera_endpoint="tcp://127.0.0.1:5558"
```

Prints live joint states and opens an OpenCV window for camera frames.

### ROS 2 rebroadcaster

```bash
# Source your ROS 2 environment first (Humble, Jazzy, etc.)
uv run src/ros2_broadcaster.py \
    --main_endpoint="tcp://127.0.0.1:5555" \
    --camera_endpoint="tcp://127.0.0.1:5558"
```

---

## Connecting from Python

Minimal `pyzmq` example:

```python
import zmq, struct, numpy as np

ctx = zmq.Context()

# Receive sensor data (Unreal binds, we connect)
sub = ctx.socket(zmq.SUB)
sub.connect("tcp://127.0.0.1:5555")
sub.setsockopt_string(zmq.SUBSCRIBE, "MyRobot/")

# Send controls (Unreal binds SUB on 5556, we connect PUB)
pub = ctx.socket(zmq.PUB)
pub.connect("tcp://127.0.0.1:5556")

while True:
    topic, data = sub.recv_multipart()
    topic_str = topic.decode()

    if "/joint/" in topic_str:
        jid, pos, vel, acc = struct.unpack("<Ifff", data)
        print(f"Joint {jid}: pos={pos:.3f}")

    # Send control: 3 actuators example
    num = 3
    payload = struct.pack("<I", num)  # count
    for i in range(num):
        payload += struct.pack("<If", i, 0.0)  # (id, value) pairs
    pub.send_multipart([b"MyRobot/control ", payload])
```

---

## Troubleshooting

**No data arriving** — check that endpoints match (protocol, IP, port). All Unreal-side sockets *bind*; external clients *connect*.

**Controls have no effect** — verify `ControlSource` is set to `ZMQ`. Note the control topic requires a trailing space in the subscription filter (e.g., `"MyRobot/control "`).

**Camera frames blank** — make sure `bEnableZmqBroadcast = true` on the camera component and the model compiled successfully.
