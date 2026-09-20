#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RiseEconomyManager.h"
#include "RiseResourceNode.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class UBoxComponent;
class UStaticMesh;

FString RiseResourceTypeToString(ERiseResourceType Type);

UCLASS()
class PROJECTRISE_API ARiseResourceNode : public AActor
{
	GENERATED_BODY()

public:
	ARiseResourceNode();

	UFUNCTION(BlueprintCallable, Category = "Rise|Resource")
	void ConfigureNode(ERiseResourceType InType, float InAmount);

	// Retira até Amount do nó e retorna o quanto foi realmente extraído
	UFUNCTION(BlueprintCallable, Category = "Rise|Resource")
	float Extract(float Amount);

	UFUNCTION(BlueprintPure, Category = "Rise|Resource")
	bool IsDepleted() const { return AmountRemaining <= 0.0f; }

	ERiseResourceType GetResourceType() const { return ResourceType; }
	float GetAmountRemaining() const { return AmountRemaining; }
	FString GetResourceName() const { return RiseResourceTypeToString(ResourceType); }

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Rise|Resource")
	USceneComponent* SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Rise|Resource")
	UStaticMeshComponent* NodeMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Rise|Resource")
	UBoxComponent* SelectionBox;

private:
	void ApplyResourceColor();
	void ApplyNodeMesh();

	UPROPERTY(EditAnywhere, Category = "Rise|Resource")
	ERiseResourceType ResourceType = ERiseResourceType::Food;

	UPROPERTY(EditAnywhere, Category = "Rise|Resource")
	float AmountRemaining = 1000.0f;

	float InitialAmount = 1000.0f;

	UPROPERTY()
	TObjectPtr<UStaticMesh> CrateMeshAsset;

	UPROPERTY()
	TObjectPtr<UStaticMesh> BarrelMeshAsset;

	UPROPERTY()
	TObjectPtr<UStaticMesh> CrystalMeshAsset;

	UPROPERTY()
	TObjectPtr<UStaticMesh> CylinderMeshAsset;
};