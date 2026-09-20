#include "RiseGameMode.h"
#include "RiseRTSCamera.h"
#include "RiseRTSPlayerController.h"
#include "RiseUnit.h"
#include "RiseTerritoryManager.h"
#include "RiseResourceHUD.h"
#include "RiseHUD.h"
#include "RiseBase.h"
#include "RiseResourceNode.h"
#include "RiseAIManager.h"
#include "RiseEnvironmentDirector.h"
#include "GameFramework/Actor.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
#include "Blueprint/UserWidget.h"
#include "EngineUtils.h"
#include "LandscapeProxy.h"
#include "Engine/Engine.h"

namespace
{
	// Precisa espelhar terrain_height() de Art/Blender/generate_world_assets.py
	// para a decoracao ficar apoiada nas colinas em vez de enterrada.
	float RiseTerrainHeightUU(float X, float Y)
	{
		const float Xm = X / 100.0f;
		const float Ym = Y / 100.0f;
		const float D = FMath::Max(FMath::Abs(Xm), FMath::Abs(Ym));

		if (D <= 120.0f)
		{
			return 0.0f;
		}

		float T = FMath::Clamp((D - 120.0f) / 80.0f, 0.0f, 1.0f);
		T = T * T * (3.0f - 2.0f * T);

		float Ridge = 24.0f;
		Ridge += 9.0f * FMath::Sin(Xm * 0.045f) * FMath::Cos(Ym * 0.052f);
		Ridge += 5.0f * FMath::Sin((Xm + Ym) * 0.11f);
		Ridge += 3.0f * FMath::Sin(Xm * 0.21f + Ym * 0.17f);

		return T * Ridge * 100.0f;
	}
}

ARiseGameMode::ARiseGameMode()
{
	DefaultPawnClass = ARiseRTSCamera::StaticClass();
	PlayerControllerClass = ARiseRTSPlayerController::StaticClass();
	HUDClass = ARiseHUD::StaticClass();
	ResourceHUDClass = URiseResourceHUD::StaticClass();

	static ConstructorHelpers::FObjectFinder<UStaticMesh> FloorMeshFinder(TEXT("/Engine/BasicShapes/Plane.Plane"));
	if (FloorMeshFinder.Succeeded())
	{
		FloorMeshAsset = FloorMeshFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> TerrainFinder(TEXT("/Game/Meshes/SM_Terrain.SM_Terrain"));
	if (TerrainFinder.Succeeded())
	{
		TerrainMeshAsset = TerrainFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> RoadFinder(TEXT("/Game/Meshes/SM_Road.SM_Road"));
	if (RoadFinder.Succeeded())
	{
		RoadMeshAsset = RoadFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> TreeAFinder(TEXT("/Game/Meshes/SM_Tree_A.SM_Tree_A"));
	if (TreeAFinder.Succeeded())
	{
		TreeMeshAssets.Add(TreeAFinder.Object);
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> TreeBFinder(TEXT("/Game/Meshes/SM_Tree_B.SM_Tree_B"));
	if (TreeBFinder.Succeeded())
	{
		TreeMeshAssets.Add(TreeBFinder.Object);
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> TreeCFinder(TEXT("/Game/Meshes/SM_Tree_C.SM_Tree_C"));
	if (TreeCFinder.Succeeded())
	{
		TreeMeshAssets.Add(TreeCFinder.Object);
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> RockAFinder(TEXT("/Game/Meshes/SM_Rock_A.SM_Rock_A"));
	if (RockAFinder.Succeeded())
	{
		RockMeshAssets.Add(RockAFinder.Object);
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> RockBFinder(TEXT("/Game/Meshes/SM_Rock_B.SM_Rock_B"));
	if (RockBFinder.Succeeded())
	{
		RockMeshAssets.Add(RockBFinder.Object);
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> BushFinder(TEXT("/Game/Meshes/SM_Bush.SM_Bush"));
	if (BushFinder.Succeeded())
	{
		BushMeshAssets.Add(BushFinder.Object);
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CommandCenterFinder(TEXT("/Game/Meshes/SM_CommandCenter.SM_CommandCenter"));
	if (CommandCenterFinder.Succeeded())
	{
		CommandCenterMeshAsset = CommandCenterFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> BarracksFinder(TEXT("/Game/Meshes/SM_Barracks.SM_Barracks"));
	if (BarracksFinder.Succeeded())
	{
		BarracksMeshAsset = BarracksFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> FactoryFinder(TEXT("/Game/Meshes/SM_Factory.SM_Factory"));
	if (FactoryFinder.Succeeded())
	{
		FactoryMeshAsset = FactoryFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> TowerFinder(TEXT("/Game/Meshes/SM_Tower.SM_Tower"));
	if (TowerFinder.Succeeded())
	{
		TowerMeshAsset = TowerFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> GeneratorFinder(TEXT("/Game/Meshes/SM_Generator.SM_Generator"));
	if (GeneratorFinder.Succeeded())
	{
		GeneratorMeshAsset = GeneratorFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> WallFinder(TEXT("/Game/Meshes/SM_Wall.SM_Wall"));
	if (WallFinder.Succeeded())
	{
		WallMeshAsset = WallFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SandbagsFinder(TEXT("/Game/Meshes/SM_Sandbags.SM_Sandbags"));
	if (SandbagsFinder.Succeeded())
	{
		SandbagsMeshAsset = SandbagsFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> HouseFinder(TEXT("/Game/Meshes/SM_House.SM_House"));
	if (HouseFinder.Succeeded())
	{
		HouseMeshAsset = HouseFinder.Object;
	}
}

void ARiseGameMode::BeginPlay()
{
	Super::BeginPlay();

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	OccupiedLocations.Empty();

	RemoveDefaultLevelGeometry();

	// Look da cena: sol suave, ceu encoberto frio, fog e exposicao travada
	if (World->SpawnActor<ARiseEnvironmentDirector>(ARiseEnvironmentDirector::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator))
	{
		UE_LOG(LogTemp, Warning, TEXT("[Visual] Diretor de ambiente iniciado"));
	}

	SpawnGroundCollider();
	SpawnTerrain();

	// Grade de território desativa por padrão (o DrawDebugBox branco poluía a cena)
	if (ARiseTerritoryManager* TerritoryManager = World->SpawnActor<ARiseTerritoryManager>(ARiseTerritoryManager::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator))
	{
		TerritoryManager->GridWidth = TerritoryGridWidth;
		TerritoryManager->GridHeight = TerritoryGridHeight;
		TerritoryManager->CellSize = TerritoryCellSize;
		TerritoryManager->bShowDebugGrid = false;
	}

	// Nós de recursos (economia) — antes das coletoras, para elas já nascerem com alvo
	SpawnResourceNodeAt(ERiseResourceType::Food, FVector(2500.0f, -3500.0f, 0.0f), 1200.0f);
	SpawnResourceNodeAt(ERiseResourceType::MicroChips, FVector(-2500.0f, -3500.0f, 0.0f), 900.0f);
	SpawnResourceNodeAt(ERiseResourceType::RareMetal, FVector(4200.0f, -1200.0f, 0.0f), 800.0f);
	SpawnResourceNodeAt(ERiseResourceType::Petrol, FVector(-4200.0f, -1200.0f, 0.0f), 800.0f);
	SpawnResourceNodeAt(ERiseResourceType::Dollars, FVector(3500.0f, 3500.0f, 0.0f), 1000.0f);
	SpawnResourceNodeAt(ERiseResourceType::Knowledge, FVector(-3500.0f, 3500.0f, 0.0f), 700.0f);
	SpawnResourceNodeAt(ERiseResourceType::Food, FVector(9500.0f, 7500.0f, 0.0f), 1200.0f);
	SpawnResourceNodeAt(ERiseResourceType::Petrol, FVector(7500.0f, 9500.0f, 0.0f), 900.0f);
	SpawnResourceNodeAt(ERiseResourceType::RareMetal, FVector(10500.0f, 10500.0f, 0.0f), 1000.0f);

	// Base e exército do jogador (facção azul)
	const FVector PlayerBaseLocation(0.0f, -2000.0f, 0.0f);
	const FVector EnemyBaseLocation(7000.0f, 7000.0f, 0.0f);

	SpawnBaseAt(PlayerBaseLocation, 0);
	SpawnUnitAt(ERiseUnitType::Infantry, FVector(0.0f, 300.0f, 0.0f), 0);
	SpawnUnitAt(ERiseUnitType::Infantry, FVector(0.0f, 800.0f, 0.0f), 0);
	SpawnUnitAt(ERiseUnitType::Tank, FVector(500.0f, 400.0f, 0.0f), 0);
	SpawnUnitAt(ERiseUnitType::Worker, FVector(400.0f, -1400.0f, 0.0f), 0);
	SpawnUnitAt(ERiseUnitType::Worker, FVector(-400.0f, -1400.0f, 0.0f), 0);

	// Base e exército inimigo (facção vermelha)
	SpawnBaseAt(EnemyBaseLocation, 1);
	SpawnUnitAt(ERiseUnitType::Tank, FVector(7400.0f, 7400.0f, 0.0f), 1);
	SpawnUnitAt(ERiseUnitType::Infantry, FVector(7000.0f, 7800.0f, 0.0f), 1);
	SpawnUnitAt(ERiseUnitType::Infantry, FVector(7800.0f, 7000.0f, 0.0f), 1);
	SpawnUnitAt(ERiseUnitType::Infantry, FVector(7600.0f, 7600.0f, 0.0f), 1);
	SpawnUnitAt(ERiseUnitType::Worker, FVector(6600.0f, 6400.0f, 0.0f), 1);
	SpawnUnitAt(ERiseUnitType::Worker, FVector(7400.0f, 6400.0f, 0.0f), 1);

	// Comandante da IA inimiga (economia propria, producao e ondas de ataque)
	if (ARiseAIManager* AIManager = World->SpawnActor<ARiseAIManager>(ARiseAIManager::StaticClass(), EnemyBaseLocation, FRotator::ZeroRotator))
	{
		AIManager->FactionID = 1;
		UE_LOG(LogTemp, Warning, TEXT("[IA] Comandante inimigo iniciado"));
	}

	// Complexos militares (visual) e vila neutra
	SpawnBaseComplex(PlayerBaseLocation, FLinearColor(0.15f, 0.55f, 1.0f), 1.0f);
	SpawnBaseComplex(EnemyBaseLocation, FLinearColor(1.0f, 0.25f, 0.25f), -1.0f);
	SpawnVillage(FVector(-6500.0f, -6500.0f, 0.0f), 6);
	SpawnVillage(FVector(6500.0f, -7000.0f, 0.0f), 4);

	ScatterDecor();

	// HUD de recursos
	if (ResourceHUDClass)
	{
		if (URiseResourceHUD* HUDWidget = CreateWidget<URiseResourceHUD>(World, ResourceHUDClass))
		{
			HUDWidget->AddToViewport(50);
		}
	}
}

void ARiseGameMode::SpawnGroundCollider()
{
	UWorld* World = GetWorld();
	UStaticMesh* PlaneMesh = FloorMeshAsset;
	if (!World || !PlaneMesh)
	{
		return;
	}

	AActor* FloorActor = World->SpawnActor<AActor>(AActor::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator);
	if (!FloorActor)
	{
		return;
	}

	UStaticMeshComponent* FloorComponent = NewObject<UStaticMeshComponent>(FloorActor);
	FloorComponent->SetStaticMesh(PlaneMesh);
	// 240 x 240 m: exatamente a area plana do terreno (planalto jogavel)
	FloorComponent->SetRelativeScale3D(FVector(240.0f, 240.0f, 1.0f));
	FloorComponent->SetCollisionProfileName(TEXT("BlockAll"));
	FloorComponent->SetVisibility(false);
	FloorComponent->RegisterComponent();
	FloorActor->SetRootComponent(FloorComponent);
}

void ARiseGameMode::SpawnTerrain()
{
	auto SpawnGroundVisual = [this](UStaticMesh* Mesh, const FLinearColor& Color, float Roughness)
	{
		AActor* Ground = SpawnProp(Mesh, FVector::ZeroVector, 0.0f, FVector(1.0f), false);
		if (!Ground)
		{
			return;
		}

		if (UStaticMeshComponent* MeshComponent = Ground->FindComponentByClass<UStaticMeshComponent>())
		{
			if (UMaterialInstanceDynamic* Dyn = MeshComponent->CreateAndSetMaterialInstanceDynamic(0))
			{
				Dyn->SetVectorParameterValue(TEXT("BaseColor"), Color);
				Dyn->SetScalarParameterValue(TEXT("Roughness"), Roughness);
			}
		}
	};

	SpawnGroundVisual(TerrainMeshAsset, TerrainColor, TerrainRoughness);
	SpawnGroundVisual(RoadMeshAsset, RoadColor, RoadRoughness);
}

AActor* ARiseGameMode::SpawnProp(UStaticMesh* Mesh, const FVector& Location, float Yaw, const FVector& Scale, bool bCastShadow)
{
	UWorld* World = GetWorld();
	if (!World || !Mesh)
	{
		return nullptr;
	}

	AActor* Prop = World->SpawnActor<AActor>(AActor::StaticClass(), Location, FRotator(0.0f, Yaw, 0.0f));
	if (!Prop)
	{
		return nullptr;
	}

	UStaticMeshComponent* MeshComponent = NewObject<UStaticMeshComponent>(Prop);
	MeshComponent->SetStaticMesh(Mesh);
	MeshComponent->SetRelativeScale3D(Scale);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MeshComponent->SetCastShadow(bCastShadow);
	MeshComponent->RegisterComponent();
	Prop->SetRootComponent(MeshComponent);

	return Prop;
}

AActor* ARiseGameMode::SpawnBuilding(UStaticMesh* Mesh, const FVector& Location, float Yaw, const FLinearColor& Tint)
{
	AActor* Building = SpawnProp(Mesh, Location, Yaw, FVector(1.0f));
	if (!Building)
	{
		return nullptr;
	}

	if (UStaticMeshComponent* MeshComponent = Building->FindComponentByClass<UStaticMeshComponent>())
	{
		if (UMaterialInstanceDynamic* Dyn = MeshComponent->CreateAndSetMaterialInstanceDynamic(0))
		{
			Dyn->SetVectorParameterValue(TEXT("Color"), Tint);
		}
	}

	OccupiedLocations.Add(Location);
	return Building;
}

void ARiseGameMode::SpawnBaseComplex(const FVector& BaseLocation, const FLinearColor& Tint, float Sign)
{
	// Layout local (x espelhado pelo Sign): frente = +Y
	struct FLayoutPart
	{
		UStaticMesh* Mesh;
		FVector Offset;
		float Yaw;
	};

	const FLayoutPart Layout[] = {
		{ CommandCenterMeshAsset, FVector(0.0f, -2200.0f, 0.0f), 0.0f },
		{ BarracksMeshAsset, FVector(-2400.0f, -600.0f, 0.0f), 0.0f },
		{ BarracksMeshAsset, FVector(2400.0f, -600.0f, 0.0f), 0.0f },
		{ FactoryMeshAsset, FVector(-2200.0f, 800.0f, 0.0f), 0.0f },
		{ GeneratorMeshAsset, FVector(2200.0f, 800.0f, 0.0f), 0.0f },
		{ TowerMeshAsset, FVector(-1800.0f, -1800.0f, 0.0f), 0.0f },
		{ TowerMeshAsset, FVector(1800.0f, -1800.0f, 0.0f), 0.0f },
	};

	const float YawOffset = (Sign > 0.0f) ? 0.0f : 180.0f;

	for (const FLayoutPart& Part : Layout)
	{
		if (!Part.Mesh)
		{
			continue;
		}

		const FVector Location = BaseLocation + FVector(Part.Offset.X * Sign, Part.Offset.Y * Sign, 0.0f);
		SpawnBuilding(Part.Mesh, Location, Part.Yaw + YawOffset, Tint);
	}

	// Sacos de areia em anel ao redor do complexo
	if (SandbagsMeshAsset)
	{
		const int32 SandbagCount = 12;
		for (int32 i = 0; i < SandbagCount; ++i)
		{
			const float Angle = (2.0f * PI * i) / SandbagCount;
			const FVector Location = BaseLocation + FVector(FMath::Cos(Angle) * 3000.0f, FMath::Sin(Angle) * 3000.0f, 0.0f);
			const float Yaw = FMath::RadiansToDegrees(Angle) + 90.0f;
			SpawnBuilding(SandbagsMeshAsset, Location, Yaw, Tint);
		}
	}
}

void ARiseGameMode::SpawnVillage(const FVector& Center, int32 HouseCount)
{
	FRandomStream Stream(static_cast<int32>(Center.X + Center.Y));
	OccupiedLocations.Add(Center);

	for (int32 i = 0; i < HouseCount; ++i)
	{
		const float X = Center.X + Stream.FRandRange(-2200.0f, 2200.0f);
		const float Y = Center.Y + Stream.FRandRange(-2200.0f, 2200.0f);
		const float Yaw = Stream.FRandRange(0.0f, 360.0f);

		if (HouseMeshAsset)
		{
			SpawnBuilding(HouseMeshAsset, FVector(X, Y, 0.0f), Yaw, FLinearColor(0.75f, 0.68f, 0.55f));
		}
		if (WallMeshAsset && (i % 2) == 0)
		{
			SpawnProp(WallMeshAsset, FVector(X + Stream.FRandRange(-2600.0f, 2600.0f), Y + Stream.FRandRange(-2600.0f, 2600.0f), 0.0f), Yaw + 90.0f, FVector(1.0f));
		}
	}
}

void ARiseGameMode::ScatterDecor()
{
	FRandomStream Stream(20260917);

	const float PlateauLimit = 11800.0f;   // dentro do planalto (120 m), com folga das colinas
	const float MapLimit = 18800.0f;       // ate perto da borda do terreno (200 m)

	// Raio livre ao redor de bases, vilas e nos de recurso: cobre o anel de sacos de
	// areia (3000) e as casas das vilas, para nada nascer dentro das construcoes.
	const float StructureClearance = 4500.0f;

	auto IsBlocked = [this](const FVector& Location, float Radius)
	{
		for (const FVector& Occupied : OccupiedLocations)
		{
			if (FVector::DistSquared2D(Location, Occupied) < Radius * Radius)
			{
				return true;
			}
		}
		return false;
	};

	// Estradas: cruz principal (|X| ou |Y| < 11 m) + os dois ramais perpendiculares
	// desenhados em build_road() (spur_nw em -62,72 e spur_se em 72,-62, em metros).
	auto IsOnRoad = [](float X, float Y)
	{
		const float Xm = X / 100.0f;
		const float Ym = Y / 100.0f;

		if (FMath::Abs(Xm) < 11.0f || FMath::Abs(Ym) < 11.0f)
		{
			return true;
		}

		const bool bSpurNW = (Xm > -72.0f && Xm < -52.0f && Ym > 53.0f && Ym < 91.0f);
		const bool bSpurSE = (Xm > 53.0f && Xm < 91.0f && Ym > -72.0f && Ym < -52.0f);
		return bSpurNW || bSpurSE;
	};

	TArray<FVector> PlacedLocations;

	auto IsTooClose = [&PlacedLocations](const FVector& Location, float MinSpacing)
	{
		for (const FVector& Placed : PlacedLocations)
		{
			if (FVector::DistSquared2D(Location, Placed) < MinSpacing * MinSpacing)
			{
				return true;
			}
		}
		return false;
	};

	auto PlaceScatter = [this, &Stream, &IsBlocked, &IsOnRoad, &IsTooClose, &PlacedLocations, MapLimit, PlateauLimit, StructureClearance](const TArray<TObjectPtr<UStaticMesh>>& Meshes, int32 Count, float MinScale, float MaxScale, float MinSpacing, bool bOnHills)
	{
		if (Meshes.Num() == 0)
		{
			return;
		}

		for (int32 i = 0; i < Count; ++i)
		{
			const float X = Stream.FRandRange(-MapLimit, MapLimit);
			const float Y = Stream.FRandRange(-MapLimit, MapLimit);
			const float Chebyshev = FMath::Max(FMath::Abs(X), FMath::Abs(Y));

			if (bOnHills != (Chebyshev > 12600.0f))
			{
				continue;
			}
			if (!bOnHills)
			{
				if (Chebyshev > PlateauLimit)
				{
					continue;
				}
				// deixa as estradas e ramais livres
				if (IsOnRoad(X, Y))
				{
					continue;
				}
				if (IsBlocked(FVector(X, Y, 0.0f), StructureClearance))
				{
					continue;
				}
			}
			else if (IsBlocked(FVector(X, Y, 0.0f), 900.0f))
			{
				continue;
			}

			if (IsTooClose(FVector(X, Y, 0.0f), MinSpacing))
			{
				continue;
			}

			UStaticMesh* Mesh = Meshes[Stream.RandRange(0, Meshes.Num() - 1)];
			const float Scale = Stream.FRandRange(MinScale, MaxScale);
			const float Z = RiseTerrainHeightUU(X, Y) - 20.0f;
			SpawnProp(Mesh, FVector(X, Y, Z), Stream.FRandRange(0.0f, 360.0f), FVector(Scale), false);
			PlacedLocations.Add(FVector(X, Y, 0.0f));
		}
	};

	PlaceScatter(TreeMeshAssets, 120, 0.8f, 1.35f, 420.0f, false);
	PlaceScatter(BushMeshAssets, 70, 0.8f, 1.3f, 260.0f, false);
	PlaceScatter(RockMeshAssets, 45, 0.6f, 1.5f, 380.0f, false);

	PlaceScatter(TreeMeshAssets, 180, 0.9f, 1.6f, 420.0f, true);
	PlaceScatter(RockMeshAssets, 90, 0.8f, 2.2f, 380.0f, true);
}

void ARiseGameMode::RemoveDefaultLevelGeometry()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// O mapa padrao (Basic) traz um Landscape com material xadrez (WorldGrid) que fica
	// exatamente sobre o planalto: gera z-fighting no chao e intercepta os traces de clique.
	int32 Removed = 0;
	for (TActorIterator<ALandscapeProxy> It(World); It; ++It)
	{
		ALandscapeProxy* Landscape = *It;
		if (!IsValid(Landscape))
		{
			continue;
		}

		Landscape->SetActorEnableCollision(false);
		Landscape->SetActorHiddenInGame(true);
		++Removed;
	}

	if (Removed > 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Mundo] Landscape padrao escondido (%d ator(es))"), Removed);
	}
}

void ARiseGameMode::SpawnUnitAt(ERiseUnitType Type, const FVector& Location, int32 Faction)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (ARiseUnit* Unit = World->SpawnActor<ARiseUnit>(ARiseUnit::StaticClass(), Location + FVector(0.0f, 0.0f, 800.0f), FRotator::ZeroRotator))
	{
		Unit->ConfigureUnit(Type, Faction);
		Unit->SetActorLocation(Location);
	}
}

void ARiseGameMode::SpawnResourceNodeAt(ERiseResourceType Type, const FVector& Location, float Amount)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (ARiseResourceNode* Node = World->SpawnActor<ARiseResourceNode>(ARiseResourceNode::StaticClass(), Location + FVector(0.0f, 0.0f, 800.0f), FRotator::ZeroRotator))
	{
		Node->ConfigureNode(Type, Amount);
		Node->SetActorLocation(Location);
		OccupiedLocations.Add(Location);
	}
}

void ARiseGameMode::SpawnBaseAt(const FVector& Location, int32 Faction)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (ARiseBase* Base = World->SpawnActor<ARiseBase>(ARiseBase::StaticClass(), Location + FVector(0.0f, 0.0f, 800.0f), FRotator::ZeroRotator))
	{
		Base->ConfigureBase(Faction);
		Base->SetActorLocation(Location);
		OccupiedLocations.Add(Location);
	}
}
