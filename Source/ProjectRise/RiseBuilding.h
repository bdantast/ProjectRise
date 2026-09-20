#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RiseEconomyManager.h"
#include "RiseSelectableInterface.h"
#include "RiseBuilding.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class UStaticMesh;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class ARiseResourceNode;

UENUM(BlueprintType)
enum class ERiseBuildingType : uint8
{
	Extractor UMETA(DisplayName = "Extractor"),
	Refinery  UMETA(DisplayName = "Refinery")
};

UENUM(BlueprintType)
enum class ERiseConstructionState : uint8
{
	UnderConstruction UMETA(DisplayName = "UnderConstruction"),
	Active             UMETA(DisplayName = "Active")
};

USTRUCT(BlueprintType)
struct FRiseBuildingStats
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rise|Building")
	FRiseResourceContainer BuildCost;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rise|Building")
	float BuildTime = 40.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rise|Building")
	float MaxHealth = 600.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rise|Building")
	float ExtractRate = 6.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rise|Building")
	float PlacementRadius = 1200.0f;
};

FRiseBuildingStats GetBuildingStats(ERiseBuildingType Type);
FString RiseBuildingTypeToString(ERiseBuildingType Type);

UCLASS()
class PROJECTRISE_API ARiseBuilding : public AActor, public IRiseSelectableInterface
{
	GENERATED_BODY()

public:
	ARiseBuilding();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	UFUNCTION(BlueprintCallable, Category = "Rise|Building")
	void ConfigureBuilding(ERiseBuildingType InType, int32 InFaction);

	// Workers chamam enquanto estiverem no raio de construcao. Retorna true quando concluir.
	bool AddBuildProgress(float Amount);

	bool IsActive() const { return ConstructionState == ERiseConstructionState::Active; }
	bool IsAlive() const { return CurrentHealth > 0.0f; }
	int32 GetFactionID() const { return FactionID; }
	ERiseBuildingType GetBuildingType() const { return BuildingType; }
	ERiseConstructionState GetConstructionState() const { return ConstructionState; }
	float GetBuildProgressPercent() const { return BuildTime > 0.0f ? FMath::Clamp(BuildProgress / BuildTime, 0.0f, 1.0f) : 0.0f; }
	float GetHealthPercent() const { return MaxHealth > 0.0f ? CurrentHealth / MaxHealth : 0.0f; }
	float GetExtractRate() const { return ExtractRate; }
	ERiseResourceType GetExtractedResource() const { return ExtractedResource; }
	TWeakObjectPtr<ARiseResourceNode> GetResourceNode() const;

	UFUNCTION(BlueprintCallable, Category = "Rise|Building")
	void AssignResourceNode(ARiseResourceNode* Node);

	// IRiseSelectableInterface
	virtual void SelectEntity_Implementation() override;
	virtual void DeselectEntity_Implementation() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Rise|Building")
	USceneComponent* SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Rise|Building")
	UStaticMeshComponent* BuildingMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Rise|Building")
	UStaticMeshComponent* SelectionRing;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Rise|Building")
	UStaticMeshComponent* BasePad;

private:
	void ApplyConstructionVisual();
	void ApplyActiveVisual();
	void ApplyTeamColor();
	void FinalizeConstruction();
	ARiseResourceNode* FindNearestResourceNode() const;

	UPROPERTY(EditAnywhere, Category = "Rise|Building")
	ERiseBuildingType BuildingType = ERiseBuildingType::Extractor;

	UPROPERTY()
	ERiseConstructionState ConstructionState = ERiseConstructionState::UnderConstruction;

	UPROPERTY(EditAnywhere, Category = "Rise|Building")
	float MaxHealth = 600.0f;

	UPROPERTY(EditAnywhere, Category = "Rise|Building")
	float BuildTime = 40.0f;

	UPROPERTY(EditAnywhere, Category = "Rise|Building")
	float ExtractRate = 6.0f;

	UPROPERTY()
	TObjectPtr<UStaticMesh> ExtractorMeshAsset;

	UPROPERTY()
	TObjectPtr<UStaticMesh> RefineryMeshAsset;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> GhostMaterialAsset;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> ConstructionMaterial;

	float CurrentHealth = 600.0f;
	float BuildProgress = 0.0f;
	int32 FactionID = 0;
	ERiseResourceType ExtractedResource = ERiseResourceType::Food;
	float DepositTimer = 0.0f;
	float ConvertTimer = 0.0f;
	TWeakObjectPtr<ARiseResourceNode> ResourceNode;
};