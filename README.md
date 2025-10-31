# Mujoco 插件

该插件将 Mujoco 物理引擎与引擎集成在一起，使您可以将 Mujoco XML 文件直接加载到引擎中并运行高级物理模拟。

## 特性

- 将 Mujoco XML 文件加载到虚幻引擎中
- 运行Mujoco模拟并显示实时结果
- 支持非主要 MuJoCo 形状的程序化网格生成
- 从 MuJoCo 模型导入对象的颜色
- 支持多个同时模拟实例

## 演示

![Simulation Demo](https://cdn.loom.com/sessions/thumbnails/ec26c892b5014a03afb7d016b2d4b4d5-88ba2fce00140e4d-full-play.gif)

![Simulation Demo 2](https://cdn.loom.com/sessions/thumbnails/c750fc543f3548208ad88d14b0447251-beb5032d0c1cdf27-full-play.gif)

![Simulation Demo 3](https://cdn.loom.com/sessions/thumbnails/81d84c9a8565465199aae22d4d5e627c-47d9ea28f3266022-full-play.gif)

## 安装

1. 克隆此存储库到您的虚幻引擎项目的插件`Plugins`文件夹
2. 重新构建您的项目
3. 在项目设置中启用 Mujoco 插件


## 用法

### 基本设置

1. 在您的关卡中放置一个`MuJoCoSimulation`参与者
2. 在参与者属性中设置 XML 文件路径，比如人形机器人`../Plugins/mujoco_plugin/Content/model/humanoid/humanoid.xml`、摆锤：
`../Plugins/mujoco_plugin/Content/model/pendulum.xml`


3. 开始播放模式以查看模拟

### 控制

- **Z 键**: 保持则运行模拟，释放则暂停
- **R 键**: 重置模拟到初始状态
- **C 键**: 测试 Mujoco 执行器的控制（将执行器 0 设置为一个小值，可用于诸如 car.xml 之类的测试模型）

## 当前的限制

- 尚未实现纹理的支持（仅导入颜色）
- 它仍然很粗糙，并且没有针对性能进行优化


### 移植到 UE 4.26

根据`MujocoTest.uproject`生成vs工程报错：
```shell

Running D:/work/workspace/engine/Engine/Binaries/DotNET/UnrealBuildTool.exe  -projectfiles -project="D:/hutb/Unreal/CarlaUE4/Plugins/mujoco_plugin/MujocoTest.uproject" -game -engine -progress -log="D:\hutb\Unreal\CarlaUE4\Plugins\mujoco_plugin/Saved/Logs/UnrealVersionSelector-2025.09.10-09.45.53.log"
Discovering modules, targets and source code for project...
While compiling D:\hutb\Unreal\CarlaUE4\Plugins\mujoco_plugin\Intermediate\Build\BuildRules\MujocoTestModuleRules.dll:
d:\hutb\Unreal\CarlaUE4\Plugins\mujoco_plugin\Source\MujocoTest.Target.cs(11,47) : error CS0117: “UnrealBuildTool.BuildSettingsVersion”并不包含“V5”的定义
d:\hutb\Unreal\CarlaUE4\Plugins\mujoco_plugin\Source\MujocoTest.Target.cs(12,3) : error CS0103: 当前上下文中不存在名称“IncludeOrderVersion”
d:\hutb\Unreal\CarlaUE4\Plugins\mujoco_plugin\Source\MujocoTest.Target.cs(12,25) : error CS0103: 当前上下文中不存在名称“EngineIncludeOrderVersion”
d:\hutb\Unreal\CarlaUE4\Plugins\mujoco_plugin\Source\MujocoTestEditor.Target.cs(11,47) : error CS0117: “UnrealBuildTool.BuildSettingsVersion”并不包含“V5”的定义
d:\hutb\Unreal\CarlaUE4\Plugins\mujoco_plugin\Source\MujocoTestEditor.Target.cs(12,3) : error CS0103: 当前上下文中不存在名称“IncludeOrderVersion”
d:\hutb\Unreal\CarlaUE4\Plugins\mujoco_plugin\Source\MujocoTestEditor.Target.cs(12,25) : error CS0103: 当前上下文中不存在名称“EngineIncludeOrderVersion”
ERROR: Unable to compile source files.

```



编译报错：
```shell
  Plugins\MuJoCoUE\Source\mujoco\include\mujoco/mjspec.h(43): error C2039: byte' is not a member of 'std'
  D:\hutb\Unreal\CarlaUE4\Plugins\mujoco_plugin\Plugins\MuJoCoUE\Source\mujoco\include\mujoco/mjspec.h(43): error C2039: "byte": 不是 "std" 的成员
  C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\INCLUDE\vector(29): note: 参见“std”的声明
  D:\hutb\Unreal\CarlaUE4\Plugins\mujoco_plugin\Plugins\MuJoCoUE\Source\mujoco\include\mujoco/mjspec.h(43): error C2065: “byte”: 未声明的标识符
  D:\hutb\Unreal\CarlaUE4\Plugins\mujoco_plugin\Plugins\MuJoCoUE\Source\mujoco\include\mujoco/mjspec.h(43): error C2923: "std::vector": "byte" 不是参数 "_Ty" 的有效 模板 类型参数
  D:\hutb\Unreal\CarlaUE4\Plugins\mujoco_plugin\Plugins\MuJoCoUE\Source\mujoco\include\mujoco/mjspec.h(43): note: 参见“byte”的声明
  D:\hutb\Unreal\CarlaUE4\Plugins\mujoco_plugin\Plugins\MuJoCoUE\Source\mujoco\include\mujoco/mjspec.h(43): error C2976: “std::vector'”: 模板 参数太少
  C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\INCLUDE\vector(442): note: 参见“std::vector”的声明
  [6/13] Module.MuJoCoUE.cpp
  D:\hutb\Unreal\CarlaUE4\Plugins\mujoco_plugin\Plugins\MuJoCoUE\Source\mujoco\include\mujoco/mjspec.h(43): error C2039: "byte": 不是 "std" 的成员
  C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\INCLUDE\vector(29): note: 参见“std”的声明
  D:\hutb\Unreal\CarlaUE4\Plugins\mujoco_plugin\Plugins\MuJoCoUE\Source\mujoco\include\mujoco/mjspec.h(43): error C2065: “byte”: 未声明的标识符
  D:\hutb\Unreal\CarlaUE4\Plugins\mujoco_plugin\Plugins\MuJoCoUE\Source\mujoco\include\mujoco/mjspec.h(43): error C2923: "std::vector": "byte" 不是参数 "_Ty" 的有效 模板 类型参数
  D:\hutb\Unreal\CarlaUE4\Plugins\mujoco_plugin\Plugins\MuJoCoUE\Source\mujoco\include\mujoco/mjspec.h(43): note: 参见“byte”的声明
  D:\hutb\Unreal\CarlaUE4\Plugins\mujoco_plugin\Plugins\MuJoCoUE\Source\mujoco\include\mujoco/mjspec.h(43): error C2976: “std::vector'”: 模板 参数太少
  C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\INCLUDE\vector(442): note: 参见“std::vector”的声明
  [7/13] UE4Editor-WmfMedia.dll
    正在创建库 D:\work\workspace\engine\Engine\Plugins\Media\WmfMedia\Intermediate\Build\Win64\UE4Editor\Development\WmfMedia\UE4Editor-WmfMedia.suppressed.lib 和对象 D:\work\workspace\engine\Engine\Plugins\Media\WmfMedia\Intermediate\Build\Win64\UE4Editor\Development\WmfMedia\UE4Editor-WmfMedia.suppressed.exp
  [8/13] Module.PixelStreaming.cpp
make: *** [CarlaUE4Editor] 错误 6
```

C++17引入了类型std::byte，代码中使用了using namespace std 与原C++定义的unsigned char byte冲突；或者其它重名冲突。C++17还引进了std::array ，std::size，也会导致同样的问题。

解决方法：
如果代码中加了using namespace std ，可以将它移除；


虚幻引擎 `virtual FArchive& operator<<(FSoftObjectPath& Value) override;` 报错：
```text
E1455: member function declared with ‘override’ does not override a base class member.
```

编辑器偏好设置（Editor Preferences） > 通用（General） > 实时编码（Live Coding）  启用 live coding

Live Coding 是 Unreal Engine 4.22+ 引入的更强大的实时编译功能，允许在编辑器运行时直接修改和重新编译 C++ 代码，甚至支持新增类、函数、变量等。


```
Building 5 actions with 7 processes...
  [1/5] Module.MuJoCoUE.gen.cpp
  [2/5] Module.MuJoCoUE.cpp
  [3/5] UE4Editor-MuJoCoUE.lib
    正在创建库 D:\hutb\Unreal\CarlaUE4\Plugins\mujoco_plugin\Plugins\MuJoCoUE\Intermediate\Build\Win64\UE4Editor\Development\MuJoCoUE\UE4Editor-MuJoCoUE.lib 和对象 D:\hutb\Unreal\CarlaUE4\Plugins\mujoco_plugin\Plugins\MuJoCoUE\Intermediate\Build\Win64\UE4Editor\Development\MuJoCoUE\UE4Editor-MuJoCoUE.exp
  [4/5] UE4Editor-MuJoCoUE.dll
    正在创建库 D:\hutb\Unreal\CarlaUE4\Plugins\mujoco_plugin\Plugins\MuJoCoUE\Intermediate\Build\Win64\UE4Editor\Development\MuJoCoUE\UE4Editor-MuJoCoUE.suppressed.lib 和对象 D:\hutb\Unreal\CarlaUE4\Plugins\mujoco_plugin\Plugins\MuJoCoUE\Intermediate\Build\Win64\UE4Editor\Development\MuJoCoUE\UE4Editor-MuJoCoUE.suppressed.exp
  Module.MuJoCoUE.cpp.obj : error LNK2019: 无法解析的外部符号 "__declspec(dllimport) public: __cdecl FMeshDescription::~FMeshDescription(void)" (__imp_??1FMeshDescription@@QEAA@XZ)，函数 "protected: void __cdecl AMuJoCoSimulation::GenerateMeshes(struct ModelInfo &)" (?GenerateMeshes@AMuJoCoSimulation@@IEAAXAEAUModelInfo@@@Z) 中引用了该符号
  Module.MuJoCoUE.cpp.obj : error LNK2001: 无法解析的外部符号 "public: __cdecl FMeshDescription::~FMeshDescription(void)" (??1FMeshDescription@@QEAA@XZ)
  D:\hutb\Unreal\CarlaUE4\Plugins\mujoco_plugin\Plugins\MuJoCoUE\Binaries\Win64\UE4Editor-MuJoCoUE.dll : fatal error LNK1120: 2 个无法解析的外部命令
```
解决：在.cs中添加相应的模块。



打包时因为资产不兼容报错：
```
LogInit: Display:
  LogInit: Display: Warning/Error Summary (Unique only)
  LogInit: Display: -----------------------------------
  LogInit: Display: LogAssetRegistry: Error: Package ../../../../../../hutb/Unreal/CarlaUE4/Content/mujoco/test1_HLOD0_Instancing.uasset is too old
  LogInit: Display: LogAssetRegistry: Error: Package ../../../../../../hutb/Unreal/CarlaUE4/Content/mujoco/PC_Test.uasset is too old
  LogInit: Display: LogAssetRegistry: Error: Package ../../../../../../hutb/Unreal/CarlaUE4/Content/mujoco/GM_test.uasset is too old
  LogInit: Display: LogAssetRegistry: Error: Package ../../../../../../hutb/Unreal/CarlaUE4/Content/mujoco/MuJoCo/Cone1.uasset is too old
  LogInit: Display: LogAssetRegistry: Error: Package ../../../../../../hutb/Unreal/CarlaUE4/Content/mujoco/MujoCoSim.uasset is too old
  LogInit: Display: LogAssetRegistry: Error: Package ../../../../../../hutb/Unreal/CarlaUE4/Content/mujoco/MuJoCo/Cube2.uasset is too old
  LogInit: Display: LogAssetRegistry: Error: Package ../../../../../../hutb/Unreal/CarlaUE4/Content/mujoco/TestMap.umap is too old
  LogInit: Display: LogAssetRegistry: Error: Package ../../../../../../hutb/Unreal/CarlaUE4/Content/mujoco/MuJoCo/Cylinder1.uasset is too old
  LogInit: Display: LogAssetRegistry: Error: Package ../../../../../../hutb/Unreal/CarlaUE4/Content/mujoco/MuJoCo/myCylinder_A112A751.uasset is too old
  LogInit: Display: LogAssetRegistry: Error: Package ../../../../../../hutb/Unreal/CarlaUE4/Content/mujoco/MuJoCo/myCylinder2_116D46AF.uasset is too old
  LogInit: Display: LogAssetRegistry: Error: Package ../../../../../../hutb/Unreal/CarlaUE4/Content/mujoco/MuJoCo/Shape_Cylinder.uasset is too old
  LogInit: Display: LogAssetRegistry: Error: Package ../../../../../../hutb/Unreal/CarlaUE4/Content/mujoco/MuJoCo/Shape_NarrowCapsule.uasset is too old
  LogInit: Display: LogAssetRegistry: Error: Package ../../../../../../hutb/Unreal/CarlaUE4/Content/mujoco/MuJoCo/M_BaseColor.uasset is too old
  LogInit: Display: LogAssetRegistry: Error: Package ../../../../../../hutb/Unreal/CarlaUE4/Content/mujoco/MuJoCo/Shape_WideCapsule.uasset is too old
  LogInit: Display: LogAssetRegistry: Error: Package ../../../../../../hutb/Unreal/CarlaUE4/Content/mujoco/StarterContent/Architecture/SM_AssetPlatform.uasset is too old
  LogInit: Display: LogAssetRegistry: Error: Package ../../../../../../hutb/Unreal/CarlaUE4/Content/mujoco/MuJoCo/Sphere1.uasset is too old
  LogInit: Display: LogAssetRegistry: Error: Package ../../../../../../hutb/Unreal/CarlaUE4/Content/mujoco/StarterContent/Architecture/Floor_400x400.uasset is too old
  LogInit: Display: LogAssetRegistry: Error: Package ../../../../../../hutb/Unreal/CarlaUE4/Content/mujoco/MuJoCo/Plane1.uasset is too old
  LogInit: Display: LogAssetRegistry: Error: Package ../../../../../../hutb/Unreal/CarlaUE4/Content/mujoco/StarterContent/Architecture/Wall_400x300.uasset is too old
  LogInit: Display: LogAssetRegistry: Error: Package ../../../../../../hutb/Unreal/CarlaUE4/Content/mujoco/StarterContent/Architecture/Pillar_50x500.uasset is too old
  LogInit: Display: LogAssetRegistry: Error: Package ../../../../../../hutb/Unreal/CarlaUE4/Content/mujoco/StarterContent/Architecture/Wall_400x200.uasset is too old
  LogInit: Display: LogAssetRegistry: Error: Package ../../../../../../hutb/Unreal/CarlaUE4/Content/mujoco/StarterContent/Architecture/Wall_Door_400x400.uasset is too old
  LogInit: Display: LogAssetRegistry: Error: Package ../../../../../../hutb/Unreal/CarlaUE4/Content/mujoco/StarterContent/Architecture/Wall_500x500.uasset is too old
  LogInit: Display: LogAssetRegistry: Error: Package ../../../../../../hutb/Unreal/CarlaUE4/Content/mujoco/StarterContent/Architecture/Wall_Door_400x300.uasset is too old
  LogInit: Display: LogAssetRegistry: Error: Package ../../../../../../hutb/Unreal/CarlaUE4/Content/mujoco/StarterContent/Architecture/Wall_400x400.uasset is too old
  LogInit: Display: LogAssetRegistry: Error: Package ../../../../../../hutb/Unreal/CarlaUE4/Content/mujoco/StarterContent/Architecture/Wall_Window_400x300.uasset is too old
  LogInit: Display: LogAssetRegistry: Error: Package ../../../../../../hutb/Unreal/CarlaUE4/Content/mujoco/StarterContent/Architecture/Wall_Window_400x400.uasset is too old
  LogInit: Display: LogAssetRegistry: Error: Package ../../../../../../hutb/Unreal/CarlaUE4/Content/mujoco/StarterContent/Blueprints/Assets/M_LightStage_Arrows.uasset is too old
  LogInit: Display: LogAssetRegistry: Error: Package ../../../../../../hutb/Unreal/CarlaUE4/Content/mujoco/StarterContent/Blueprints/Assets/M_LightStage_Skybox_Black.uasset is too old
  LogInit: Display: LogAssetRegistry: Error: Package ../../../../../../hutb/Unreal/CarlaUE4/Content/mujoco/StarterContent/Blueprints/Assets/FogBrightnessLUT.uasset is too old
  LogInit: Display: LogAssetRegistry: Error: Package ../../../../../../hutb/Unreal/CarlaUE4/Content/mujoco/StarterContent/Blueprints/BP_LightStudio.uasset has newer custom version of Dev-Rendering
  LogInit: Display: LogAssetRegistry: Error: Package ../../../../../../hutb/Unreal/CarlaUE4/Content/mujoco/StarterContent/Blueprints/Assets/SunlightColorLUT.uasset is too old
  LogInit: Display: LogAssetRegistry: Error: Package ../../../../../../hutb/Unreal/CarlaUE4/Content/mujoco/StarterContent/Blueprints/Assets/M_LightStage_Skybox_HDRI.uasset is too old
  LogInit: Display: LogAssetRegistry: Error: Package ../../../../../../hutb/Unreal/CarlaUE4/Content/mujoco/StarterContent/Blueprints/Assets/SM_Arrows.uasset is too old
  LogInit: Display: LogAssetRegistry: Error: Package ../../../../../../hutb/Unreal/CarlaUE4/Content/mujoco/StarterContent/Blueprints/Assets/M_LightStage_Skybox_Master.uasset is too old
  LogInit: Display: LogAssetRegistry: Error: Package ../../../../../../hutb/Unreal/CarlaUE4/Content/mujoco/StarterContent/Maps/StarterMap.umap is too old
  LogInit: Display: LogAssetRegistry: Error: Package ../../../../../../hutb/Unreal/CarlaUE4/Content/mujoco/StarterContent/Maps/Minimal_Default.umap is too old
  LogInit: Display: LogAssetRegistry: Error: Package ../../../../../../hutb/Unreal/CarlaUE4/Content/mujoco/StarterContent/Blueprints/Assets/Skybox.uasset is too old
  LogInit: Display: LogAssetRegistry: Error: Package ../../../../../../hutb/Unreal/CarlaUE4/Content/mujoco/StarterContent/Materials/M_AssetPlatform.uasset is too old
  LogInit: Display: LogAssetRegistry: Error: Package ../../../../../../hutb/Unreal/CarlaUE4/Content/mujoco/StarterContent/Materials/M_Basic_Wall.uasset is too old
  LogInit: Display: LogAssetRegistry: Error: Package ../../../../../../hutb/Unreal/CarlaUE4/Content/mujoco/StarterContent/Materials/M_Basic_Floor.uasset is too old
  LogInit: Display: LogAssetRegistry: Error: Package ../../../../../../hutb/Unreal/CarlaUE4/Content/mujoco/StarterContent/Materials/M_Brick_Clay_Beveled.uasset is too old
  LogInit: Display: LogAssetRegistry: Error: Package ../../../../../../hutb/Unreal/CarlaUE4/Content/mujoco/StarterContent/Materials/M_Brick_Clay_New.uasset is too old
  LogInit: Display: LogAssetRegistry: Error: Package ../../../../../../hutb/Unreal/CarlaUE4/Content/mujoco/StarterContent/Materials/M_Brick_Cut_Stone.uasset is too old
  LogInit: Display: LogAssetRegistry: Error: Package ../../../../../../hutb/Unreal/CarlaUE4/Content/mujoco/StarterContent/Materials/M_Brick_Clay_Old.uasset is too old
  LogInit: Display: LogAssetRegistry: Error: Package ../../../../../../hutb/Unreal/CarlaUE4/Content/mujoco/StarterContent/Materials/M_CobbleStone_Rough.uasset is too old
  LogInit: Display: LogAssetRegistry: Error: Package ../../../../../../hutb/Unreal/CarlaUE4/Content/mujoco/StarterContent/Materials/M_Concrete_Grime.uasset is too old
  LogInit: Display: LogAssetRegistry: Error: Package ../../../../../../hutb/Unreal/CarlaUE4/Content/mujoco/StarterContent/Materials/M_Concrete_Panels.uasset is too old
  LogInit: Display: LogAssetRegistry: Error: Package ../../../../../../hutb/Unreal/CarlaUE4/Content/mujoco/StarterContent/Materials/M_Brick_Hewn_Stone.uasset is too old
  LogInit: Display: LogAssetRegistry: Error: Package ../../../../../../hutb/Unreal/CarlaUE4/Content/mujoco/StarterContent/Materials/M_Ceramic_Tile_Checker.uasset is too old
  LogInit: Display: NOTE: Only first 50 errors displayed.
  LogInit: Display: LogConsoleManager: Warning: Setting the console variable 'r.Streaming.PoolSize' with 'SetByScalability' was ignored as it is lower priority than the previous 'SetByProjectSetting'. Value remains '4000'
  LogInit: Display: LogModuleManager: Warning: ModuleManager: Unable to load module 'ExampleDeviceProfileSelector'  - 0 instances of that module name found.
  LogInit: Display: LogScript: Warning: Script Msg: A null object was passed as a world context object to UEngine::GetWorldFromContextObject().
  LogInit: Display: LogUObjectGlobals: Warning: [AssetLog] D:\hutb\Unreal\CarlaUE4\Content\Carla\Blueprints\Walkers\BP_Walker_AB001_G3.uasset: ?????{FileName}?????????
  LogInit: Display: LogUObjectGlobals: Warning: [AssetLog] D:\hutb\Unreal\CarlaUE4\Content\Carla\Blueprints\Walkers\WalkerFactory.uasset: ?????{FileName}?????????
  LogInit: Display: LogUObjectGlobals: Warning: [AssetLog] D:\hutb\Unreal\CarlaUE4\Content\Carla\Blueprints\Walkers\BP_Walker_AG001_G3.uasset: ?????{FileName}?????????
  LogInit: Display: LogUObjectGlobals: Warning: [AssetLog] D:\hutb\Unreal\CarlaUE4\Content\Carla\Blueprints\Walkers\BP_Walker_AsiaM03_G3.uasset: ?????{FileName}?????????
  LogInit: Display: LogTemp: Warning: LogitechWheelPlugin initiated!
  LogInit: Display: LogUObjectGlobals: Warning: [AssetLog] D:\hutb\Unreal\CarlaUE4\Content\Carla\Maps\Town10HD_Opt.umap: ?????{FileName}?????????
  LogInit: Display: LogUObjectGlobals: Warning: ?????{FileName}?????????
  LogInit: Display: LogUObjectGlobals: Warning: [AssetLog] D:\hutb\Unreal\CarlaUE4\Content\Carla\Maps\Town10HD.umap: ?????{FileName}?????????
  LogInit: Display: LogScript: Warning: ????????????? CallFunc_FindSocket_ReturnValue

        BP_Procedural_WindowPieces_C /Game/Carla/Maps/TestMaps/EmptyMap.EmptyMap:PersistentLevel.BP_Procedural_WindowPieces11

        Function /Game/Carla/Blueprints/LevelDesign/BP_Procedural_WindowPieces.BP_Procedural_WindowPieces_C:AddAtoms:0490
  LogInit: Display: LogScript: Warning: Script call stack:
        Function /Game/Carla/Blueprints/LevelDesign/BP_Procedural_WindowPieces.BP_Procedural_WindowPieces_C:UserConstructionScript
        Function /Game/Carla/Blueprints/LevelDesign/BP_Procedural_WindowPieces.BP_Procedural_WindowPieces_C:AddAtoms

  LogInit: Display: LogScript: Warning: ????????????? CallFunc_FindSocket_ReturnValue

        BP_Procedural_WindowPieces_C /Game/Carla/Maps/TestMaps/EmptyMap.EmptyMap:PersistentLevel.BP_Procedural_WindowPieces12

        Function /Game/Carla/Blueprints/LevelDesign/BP_Procedural_WindowPieces.BP_Procedural_WindowPieces_C:AddAtoms:0490
  LogInit: Display: LogScript: Warning: ????????????? CallFunc_FindSocket_ReturnValue

        BP_Procedural_WindowPieces_C /Game/Carla/Maps/TestMaps/EmptyMap.EmptyMap:PersistentLevel.BP_Procedural_WindowPieces13

        Function /Game/Carla/Blueprints/LevelDesign/BP_Procedural_WindowPieces.BP_Procedural_WindowPieces_C:AddAtoms:0490
  LogInit: Display: LogScript: Warning: ????????????? CallFunc_FindSocket_ReturnValue

        BP_Procedural_WindowPieces_C /Game/Carla/Maps/TestMaps/EmptyMap.EmptyMap:PersistentLevel.BP_Procedural_WindowPieces14

        Function /Game/Carla/Blueprints/LevelDesign/BP_Procedural_WindowPieces.BP_Procedural_WindowPieces_C:AddAtoms:0490
  LogInit: Display: LogScript: Warning: ????????????? CallFunc_FindSocket_ReturnValue

        BP_Procedural_WindowPieces_C /Game/Carla/Maps/TestMaps/EmptyMap.EmptyMap:PersistentLevel.BP_Procedural_WindowPieces15

        Function /Game/Carla/Blueprints/LevelDesign/BP_Procedural_WindowPieces.BP_Procedural_WindowPieces_C:AddAtoms:0490
  LogInit: Display: LogScript: Warning: ????????????? CallFunc_FindSocket_ReturnValue

        BP_Procedural_WindowPieces_C /Game/Carla/Maps/TestMaps/EmptyMap.EmptyMap:PersistentLevel.BP_Procedural_WindowPieces16

        Function /Game/Carla/Blueprints/LevelDesign/BP_Procedural_WindowPieces.BP_Procedural_WindowPieces_C:AddAtoms:0490
  LogInit: Display: LogScript: Warning: ????????????? CallFunc_FindSocket_ReturnValue

        BP_Procedural_WindowPieces_C /Game/Carla/Maps/TestMaps/EmptyMap.EmptyMap:PersistentLevel.BP_Procedural_WindowPieces17

        Function /Game/Carla/Blueprints/LevelDesign/BP_Procedural_WindowPieces.BP_Procedural_WindowPieces_C:AddAtoms:0490
  LogInit: Display: LogScript: Warning: ????????????? CallFunc_FindSocket_ReturnValue

        BP_Procedural_WindowPieces_C /Game/Carla/Maps/TestMaps/EmptyMap.EmptyMap:PersistentLevel.BP_Procedural_WindowPieces18

        Function /Game/Carla/Blueprints/LevelDesign/BP_Procedural_WindowPieces.BP_Procedural_WindowPieces_C:AddAtoms:0490
  LogInit: Display: LogScript: Warning: ????????????? CallFunc_FindSocket_ReturnValue

        BP_Procedural_WindowPieces_C /Game/Carla/Maps/TestMaps/EmptyMap.EmptyMap:PersistentLevel.BP_Procedural_WindowPieces19

        Function /Game/Carla/Blueprints/LevelDesign/BP_Procedural_WindowPieces.BP_Procedural_WindowPieces_C:AddAtoms:0490
  LogInit: Display: LogScript: Warning: ????????????? CallFunc_FindSocket_ReturnValue

        BP_Procedural_WindowPieces_C /Game/Carla/Maps/TestMaps/EmptyMap.EmptyMap:PersistentLevel.BP_Procedural_WindowPieces2

        Function /Game/Carla/Blueprints/LevelDesign/BP_Procedural_WindowPieces.BP_Procedural_WindowPieces_C:AddAtoms:0490
  LogInit: Display: LogScript: Warning: ????????????? CallFunc_FindSocket_ReturnValue

        BP_Procedural_WindowPieces_C /Game/Carla/Maps/TestMaps/EmptyMap.EmptyMap:PersistentLevel.BP_Procedural_WindowPieces20

        Function /Game/Carla/Blueprints/LevelDesign/BP_Procedural_WindowPieces.BP_Procedural_WindowPieces_C:AddAtoms:0490
  LogInit: Display: LogScript: Warning: ????????????? CallFunc_FindSocket_ReturnValue

        BP_Procedural_WindowPieces_C /Game/Carla/Maps/TestMaps/EmptyMap.EmptyMap:PersistentLevel.BP_Procedural_WindowPieces21_42

        Function /Game/Carla/Blueprints/LevelDesign/BP_Procedural_WindowPieces.BP_Procedural_WindowPieces_C:AddAtoms:0490
  LogInit: Display: LogScript: Warning: ????????????? CallFunc_FindSocket_ReturnValue

        BP_Procedural_WindowPieces_C /Game/Carla/Maps/TestMaps/EmptyMap.EmptyMap:PersistentLevel.BP_Procedural_WindowPieces22_43

        Function /Game/Carla/Blueprints/LevelDesign/BP_Procedural_WindowPieces.BP_Procedural_WindowPieces_C:AddAtoms:0490
  LogInit: Display: LogScript: Warning: ????????????? CallFunc_FindSocket_ReturnValue

        BP_Procedural_WindowPieces_C /Game/Carla/Maps/TestMaps/EmptyMap.EmptyMap:PersistentLevel.BP_Procedural_WindowPieces23_44

        Function /Game/Carla/Blueprints/LevelDesign/BP_Procedural_WindowPieces.BP_Procedural_WindowPieces_C:AddAtoms:0490
  LogInit: Display: LogScript: Warning: ????????????? CallFunc_FindSocket_ReturnValue

        BP_Procedural_WindowPieces_C /Game/Carla/Maps/TestMaps/EmptyMap.EmptyMap:PersistentLevel.BP_Procedural_WindowPieces24_45

        Function /Game/Carla/Blueprints/LevelDesign/BP_Procedural_WindowPieces.BP_Procedural_WindowPieces_C:AddAtoms:0490
  LogInit: Display: LogScript: Warning: ????????????? CallFunc_FindSocket_ReturnValue

        BP_Procedural_WindowPieces_C /Game/Carla/Maps/TestMaps/EmptyMap.EmptyMap:PersistentLevel.BP_Procedural_WindowPieces25_46

        Function /Game/Carla/Blueprints/LevelDesign/BP_Procedural_WindowPieces.BP_Procedural_WindowPieces_C:AddAtoms:0490
  LogInit: Display: LogScript: Warning: ????????????? CallFunc_FindSocket_ReturnValue

        BP_Procedural_WindowPieces_C /Game/Carla/Maps/TestMaps/EmptyMap.EmptyMap:PersistentLevel.BP_Procedural_WindowPieces3

        Function /Game/Carla/Blueprints/LevelDesign/BP_Procedural_WindowPieces.BP_Procedural_WindowPieces_C:AddAtoms:0490
  LogInit: Display: LogScript: Warning: ????????????? CallFunc_FindSocket_ReturnValue

        BP_Procedural_WindowPieces_C /Game/Carla/Maps/TestMaps/EmptyMap.EmptyMap:PersistentLevel.BP_Procedural_WindowPieces4

        Function /Game/Carla/Blueprints/LevelDesign/BP_Procedural_WindowPieces.BP_Procedural_WindowPieces_C:AddAtoms:0490
  LogInit: Display: LogScript: Warning: ????????????? CallFunc_FindSocket_ReturnValue

        BP_Procedural_WindowPieces_C /Game/Carla/Maps/TestMaps/EmptyMap.EmptyMap:PersistentLevel.BP_Procedural_WindowPieces5

        Function /Game/Carla/Blueprints/LevelDesign/BP_Procedural_WindowPieces.BP_Procedural_WindowPieces_C:AddAtoms:0490
  LogInit: Display: LogScript: Warning: ????????????? CallFunc_FindSocket_ReturnValue

        BP_Procedural_WindowPieces_C /Game/Carla/Maps/TestMaps/EmptyMap.EmptyMap:PersistentLevel.BP_Procedural_WindowPieces6

        Function /Game/Carla/Blueprints/LevelDesign/BP_Procedural_WindowPieces.BP_Procedural_WindowPieces_C:AddAtoms:0490
  LogInit: Display: LogScript: Warning: ????????????? CallFunc_FindSocket_ReturnValue

        BP_Procedural_WindowPieces_C /Game/Carla/Maps/TestMaps/EmptyMap.EmptyMap:PersistentLevel.BP_Procedural_WindowPieces7

        Function /Game/Carla/Blueprints/LevelDesign/BP_Procedural_WindowPieces.BP_Procedural_WindowPieces_C:AddAtoms:0490
  LogInit: Display: LogScript: Warning: ????????????? CallFunc_FindSocket_ReturnValue

        BP_Procedural_WindowPieces_C /Game/Carla/Maps/TestMaps/EmptyMap.EmptyMap:PersistentLevel.BP_Procedural_WindowPieces8

        Function /Game/Carla/Blueprints/LevelDesign/BP_Procedural_WindowPieces.BP_Procedural_WindowPieces_C:AddAtoms:0490
  LogInit: Display: LogScript: Warning: ????????????? CallFunc_FindSocket_ReturnValue

        BP_Procedural_WindowPieces_C /Game/Carla/Maps/TestMaps/EmptyMap.EmptyMap:PersistentLevel.BP_Procedural_WindowPieces9

        Function /Game/Carla/Blueprints/LevelDesign/BP_Procedural_WindowPieces.BP_Procedural_WindowPieces_C:AddAtoms:0490
  LogInit: Display: LogScript: Warning: ????????????? CallFunc_FindSocket_ReturnValue

        BP_Procedural_WindowPieces_C /Game/Carla/Maps/TestMaps/EmptyMap.EmptyMap:PersistentLevel.BP_Procedural_WindowPieces_2

        Function /Game/Carla/Blueprints/LevelDesign/BP_Procedural_WindowPieces.BP_Procedural_WindowPieces_C:AddAtoms:0490
  LogInit: Display: LogScript: Warning: ????????????? CallFunc_FindSocket_ReturnValue

        BP_Procedural_WindowPieces_C /Game/Carla/Maps/TestMaps/EmptyMap.EmptyMap:PersistentLevel.BP_Procedural_WindowPieces10

        Function /Game/Carla/Blueprints/LevelDesign/BP_Procedural_WindowPieces.BP_Procedural_WindowPieces_C:AddAtoms:0490
  LogInit: Display:
  LogInit: Display: Failure - 250 error(s), 277 warning(s)
  LogInit: Display:

  Execution of commandlet took:  693.94 seconds
  LogShaderCompilers: Display: Shaders left to compile 0
  LogTemp: Warning: LogitechWheelPlugin shut down!
  LogHttp: Display: cleaning up 0 outstanding Http requests.
  LogContentStreaming: Display: There are 1 unreleased StreamingManagers
```


## 参考

* [MuJoCo-Unreal-Engine-Plugin](https://github.com/oneclicklabs/MuJoCo-Unreal-Engine-Plugin)
* [mujoco-unreal-plugin](https://github.com/carTloyal123/mujoco-unreal-plugin)
