#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "RiseUnit.h"
#include "RiseRTSPlayerController.generated.h"

class ARiseBase;
class ARiseResourceNode;
class ARiseBuilding;
enum class ERiseBuildingType : uint8;

UENUM(BlueprintType)
enum class ERiseBuildMode : uint8
{
	None     UMETA(DisplayName = "None"),
	Extractor UMETA(DisplayName = "Extractor"),
	Refinery  UMETA(DisplayName = "Refinery")
};

UCLASS()
class PROJECTRISE_API ARiseRTSPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupInputComponent() override;

	int32 GetSelectionCount() const { return SelectedActors.Num(); }
	ARiseBase* GetSelectedBase() const;
	const TArray<TWeakObjectPtr<AActor>>& GetSelectedActors() const { return SelectedActors; }

	// ---- Chamadas do HUD RTS (botoes do menu) ----
	UFUNCTION(BlueprintCallable, Category = "Rise|HUD")
	void HUDQueueUnit(ERiseUnitType Type);

	UFUNCTION(BlueprintCallable, Category = "Rise|HUD")
	void HUDSelectBuild(ERiseBuildMode Mode);

	UFUNCTION(BlueprintCallable, Category = "Rise|HUD")
	void HUDIssueMove();

	UFUNCTION(BlueprintCallable, Category = "Rise|HUD")
	void HUDStopSelected();

	UFUNCTION(BlueprintCallable, Category = "Rise|HUD")
	void HUDWorkersBuild();

protected:
	UFUNCTION()
	void OnLeftMousePressed();

	UFUNCTION()
	void OnLeftMouseReleased();

	UFUNCTION()
	void OnRightMousePressed();

	UFUNCTION()
	void OnBuildInfantry();

	UFUNCTION()
	void OnBuildTank();

	UFUNCTION()
	void OnBuildDrone();

	UFUNCTION()
	void OnBuildWorker();

	UFUNCTION()
	void OnBuildHummer();

	UFUNCTION()
	void OnBuildHelicopter();

	UFUNCTION()
	void OnBuildMode();

	UFUNCTION()
	void OnBuildExtractor();

	UFUNCTION()
	void OnBuildRefinery();

	UFUNCTION()
	void OnCancelBuild();

private:
	void StartSelectionDrag();
	void EndSelectionDrag();
	void SelectInRectangle(const FVector2D& Start, const FVector2D& End);
	void SelectActorUnderCursor();
	void IssueMoveOrder(const FVector& Destination);
	void IssueHarvestOrder(ARiseResourceNode* Node);
	void IssueBuildOrder(ARiseBuilding* Building);
	void IssueContextOrder(const FHitResult& Hit);
	ARiseBase* FindPlayerBase() const;
	void ClearSelection();
	void AddToSelection(AActor* Actor);
	void QueueProduction(ERiseUnitType Type);
	bool IsBuildModeActive() const { return BuildMode != ERiseBuildMode::None; }
	void EnterBuildMode(ERiseBuildMode Mode);
	void ExitBuildMode();
	void UpdateBuildGhostIfNeeded();
	void PlaceBuildAtCursor();
	bool GetFloorPointUnderCursor(FVector& Out) const;
	bool IsValidBuildLocation(const FVector& Location) const;
	void ShowBuildModeHint();

	ERiseBuildingType GetBuildTypeForMode() const;

	UPROPERTY()
	TArray<TWeakObjectPtr<AActor>> SelectedActors;

	UPROPERTY(EditAnywhere, Category = "Rise|Selection")
	float SelectionSpacing = 150.0f;

	bool bIsSelectingDrag = false;
	FVector2D DragStartScreen = FVector2D::ZeroVector;

	ERiseBuildMode BuildMode = ERiseBuildMode::None;
	TWeakObjectPtr<AActor> BuildGhost;
	bool bBuildPlacementPending = false;
	bool bPendingContextOrder = false;
};