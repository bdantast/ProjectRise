#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RiseUnit.h"
#include "RiseSelectableInterface.h"
#include "RiseBase.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class UBoxComponent;
class UStaticMesh;

USTRUCT(BlueprintType)
struct FRiseProductionJob
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rise|Production")
	ERiseUnitType UnitType = ERiseUnitType::Infantry;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rise|Production")
	float TimeRemaining = 0.0f;
};

UCLASS()
class PROJECTRISE_API ARiseBase : public AActor, public IRiseSelectableInterface
{
	GENERATED_BODY()

public:
	ARiseBase();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	UFUNCTION(BlueprintCallable, Category = "Rise|Base")
	void ConfigureBase(int32 InFaction);

	UFUNCTION(BlueprintCallable, Category = "Rise|Base")
	bool QueueBuildUnit(ERiseUnitType Type);

	bool IsAlive() const { return CurrentHealth > 0.0f; }
	int32 GetFactionID() const { return FactionID; }
	int32 GetQueueCount() const { return ProductionQueue.Num(); }
	FString GetQueueSummary() const;
	float GetHealthPercent() const { return MaxHealth > 0.0f ? CurrentHealth / MaxHealth : 0.0f; }

	// IRiseSelectableInterface
	virtual void SelectEntity_Implementation() override;
	virtual void DeselectEntity_Implementation() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Rise|Base")
	USceneComponent* SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Rise|Base")
	UStaticMeshComponent* BaseMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Rise|Base")
	UStaticMeshComponent* SelectionRing;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Rise|Base")
	UBoxComponent* SelectionBox;

private:
	void AcquireAndAttack(float DeltaTime);
	void FireProjectile(AActor* Target);
	void UpdateProduction(float DeltaTime);
	void SpawnProducedUnit(ERiseUnitType Type);
	void ApplyTeamColor();
	void ApplyBuiltInMesh();

	UPROPERTY(EditAnywhere, Category = "Rise|Base")
	float MaxHealth = 2500.0f;

	UPROPERTY(EditAnywhere, Category = "Rise|Base")
	float AttackDamage = 30.0f;

	UPROPERTY(EditAnywhere, Category = "Rise|Base")
	float AttackRange = 1000.0f;

	UPROPERTY(EditAnywhere, Category = "Rise|Base")
	float AttackCooldown = 1.5f;

	UPROPERTY()
	TArray<FRiseProductionJob> ProductionQueue;

	UPROPERTY()
	TObjectPtr<UStaticMesh> BaseMeshAsset;

	UPROPERTY()
	TObjectPtr<UStaticMesh> CubeMeshAsset;

	float CurrentHealth = 2500.0f;
	float AttackTimer = 0.0f;
	int32 FactionID = 0;
};