#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RiseEconomyManager.h"
#include "RiseBuilding.h"
#include "RiseRTSPlayerController.h"
#include "RiseResourceHUD.generated.h"

class UVerticalBox;
class UHorizontalBox;
class UTextBlock;
class UBorder;
class UProgressBar;
class UCanvasPanel;
class UButton;
class UWrapBox;

UCLASS()
class PROJECTRISE_API URiseResourceHUD : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	UFUNCTION()
	void OnResourcesChangedHandler(const FRiseResourceContainer& UpdatedResources);

	void Refresh(const FRiseResourceContainer& Resources);

	void BuildResourcePanel(UCanvasPanel* Canvas);
	void BuildCommandPanel(UCanvasPanel* Canvas);
	void BuildMinimapPanel(UCanvasPanel* Canvas);

	void RefreshSelection();
	void RefreshMinimap();
	FString BuildSelectionSignature(const TArray<AActor*>& Actors);

	UButton* MakeActionButton(const FString& Label, const FLinearColor& Color);

	// Botoes do cartao de comando
	UFUNCTION() void OnQueueInfantry();
	UFUNCTION() void OnQueueTank();
	UFUNCTION() void OnQueueDrone();
	UFUNCTION() void OnQueueWorker();
	UFUNCTION() void OnQueueHummer();
	UFUNCTION() void OnQueueHelicopter();
	UFUNCTION() void OnSelectBuildExtractor();
	UFUNCTION() void OnSelectBuildRefinery();
	UFUNCTION() void OnMoveClicked();
	UFUNCTION() void OnStopClicked();
	UFUNCTION() void OnWorkersBuildClicked();

	UPROPERTY()
	TObjectPtr<UVerticalBox> ResourceBox;

	UPROPERTY()
	TArray<TObjectPtr<UTextBlock>> ResourceValueTexts;

	// Cartao de comando (inferior esquerdo)
	UPROPERTY()
	TObjectPtr<UBorder> PanelBorder;

	UPROPERTY()
	TObjectPtr<UBorder> PanelIcon;

	UPROPERTY()
	TObjectPtr<UTextBlock> PanelTitleText;

	UPROPERTY()
	TObjectPtr<UProgressBar> PanelHealthBar;

	UPROPERTY()
	TObjectPtr<UTextBlock> PanelHealthText;

	UPROPERTY()
	TObjectPtr<UTextBlock> PanelDetailText;

	UPROPERTY()
	TObjectPtr<UWrapBox> ActionRow;

	// Minimapa (inferior direito)
	UPROPERTY()
	TObjectPtr<UBorder> MinimapBorder;

	UPROPERTY()
	TObjectPtr<UCanvasPanel> MinimapCanvas;

	FVector2D MinimapTopLeft = FVector2D(-12000.0f, -12000.0f);
	FVector2D MinimapWorldSize = FVector2D(24000.0f, 24000.0f);

	FString SelectionSignature;
	float HudRefreshTimer = 0.0f;
	float MinimapTimer = 0.0f;
};