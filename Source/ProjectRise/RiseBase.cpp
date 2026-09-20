#include "RiseBase.h"
#include "RiseProjectile.h"
#include "UObject/ConstructorHelpers.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "EngineUtils.h"
#include "Engine/DamageEvents.h"
#include "Engine/Engine.h"

ARiseBase::ARiseBase()
{
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	BaseMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BaseMesh"));
	BaseMesh->SetupAttachment(RootComponent);
	BaseMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMeshFinder.Succeeded())
	{
		CubeMeshAsset = CubeMeshFinder.Object;
		BaseMesh->SetStaticMesh(CubeMeshFinder.Object);
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> BaseFinder(TEXT("/Game/Meshes/SM_Base.SM_Base"));
	if (BaseFinder.Succeeded())
	{
		BaseMeshAsset = BaseFinder.Object;
	}

	SelectionRing = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SelectionRing"));
	SelectionRing->SetupAttachment(RootComponent);
	SelectionRing->SetRelativeLocation(FVector(0.0f, 0.0f, 5.0f));
	SelectionRing->SetRelativeScale3D(FVector(3.5f, 3.5f, 0.05f));
	SelectionRing->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SelectionRing->SetVisibility(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMeshFinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CylinderMeshFinder.Succeeded())
	{
		SelectionRing->SetStaticMesh(CylinderMeshFinder.Object);
	}

	SelectionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("SelectionBox"));
	SelectionBox->SetupAttachment(RootComponent);
	SelectionBox->SetRelativeLocation(FVector(0.0f, 0.0f, 150.0f));
	SelectionBox->SetBoxExtent(FVector(250.0f, 250.0f, 150.0f));
	SelectionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SelectionBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	SelectionBox->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	CurrentHealth = MaxHealth;
}

void ARiseBase::ApplyBuiltInMesh()
{
	if (BaseMeshAsset)
	{
		BaseMesh->SetStaticMesh(BaseMeshAsset);
		BaseMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 0.0f));
		BaseMesh->SetRelativeScale3D(FVector(1.0f));
		SelectionRing->SetRelativeScale3D(FVector(11.0f, 11.0f, 0.05f));
		SelectionBox->SetBoxExtent(FVector(430.0f, 370.0f, 190.0f));
		SelectionBox->SetRelativeLocation(FVector(0.0f, 0.0f, 190.0f));
	}
	else if (CubeMeshAsset)
	{
		BaseMesh->SetStaticMesh(CubeMeshAsset);
		BaseMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 75.0f));
		BaseMesh->SetRelativeScale3D(FVector(2.0f, 2.0f, 1.5f));
		SelectionRing->SetRelativeScale3D(FVector(3.5f, 3.5f, 0.05f));
		SelectionBox->SetBoxExtent(FVector(250.0f, 250.0f, 150.0f));
		SelectionBox->SetRelativeLocation(FVector(0.0f, 0.0f, 150.0f));
	}
}

void ARiseBase::BeginPlay()
{
	Super::BeginPlay();
}

void ARiseBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!IsAlive())
	{
		return;
	}

	AttackTimer = FMath::Max(0.0f, AttackTimer - DeltaTime);

	AcquireAndAttack(DeltaTime);
	UpdateProduction(DeltaTime);
}

float ARiseBase::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (!IsAlive())
	{
		return 0.0f;
	}

	CurrentHealth = FMath::Max(0.0f, CurrentHealth - DamageAmount);

	if (CurrentHealth <= 0.0f)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Base] Base da faccao %d destruida"), FactionID);
		if (GEngine)
		{
			if (FactionID == 0)
			{
				GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Red, TEXT("DERROTA! SUA BASE FOI DESTRUIDA"));
				UE_LOG(LogTemp, Warning, TEXT("[Mundo] FIM DE JOGO: DERROTA (base do jogador destruida)"));
			}
			else
			{
				GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Green, TEXT("VITORIA! Base inimiga destruida"));
				UE_LOG(LogTemp, Warning, TEXT("[Mundo] FIM DE JOGO: VITORIA (base inimiga destruida)"));
			}
		}
		Destroy();
	}

	return DamageAmount;
}

void ARiseBase::ConfigureBase(int32 InFaction)
{
	FactionID = InFaction;
	CurrentHealth = MaxHealth;
	ApplyBuiltInMesh();
	ApplyTeamColor();
}

void ARiseBase::ApplyTeamColor()
{
	if (!BaseMesh)
	{
		return;
	}

	const FLinearColor TeamColor = (FactionID == 0) ? FLinearColor(0.15f, 0.55f, 1.0f) : FLinearColor(1.0f, 0.25f, 0.25f);
	if (UMaterialInstanceDynamic* Dyn = BaseMesh->CreateAndSetMaterialInstanceDynamic(0))
	{
		Dyn->SetVectorParameterValue(TEXT("Color"), TeamColor);
	}
}

bool ARiseBase::QueueBuildUnit(ERiseUnitType Type)
{
	const FRiseUnitStats UnitStats = GetUnitStats(Type);

	// Custo sai do saldo da faccao dona da base (jogador = GameInstance, IA = pool proprio)
	if (!RiseFactionConsumeResources(this, FactionID, UnitStats.BuildCost))
	{
		return false;
	}

	FRiseProductionJob Job;
	Job.UnitType = Type;
	Job.TimeRemaining = UnitStats.BuildTime;
	ProductionQueue.Add(Job);

	return true;
}

FString ARiseBase::GetQueueSummary() const
{
	if (ProductionQueue.Num() == 0)
	{
		return TEXT("vazia");
	}

	FString Out;
	for (int32 i = 0; i < ProductionQueue.Num(); ++i)
	{
		if (i > 0)
		{
			Out += TEXT(" | ");
		}
		Out += FString::Printf(TEXT("%s %.0fs"), *RiseUnitTypeToString(ProductionQueue[i].UnitType), FMath::Max(0.0f, ProductionQueue[i].TimeRemaining));
	}
	return Out;
}

void ARiseBase::UpdateProduction(float DeltaTime)
{
	if (ProductionQueue.Num() == 0)
	{
		return;
	}

	for (int32 i = ProductionQueue.Num() - 1; i >= 0; --i)
	{
		ProductionQueue[i].TimeRemaining -= DeltaTime;
		if (ProductionQueue[i].TimeRemaining <= 0.0f)
		{
			SpawnProducedUnit(ProductionQueue[i].UnitType);
			ProductionQueue.RemoveAt(i);
		}
	}
}

void ARiseBase::SpawnProducedUnit(ERiseUnitType Type)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const float Angle = FMath::FRandRange(0.0f, 2.0f * PI);
	const FVector SpawnLocation = GetActorLocation() + FVector(FMath::Cos(Angle) * 720.0f, FMath::Sin(Angle) * 720.0f, 0.0f);

	ARiseUnit* Unit = World->SpawnActor<ARiseUnit>(ARiseUnit::StaticClass(), SpawnLocation + FVector(0.0f, 0.0f, 800.0f), FRotator::ZeroRotator);
	if (Unit)
	{
		Unit->ConfigureUnit(Type, FactionID);
		Unit->SetActorLocation(SpawnLocation);

		if (FactionID == 0 && GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Cyan, TEXT("Unidade pronta!"));
		}
	}
}

void ARiseBase::AcquireAndAttack(float DeltaTime)
{
	AActor* BestTarget = nullptr;
	float BestDistSq = AttackRange * AttackRange;

	if (UWorld* World = GetWorld())
	{
		const FVector MyLocation = GetActorLocation();

		for (TActorIterator<ARiseUnit> UnitIt(World); UnitIt; ++UnitIt)
		{
			ARiseUnit* Other = *UnitIt;
			if (!Other->IsAlive() || Other->GetFactionID() == FactionID)
			{
				continue;
			}

			const float DistSq = FVector::DistSquared(MyLocation, Other->GetActorLocation());
			if (DistSq < BestDistSq)
			{
				BestDistSq = DistSq;
				BestTarget = Other;
			}
		}

		for (TActorIterator<ARiseBase> BaseIt(World); BaseIt; ++BaseIt)
		{
			ARiseBase* OtherBase = *BaseIt;
			if (OtherBase == this || !OtherBase->IsAlive() || OtherBase->GetFactionID() == FactionID)
			{
				continue;
			}

			const float DistSq = FVector::DistSquared(MyLocation, OtherBase->GetActorLocation());
			if (DistSq < BestDistSq)
			{
				BestDistSq = DistSq;
				BestTarget = OtherBase;
			}
		}
	}

	if (BestTarget && AttackTimer <= 0.0f)
	{
		FireProjectile(BestTarget);
		AttackTimer = AttackCooldown;
	}
}

void ARiseBase::FireProjectile(AActor* Target)
{
	UWorld* World = GetWorld();
	if (!World || !Target)
	{
		return;
	}

	const FVector AimDirection = (Target->GetActorLocation() - GetActorLocation()).GetSafeNormal();
	const FRotator AimRotation = AimDirection.Rotation();
	const FVector MuzzleLocation = GetActorLocation() + FVector(0.0f, 0.0f, 300.0f);

	FActorSpawnParameters Params;
	Params.Owner = this;
	Params.Instigator = nullptr;

	if (ARiseProjectile* Projectile = World->SpawnActor<ARiseProjectile>(ARiseProjectile::StaticClass(), MuzzleLocation, AimRotation, Params))
	{
		Projectile->Initialize(Target, AttackDamage, FactionID);
	}
}

void ARiseBase::SelectEntity_Implementation()
{
	if (SelectionRing)
	{
		SelectionRing->SetVisibility(true);
	}
}

void ARiseBase::DeselectEntity_Implementation()
{
	if (SelectionRing)
	{
		SelectionRing->SetVisibility(false);
	}
}