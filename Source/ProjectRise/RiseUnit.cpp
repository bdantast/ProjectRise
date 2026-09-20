#include "RiseUnit.h"
#include "RiseProjectile.h"
#include "RiseBase.h"
#include "RiseResourceNode.h"
#include "RiseBuilding.h"
#include "UObject/ConstructorHelpers.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "EngineUtils.h"
#include "Engine/DamageEvents.h"
#include "Kismet/KismetMathLibrary.h"

FRiseUnitStats GetUnitStats(ERiseUnitType Type)
{
	FRiseUnitStats Stats;

	switch (Type)
	{
	case ERiseUnitType::Infantry:
		Stats.MaxHealth = 80.0f;
		Stats.Speed = 900.0f;
		Stats.AttackDamage = 15.0f;
		Stats.AttackRange = 500.0f;
		Stats.AttackCooldown = 1.2f;
		Stats.BuildTime = 2.0f;
		Stats.HealthBarHeight = 210.0f;
		Stats.BuildCost.Food = 50.0f;
		Stats.BuildCost.MicroChips = 10.0f;
		Stats.BuildCost.Dollars = 100.0f;
		break;

	case ERiseUnitType::Tank:
		Stats.MaxHealth = 260.0f;
		Stats.Speed = 600.0f;
		Stats.AttackDamage = 40.0f;
		Stats.AttackRange = 900.0f;
		Stats.AttackCooldown = 2.0f;
		Stats.BuildTime = 5.0f;
		Stats.HealthBarHeight = 330.0f;
		Stats.BuildCost.RareMetal = 60.0f;
		Stats.BuildCost.Petrol = 40.0f;
		Stats.BuildCost.Dollars = 250.0f;
		break;

	case ERiseUnitType::Drone:
		Stats.MaxHealth = 120.0f;
		Stats.Speed = 1400.0f;
		Stats.AttackDamage = 30.0f;
		Stats.AttackRange = 1200.0f;
		Stats.AttackCooldown = 1.8f;
		Stats.BuildTime = 4.0f;
		Stats.HealthBarHeight = 300.0f;
		Stats.BuildCost.MicroChips = 60.0f;
		Stats.BuildCost.Knowledge = 30.0f;
		Stats.BuildCost.Dollars = 200.0f;
		break;

	case ERiseUnitType::Worker:
		Stats.MaxHealth = 60.0f;
		Stats.Speed = 700.0f;
		Stats.AttackDamage = 0.0f;
		Stats.AttackRange = 0.0f;
		Stats.AttackCooldown = 0.0f;
		Stats.BuildTime = 2.0f;
		Stats.HealthBarHeight = 290.0f;
		Stats.HarvestRate = 12.0f;
		Stats.CarryCapacity = 30.0f;
		Stats.BuildCost.Food = 30.0f;
		Stats.BuildCost.MicroChips = 5.0f;
		Stats.BuildCost.Dollars = 60.0f;
		break;

	case ERiseUnitType::Hummer:
		Stats.MaxHealth = 140.0f;
		Stats.Speed = 1700.0f;
		Stats.AttackDamage = 18.0f;
		Stats.AttackRange = 550.0f;
		Stats.AttackCooldown = 1.0f;
		Stats.BuildTime = 3.0f;
		Stats.HealthBarHeight = 210.0f;
		Stats.BuildCost.Food = 40.0f;
		Stats.BuildCost.RareMetal = 30.0f;
		Stats.BuildCost.Petrol = 25.0f;
		Stats.BuildCost.Dollars = 150.0f;
		break;

	case ERiseUnitType::Helicopter:
		Stats.MaxHealth = 220.0f;
		Stats.Speed = 1900.0f;
		Stats.AttackDamage = 45.0f;
		Stats.AttackRange = 1100.0f;
		Stats.AttackCooldown = 1.5f;
		Stats.BuildTime = 6.0f;
		Stats.HealthBarHeight = 380.0f;
		Stats.BuildCost.MicroChips = 80.0f;
		Stats.BuildCost.RareMetal = 70.0f;
		Stats.BuildCost.Petrol = 60.0f;
		Stats.BuildCost.Dollars = 320.0f;
		break;
	}

	return Stats;
}

FString RiseUnitTypeToString(ERiseUnitType Type)
{
	switch (Type)
	{
	case ERiseUnitType::Infantry: return TEXT("Infantaria");
	case ERiseUnitType::Tank:     return TEXT("Tanque");
	case ERiseUnitType::Drone:    return TEXT("Drone");
	case ERiseUnitType::Worker:   return TEXT("Coletora");
	case ERiseUnitType::Hummer:   return TEXT("Hummer");
	case ERiseUnitType::Helicopter: return TEXT("Helicoptero");
	}
	return TEXT("?");
}

ARiseUnit::ARiseUnit()
{
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	UnitMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("UnitMesh"));
	UnitMesh->SetupAttachment(RootComponent);
	UnitMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMeshFinder.Succeeded())
	{
		CubeMeshAsset = CubeMeshFinder.Object;
		UnitMesh->SetStaticMesh(CubeMeshFinder.Object);
	}

	TurretMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TurretMesh"));
	TurretMesh->SetupAttachment(RootComponent);
	TurretMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (CubeMeshFinder.Succeeded())
	{
		TurretMesh->SetStaticMesh(CubeMeshFinder.Object);
	}
	TurretMesh->SetVisibility(false);

	SelectionRing = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SelectionRing"));
	SelectionRing->SetupAttachment(RootComponent);
	SelectionRing->SetRelativeLocation(FVector(0.0f, 0.0f, 5.0f));
	SelectionRing->SetRelativeScale3D(FVector(1.2f, 1.2f, 0.05f));
	SelectionRing->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SelectionRing->SetVisibility(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMeshFinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CylinderMeshFinder.Succeeded())
	{
		SelectionRing->SetStaticMesh(CylinderMeshFinder.Object);
	}

	SelectionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("SelectionBox"));
	SelectionBox->SetupAttachment(RootComponent);
	SelectionBox->SetRelativeLocation(FVector(0.0f, 0.0f, 40.0f));
	SelectionBox->SetBoxExtent(FVector(50.0f, 50.0f, 40.0f));
	SelectionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SelectionBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	SelectionBox->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> InfantryFinder(TEXT("/Game/Meshes/SM_Infantry.SM_Infantry"));
	if (InfantryFinder.Succeeded())
	{
		InfantryMeshAsset = InfantryFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> TankFinder(TEXT("/Game/Meshes/Military/SM_MilitaryTank.SM_MilitaryTank"));
	if (TankFinder.Succeeded())
	{
		TankMeshAsset = TankFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> TurretFinder(TEXT("/Game/Meshes/Military/SM_MilitaryTankTurret.SM_MilitaryTankTurret"));
	if (TurretFinder.Succeeded())
	{
		TankTurretMeshAsset = TurretFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> DroneFinder(TEXT("/Game/Meshes/SM_Drone.SM_Drone"));
	if (DroneFinder.Succeeded())
	{
		DroneMeshAsset = DroneFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> WorkerFinder(TEXT("/Game/Meshes/SM_Worker.SM_Worker"));
	if (WorkerFinder.Succeeded())
	{
		WorkerMeshAsset = WorkerFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> HummerFinder(TEXT("/Game/Meshes/Military/SM_Hummer.SM_Hummer"));
	if (HummerFinder.Succeeded())
	{
		HummerMeshAsset = HummerFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> HelicopterFinder(TEXT("/Game/Meshes/Military/SM_Helicopter.SM_Helicopter"));
	if (HelicopterFinder.Succeeded())
	{
		HelicopterMeshAsset = HelicopterFinder.Object;
	}

	Stats = GetUnitStats(UnitType);
	CurrentHealth = Stats.MaxHealth;
}

void ARiseUnit::BeginPlay()
{
	Super::BeginPlay();
}

void ARiseUnit::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!IsAlive())
	{
		return;
	}

	AttackTimer = FMath::Max(0.0f, AttackTimer - DeltaTime);

	if (IsWorker())
	{
		TickWorker(DeltaTime);
		return;
	}

	// Ordem de movimento tem prioridade (sobrevive a ordens antigas de ataque)
	if (bHasMoveOrder)
	{
		MoveToward(Destination, DeltaTime);
		return;
	}

	// Aquisição/alvo atual
	if (!CurrentTarget.IsValid())
	{
		const float AggroRadius = (FactionID == 0) ? 3500.0f : 5000.0f;
		CurrentTarget = AcquireTarget(AggroRadius);
	}

	if (CurrentTarget.IsValid())
	{
		AActor* Target = CurrentTarget.Get();
		const float DistToTarget = GetDistanceTo(Target);
		const float EngagementRange = Stats.AttackRange + 120.0f;

		if (DistToTarget > EngagementRange)
		{
			// Persegue o alvo, parando um pouco antes para nao empilhar unidades dentro dele
			const float StopDistance = FMath::Max(60.0f, EngagementRange * 0.8f);
			MoveToward(Target->GetActorLocation(), DeltaTime, StopDistance);
			return;
		}

		if (AttackTimer <= 0.0f)
		{
			FireProjectile(Target);
			AttackTimer = Stats.AttackCooldown;
		}
	}
	else if (bHasMoveOrder)
	{
		MoveToward(Destination, DeltaTime);
	}
}

float ARiseUnit::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (!IsAlive())
	{
		return 0.0f;
	}

	CurrentHealth = FMath::Max(0.0f, CurrentHealth - DamageAmount);

	// Unidade de combate sob fogo interrompe a ordem de deslocamento e revida
	if (!IsWorker() && CurrentHealth > 0.0f)
	{
		bHasMoveOrder = false;
		CurrentTarget = nullptr;
	}

	if (CurrentHealth <= 0.0f)
	{
		UE_LOG(LogTemp, Log, TEXT("[Unidade] %s faccao %d destruida (causer=%s)"),
			*RiseUnitTypeToString(UnitType), FactionID, DamageCauser ? *DamageCauser->GetName() : TEXT("?"));
		Destroy();
	}

	return DamageAmount;
}

void ARiseUnit::ConfigureUnit(ERiseUnitType InType, int32 InFaction)
{
	UnitType = InType;
	FactionID = InFaction;
	Stats = GetUnitStats(InType);
	CurrentHealth = Stats.MaxHealth;

	ApplyTypeVisuals();
	ApplyTeamColor();

	if (UnitType == ERiseUnitType::Worker && FactionID == 0)
	{
		// Somente as coletoras do jogador recebem um no automaticamente.
		// A IA (faccao 1) atribui os nos explicitamente pelo ARiseAIManager.
		if (ARiseResourceNode* Node = FindNearestResourceNode())
		{
			AssignResourceNode(Node);
		}
	}
}

void ARiseUnit::ApplyTypeVisuals()
{
	switch (UnitType)
	{
	case ERiseUnitType::Infantry:
		TurretMesh->SetVisibility(false);
		if (InfantryMeshAsset)
		{
			SetUnitVisual(InfantryMeshAsset, FVector(0.0f, 0.0f, 0.0f), FVector(1.0f), 1.6f, FVector(50.0f, 50.0f, 95.0f), 95.0f);
		}
		else
		{
			SetUnitVisual(CubeMeshAsset, FVector(0.0f, 0.0f, 25.0f), FVector(0.25f, 0.25f, 0.35f), 1.2f, FVector(45.0f, 45.0f, 45.0f), 40.0f);
		}
		break;

	case ERiseUnitType::Tank:
		if (TankMeshAsset)
		{
			SetUnitVisual(TankMeshAsset, FVector(0.0f, 0.0f, 0.0f), FVector(1.0f), 6.5f, FVector(140.0f, 300.0f, 95.0f), 65.0f);
		}
		else
		{
			SetUnitVisual(CubeMeshAsset, FVector(0.0f, 0.0f, 25.0f), FVector(0.8f, 0.8f, 0.5f), 2.0f, FVector(150.0f, 150.0f, 100.0f), 100.0f);
		}

		if (TankTurretMeshAsset)
		{
			TurretMesh->SetStaticMesh(TankTurretMeshAsset);
			TurretMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 115.0f));
			TurretMesh->SetRelativeScale3D(FVector(1.0f));
		}
		else
		{
			TurretMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 60.0f));
			TurretMesh->SetRelativeScale3D(FVector(0.5f, 0.5f, 0.3f));
		}
		TurretMesh->SetVisibility(true);
		break;

	case ERiseUnitType::Drone:
		TurretMesh->SetVisibility(false);
		if (DroneMeshAsset)
		{
			SetUnitVisual(DroneMeshAsset, FVector(0.0f, 0.0f, 150.0f), FVector(1.0f), 6.0f, FVector(280.0f, 280.0f, 45.0f), 150.0f);
		}
		else
		{
			SetUnitVisual(CubeMeshAsset, FVector(0.0f, 0.0f, 180.0f), FVector(0.35f, 0.35f, 0.2f), 2.0f, FVector(120.0f, 120.0f, 50.0f), 180.0f);
		}
		break;

	case ERiseUnitType::Worker:
		TurretMesh->SetVisibility(false);
		if (WorkerMeshAsset)
		{
			SetUnitVisual(WorkerMeshAsset, FVector(0.0f, 0.0f, 0.0f), FVector(1.0f), 4.5f, FVector(120.0f, 200.0f, 130.0f), 130.0f);
		}
		else
		{
			SetUnitVisual(CubeMeshAsset, FVector(0.0f, 0.0f, 22.0f), FVector(0.3f, 0.3f, 0.3f), 1.5f, FVector(80.0f, 80.0f, 60.0f), 60.0f);
		}
		break;

	case ERiseUnitType::Hummer:
		TurretMesh->SetVisibility(false);
		if (HummerMeshAsset)
		{
			SetUnitVisual(HummerMeshAsset, FVector(0.0f, 0.0f, 0.0f), FVector(1.0f), 2.6f, FVector(95.0f, 130.0f, 70.0f), 75.0f);
		}
		else
		{
			SetUnitVisual(CubeMeshAsset, FVector(0.0f, 0.0f, 25.0f), FVector(0.45f, 0.25f, 0.2f), 1.5f, FVector(90.0f, 90.0f, 60.0f), 60.0f);
		}
		break;

	case ERiseUnitType::Helicopter:
		TurretMesh->SetVisibility(false);
		if (HelicopterMeshAsset)
		{
			SetUnitVisual(HelicopterMeshAsset, FVector(0.0f, 0.0f, 180.0f), FVector(1.0f), 3.2f, FVector(150.0f, 150.0f, 55.0f), 180.0f);
		}
		else
		{
			SetUnitVisual(CubeMeshAsset, FVector(0.0f, 0.0f, 200.0f), FVector(0.4f, 0.4f, 0.2f), 2.0f, FVector(140.0f, 140.0f, 50.0f), 200.0f);
		}
		break;
	}
}

void ARiseUnit::SetUnitVisual(UStaticMesh* Mesh, const FVector& RelativeLocation, const FVector& Scale, float RingScale, const FVector& BoxExtent, float BoxCenterZ)
{
	if (Mesh && UnitMesh->GetStaticMesh() != Mesh)
	{
		UnitMesh->SetStaticMesh(Mesh);
	}

	UnitMesh->SetRelativeLocation(RelativeLocation);
	UnitMesh->SetRelativeScale3D(Scale);

	if (SelectionRing)
	{
		SelectionRing->SetRelativeScale3D(FVector(RingScale, RingScale, 0.05f));
	}

	if (SelectionBox)
	{
		SelectionBox->SetBoxExtent(BoxExtent);
		SelectionBox->SetRelativeLocation(FVector(0.0f, 0.0f, BoxCenterZ));
	}
}

void ARiseUnit::ApplyTeamColor()
{
	if (!UnitMesh)
	{
		return;
	}

	const FLinearColor TeamColor = IsWorker()
		? FLinearColor(0.95f, 0.85f, 0.2f)
		: ((FactionID == 0) ? FLinearColor(0.15f, 0.55f, 1.0f) : FLinearColor(1.0f, 0.25f, 0.25f));

	if (UMaterialInstanceDynamic* Dyn = UnitMesh->CreateAndSetMaterialInstanceDynamic(0))
	{
		Dyn->SetVectorParameterValue(TEXT("Color"), TeamColor);
	}
	if (TurretMesh && TurretMesh->IsVisible())
	{
		if (UMaterialInstanceDynamic* Dyn = TurretMesh->CreateAndSetMaterialInstanceDynamic(0))
		{
			Dyn->SetVectorParameterValue(TEXT("Color"), TeamColor);
		}
	}
}

void ARiseUnit::MoveTo(const FVector& NewDestination)
{
	ResourceNode = nullptr;
	WorkerState = ERiseWorkerState::Idle;
	Destination = NewDestination;
	Destination.Z = GetActorLocation().Z;
	bHasMoveOrder = true;
	CurrentTarget = nullptr;
}

void ARiseUnit::Stop()
{
	bHasMoveOrder = false;
	Destination = FVector::ZeroVector;
	CurrentTarget = nullptr;
	ResourceNode = nullptr;
	BuildTarget = nullptr;
	WorkerState = ERiseWorkerState::Idle;
	HarvestTimer = 0.0f;
}

void ARiseUnit::OrderAttack(AActor* Target)
{
	if (!IsValid(Target) || IsWorker())
	{
		return;
	}

	CurrentTarget = Target;
	bHasMoveOrder = false;
	ResourceNode = nullptr;
	WorkerState = ERiseWorkerState::Idle;
}

bool ARiseUnit::HasEngagement() const
{
	return CurrentTarget.IsValid() && !bHasMoveOrder;
}

void ARiseUnit::MoveToward(const FVector& Goal, float DeltaTime, float AcceptanceRadius)
{
	const FVector Current = GetActorLocation();
	const FVector ToGoal = Goal - Current;
	const float Distance = ToGoal.Size();
	const float StepDistance = Stats.Speed * DeltaTime;

	if (Distance <= AcceptanceRadius)
	{
		if (bHasMoveOrder)
		{
			bHasMoveOrder = false;
		}
		return;
	}

	const float MoveStep = FMath::Min(StepDistance, Distance - AcceptanceRadius);
	const FVector NewLocation = Current + ToGoal.GetSafeNormal() * MoveStep;
	SetActorLocation(NewLocation);

	FRotator Facing = FRotationMatrix::MakeFromX(ToGoal).Rotator();
	Facing.Pitch = 0.0f;
	Facing.Roll = 0.0f;
	SetActorRotation(Facing);
}

TWeakObjectPtr<AActor> ARiseUnit::AcquireTarget(float Radius)
{
	AActor* BestActor = nullptr;
	float BestDistSq = Radius * Radius;

	if (UWorld* World = GetWorld())
	{
		const FVector MyLocation = GetActorLocation();

		for (TActorIterator<ARiseUnit> UnitIt(World); UnitIt; ++UnitIt)
		{
			ARiseUnit* Other = *UnitIt;
			if (Other == this || !Other->IsAlive() || Other->FactionID == FactionID)
			{
				continue;
			}

			const float DistSq = FVector::DistSquared(MyLocation, Other->GetActorLocation());
			if (DistSq < BestDistSq)
			{
				BestDistSq = DistSq;
				BestActor = Other;
			}
		}

		for (TActorIterator<ARiseBase> BaseIt(World); BaseIt; ++BaseIt)
		{
			ARiseBase* OtherBase = *BaseIt;
			if (!IsValid(OtherBase) || !OtherBase->IsAlive() || OtherBase->GetFactionID() == FactionID)
			{
				continue;
			}

			const float DistSq = FVector::DistSquared(MyLocation, OtherBase->GetActorLocation());
			if (DistSq < BestDistSq)
			{
				BestDistSq = DistSq;
				BestActor = OtherBase;
			}
		}
	}

	return BestActor;
}

void ARiseUnit::FireProjectile(AActor* Target)
{
	UWorld* World = GetWorld();
	if (!World || !Target)
	{
		return;
	}

	const FVector AimDirection = (Target->GetActorLocation() - GetActorLocation()).GetSafeNormal();
	const FRotator AimRotation = FRotationMatrix::MakeFromX(AimDirection).Rotator();

	if (TurretMesh && TurretMesh->IsVisible())
	{
		TurretMesh->SetWorldRotation(AimRotation);
	}
	else
	{
		FRotator Facing = AimRotation;
		Facing.Pitch = 0.0f;
		Facing.Roll = 0.0f;
		SetActorRotation(Facing);
	}

	const FVector MuzzleLocation = (TurretMesh && TurretMesh->IsVisible())
		? TurretMesh->GetComponentLocation()
		: (UnitMesh->GetComponentLocation() + FVector(0.0f, 0.0f, 30.0f));

	FActorSpawnParameters Params;
	Params.Owner = this;
	Params.Instigator = this;

	if (ARiseProjectile* Projectile = World->SpawnActor<ARiseProjectile>(ARiseProjectile::StaticClass(), MuzzleLocation, AimRotation, Params))
	{
		Projectile->Initialize(Target, Stats.AttackDamage, FactionID);
	}
}

void ARiseUnit::AssignResourceNode(ARiseResourceNode* Node)
{
	if (!IsWorker() || !IsValid(Node) || Node->IsDepleted())
	{
		return;
	}

	ResourceNode = Node;
	BuildTarget = nullptr;
	bHasMoveOrder = false;
	CarriedAmount = 0.0f;
	CarriedResourceType = Node->GetResourceType();
	HarvestTimer = 0.0f;
	WorkerState = ERiseWorkerState::MovingToNode;
}

ARiseBase* ARiseUnit::FindNearestBase() const
{
	ARiseBase* BestBase = nullptr;
	float BestDistSq = TNumericLimits<float>::Max();

	if (UWorld* World = GetWorld())
	{
		const FVector MyLocation = GetActorLocation();

		for (TActorIterator<ARiseBase> BaseIt(World); BaseIt; ++BaseIt)
		{
			ARiseBase* Base = *BaseIt;
			if (!IsValid(Base) || !Base->IsAlive() || Base->GetFactionID() != FactionID)
			{
				continue;
			}

			const float DistSq = FVector::DistSquared(MyLocation, Base->GetActorLocation());
			if (DistSq < BestDistSq)
			{
				BestDistSq = DistSq;
				BestBase = Base;
			}
		}
	}

	return BestBase;
}

ARiseResourceNode* ARiseUnit::FindNearestResourceNode() const
{
	ARiseResourceNode* BestNode = nullptr;
	float BestDistSq = TNumericLimits<float>::Max();

	if (UWorld* World = GetWorld())
	{
		const FVector MyLocation = GetActorLocation();

		for (TActorIterator<ARiseResourceNode> NodeIt(World); NodeIt; ++NodeIt)
		{
			ARiseResourceNode* Node = *NodeIt;
			if (!IsValid(Node) || Node->IsDepleted())
			{
				continue;
			}

			const float DistSq = FVector::DistSquared2D(MyLocation, Node->GetActorLocation());
			if (DistSq < BestDistSq)
			{
				BestDistSq = DistSq;
				BestNode = Node;
			}
		}
	}

	return BestNode;
}

void ARiseUnit::OrderBuild(ARiseBuilding* Building)
{
	if (!IsWorker() || !IsValid(Building) || Building->GetFactionID() != FactionID || Building->IsActive())
	{
		return;
	}

	BuildTarget = Building;
	bHasMoveOrder = false;
	ResourceNode = nullptr;
	CarriedAmount = 0.0f;
	WorkerState = ERiseWorkerState::MovingToBuild;
}

ARiseBuilding* ARiseUnit::FindNearestConstructionSite() const
{
	ARiseBuilding* BestSite = nullptr;
	float BestDistSq = FMath::Square(3500.0f);

	if (UWorld* World = GetWorld())
	{
		const FVector MyLocation = GetActorLocation();

		for (TActorIterator<ARiseBuilding> SiteIt(World); SiteIt; ++SiteIt)
		{
			ARiseBuilding* Site = *SiteIt;
			if (!IsValid(Site) || Site->GetFactionID() != FactionID || Site->IsActive())
			{
				continue;
			}

			const float DistSq = FVector::DistSquared2D(MyLocation, Site->GetActorLocation());
			if (DistSq < BestDistSq)
			{
				BestDistSq = DistSq;
				BestSite = Site;
			}
		}
	}

	return BestSite;
}

void ARiseUnit::TickWorker(float DeltaTime)
{
	if (bHasMoveOrder)
	{
		MoveToward(Destination, DeltaTime);
		return;
	}

	ARiseBuilding* BuildingSite = BuildTarget.Get();
	if (BuildingSite && (BuildingSite->GetFactionID() != FactionID || BuildingSite->IsActive()))
	{
		BuildTarget = nullptr;
		BuildingSite = nullptr;
		WorkerState = ERiseWorkerState::Idle;
	}

	// Obra da propria facao tem prioridade: qualquer coletora (ate as de coleta) vai construir
	if (!BuildingSite)
	{
		if (ARiseBuilding* Site = FindNearestConstructionSite())
		{
			BuildTarget = Site;
			BuildingSite = Site;
			WorkerState = ERiseWorkerState::MovingToBuild;
			ResourceNode = nullptr;
			CarriedAmount = 0.0f;
		}
	}

	ARiseResourceNode* Node = ResourceNode.Get();

	if (!Node || Node->IsDepleted())
	{
		if (CarriedAmount > 0.0f)
		{
			WorkerState = ERiseWorkerState::Returning;
		}
		else
		{
			ResourceNode = nullptr;
			WorkerState = ERiseWorkerState::Idle;
		}
	}

	switch (WorkerState)
	{
	case ERiseWorkerState::MovingToNode:
	{
		if (!Node)
		{
			WorkerState = ERiseWorkerState::Idle;
			return;
		}

		MoveToward(Node->GetActorLocation(), DeltaTime, 240.0f);
		if (FVector::Dist2D(GetActorLocation(), Node->GetActorLocation()) <= 260.0f)
		{
			WorkerState = ERiseWorkerState::Harvesting;
		}
		break;
	}

	case ERiseWorkerState::Harvesting:
	{
		if (!Node)
		{
			WorkerState = ERiseWorkerState::Idle;
			return;
		}

		CarriedResourceType = Node->GetResourceType();
		const float Extracted = Node->Extract(Stats.HarvestRate * DeltaTime);
		CarriedAmount += Extracted;

		if (Extracted <= 0.0f || CarriedAmount >= Stats.CarryCapacity)
		{
			WorkerState = ERiseWorkerState::Returning;
		}
		break;
	}

	case ERiseWorkerState::Returning:
	{
		ARiseBase* Base = FindNearestBase();
		if (!Base)
		{
			return;
		}

		MoveToward(Base->GetActorLocation(), DeltaTime, 320.0f);
		if (FVector::Dist2D(GetActorLocation(), Base->GetActorLocation()) <= 340.0f)
		{
			if (CarriedAmount > 0.0f)
			{
				RiseDepositToFaction(this, FactionID, CarriedResourceType, CarriedAmount);
				CarriedAmount = 0.0f;
			}

			if (Node && !Node->IsDepleted())
			{
				WorkerState = ERiseWorkerState::MovingToNode;
			}
			else
			{
				ResourceNode = nullptr;
				WorkerState = ERiseWorkerState::Idle;
			}
		}
		break;
	}

	case ERiseWorkerState::MovingToBuild:
	{
		if (!BuildingSite)
		{
			WorkerState = ERiseWorkerState::Idle;
			return;
		}

		MoveToward(BuildingSite->GetActorLocation(), DeltaTime, 500.0f);
		if (FVector::Dist2D(GetActorLocation(), BuildingSite->GetActorLocation()) <= 560.0f)
		{
			WorkerState = ERiseWorkerState::Building;
		}
		break;
	}

	case ERiseWorkerState::Building:
	{
		if (!BuildingSite)
		{
			WorkerState = ERiseWorkerState::Idle;
			return;
		}

		if (FVector::Dist2D(GetActorLocation(), BuildingSite->GetActorLocation()) > 700.0f)
		{
			WorkerState = ERiseWorkerState::MovingToBuild;
			break;
		}

		static constexpr float WorkerBuildRate = 5.0f;
		if (BuildingSite->AddBuildProgress(WorkerBuildRate * DeltaTime))
		{
			BuildTarget = nullptr;
			WorkerState = ERiseWorkerState::Idle;
		}
		break;
	}

	default:
		break;
	}
}

void ARiseUnit::SelectEntity_Implementation()
{
	if (SelectionRing)
	{
		SelectionRing->SetVisibility(true);
	}
}

void ARiseUnit::DeselectEntity_Implementation()
{
	if (SelectionRing)
	{
		SelectionRing->SetVisibility(false);
	}
}

bool ARiseUnit::IsAlive() const
{
	return CurrentHealth > 0.0f;
}

float ARiseUnit::GetHealth() const
{
	return CurrentHealth;
}

float ARiseUnit::GetHealthPercent() const
{
	return Stats.MaxHealth > 0.0f ? CurrentHealth / Stats.MaxHealth : 0.0f;
}

float ARiseUnit::GetHealthBarHeight() const
{
	return Stats.HealthBarHeight;
}

void ARiseUnit::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}