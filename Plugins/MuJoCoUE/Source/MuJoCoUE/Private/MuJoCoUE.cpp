// Copyright Epic Games, Inc. All Rights Reserved.
// FMuJoCoUEModule (模块管理)
// 功能: 负责插件的初始化和资源管理

#include "MuJoCoUE.h"

#include "Interfaces/IPluginManager.h"

#define LOCTEXT_NAMESPACE "FMuJoCoUEModule"

// 模块初始化：加载 dll 文件
void FMuJoCoUEModule::StartupModule()
{
	// 这段代码会在模块加载到内存后执行；具体的执行时间在每个模块的 .uplugin 文件中指定。
	FString DLLPath = IPluginManager::Get().FindPlugin("MuJoCoUE")->GetBaseDir();

	// hutb\Unreal\CarlaUE4\Plugins\mujoco_plugin\Plugins\MuJoCoUE\Binaries\mujoco.dll
	DLLPath = DLLPath + TEXT("/Binaries/mujoco.dll");

	DLLHandle = FPlatformProcess::GetDllHandle(*DLLPath);
	// IModularFeatures::Get().RegisterModularFeature(IInputDeviceModule::GetModularFeatureName(), this);
}

void FMuJoCoUEModule::ShutdownModule()
{
	// 此函数可能会在关闭期间调用，用于清理模块。
	// 对于支持动态重载的模块，我们会在卸载模块之前调用此函数。
	FPlatformProcess::FreeDllHandle(DLLHandle);
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FMuJoCoUEModule, MuJoCoUE)