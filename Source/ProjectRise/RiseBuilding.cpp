#include "RiseBuilding.h"
#include "RiseResourceNode.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"
#include "EngineUtils.h"
#include "Engine/Engine.h"

FRiseBuildingStats GetBuildingStats(ERiseBuildingType Type)
{
	FRiseBuildingStats Stats;

	switch (Type)
	{
	case ERiseBuildingType::Extractor:
		Stats.BuildCost.Food = 200.0f;
		Stats.BuildCost.MicroChips = 50.0f;
		Stats.BuildCost.RareMetal = 100.0f;
		Stats.BuildCost.Petrol = 80.0f;
		Stats.BuildCost.Dollars = 250.0f;
		Stats.BuildCost.Knowledge = 0.0f;
		Stats.BuildTime = 40.0f;
		Stats.MaxHealth = 600.0f;
		Stats.ExtractRate = 6.0f;
		Stats.PlacementRadius = 1200.0f;
		break;

	case ERiseBuildingType::Refinery:
		Stats.BuildCost.Food = 120.0f;
		Stats.BuildCost.MicroChips = 60.0f;
		Stats.BuildCost.RareMetal = 60.0f;
		Stats.BuildCost.Petrol = 40.0f;
		Stats.BuildCost.Dollars = 180.0f;
		Stats.BuildCost.Knowledge = 0.0f;
		Stats.BuildTime = 30.0f;
		Stats.MaxHealth = 500.0f;
		Stats.ExtractRate = 0.0f;
		Stats.PlacementRadius = 900.0f;
		break;
	}

	return Stats;
}

FString RiseBuildingTypeToString(ERiseBuildingType Type)
{
	switch (Type)
	{
	case ERiseBuildingType::Extractor: return TEXT("Extrator");
	case ERiseBuildingType::Refinery: return TEXT("Refinaria");
	}
	return TEXT("?");
}

ARiseBuilding::ARiseBuilding()
{
	PrimaryActorTick.bCanEverTick = true;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> GeneratorFinder(TEXT("/Game/Meshes/Eco/Eco_Building_Grid.Eco_Building_Grid"));
	ExtractorMeshAsset = GeneratorFinder.Object;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> RefineryFinder(TEXT("/Game/Meshes/Refinery/SM_RefineryTwistedTower.SM_RefineryTwistedTower"));
	RefineryMeshAsset = RefineryFinder.Object;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderFinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	TObjectPtr<UStaticMesh> RingMesh = CylinderFinder.Object;

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> GhostFinder(TEXT("/Game/Materials/M_Ghost.M_Ghost"));
	GhostMaterialAsset = GhostFinder.Object;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = SceneRoot;

	BuildingMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BuildingMesh"));
	BuildingMesh->SetupAttachment(SceneRoot);
	BuildingMesh->SetStaticMesh(ExtractorMeshAsset);
	BuildingMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	BuildingMesh->SetCollisionObjectType(ECC_WorldDynamic);
	BuildingMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	BuildingMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	SelectionRing = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SelectionRing"));
	SelectionRing->SetupAttachment(SceneRoot);
	SelectionRing->SetRelativeLocation(FVector(0.0f, 0.0f, 10.0f));
	SelectionRing->SetRelativeScale3D(FVector(12.0f, 12.0f, 0.02f));
	SelectionRing->SetStaticMesh(RingMesh);
	SelectionRing->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SelectionRing->SetVisibility(false);

	BasePad = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BasePad"));
	BasePad->SetupAttachment(SceneRoot);
	BasePad->SetRelativeLocation(FVector(0.0f, 0.0f, 5.0f));
	BasePad->SetRelativeScale3D(FVector(7.0f, 4.2f, 0.16f));
	BasePad->SetStaticMesh(RingMesh);
	BasePad->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ARiseBuilding::BeginPlay()
{
	Super::BeginPlay();
}

void ARiseBuilding::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Construcao avanca sozinha devagar (garantia de terminar mesmo sem coletoras livres)
	if (ConstructionState == ERiseConstructionState::UnderConstruction)
	{
		AddBuildProgress(2.0f * DeltaTime);
		return;
	}

	if (ConstructionState != ERiseConstructionState::Active || BuildingType == ERiseBuildingType::Extractor)
	{
		if (ConstructionState != ERiseConstructionState::Active)
		{
			return;
		}
	}

	if (BuildingType == ERiseBuildingType::Extractor)
	{
		DepositTimer += DeltaTime;
		if (DepositTimer >= 1.0f)
		{
			DepositTimer -= 1.0f;

			ARiseResourceNode* Node = ResourceNode.Get();
			if (Node && !Node->IsDepleted())
			{
				RiseDepositToFaction(this, FactionID, ExtractedResource, ExtractRate);
			}
		}
	}
	else if (BuildingType == ERiseBuildingType::Refinery)
	{
		ConvertTimer += DeltaTime;
		if (ConvertTimer >= 1.0f)
		{
			ConvertTimer -= 1.0f;

			FRiseResourceContainer Consumed;
			Consumed.RareMetal = 5.0f;
			if (RiseFactionConsumeResources(this, FactionID, Consumed))
			{
				RiseDepositToFaction(this, FactionID, ERiseResourceType::Dollars, 8.0f);
				RiseDepositToFaction(this, FactionID, ERiseResourceType::MicroChips, 2.0f);
			}
		}
	}
}

float ARiseBuilding::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (ConstructionState != ERiseConstructionState::Active || CurrentHealth <= 0.0f)
	{
		return 0.0f;
	}

	CurrentHealth = FMath::Max(0.0f, CurrentHealth - DamageAmount);
	if (CurrentHealth <= 0.0f)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red,
				FString::Printf(TEXT("%s destruido!"), *RiseBuildingTypeToString(BuildingType)));
		}
		Destroy();
	}

	return DamageAmount;
}

void ARiseBuilding::ConfigureBuilding(ERiseBuildingType InType, int32 InFaction)
{
	FactionID = InFaction;
	BuildingType = InType;

	const FRiseBuildingStats Stats = GetBuildingStats(InType);
	MaxHealth = Stats.MaxHealth;
	CurrentHealth = MaxHealth;
	BuildTime = Stats.BuildTime;
	ExtractRate = Stats.ExtractRate;

	BuildingMesh->SetStaticMesh(InType == ERiseBuildingType::Refinery ? RefineryMeshAsset : ExtractorMeshAsset);

	SetActorScale3D(FVector(1.0f));
	ApplyConstructionVisual();

	// Forca a base do mesh exatamente no ponto de construcao (pivot alinhado)
	{
		FVector BuildingOrigin;
		FVector BuildingExtent;
		BuildingMesh->GetLocalBounds(BuildingOrigin, BuildingExtent);
		BuildingMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -(BuildingOrigin.Z - BuildingExtent.Z)));

		// Fundacao acompanha a pegada do predio (cilindro basico: raio 50 u, altura 100 u)
		if (BasePad)
		{
			const FVector FootprintScale = FVector(
				BuildingExtent.X * 2.0f * 1.15f / 100.0f,
				BuildingExtent.Y * 2.0f * 1.15f / 100.0f,
				0.06f);
			const FVector PadScale = FVector::Max(FootprintScale, FVector(0.4f, 0.4f, 0.06f));
			BasePad->SetRelativeScale3D(PadScale);

			// Rente ao chao: centro a meia altura, afundando 0.5 u pra evitar z-fighting
			const float PadHalfHeight = 50.0f * PadScale.Z;
			BasePad->SetRelativeLocation(FVector(0.0f, 0.0f, PadHalfHeight - 0.5f));

			// Herda o material do predio (BasicShapeMaterial padrao nao carrega na build)
			if (BuildingMesh->GetStaticMesh() && BuildingMesh->GetStaticMesh()->GetMaterial(0))
			{
				BasePad->SetMaterial(0, BuildingMesh->GetStaticMesh()->GetMaterial(0));
			}
		}
	}

	if (InType == ERiseBuildingType::Extractor)
	{
		AssignResourceNode(FindNearestResourceNode());
	}
}

bool ARiseBuilding::AddBuildProgress(float Amount)
{
	if (ConstructionState != ERiseConstructionState::UnderConstruction)
	{
		return false;
	}

	BuildProgress += Amount;

	const float ProgressFraction = GetBuildProgressPercent();

	// Mantem o predio sempre do tamanho real; a opacidade/tonalidade cresce com o progresso
	if (ConstructionMaterial)
	{
		ConstructionMaterial->SetScalarParameterValue(TEXT("GhostOpacity"), 0.30f + 0.60f * ProgressFraction);
		const FLinearColor ProgressColor = FLinearColor::LerpUsingHSV(
			FLinearColor(1.0f, 0.55f, 0.10f), FLinearColor(0.20f, 1.0f, 0.45f), ProgressFraction);
		ConstructionMaterial->SetVectorParameterValue(TEXT("GhostColor"), ProgressColor);
	}

	if (BuildProgress >= BuildTime)
	{
		FinalizeConstruction();
		return true;
	}

	return false;
}

void ARiseBuilding::FinalizeConstruction()
{
	ConstructionState = ERiseConstructionState::Active;
	BuildProgress = BuildTime;
	SetActorScale3D(FVector(1.0f));
	ApplyActiveVisual();

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Green,
			FString::Printf(TEXT("%s pronto!"), *RiseBuildingTypeToString(BuildingType)));
	}
}

TWeakObjectPtr<ARiseResourceNode> ARiseBuilding::GetResourceNode() const
{
	return ResourceNode;
}

void ARiseBuilding::AssignResourceNode(ARiseResourceNode* Node)
{
	ResourceNode = Node;
	if (Node)
	{
		ExtractedResource = Node->GetResourceType();
	}
}

ARiseResourceNode* ARiseBuilding::FindNearestResourceNode() const
{
	ARiseResourceNode* BestNode = nullptr;
	float BestDistanceSq = FMath::Square(2000.0f);

	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	for (TActorIterator<ARiseResourceNode> NodeIt(World); NodeIt; ++NodeIt)
	{
		ARiseResourceNode* Node = *NodeIt;
		if (!IsValid(Node))
		{
			continue;
		}

		const float DistanceSq = FVector::DistSquared2D(GetActorLocation(), Node->GetActorLocation());
		if (DistanceSq < BestDistanceSq)
		{
			BestDistanceSq = DistanceSq;
			BestNode = Node;
		}
	}

	return BestNode;
}

void ARiseBuilding::ApplyConstructionVisual()
{
	if (!GhostMaterialAsset || !BuildingMesh)
	{
		return;
	}

	UMaterialInstanceDynamic* Dyn = UMaterialInstanceDynamic::Create(GhostMaterialAsset, this);
	if (Dyn)
	{
		Dyn->SetVectorParameterValue(TEXT("GhostColor"), FLinearColor(1.0f, 0.55f, 0.10f));
		Dyn->SetScalarParameterValue(TEXT("GhostOpacity"), 0.30f);
		const int32 NumMats = BuildingMesh->GetNumMaterials();
		for (int32 i = 0; i < NumMats; ++i)
		{
			BuildingMesh->SetMaterial(i, Dyn);
		}
		ConstructionMaterial = Dyn;
	}
}

void ARiseBuilding::ApplyActiveVisual()
{
	if (BuildingMesh && BuildingMesh->GetStaticMesh())
	{
		const int32 NumMats = BuildingMesh->GetNumMaterials();
		for (int32 i = 0; i < NumMats; ++i)
		{
			if (UMaterialInterface* Original = BuildingMesh->GetStaticMesh()->GetMaterial(i))
			{
				BuildingMesh->SetMaterial(i, Original);
			}
		}
	}
	ConstructionMaterial = nullptr;

	const FLinearColor TeamColor = (FactionID == 0) ? FLinearColor(0.15f, 0.55f, 1.0f) : FLinearColor(1.0f, 0.25f, 0.25f);
	if (UMaterialInstanceDynamic* Dyn = BuildingMesh->CreateAndSetMaterialInstanceDynamic(0))
	{
		Dyn->SetVectorParameterValue(TEXT("Color"), TeamColor);
	}
}

void ARiseBuilding::SelectEntity_Implementation()
{
	if (SelectionRing)
	{
		SelectionRing->SetVisibility(true);
	}
}

void ARiseBuilding::DeselectEntity_Implementation()
{
	if (SelectionRing)
	{
		SelectionRing->SetVisibility(false);
	}
}