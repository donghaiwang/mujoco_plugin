# 柔性组件（flexible component, Flexcomp） — 可变形体

Flexcomp 是 MuJoCo 的宏，用于生成可变形软体：绳索（ropes, 1D）、布料（cloth, 2D）和体积体（volumetric bodies, 3D）。ULab 的 `UMjFlexcomp` 组件封装了此功能，使您可以在 Unreal 中创建、导入、模拟和可视化柔性体。


## 生成内容

单个 `<flexcomp>` 在编译时会展开为：

- 一个 `<flex>` 元素（实际可变形部分，具有物理属性）
- 多个子体 — 每个非固定顶点对应一个子体（对于 `dof="full"` / `"radial"`)，或 8 个角节点（对于 `dof="trilinear"`)，或 27 个角节点（对于 `dof="quadratic"`)） 
- 每个生成的子体上都有滑块关节

URLab 利用 MuJoCo 自带的宏展开器——我们将组件的属性序列化回 `<flexcomp>` XML 片段，使用 `mj_parseXMLString` 解析它，然后通过 `mjs_attach` 将生成的子规范附加到父组件主体中。MuJoCo 支持的所有 flex 特性（网格/面片/直接几何体、引脚、弹性、边刚度、接触过滤）均可自动工作。


## 快速导入

标准的拖放式 MJCF 导入流程可原生处理 `<flexcomp>`。引用网格的文件（例如 `<flexcomp type="mesh" file="bunny.obj">`）会与其他网格资源一起，由 Python 预处理器自动转换为 GLB 格式。导入的蓝图包含一个所有属性都已填充的 `UMjFlexcomp` 组件，并且对于 `type="mesh`，还包含一个用作渲染源的子 `UStaticMeshComponent`。

请参阅兔子（bunny）示例：

```xml
<flexcomp type="mesh" file="bunny.obj" pos="0 0 .1" dim="2" euler="90 0 0"
          radius=".001" rgba="0 .7 .7 1" mass=".05" name="softbody" dof="trilinear">
    <elasticity young="1e3" poisson="0.1" damping="0.001" elastic2d="stretch"/>
    <contact selfcollide="none" internal="false"/>
</flexcomp>
```

## 在蓝图中进行创作

向 `AMjArticulation` 蓝图添加 `UMjFlexcomp` 组件。必需属性：

| 属性 | 默认 | 笔记 |
|---|---|---|
| `Type` | Grid | grid / box / cylinder / ellipsoid / square / disc / circle / mesh / direct |
| `Dim` | 2 | 1 = lines, 2 = triangles, 3 = tetrahedra |
| `DofType` | Full | full (3 joints/vert), radial (1 joint/vert along radius), trilinear (8 corner bodies), quadratic (27 corner bodies) |
| `Count` / `Spacing` | 10×10×1 / 0.05 | For grid/box/cylinder/ellipsoid types |
| `MeshFile` | *(empty)* | For `type=mesh`, set automatically on import. Also add a child `UStaticMeshComponent` with the mesh asset. |
| `PointData` / `ElementData` | empty | For `type=direct`, raw vertex positions and element indices |

可选的物理/视觉属性遵循插件中通用的 `bOverride_X` 模式——切换以覆盖 MuJoCo 默认值，关闭则继承默认值。类别：接触、边缘、弹性、销钉。

## 可视化

运行时，`UMjFlexcomp::Bind` 会创建一个 `UDynamicMeshComponent`，该组件镜像源静态网格（几何体、UV 和法线均从 LOD 0 复制）。每个游戏节拍，都会从 `mjData.flexvert_xpos` 读取变形后的顶点位置，通过 welded→raw 重映射表进行映射，并通过 `FastNotifyPositionsUpdated` 推送。源静态网格会被隐藏；其材质会在动态网格上重用。

**已知限制——TAA 重影。** 由于 UE 的 `FLocalVertexFactory` 没有前一帧位置顶点流，也没有公开的 API 提供此功能，因此 CPU 变形的网格会产生零运动矢量。时间抗锯齿 (Temporal Anti-Aliasing, TAA) 会累积过时的前一帧采样，从而在快速变形时产生可见的拖影/重影。解决方法：

- 对于弹性较大的场景，请将**Project Settings → Rendering → Anti-Aliasing Method**设置为 `FXAA`（或在 DefaultEngine.ini 中设置 `r.AntiAliasingMethod=1`）。 
- 使用缓慢、柔和的模拟，以减少影响。
- 未来的彻底修复方案需要自定义顶点工厂，并添加前一帧位置流（约 1000 行代码 + 分支着色器），或者采用基于材质的 WPO 方法，并使用配对的当前位置/前一帧位置纹理。

## 支持的自由度（Degree of Freedom, DOF）模式

| DOF | 生成的刚体 | 用例 |
|---|---|---|
| `full` | 每个非固定顶点对应一个实体，每个实体有3个滑块关节。 | 高保真软体，其中每个顶点均可独立运动 |
| `radial` | 每个非铰接顶点一个实体，1 个径向滑块关节 | 充气形状（仅限径向变形） |
| `trilinear` | 8 个角体，每个角体 3 个滑块（总共 24 个自由度） | 整体形状的平滑形变；可扩展至高顶点数 |
| `quadratic` | 27 个角体，每个角体 3 个滑块（总共 81 个自由度） | 高阶形变；比三线性形变细节更丰富 |

对于拥有数千个顶点的网格，请使用三线性插值 `trilinear`。对一个拥有 2500 个顶点的兔子进行完全（`full`）自由度建模会生成大约 7500 个关节，速度很慢。

## 已知限制和待办事项

- 程序化可视化中的 **TAA 重影**（见上文）。
- 刚性柔性组件（**`bRigid` flexcomps**）作为静态表面附加到父物体上；它们不会变形。 
- **原始 `<flex>` 元素** （不带 `<flexcomp>` 宏）不被直接支持——作者应使用 `<flexcomp>` 来生成它们。 
- 尚未实现将**每个顶点的速度输出**到 UE 的速度缓冲区——这会阻止对变形几何体进行正确的运动模糊和时间超分辨率（Temporal Super Resolution, TSR） 处理。  
