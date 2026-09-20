#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "RiseGameMode.generated.h"

enum class ERiseUnitType : uint8;

class ARiseBase;
class URiseResourceHUD;

enum class ERiseResourceType : uint8;

UCLASS()
class PROJECTRISE_API ARiseGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ARiseGameMode();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rise|Territory")
	int32 TerritoryGridWidth = 16;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rise|Territory")
	int32 TerritoryGridHeight = 16;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rise|Territory")
	float TerritoryCellSize = 2000.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rise|UI")
	TSubclassOf<URiseResourceHUD> ResourceHUDClass;

	// Fase 0 (visual): albedo do chao em espaco linear. O concept pede cinza-esverdeado frio e escuro.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rise|Visual|Terreno")
	FLinearColor TerrainColor = FLinearColor(0.058f, 0.070f, 0.060f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rise|Visual|Terreno")
	float TerrainRoughness = 0.92f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rise|Visual|Terreno")
	FLinearColor RoadColor = FLinearColor(0.030f, 0.033f, 0.038f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rise|Visual|Terreno")
	float RoadRoughness = 0.78f;

protected:
	virtual void BeginPlay() override;

private:
	void SpawnUnitAt(ERiseUnitType Type, const FVector& Location, int32 Faction);
	void SpawnBaseAt(const FVector& Location, int32 Faction);
	void SpawnResourceNodeAt(ERiseResourceType Type, const FVector& Location, float Amount);

	void SpawnGroundCollider();
	void SpawnTerrain();
	void RemoveDefaultLevelGeometry();
	void SpawnBaseComplex(const FVector& BaseLocation, const FLinearColor& Tint, float Sign);
	void SpawnVillage(const FVector& Center, int32 HouseCount);
	void ScatterDecor();

	AActor* SpawnProp(UStaticMesh* Mesh, const FVector& Location, float Yaw, const FVector& Scale, bool bCastShadow = true);
	AActor* SpawnBuilding(UStaticMesh* Mesh, const FVector& Location, float Yaw, const FLinearColor& Tint);

	UPROPERTY()
	TObjectPtr<UStaticMesh> FloorMeshAsset;

	UPROPERTY()
	TObjectPtr<UStaticMesh> TerrainMeshAsset;

	UPROPERTY()
	TObjectPtr<UStaticMesh> RoadMeshAsset;

	UPROPERTY()
	TArray<TObjectPtr<UStaticMesh>> TreeMeshAssets;

	UPROPERTY()
	TArray<TObjectPtr<UStaticMesh>> RockMeshAssets;

	UPROPERTY()
	TArray<TObjectPtr<UStaticMesh>> BushMeshAssets;

	UPROPERTY()
	TObjectPtr<UStaticMesh> CommandCenterMeshAsset;

	UPROPERTY()
	TObjectPtr<UStaticMesh> BarracksMeshAsset;

	UPROPERTY()
	TObjectPtr<UStaticMesh> FactoryMeshAsset;

	UPROPERTY()
	TObjectPtr<UStaticMesh> TowerMeshAsset;

	UPROPERTY()
	TObjectPtr<UStaticMesh> GeneratorMeshAsset;

	UPROPERTY()
	TObjectPtr<UStaticMesh> WallMeshAsset;

	UPROPERTY()
	TObjectPtr<UStaticMesh> SandbagsMeshAsset;

	UPROPERTY()
	TObjectPtr<UStaticMesh> HouseMeshAsset;

	// Bases, nos e vila: usado para nao espalhar decoracao em cima deles
	TArray<FVector> OccupiedLocations;
};
