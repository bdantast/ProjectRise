#include "RiseHUD.h"
#include "RiseUnit.h"
#include "RiseBase.h"
#include "RiseResourceNode.h"
#include "RiseRTSPlayerController.h"
#include "RiseEconomyManager.h"
#include "EngineUtils.h"
#include "Engine/Engine.h"
#include "Engine/Canvas.h"

namespace
{
void DrawWorldLabel(AHUD* HUD, AActor* Actor, float HeightOffset, const FString& Text, const FLinearColor& Color, float Scale = 1.0f)
{
	APlayerController* PC = HUD->GetOwningPlayerController();
	FVector2D Screen;
	if (!PC || !PC->ProjectWorldLocationToScreen(Actor->GetActorLocation() + FVector(0.0f, 0.0f, HeightOffset), Screen))
	{
		return;
	}

	HUD->DrawText(Text, Color, Screen.X + 12.0f, Screen.Y - 6.0f, GEngine->GetSmallFont(), Scale);
}
}

void ARiseHUD::SetSelectionDrag(const FVector2D& Start, const FVector2D& Current, bool bActive)
{
	DragStart = Start;
	DragCurrent = Current;
	bIsDragActive = bActive;
}

namespace
{
	void DrawHealthBar(AHUD* HUD, AActor* Actor, float HeightOffset, float Percent, bool bPlayerFaction)
	{
		APlayerController* PC = HUD->GetOwningPlayerController();
		FVector2D Screen;
		if (!PC || !PC->ProjectWorldLocationToScreen(Actor->GetActorLocation() + FVector(0.0f, 0.0f, HeightOffset), Screen))
		{
			return;
		}

		const float BarWidth = 60.0f;
		const float BarHeight = 6.0f;
		const float X = Screen.X - BarWidth * 0.5f;
		const float Y = Screen.Y;

		const FLinearColor Background(0.0f, 0.0f, 0.0f, 0.6f);
		const FLinearColor BarColor = bPlayerFaction ? FLinearColor(0.15f, 1.0f, 0.4f) : FLinearColor(1.0f, 0.25f, 0.2f);

		HUD->DrawRect(Background, X, Y, BarWidth, BarHeight);
		HUD->DrawRect(BarColor, X + 1.0f, Y + 1.0f, (BarWidth - 2.0f) * FMath::Clamp(Percent, 0.0f, 1.0f), BarHeight - 2.0f);
	}
}

void ARiseHUD::DrawHUD()
{
	Super::DrawHUD();

	// Caixa de seleção por arrastar
	if (bIsDragActive)
	{
		const float X = FMath::Min(DragStart.X, DragCurrent.X);
		const float Y = FMath::Min(DragStart.Y, DragCurrent.Y);
		const float W = FMath::Abs(DragCurrent.X - DragStart.X);
		const float H = FMath::Abs(DragCurrent.Y - DragStart.Y);

		const FLinearColor FillColor(0.0f, 0.6f, 1.0f, 0.2f);
		const FLinearColor BorderColor(0.2f, 0.8f, 1.0f, 1.0f);

		DrawRect(FillColor, X, Y, W, H);
		DrawLine(X, Y, X + W, Y, BorderColor, 1.0f);
		DrawLine(X + W, Y, X + W, Y + H, BorderColor, 1.0f);
		DrawLine(X + W, Y + H, X, Y + H, BorderColor, 1.0f);
		DrawLine(X, Y + H, X, Y, BorderColor, 1.0f);
	}

	if (!GetWorld())
	{
		return;
	}

	// Barras de vida de unidades e bases
	for (TActorIterator<ARiseUnit> UnitIt(GetWorld()); UnitIt; ++UnitIt)
	{
		ARiseUnit* Unit = *UnitIt;
		if (Unit->IsAlive())
		{
			DrawHealthBar(this, Unit, Unit->GetHealthBarHeight(), Unit->GetHealthPercent(), Unit->GetFactionID() == 0);
		}
	}

	for (TActorIterator<ARiseBase> BaseIt(GetWorld()); BaseIt; ++BaseIt)
	{
		ARiseBase* Base = *BaseIt;
		if (Base->IsAlive())
		{
			DrawHealthBar(this, Base, 450.0f, Base->GetHealthPercent(), Base->GetFactionID() == 0);
		}
	}

	// Nós de recursos: nome e quantidade restante
	for (TActorIterator<ARiseResourceNode> NodeIt(GetWorld()); NodeIt; ++NodeIt)
	{
		ARiseResourceNode* Node = *NodeIt;
		if (!Node->IsDepleted())
		{
			const FString NodeText = FString::Printf(TEXT("%s: %d"), *Node->GetResourceName(), FMath::RoundToInt(Node->GetAmountRemaining()));
			DrawWorldLabel(this, Node, 220.0f, NodeText, FLinearColor(0.95f, 0.95f, 0.95f), 1.0f);
		}
	}

	// Coletoras: carga atual
	for (TActorIterator<ARiseUnit> WorkerIt(GetWorld()); WorkerIt; ++WorkerIt)
	{
		ARiseUnit* Worker = *WorkerIt;
		if (Worker->IsWorker() && Worker->GetCarriedAmount() > 0.0f)
		{
			const FString CargoText = FString::Printf(TEXT("%d/%d %s"), FMath::RoundToInt(Worker->GetCarriedAmount()), FMath::RoundToInt(Worker->GetCarryCapacity()), *RiseResourceTypeToString(Worker->GetCarriedResourceType()));
			DrawWorldLabel(this, Worker, 90.0f, CargoText, FLinearColor(0.95f, 0.85f, 0.2f), 1.0f);
		}
	}
}