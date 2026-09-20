#include "RiseResourceNode.h"
#include "UObject/ConstructorHelpers.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"

FString RiseResourceTypeToString(ERiseResourceType Type)
{
	switch (Type)
	{
	case ERiseResourceType::Food:       return TEXT("Comida");
	case ERiseResourceType::MicroChips: return TEXT("MicroChips");
	case ERiseResourceType::RareMetal:  return TEXT("Metal Raro");
	case ERiseResourceType::Petrol:     return TEXT("Petroleo");
	case ERiseResourceType::Dollars:    return TEXT("Dolares");
	case ERiseResourceType::Knowledge:  return TEXT("Conhecimento");
	}
	return TEXT("?");
}

ARiseResourceNode::ARiseResourceNode()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	NodeMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("NodeMesh"));
	NodeMesh->SetupAttachment(RootComponent);
	NodeMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMeshFinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CylinderMeshFinder.Succeeded())
	{
		CylinderMeshAsset = CylinderMeshFinder.Object;
		NodeMesh->SetStaticMesh(CylinderMeshFinder.Object);
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CrateFinder(TEXT("/Game/Meshes/SM_Crate.SM_Crate"));
	if (CrateFinder.Succeeded())
	{
		CrateMeshAsset = CrateFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> BarrelFinder(TEXT("/Game/Meshes/SM_Barrel.SM_Barrel"));
	if (BarrelFinder.Succeeded())
	{
		BarrelMeshAsset = BarrelFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CrystalFinder(TEXT("/Game/Meshes/SM_Crystal.SM_Crystal"));
	if (CrystalFinder.Succeeded())
	{
		CrystalMeshAsset = CrystalFinder.Object;
	}

	SelectionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("SelectionBox"));
	SelectionBox->SetupAttachment(RootComponent);
	SelectionBox->SetRelativeLocation(FVector(0.0f, 0.0f, 70.0f));
	SelectionBox->SetBoxExtent(FVector(90.0f, 90.0f, 70.0f));
	SelectionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SelectionBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	SelectionBox->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
}

void ARiseResourceNode::ConfigureNode(ERiseResourceType InType, float InAmount)
{
	ResourceType = InType;
	AmountRemaining = InAmount;
	InitialAmount = FMath::Max(InAmount, 1.0f);
	ApplyNodeMesh();
	ApplyResourceColor();
}

void ARiseResourceNode::ApplyNodeMesh()
{
	UStaticMesh* Mesh = CrateMeshAsset;
	FVector Extent(90.0f, 90.0f, 70.0f);

	switch (ResourceType)
	{
	case ERiseResourceType::Petrol:
		if (BarrelMeshAsset)
		{
			Mesh = BarrelMeshAsset;
		}
		Extent = FVector(75.0f, 75.0f, 85.0f);
		break;

	case ERiseResourceType::RareMetal:
	case ERiseResourceType::Knowledge:
		if (CrystalMeshAsset)
		{
			Mesh = CrystalMeshAsset;
		}
		Extent = FVector(100.0f, 100.0f, 105.0f);
		break;

	case ERiseResourceType::Food:
	case ERiseResourceType::MicroChips:
	case ERiseResourceType::Dollars:
	default:
		break;
	}

	if (Mesh)
	{
		NodeMesh->SetStaticMesh(Mesh);
		NodeMesh->SetRelativeLocation(FVector::ZeroVector);
		NodeMesh->SetRelativeScale3D(FVector(1.0f));
	}

	SelectionBox->SetBoxExtent(Extent);
	SelectionBox->SetRelativeLocation(FVector(0.0f, 0.0f, Extent.Z));
}

void ARiseResourceNode::ApplyResourceColor()
{
	if (!NodeMesh)
	{
		return;
	}

	FLinearColor NodeColor = FLinearColor::White;
	switch (ResourceType)
	{
	case ERiseResourceType::Food:       NodeColor = FLinearColor(0.2f, 0.9f, 0.3f); break;
	case ERiseResourceType::MicroChips: NodeColor = FLinearColor(0.1f, 0.9f, 0.9f); break;
	case ERiseResourceType::RareMetal:  NodeColor = FLinearColor(0.8f, 0.8f, 0.85f); break;
	case ERiseResourceType::Petrol:     NodeColor = FLinearColor(0.6f, 0.3f, 0.1f); break;
	case ERiseResourceType::Dollars:    NodeColor = FLinearColor(1.0f, 0.85f, 0.2f); break;
	case ERiseResourceType::Knowledge:  NodeColor = FLinearColor(0.7f, 0.3f, 1.0f); break;
	}

	if (UMaterialInstanceDynamic* Dyn = NodeMesh->CreateAndSetMaterialInstanceDynamic(0))
	{
		Dyn->SetVectorParameterValue(TEXT("Color"), NodeColor);
	}
}

float ARiseResourceNode::Extract(float Amount)
{
	if (AmountRemaining <= 0.0f || Amount <= 0.0f)
	{
		return 0.0f;
	}

	const float Extracted = FMath::Min(Amount, AmountRemaining);
	AmountRemaining -= Extracted;

	if (IsDepleted())
	{
		if (NodeMesh)
		{
			NodeMesh->SetVisibility(false);
		}
		if (SelectionBox)
		{
			SelectionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}

	return Extracted;
}