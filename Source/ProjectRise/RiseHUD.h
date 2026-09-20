#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "RiseHUD.generated.h"

UCLASS()
class PROJECTRISE_API ARiseHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

	void SetSelectionDrag(const FVector2D& Start, const FVector2D& Current, bool bActive);

private:
	FVector2D DragStart = FVector2D::ZeroVector;
	FVector2D DragCurrent = FVector2D::ZeroVector;
	bool bIsDragActive = false;
};