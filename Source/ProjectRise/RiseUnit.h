#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "RiseEconomyManager.h"
#include "RiseSelectableInterface.h"
#include "RiseUnit.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class UBoxComponent;
class UStaticMesh;
class ARiseResourceNode;
class ARiseBase;
class ARiseBuilding;

UENUM(BlueprintType)
enum class ERiseUnitType : uint8
{
	Infantry UMETA(DisplayName = "Infantry"),
	Tank     UMETA(DisplayName = "Tank"),
	Drone    UMETA(DisplayName = "Drone"),
	Worker   UMETA(DisplayName = "Worker"),
	Hummer   UMETA(DisplayName = "Hummer"),
	Helicopter UMETA(DisplayName = "Helicopter")
};

UENUM(BlueprintType)
enum class ERiseWorkerState : uint8
{
	Idle          UMETA(DisplayName = "Idle"),
	MovingToNode  UMETA(DisplayName = "MovingToNode"),
	Harvesting    UMETA(DisplayName = "Harvesting"),
	Returning     UMETA(DisplayName = "Returning"),
	MovingToBuild UMETA(DisplayName = "MovingToBuild"),
	Building      UMETA(DisplayName = "Building")
};

USTRUCT(BlueprintType)
struct FRiseUnitStats
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rise|Stats")
	float MaxHealth = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rise|Stats")
	float Speed = 800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rise|Stats")
	float AttackDamage = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rise|Stats")
	float AttackRange = 600.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rise|Stats")
	float AttackCooldown = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rise|Stats")
	float BuildTime = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rise|Stats")
	float HealthBarHeight = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rise|Stats")
	float HarvestRate = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rise|Stats")
	float CarryCapacity = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rise|Stats")
	FRiseResourceContainer BuildCost;
};

FRiseUnitStats GetUnitStats(ERiseUnitType Type);
FString RiseUnitTypeToString(ERiseUnitType Type);

UCLASS()
class PROJECTRISE_API ARiseUnit : public APawn, public IRiseSelectableInterface
{
	GENERATED_BODY()

public:
	ARiseUnit();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	UFUNCTION(BlueprintCallable, Category = "Rise|Unit")
	void ConfigureUnit(ERiseUnitType InType, int32 InFaction);

	UFUNCTION(BlueprintCallable, Category = "Rise|Unit")
	void MoveTo(const FVector& Destination);

	// Interrompe todas as ordens atuais (mover, atacar, coletar, construir)
	UFUNCTION(BlueprintCallable, Category = "Rise|Unit")
	void Stop();

	// Ordem explicita de ataque (usada pela IA e por futuros comandos do jogador).
	// A unidade passa a perseguir/engajar o alvo mesmo fora do raio de aggro.
	UFUNCTION(BlueprintCallable, Category = "Rise|Unit")
	void OrderAttack(AActor* Target);

	// True se a unidade ja esta perseguindo/atacando algo
	UFUNCTION(BlueprintPure, Category = "Rise|Unit")
	bool HasEngagement() const;

	UFUNCTION(BlueprintPure, Category = "Rise|Unit")
	bool IsAlive() const;

	UFUNCTION(BlueprintPure, Category = "Rise|Unit")
	float GetHealth() const;

	UFUNCTION(BlueprintPure, Category = "Rise|Unit")
	float GetHealthPercent() const;

	float GetMaxHealth() const { return Stats.MaxHealth; }

	UFUNCTION(BlueprintPure, Category = "Rise|Unit")
	float GetHealthBarHeight() const;

	int32 GetFactionID() const { return FactionID; }
	ERiseUnitType GetUnitType() const { return UnitType; }
	bool IsWorker() const { return UnitType == ERiseUnitType::Worker; }

	// Economia: coletora
	UFUNCTION(BlueprintCallable, Category = "Rise|Unit")
	void AssignResourceNode(ARiseResourceNode* Node);

	// Construcao: coletora recebe ordem de construir um predio em obra
	UFUNCTION(BlueprintCallable, Category = "Rise|Unit")
	void OrderBuild(ARiseBuilding* Building);

	ERiseWorkerState GetWorkerState() const { return WorkerState; }
	float GetCarriedAmount() const { return CarriedAmount; }
	float GetCarryCapacity() const { return Stats.CarryCapacity; }
	ERiseResourceType GetCarriedResourceType() const { return CarriedResourceType; }

	// IRiseSelectableInterface
	virtual void SelectEntity_Implementation() override;
	virtual void DeselectEntity_Implementation() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Rise|Unit")
	USceneComponent* SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Rise|Unit")
	UStaticMeshComponent* UnitMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Rise|Unit")
	UStaticMeshComponent* TurretMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Rise|Unit")
	UStaticMeshComponent* SelectionRing;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Rise|Unit")
	UBoxComponent* SelectionBox;

protected:
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

private:
	void ApplyTypeVisuals();
	void ApplyTeamColor();
	void SetUnitVisual(UStaticMesh* Mesh, const FVector& RelativeLocation, const FVector& Scale, float RingScale, const FVector& BoxExtent, float BoxCenterZ);
	void MoveToward(const FVector& Goal, float DeltaTime, float AcceptanceRadius = 20.0f);
	TWeakObjectPtr<AActor> AcquireTarget(float Radius);
	void FireProjectile(AActor* Target);
	void TickWorker(float DeltaTime);
	ARiseBase* FindNearestBase() const;
	ARiseResourceNode* FindNearestResourceNode() const;
	ARiseBuilding* FindNearestConstructionSite() const;

	UPROPERTY(EditAnywhere, Category = "Rise|Unit")
	ERiseUnitType UnitType = ERiseUnitType::Infantry;

	UPROPERTY()
	FRiseUnitStats Stats;

	UPROPERTY()
	TObjectPtr<UStaticMesh> InfantryMeshAsset;

	UPROPERTY()
	TObjectPtr<UStaticMesh> TankMeshAsset;

	UPROPERTY()
	TObjectPtr<UStaticMesh> TankTurretMeshAsset;

	UPROPERTY()
	TObjectPtr<UStaticMesh> DroneMeshAsset;

	UPROPERTY()
	TObjectPtr<UStaticMesh> WorkerMeshAsset;

	UPROPERTY()
	TObjectPtr<UStaticMesh> HummerMeshAsset;

	UPROPERTY()
	TObjectPtr<UStaticMesh> HelicopterMeshAsset;

	UPROPERTY()
	TObjectPtr<UStaticMesh> CubeMeshAsset;

	float CurrentHealth = 100.0f;
	int32 FactionID = 0;
	float AttackTimer = 0.0f;
	bool bHasMoveOrder = false;
	FVector Destination = FVector::ZeroVector;
	TWeakObjectPtr<AActor> CurrentTarget;

	TWeakObjectPtr<ARiseResourceNode> ResourceNode;
	ERiseWorkerState WorkerState = ERiseWorkerState::Idle;
	ERiseResourceType CarriedResourceType = ERiseResourceType::Food;
	float CarriedAmount = 0.0f;
	float HarvestTimer = 0.0f;
	TWeakObjectPtr<ARiseBuilding> BuildTarget;
};