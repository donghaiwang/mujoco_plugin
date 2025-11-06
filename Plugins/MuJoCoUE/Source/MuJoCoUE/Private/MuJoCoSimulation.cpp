// Fill out your copyright notice in the Description page of Project Settings.

#include "MuJoCoSimulation.h"

#include "mujoco/mujoco.h"
#include <vector>
#include <string>

#include "KismetProceduralMeshLibrary.h"
#include "ProceduralMeshConversion.h"

FVector CalculateWorldPosition(const FVector &BaseLocation, const FQuat &BaseRotation, const FVector &RelativeLocation)
{
	return BaseLocation + BaseRotation.RotateVector(RelativeLocation);
}

FQuat CalculateWorldRotation(const FQuat &BaseRotation, const FQuat &RelativeRotation)
{
	return RelativeRotation * BaseRotation;
}

// 抽取模型信息
ModelInfo ExtractModelInfo(const mjModel *m)
{
	ModelInfo modelInfo;  // 返回的模型信息对象

	// 抽取 MuJoCo 模型中的刚体(body)信息
	for (int i = 0; i < m->nbody; ++i)
	{
		BodyInfo bodyInfo;
		// 读取名称
		bodyInfo.name = std::string(m->names + m->name_bodyadr[i]);  // 名称字符串池的指针 + 第i个刚体名称在字符串池中的偏移量
		// 复制位置数据
		// m->body_pose: [x0,y0,z0, x1,y1,z2, ...]
		// 第i个body：从索引 3*i 到 3*(i+1)
		std::copy(m->body_pos + 3 * i, m->body_pos + 3 * (i + 1), bodyInfo.pos);  
		// 复制四元数数据
		std::copy(m->body_quat + 4 * i, m->body_quat + 4 * (i + 1), bodyInfo.quat);
		bodyInfo.parent_id = m->body_parentid[i];  // 记录父级关系
		// 调整四元数构造的顺序：MuJoCo格式为 [w,x,y,z] -> 引擎格式：FQuat(x,y,z,w)
		bodyInfo.quat2 = FQuat(bodyInfo.quat[1], bodyInfo.quat[2], bodyInfo.quat[3], bodyInfo.quat[0]);
		modelInfo.bodies.push_back(bodyInfo);
	}

	// 抽取 MuJoCo 模型中的几何体(geom)信息
	for (int i = 0; i < m->ngeom; ++i)
	{
		GeomInfo geomInfo;
		geomInfo.name = std::string(m->names + m->name_geomadr[i]);
		geomInfo.body_id = m->geom_bodyid[i];  // 所属刚体body的 ID
		geomInfo.type = m->geom_type[i];  // 几何体类型枚举
		std::copy(m->geom_size + 3 * i, m->geom_size + 3 * (i + 1), geomInfo.size);
		std::copy(m->geom_pos + 3 * i, m->geom_pos + 3 * (i + 1), geomInfo.pos);
		std::copy(m->geom_quat + 4 * i, m->geom_quat + 4 * (i + 1), geomInfo.quat);
		geomInfo.quat2 = FQuat(geomInfo.quat[1], geomInfo.quat[2], geomInfo.quat[3], geomInfo.quat[0]);
		// 检查此几何体是否具有材质或纹理信息
		if (m->geom_matid[i] >= 0)  // 有材质的情况
		{
			int matid = m->geom_matid[i];
			geomInfo.color = FLinearColor(
				m->mat_rgba[matid * 4 + 0],  // R 颜色分量
				m->mat_rgba[matid * 4 + 1],  // G
				m->mat_rgba[matid * 4 + 2],  // B
				m->mat_rgba[matid * 4 + 3]); // A
	
			if (m->mat_texid[matid] >= 0)  // 存在纹理则进行处理
			{
				int texid = m->mat_texid[matid];
				// 纹理是存在的，但我们将使用几何体颜色作为基础色。
				geomInfo.color = FLinearColor(
					m->geom_rgba[i * 4 + 0],
					m->geom_rgba[i * 4 + 1],
					m->geom_rgba[i * 4 + 2],
					m->geom_rgba[i * 4 + 3]);
				//	geomInfo.texId = texid;
			}
		}
		// 否则，使用几何体特定的 RGBA 颜色
		else
		{
			geomInfo.color = FLinearColor(
				m->geom_rgba[i * 4 + 0],
				m->geom_rgba[i * 4 + 1],
				m->geom_rgba[i * 4 + 2],
				m->geom_rgba[i * 4 + 3]);
		}
		
		// 调整尺寸以用作比例尺（不同几何体类型需要不同的尺寸转换策略，以适应引擎的表示方式）
		// 假设所有网格均为原始网格。
		// 转换坐标系和单位（右手系->左手系；米->厘米）
		// 1 米 大小 -> 在 UE 中为 100 厘米。
		switch (geomInfo.type)
		{
		case mjGEOM_CYLINDER:  // 圆柱体
			geomInfo.size[0] *= 2;  // 半径 -> 直径
			geomInfo.size[2] = geomInfo.size[1] * 2;  // 高度调整
			geomInfo.size[1] = geomInfo.size[0];
			break;
		case mjGEOM_CAPSULE:  // 胶囊体
			geomInfo.size[2] = geomInfo.size[1] + geomInfo.size[0];  // 高度调整
			geomInfo.size[0] *= 2;
			geomInfo.size[1] = geomInfo.size[0];
			break;
		case mjGEOM_SPHERE:  // 球体
			geomInfo.size[0] *= 2;
			geomInfo.size[1] = geomInfo.size[0];
			geomInfo.size[2] = geomInfo.size[0];
			break;
		case mjGEOM_BOX:  // 长方体
			geomInfo.size[0] *= 2;
			geomInfo.size[1] *= 2;
			geomInfo.size[2] *= 2;
			break;
		case mjGEOM_ELLIPSOID:  // 椭圆体
			geomInfo.size[0] *= 2;
			geomInfo.size[1] *= 2;
			geomInfo.size[2] *= 2;
			break;
			break;
		}
		modelInfo.geoms.push_back(geomInfo);
	}

	return modelInfo;
}

// 将提取的模型信息转换为Unreal Engine中的实际场景组件，建立完整的层级化场景图，
// 实现物理模型到可视化场景的映射
void AMuJoCoSimulation::GenerateMeshes(ModelInfo &modelInfo)
{
	// 清空现有映射表
	BodyMap.Empty();
	GeomMap1.Empty();

	// 生成刚体(body)组件
	int BodyId = 0;
	for (const BodyInfo &bodyInfo : modelInfo.bodies)
	{
		// 场景组件(USceneComponent<-UActorComponent<-UObject)是所有具有空间变换（位置、旋转、缩放）的组件基类
		// 它为 组件的三维空间定位与层级管理 提供支持，是 场景树系统的核心
		USceneComponent *sceneComponent = NewObject<USceneComponent>(this, FName(*(FString(bodyInfo.name.c_str()) + *FString::Printf(TEXT("_Body%d"), BodyId))));

		BodyMap.Add(BodyId++, sceneComponent);  // 将 场景组件 添加到 刚体映射表 中
		sceneComponent->RegisterComponent();    // 将 场景组件 注册到引擎
		sceneComponent->SetRelativeLocation(FVector(bodyInfo.pos[0] * 100, bodyInfo.pos[1] * 100, bodyInfo.pos[2] * 100));
		sceneComponent->SetRelativeRotation(bodyInfo.quat2);  // 使用转换后的四元数
		if (bodyInfo.parent_id == 0)  // MuJoCo中0表示世界根：场景组件附着在根组件下
		{
			// 把场景组件提升为RootComponent，否则创建出来的场景组件永远在(0,0,0)点，即使Actor点位置动态改变场景组件绝对位置也不随Actor改变
			sceneComponent->AttachToComponent(GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
			// sceneComponent->SetupAttachment(RootComponent);
			// sceneComponent->SetHiddenInGame(false, true);
		}
		else  // 子刚体(body)的处理来进行层级维护
		{
			USceneComponent *parentComponent = BodyMap[bodyInfo.parent_id];  // 通过刚体映射表快速查找父类
			sceneComponent->AttachToComponent(parentComponent, FAttachmentTransformRules::KeepRelativeTransform);  // 将当前的 场景组件 附着在父组件下
		}
	}

	// 生成几何体(geom)网格组件
	int GeomId = 0;
	for (GeomInfo &geomInfo : modelInfo.geoms)
	{
		// 创建新的网格组件
		UStaticMeshComponent *staticMeshComponent = NewObject<UStaticMeshComponent>(this);//, FName(*(FString(geomInfo.name.c_str()) + *FString::Printf(TEXT("_Geom%d"), BodyId))));
		staticMeshComponent->RegisterComponent();
		geomInfo.posAdjust[2] = geomInfo.size[2] * -50;  // Z 轴偏移调整
		staticMeshComponent->SetRelativeLocation(FVector(geomInfo.pos[0] * 100, geomInfo.pos[1] * 100, geomInfo.pos[2] * 100)); //+geomInfo.posAdjust[2]
		staticMeshComponent->SetRelativeRotation(geomInfo.quat2);
		staticMeshComponent->AttachToComponent(this->BodyMap[geomInfo.body_id], FAttachmentTransformRules::KeepRelativeTransform);
		;
		// 获得该几何体的网格
		// MeshAssets映射表：预配置的几何体类型到 StaticMesh 的映射
		auto *mesh = MeshAssets.Find(geomInfo.type) ? MeshAssets[geomInfo.type] : nullptr;
		// 如果 type = mesh 生成程序化网格
		if (!mesh)
		{
			// 特殊网格处理（mjGEOM_MESH）
			if (geomInfo.type == mjGEOM_MESH && mModel->geom_dataid[GeomId] != -1)
			{
				int meshId = mModel->geom_dataid[GeomId];
				if (meshId >= 0 && meshId < ProceduralMeshes.Num())
				{
					UProceduralMeshComponent *procMesh = ProceduralMeshes[meshId];

					const FMeshDescription description = BuildMeshDescription(procMesh);
					TArray<const FMeshDescription *> descs;
					descs.Add(&description);

					UStaticMesh *NewStaticMesh = NewObject<UStaticMesh>(staticMeshComponent);
					NewStaticMesh->AddMaterial(procMesh->GetMaterial(0));
					NewStaticMesh->BuildFromMeshDescriptions(descs);
					staticMeshComponent->SetStaticMesh(NewStaticMesh);
					mesh = NewStaticMesh;
					if (mesh)
					{
						geomInfo.size[0] = 1;
						geomInfo.size[1] = 1;
						geomInfo.size[2] = 1;
					}
				}
			}
			if (!mesh)
				mesh = defaultMesh;
		}

		staticMeshComponent->SetStaticMesh(mesh);  // 设置网格
		SetMeshColor(staticMeshComponent, geomInfo.color);  // 设置颜色
		staticMeshComponent->SetSimulatePhysics(false);  // 禁用物理模拟
		staticMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);  // 禁用碰撞
		staticMeshComponent->SetWorldScale3D(FVector(geomInfo.size[0], geomInfo.size[1], geomInfo.size[2]));  // 缩放设置

		this->GeomMap1.Add(GeomId++, staticMeshComponent);
	}
}

void AMuJoCoSimulation::ExtractCurrentState(ModelInfo &info)
{
	for (int i = 0; i < mModel->nbody; ++i)
	{
		// 从全局坐标（xpos 和 xquat）获取位置数据
		BodyInfo bodyInfo;
		std::copy(mData->xpos + 3 * i, mData->xpos + 3 * (i + 1), info.bodies[i].pos);
		std::copy(mData->xquat + 4 * i, mData->xquat + 4 * (i + 1), info.bodies[i].quat);
		info.bodies[i].quat2 = FQuat(info.bodies[i].quat[1], info.bodies[i].quat[2], info.bodies[i].quat[3], info.bodies[i].quat[0]);
	}

	// 更新几何体状态
	for (int i = 0; i < mModel->ngeom; ++i)
	{   GeomInfo& geomInfo=info.geoms[i];
		std::copy(mData->geom_xpos + 3 * i, mData->geom_xpos + 3 * (i + 1), info.geoms[i].pos);
		// 将旋转矩阵转换为四元数
		const mjtNum *mat = mData->geom_xmat + 9 * i;
		mjtNum quat[4];
		mju_mat2Quat(quat, mat);

		geomInfo.quat2 = FQuat(quat[1], quat[2], quat[3], quat[0]);
	}
}

AMuJoCoSimulation::AMuJoCoSimulation()
{
	// 设置此 Actor 每帧调用 Tick() 函数。
	// 如果不需要此功能，可以将其关闭以提高性能。
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bCanEverTick = true;
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		return;
	}
	mData = nullptr;
	mModel = nullptr;
	bSimulationRunning = false;
	XmlSourcePath = TEXT("mujoco/pendulum.xml");

	// 使用一个圆柱体作为模型的替代，否则拖进去看不到
	UStaticMeshComponent* Cylinder = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualRepresentation"));
	Cylinder->SetupAttachment(RootComponent);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderAsset(TEXT("/Game/StarterContent/Shapes/Shape_Cylinder.Shape_Cylinder"));
	if (CylinderAsset.Succeeded())
	{
		Cylinder->SetStaticMesh(CylinderAsset.Object);
		Cylinder->SetRelativeLocation(FVector(0.0f, 0.0f, 0.0f));
		Cylinder->SetWorldScale3D(FVector(1.f));
	}

	// 使用对象查找器在类的构造函数中自动加载网格
	// 这只是静态网格体，要实际使用它，后面需要将其分配给 StaticMeshComponent
	static ConstructorHelpers::FObjectFinder<UStaticMesh> StaticMesh(TEXT("StaticMesh'/Game/Carla/Static/SM_Plane.SM_Plane'"));
	defaultMesh = StaticMesh.Object;
}

// 开始仿真
void AMuJoCoSimulation::BeginPlay()
{
	Super::BeginPlay();
	mData = nullptr;
	mModel = nullptr;
	LoadModel(XmlSourcePath);  // 解析 mujoco 的 xml 模型
	if (mModel)
	{
		_info = ExtractModelInfo(mModel);  // 处理基本几何体（球体、立方体等）
		ConvertMuJoCoModelToProceduralMeshes(mModel, this); // 处理复杂网格几何体：将MuJoCo模型中的网格数据转换为Engine的程序化网格组件
		GenerateMeshes(_info);  // 将提取的模型信息转换为引擎中的实际场景组件
	}
	StartSimulation();
	UE_LOG(LogTemp, Warning, TEXT("Mujoco begin play."));
}

void AMuJoCoSimulation::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (mData)
		mj_deleteData(mData);

	if (mModel)
		mj_deleteModel(mModel);
	mData = nullptr;
	mModel = nullptr;
	Super::EndPlay(EndPlayReason);
}


void AMuJoCoSimulation::UpdateSimulationView(const ModelInfo &Info)
{
	FVector BaseLocation = BodyMap[0]->GetComponentLocation();
	FQuat BaseRotation = BodyMap[0]->GetComponentRotation().Quaternion(); // GetActorRotation().Quaternion();

	int BodyId = 0;
	for (const BodyInfo &bodyInfo : Info.bodies)
	{
		USceneComponent *sceneComponent = BodyMap[BodyId];
		if (!sceneComponent)
			continue;
		FVector WorldLoc = CalculateWorldPosition(BaseLocation, BaseRotation, FVector(bodyInfo.pos[0] * 100, bodyInfo.pos[1] * 100, bodyInfo.pos[2] * 100));
		sceneComponent->SetWorldLocation(WorldLoc);
		FQuat worldRot = CalculateWorldRotation(BaseRotation, bodyInfo.quat2);
		sceneComponent->SetWorldRotation(bodyInfo.quat2 /*worldRot*/);

		//	sceneComponent->SetRelativeLocation(FVector(bodyInfo.pos[0]*100, bodyInfo.pos[1]*100, bodyInfo.pos[2]*100));
		//		sceneComponent->SetRelativeRotation(bodyInfo.quat2);

		//	UE_LOG(LogTemp, Warning, TEXT("Body %d [%hs][%f]: %f %f %f"), BodyId,bodyInfo.name.c_str(), mData->time,bodyInfo.pos[0], bodyInfo.pos[1], bodyInfo.pos[2]);
		//	UE_LOG(LogTemp, Warning, TEXT("Body %d [%hs][%f]: %f %f %f %f"), BodyId,bodyInfo.name.c_str(), mData->time,bodyInfo.quat[1], bodyInfo.quat[2], bodyInfo.quat[3], bodyInfo.quat[0]);
		BodyId++;
	}

	int GeomId = 0;
	for (const GeomInfo &geomInfo : Info.geoms)
	{
		UStaticMeshComponent *staticMeshComponent = GeomMap1[GeomId];
		if (!staticMeshComponent)
			continue;

		//	staticMeshComponent->SetRelativeLocation(FVector(geomInfo.pos[0]*100, geomInfo.pos[1]*100, geomInfo.pos[2]*100));
		//	staticMeshComponent->SetRelativeRotation(geomInfo.quat2);

		FVector WorldLoc = CalculateWorldPosition(BaseLocation, BaseRotation, FVector(geomInfo.pos[0] * 100, geomInfo.pos[1] * 100, geomInfo.pos[2] * 100));
		staticMeshComponent->SetWorldLocation(WorldLoc);
		FQuat worldRot = CalculateWorldRotation(BaseRotation, geomInfo.quat2);
		//	staticMeshComponent->SetWorldRotation(geomInfo.quat2/*worldRot*/);

		// UE_LOG(LogTemp, Warning, TEXT("Geom %d[%hs][%f]: %f %f %f"), GeomId,geomInfo.name.c_str() ,mData->time,geomInfo.pos[0], geomInfo.pos[1], geomInfo.pos[2]);
		// UE_LOG(LogTemp, Warning, TEXT("Geom %d[%hs][%f]: %f %f %f %f"), GeomId,geomInfo.name.c_str() ,mData->time,geomInfo.quat[1], geomInfo.quat[2], geomInfo.quat[3], geomInfo.quat[0]);
		GeomId++;
	}
}

void AMuJoCoSimulation::SimulateMuJoCo(float DeltaTime)
{
	if (mData == nullptr || mModel == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("Model or data is null"));
		return;
	}
	double startTime = mData->time;
	while (mData->time - startTime < DeltaTime)
		mj_step(mModel, mData);

	ModelInfo info;
	if (!_info.bodies.size())
		return;
	ExtractCurrentState(_info);

	UpdateSimulationView(_info);
}


// 从 XML 文件加载 MuJoCo 模型
bool AMuJoCoSimulation::LoadModel(FString Xml)
{
	// 相对于 Content 的路径 + 编辑器中配置的 XML Source Path
	FString FullPath = FPaths::Combine(FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir()), Xml);
	if (!FPaths::FileExists(FullPath))
	{
		UE_LOG(LogTemp, Error, TEXT("File does not exist: %s"), *FullPath);
		return false;
	}
	mModel = mj_loadXML(TCHAR_TO_ANSI(*FullPath), NULL, NULL, 0);
	if (!mModel)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to load model from %s"), *Xml);
		return false;
	}
	mData = mj_makeData(mModel);
	if (!mData)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to make data for model"));
		return false;
	}
	return true;
}

// 每一帧都被调用
void AMuJoCoSimulation::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (bSimulationRunning)
		SimulateMuJoCo(DeltaTime);
}


void AMuJoCoSimulation::SetControl(int Id, float Value)
{
	if (!mData || !mModel || mModel->nu <= Id)
		return;
	mData->ctrl[Id] = Value;
}


void AMuJoCoSimulation::StartSimulation()
{
	bSimulationRunning = true;
}


void AMuJoCoSimulation::PauseSimulation()
{
	bSimulationRunning = false;
}


void AMuJoCoSimulation::ResetSimulation()
{
	bSimulationRunning = false;
	if (mData)
		mj_deleteData(mData);
	mData = mj_makeData(mModel);
	ExtractCurrentState(_info);
	UpdateSimulationView(_info);
}

void AMuJoCoSimulation::StepSimulation()
{
	LogInfo();
	mj_step(mModel, mData);
	ExtractCurrentState(_info);
	UpdateSimulationView(_info);
	LogInfo();
}

void AMuJoCoSimulation::LogInfo()
{
	// 记录刚体(body)信息
	int BodyId = 0;
	for (const auto &bodyInfo : _info.bodies)
	{
		if (USceneComponent *bodyComponent = BodyMap[BodyId])
		{
			FVector worldLoc = bodyComponent->GetComponentLocation();
			FRotator worldRot = bodyComponent->GetComponentRotation();
			UE_LOG(LogTemp, Warning, TEXT("Body[%d] %hs - WorldLocation: (%f, %f, %f), WorldRotation: (%f, %f, %f)"),
				   BodyId,
				   bodyInfo.name.c_str(),
				   worldLoc.X, worldLoc.Y, worldLoc.Z,
				   worldRot.Pitch, worldRot.Yaw, worldRot.Roll);
		}
		BodyId++;
	}

	// 记录几何体信息
	int GeomId = 0;
	for (const auto &geomInfo : _info.geoms)
	{
		if (UStaticMeshComponent *geomComponent = GeomMap1[GeomId])
		{
			FVector worldLoc = geomComponent->GetComponentLocation();
			FRotator worldRot = geomComponent->GetComponentRotation();
			UE_LOG(LogTemp, Warning, TEXT("Geom[%d] %hs - WorldLocation: (%f, %f, %f), WorldRotation: (%f, %f, %f)"),
				   GeomId,
				   geomInfo.name.c_str(),
				   worldLoc.X, worldLoc.Y, worldLoc.Z,
				   worldRot.Pitch, worldRot.Yaw, worldRot.Roll);
		}
		GeomId++;
	}
}

// 将MuJoCo模型中的网格数据转换为Engine的程序化网格组件，
// 专门处理复杂的自定义网格几何体（mjGEOM_MESH类型）
void AMuJoCoSimulation::ConvertMuJoCoModelToProceduralMeshes(const mjModel *mjModel, UObject *Outer)
{
	// 输入模型不能为空、输出对象Out必须有效（用于组件创建）、模型中至少包含一个网格
	if (!mjModel || !Outer || mjModel->nmesh == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Invalid input parameters or no meshes in model"));
		return;
	}

	// 遍历 MuJoCo 模型中的所有网格
	for (int mesh_id = 0; mesh_id < mjModel->nmesh; mesh_id++)
	{
		// 从 MuJoCo 中抽取网格数据
		const int vert_start = mjModel->mesh_vertadr[mesh_id];  // 顶点起始索引
		const int nvert = mjModel->mesh_vertnum[mesh_id];  // 顶点数量
		const float *mj_vertices = &mjModel->mesh_vert[vert_start * 3];  // 顶点数组指针(每个顶点占用3个float)

		// 面片数据提取
		const int face_start = mjModel->mesh_faceadr[mesh_id];  // 面片起始索引
		const int nface = mjModel->mesh_facenum[mesh_id];  // 面片数量
		const int *mj_faces = &mjModel->mesh_face[face_start * 3];  // 面片数组指针

		// 跳过空网格
		if (nvert == 0 || nface == 0)
			continue;

		// 将顶点转换为引擎坐标
		TArray<FVector> UnrealVertices;
		for (int i = 0; i < nvert; i++)
		{
			const float *v = &mj_vertices[i * 3];
			UnrealVertices.Add(FVector(
				v[0] * 100.0f,	// X: 米 -> 厘米，保持朝向
				-v[1] * 100.0f, // Y: 翻转坐标轴，右手系->左手系
				v[2] * 100.0f	// Z: 米 -> 厘米
				));
		}

		// 将面片转换为引擎的缠绕顺序（顺时针而不是 MuJoCo 的逆时针）
		TArray<int32> UnrealTriangles;
		for (int i = 0; i < nface; i++)
		{
			UnrealTriangles.Add(mj_faces[i * 3 + 0]);  // 顶点 0 保持不变
			UnrealTriangles.Add(mj_faces[i * 3 + 2]); // 顶点 2 和顶点 1 交换
			UnrealTriangles.Add(mj_faces[i * 3 + 1]);
		}

		// 创建并注册 程序化网格组件
		UProceduralMeshComponent *ProcMesh = NewObject<UProceduralMeshComponent>(Outer);
		ProcMesh->RegisterComponent();

		// Generate normals/tangents (using placeholder UVs if none exist)
		TArray<FVector> Normals;
		TArray<FVector2D> UVs;
		TArray<FProcMeshTangent> Tangents;

		// Generate default UVs if needed (flat projection)
		UVs.SetNum(UnrealVertices.Num());
		for (FVector2D &uv : UVs)
		{
			uv = FVector2D(0.5f, 0.5f); // Simple default
		}

		// 自动法线切线计算
		// 计算原理: 基于顶点位置和UV坐标，自动生成平滑的法线和切线向量
		UKismetProceduralMeshLibrary::CalculateTangentsForMesh(
			UnrealVertices,   // 顶点位置
			UnrealTriangles,  // 三角形索引
			UVs,              // UV 坐标
			Normals,          // 输出：法线
			Tangents);        // 输出：切线

		// 创建网格分段
		ProcMesh->CreateMeshSection(
			0,                 // 网格分段的 ID
			UnrealVertices,    // 顶点数组
			UnrealTriangles,   // 三角形索引
			Normals,           // 法线数组
			UVs,               // UV 坐标数组
			TArray<FColor>(),  // 顶点颜色（空）
			Tangents,          // 切线数组
			true               // 启用碰撞
		);

		ProcMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		// Add to output array

		ProceduralMeshes.Add(ProcMesh);
		ProcMesh->SetVisibility(false);  // 初始不可见
	}

	return;
}

void AMuJoCoSimulation::SetMeshColor(UStaticMeshComponent *StaticMeshComponent, FLinearColor Color)
{
	if (!StaticMeshComponent)
		return;
	// TODO: 向网格添加材质仍然存在问题
	//	static UMaterialInterface* Material= FindObject<UMaterialInterface>(ANY_PACKAGE,TEXT("M_BaseColor"));         ///Game/MuJoCo/M_BaseColor.
	//	if (!Material)
	//	{
	//	               return;
	//	}

	//	StaticMeshComponent->SetMaterial(0, Material);

	// 创建动态材质实例
	UMaterialInterface *BaseMaterial = StaticMeshComponent->GetMaterial(0);
	if (!BaseMaterial)
		return;

	UMaterialInstanceDynamic *DynamicMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, StaticMeshComponent);
	if (!DynamicMaterial)
		return;

	DynamicMaterial->SetVectorParameterValue(FName("BaseColor"), Color);

	StaticMeshComponent->SetMaterial(0, DynamicMaterial);
}
