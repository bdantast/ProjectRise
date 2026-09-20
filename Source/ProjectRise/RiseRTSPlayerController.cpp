#include "RiseRTSPlayerController.h"
#include "RiseSelectableInterface.h"
#include "RiseBase.h"
#include "RiseResourceNode.h"
#include "RiseBuilding.h"
#include "RiseHUD.h"
#include "RiseEconomyManager.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "Engine/Engine.h"

void ARiseRTSPlayerController::BeginPlay()
{
	Super::BeginPlay();

	bShowMouseCursor = true;
	bEnableClickEvents = true;
	SetInputMode(FInputModeGameAndUI().SetHideCursorDuringCapture(false));
}

void ARiseRTSPlayerController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UpdateBuildGhostIfNeeded();

	if (!bIsSelectingDrag)
	{
		return;
	}

	if (ARiseHUD* RiseHUD = Cast<ARiseHUD>(GetHUD()))
	{
		FVector2D CurrentMouse;
		if (GetMousePosition(CurrentMouse.X, CurrentMouse.Y))
		{
			RiseHUD->SetSelectionDrag(DragStartScreen, CurrentMouse, bIsSelectingDrag);
		}
	}
}

void ARiseRTSPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	InputComponent->BindAction("Click", IE_Pressed, this, &ARiseRTSPlayerController::OnLeftMousePressed);
	InputComponent->BindAction("Click", IE_Released, this, &ARiseRTSPlayerController::OnLeftMouseReleased);
	InputComponent->BindAction("RightClick", IE_Pressed, this, &ARiseRTSPlayerController::OnRightMousePressed);
	InputComponent->BindAction("BuildInfantry", IE_Pressed, this, &ARiseRTSPlayerController::OnBuildInfantry);
	InputComponent->BindAction("BuildTank", IE_Pressed, this, &ARiseRTSPlayerController::OnBuildTank);
	InputComponent->BindAction("BuildDrone", IE_Pressed, this, &ARiseRTSPlayerController::OnBuildDrone);
	InputComponent->BindAction("BuildWorker", IE_Pressed, this, &ARiseRTSPlayerController::OnBuildWorker);
	InputComponent->BindAction("BuildHummer", IE_Pressed, this, &ARiseRTSPlayerController::OnBuildHummer);
	InputComponent->BindAction("BuildHelicopter", IE_Pressed, this, &ARiseRTSPlayerController::OnBuildHelicopter);
	InputComponent->BindAction("BuildMode", IE_Pressed, this, &ARiseRTSPlayerController::OnBuildMode);
	InputComponent->BindAction("BuildExtractor", IE_Pressed, this, &ARiseRTSPlayerController::OnBuildExtractor);
	InputComponent->BindAction("BuildRefinery", IE_Pressed, this, &ARiseRTSPlayerController::OnBuildRefinery);
	InputComponent->BindAction("CancelBuild", IE_Pressed, this, &ARiseRTSPlayerController::OnCancelBuild);
}

void ARiseRTSPlayerController::OnLeftMousePressed()
{
	if (IsBuildModeActive())
	{
		bBuildPlacementPending = true;
		return;
	}

	if (bPendingContextOrder)
	{
		return;
	}

	StartSelectionDrag();
}

void ARiseRTSPlayerController::OnLeftMouseReleased()
{
	if (IsBuildModeActive())
	{
		if (bBuildPlacementPending)
		{
			bBuildPlacementPending = false;
			PlaceBuildAtCursor();
		}
		return;
	}

	if (bPendingContextOrder)
	{
		bPendingContextOrder = false;
		FHitResult Hit;
		if (!GetHitResultUnderCursor(ECollisionChannel::ECC_Visibility, false, Hit))
		{
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow, TEXT("Clique no chao para ordenar a movimentacao"));
			}
			return;
		}
		IssueContextOrder(Hit);
		return;
	}

	EndSelectionDrag();
}

void ARiseRTSPlayerController::IssueContextOrder(const FHitResult& Hit)
{
	AActor* HitActor = Hit.GetActor();
	if (!HitActor)
	{
		IssueMoveOrder(Hit.Location);
		return;
	}

	if (ARiseBuilding* Building = Cast<ARiseBuilding>(HitActor))
	{
		if (Building->GetFactionID() == 0)
		{
			IssueBuildOrder(Building);
			return;
		}
	}

	if (ARiseResourceNode* Node = Cast<ARiseResourceNode>(HitActor))
	{
		IssueHarvestOrder(Node);
		return;
	}

	IssueMoveOrder(Hit.Location);
}

void ARiseRTSPlayerController::HUDQueueUnit(ERiseUnitType Type)
{
	ARiseBase* Base = GetSelectedBase();
	if (!Base)
	{
		Base = FindPlayerBase();
	}

	if (!Base)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, TEXT("Nenhuma base para produzir!"));
		}
		return;
	}

	if (!Base->QueueBuildUnit(Type))
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, TEXT("Recursos insuficientes"));
		}
	}
}

void ARiseRTSPlayerController::HUDSelectBuild(ERiseBuildMode Mode)
{
	EnterBuildMode(Mode);
}

void ARiseRTSPlayerController::HUDIssueMove()
{
	bPendingContextOrder = true;
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Green, TEXT("Clique no mapa para ordenar (mover / coletar / construir)"));
	}
}

void ARiseRTSPlayerController::HUDStopSelected()
{
	for (const TWeakObjectPtr<AActor>& WeakActor : SelectedActors)
	{
		if (AActor* Actor = WeakActor.Get())
		{
			if (ARiseUnit* Unit = Cast<ARiseUnit>(Actor))
			{
				Unit->Stop();
			}
		}
	}
}

void ARiseRTSPlayerController::HUDWorkersBuild()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	TArray<ARiseBuilding*> Sites;
	for (TActorIterator<ARiseBuilding> It(World); It; ++It)
	{
		ARiseBuilding* B = *It;
		if (IsValid(B) && B->GetFactionID() == 0 && !B->IsActive())
		{
			Sites.Add(B);
		}
	}

	if (Sites.Num() == 0)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, TEXT("Nenhuma obra em andamento! Coloque um predio no modo construcao primeiro."));
		}
		return;
	}

	FVector Centroid = FVector::ZeroVector;
	int32 Count = 0;
	for (const TWeakObjectPtr<AActor>& WeakActor : SelectedActors)
	{
		if (AActor* Actor = WeakActor.Get())
		{
			if (ARiseUnit* Unit = Cast<ARiseUnit>(Actor))
			{
				if (Unit->IsWorker())
				{
					Centroid += Unit->GetActorLocation();
					++Count;
				}
			}
		}
	}

	if (Count == 0)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, TEXT("Selecione coletoras para construir!"));
		}
		return;
	}
	Centroid /= Count;

	ARiseBuilding* Nearest = nullptr;
	float BestDist = FLT_MAX;
	for (ARiseBuilding* B : Sites)
	{
		const float D = FVector::DistSquared2D(B->GetActorLocation(), Centroid);
		if (D < BestDist)
		{
			BestDist = D;
			Nearest = B;
		}
	}

	if (Nearest)
	{
		IssueBuildOrder(Nearest);
	}
}

ARiseBase* ARiseRTSPlayerController::FindPlayerBase() const
{
	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<ARiseBase> It(World); It; ++It)
		{
			ARiseBase* Base = *It;
			if (IsValid(Base) && Base->GetFactionID() == 0)
			{
				return Base;
			}
		}
	}
	return nullptr;
}

void ARiseRTSPlayerController::OnRightMousePressed()
{
	if (IsBuildModeActive())
	{
		ExitBuildMode();
		return;
	}

	bPendingContextOrder = false;

	FHitResult Hit;
	if (!GetHitResultUnderCursor(ECollisionChannel::ECC_Visibility, false, Hit))
	{
		return;
	}

	IssueContextOrder(Hit);
}

void ARiseRTSPlayerController::OnBuildInfantry()
{
	QueueProduction(ERiseUnitType::Infantry);
}

void ARiseRTSPlayerController::OnBuildTank()
{
	QueueProduction(ERiseUnitType::Tank);
}

void ARiseRTSPlayerController::OnBuildDrone()
{
	QueueProduction(ERiseUnitType::Drone);
}

void ARiseRTSPlayerController::OnBuildWorker()
{
	QueueProduction(ERiseUnitType::Worker);
}

void ARiseRTSPlayerController::OnBuildHummer()
{
	QueueProduction(ERiseUnitType::Hummer);
}

void ARiseRTSPlayerController::OnBuildHelicopter()
{
	QueueProduction(ERiseUnitType::Helicopter);
}

void ARiseRTSPlayerController::StartSelectionDrag()
{
	GetMousePosition(DragStartScreen.X, DragStartScreen.Y);
	bIsSelectingDrag = true;
}

void ARiseRTSPlayerController::EndSelectionDrag()
{
	bIsSelectingDrag = false;

	FVector2D DragEndScreen;
	if (!GetMousePosition(DragEndScreen.X, DragEndScreen.Y))
	{
		return;
	}

	if (ARiseHUD* RiseHUD = Cast<ARiseHUD>(GetHUD()))
	{
		RiseHUD->SetSelectionDrag(DragStartScreen, DragEndScreen, false);
	}

	const float DragWidth = FMath::Abs(DragEndScreen.X - DragStartScreen.X);
	const float DragHeight = FMath::Abs(DragEndScreen.Y - DragStartScreen.Y);

	if (DragWidth > 6.0f || DragHeight > 6.0f)
	{
		SelectInRectangle(DragStartScreen, DragEndScreen);
	}
	else
	{
		SelectActorUnderCursor();
	}
}

void ARiseRTSPlayerController::SelectInRectangle(const FVector2D& Start, const FVector2D& End)
{
	const float MinX = FMath::Min(Start.X, End.X);
	const float MaxX = FMath::Max(Start.X, End.X);
	const float MinY = FMath::Min(Start.Y, End.Y);
	const float MaxY = FMath::Max(Start.Y, End.Y);

	TArray<AActor*> NewSelection;

	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<AActor> ActorIt(World); ActorIt; ++ActorIt)
		{
			AActor* Actor = *ActorIt;
			if (!IsValid(Actor) || !Actor->Implements<URiseSelectableInterface>())
			{
				continue;
			}

			if (ARiseUnit* Unit = Cast<ARiseUnit>(Actor))
			{
				if (Unit->GetFactionID() != 0)
				{
					continue;
				}
			}
			else if (ARiseBase* Base = Cast<ARiseBase>(Actor))
			{
				if (Base->GetFactionID() != 0)
				{
					continue;
				}
			}

			FVector2D ScreenPos;
			if (ProjectWorldLocationToScreen(Actor->GetActorLocation(), ScreenPos))
			{
				if (ScreenPos.X >= MinX && ScreenPos.X <= MaxX && ScreenPos.Y >= MinY && ScreenPos.Y <= MaxY)
				{
					NewSelection.Add(Actor);
				}
			}
		}
	}

	ClearSelection();
	for (AActor* Actor : NewSelection)
	{
		AddToSelection(Actor);
	}
}

void ARiseRTSPlayerController::SelectActorUnderCursor()
{
	FHitResult Hit;
	if (!GetHitResultUnderCursor(ECollisionChannel::ECC_Visibility, false, Hit))
	{
		ClearSelection();
		return;
	}

	AActor* HitActor = Hit.GetActor();
	if (IsValid(HitActor) && HitActor->Implements<URiseSelectableInterface>())
	{
		if (ARiseUnit* Unit = Cast<ARiseUnit>(HitActor))
		{
			if (Unit->GetFactionID() != 0)
			{
				ClearSelection();
				return;
			}
		}
		else if (ARiseBase* Base = Cast<ARiseBase>(HitActor))
		{
			if (Base->GetFactionID() != 0)
			{
				ClearSelection();
				return;
			}
		}

		ClearSelection();
		AddToSelection(HitActor);
	}
	else
	{
		ClearSelection();
	}
}

void ARiseRTSPlayerController::AddToSelection(AActor* Actor)
{
	SelectedActors.Add(Actor);
	IRiseSelectableInterface::Execute_SelectEntity(Actor);
}

void ARiseRTSPlayerController::ClearSelection()
{
	for (const TWeakObjectPtr<AActor>& WeakActor : SelectedActors)
	{
		if (AActor* Actor = WeakActor.Get())
		{
			IRiseSelectableInterface::Execute_DeselectEntity(Actor);
		}
	}
	SelectedActors.Empty();
}

void ARiseRTSPlayerController::IssueMoveOrder(const FVector& Destination)
{
	TArray<ARiseUnit*> Units;
	for (const TWeakObjectPtr<AActor>& WeakActor : SelectedActors)
	{
		if (AActor* Actor = WeakActor.Get())
		{
			if (ARiseUnit* Unit = Cast<ARiseUnit>(Actor))
			{
				Units.Add(Unit);
			}
		}
	}

	if (Units.Num() == 0)
	{
		return;
	}

	FVector Centroid = FVector::ZeroVector;
	for (ARiseUnit* Unit : Units)
	{
		Centroid += Unit->GetActorLocation();
	}
	Centroid /= Units.Num();

	const FVector MoveDir = (Destination - Centroid).GetSafeNormal2D();
	const float MoveYaw = FMath::Atan2(MoveDir.Y, MoveDir.X);

	const int32 RowSize = 5;
	const int32 Rows = FMath::Max(1, FMath::CeilToInt(static_cast<float>(Units.Num()) / static_cast<float>(RowSize)));

	for (int32 i = 0; i < Units.Num(); ++i)
	{
		const int32 Col = i % RowSize;
		const int32 Row = i / RowSize;

		const float OffsetX = (Col - (RowSize - 1) * 0.5f) * SelectionSpacing;
		const float OffsetY = (Row - (Rows - 1) * 0.5f) * SelectionSpacing;

		const float RotX = OffsetX * FMath::Cos(MoveYaw) - OffsetY * FMath::Sin(MoveYaw);
		const float RotY = OffsetX * FMath::Sin(MoveYaw) + OffsetY * FMath::Cos(MoveYaw);

		Units[i]->MoveTo(Destination + FVector(RotX, RotY, 0.0f));
	}
}

void ARiseRTSPlayerController::IssueHarvestOrder(ARiseResourceNode* Node)
{
	if (!Node)
	{
		return;
	}

	bool bAnyWorker = false;
	for (const TWeakObjectPtr<AActor>& WeakActor : SelectedActors)
	{
		if (AActor* Actor = WeakActor.Get())
		{
			if (ARiseUnit* Unit = Cast<ARiseUnit>(Actor))
			{
				if (Unit->IsWorker())
				{
					Unit->AssignResourceNode(Node);
					bAnyWorker = true;
				}
			}
		}
	}

	if (!bAnyWorker)
	{
		IssueMoveOrder(Node->GetActorLocation());
	}
}

void ARiseRTSPlayerController::QueueProduction(ERiseUnitType Type)
{
	for (const TWeakObjectPtr<AActor>& WeakActor : SelectedActors)
	{
		if (AActor* Actor = WeakActor.Get())
		{
			if (ARiseBase* Base = Cast<ARiseBase>(Actor))
			{
				if (!Base->QueueBuildUnit(Type))
				{
					GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, TEXT("Recursos insuficientes"));
				}
				break;
			}
		}
	}
}

ARiseBase* ARiseRTSPlayerController::GetSelectedBase() const
{
	for (const TWeakObjectPtr<AActor>& WeakActor : SelectedActors)
	{
		if (AActor* Actor = WeakActor.Get())
		{
			if (ARiseBase* Base = Cast<ARiseBase>(Actor))
			{
				return Base;
			}
		}
	}
	return nullptr;
}

void ARiseRTSPlayerController::OnBuildMode()
{
	if (IsBuildModeActive())
	{
		ExitBuildMode();
	}
	else
	{
		EnterBuildMode(ERiseBuildMode::Extractor);
	}
}

void ARiseRTSPlayerController::OnBuildExtractor()
{
	EnterBuildMode(ERiseBuildMode::Extractor);
}

void ARiseRTSPlayerController::OnBuildRefinery()
{
	EnterBuildMode(ERiseBuildMode::Refinery);
}

ERiseBuildingType ARiseRTSPlayerController::GetBuildTypeForMode() const
{
	switch (BuildMode)
	{
	case ERiseBuildMode::Refinery:
		return ERiseBuildingType::Refinery;
	case ERiseBuildMode::Extractor:
	default:
		return ERiseBuildingType::Extractor;
	}
}

void ARiseRTSPlayerController::OnCancelBuild()
{
	if (IsBuildModeActive())
	{
		ExitBuildMode();
	}
}

void ARiseRTSPlayerController::EnterBuildMode(ERiseBuildMode Mode)
{
	if (BuildMode == Mode)
	{
		return;
	}

	ExitBuildMode();

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	AActor* Ghost = World->SpawnActor<AActor>(AActor::StaticClass(), FVector(0.0f, 0.0f, -10000.0f), FRotator::ZeroRotator);
	if (!Ghost)
	{
		return;
	}

	UStaticMesh* GhostMesh = LoadObject<UStaticMesh>(nullptr,
		Mode == ERiseBuildMode::Refinery
			? TEXT("/Game/Meshes/Refinery/SM_RefineryTwistedTower.SM_RefineryTwistedTower")
			: TEXT("/Game/Meshes/Eco/Eco_Building_Grid.Eco_Building_Grid"));
	UStaticMeshComponent* MeshComponent = NewObject<UStaticMeshComponent>(Ghost);
	if (GhostMesh)
	{
		MeshComponent->SetStaticMesh(GhostMesh);
	}
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MeshComponent->SetCastShadow(false);
	MeshComponent->RegisterComponent();
	Ghost->SetRootComponent(MeshComponent);

	// Base do mesh alinhada ao ponto do cursor (visual e posicao batem)
	FVector GhostOrigin;
	FVector GhostExtent;
	MeshComponent->GetLocalBounds(GhostOrigin, GhostExtent);
	MeshComponent->SetRelativeLocation(FVector(0.0f, 0.0f, -(GhostOrigin.Z - GhostExtent.Z)));

	UMaterialInterface* GhostMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materials/M_Ghost.M_Ghost"));
	if (GhostMaterial)
	{
		UMaterialInstanceDynamic* Dyn = UMaterialInstanceDynamic::Create(GhostMaterial, Ghost);
		if (Dyn)
		{
			const int32 NumMats = MeshComponent->GetNumMaterials();
			for (int32 i = 0; i < NumMats; ++i)
			{
				MeshComponent->SetMaterial(i, Dyn);
			}
		}
	}

	BuildGhost = Ghost;
	BuildMode = Mode;

	const ERiseBuildingType BuildType = GetBuildTypeForMode();
	const FString BuildName = RiseBuildingTypeToString(BuildType);
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Cyan,
			FString::Printf(TEXT("Modo construcao: %s — move o mouse, [Clique Esq] coloca, [Esc]/[Dir] cancela"), *BuildName));
	}
}

void ARiseRTSPlayerController::ExitBuildMode()
{
	if (AActor* Ghost = BuildGhost.Get())
	{
		Ghost->Destroy();
	}
	BuildGhost = nullptr;
	BuildMode = ERiseBuildMode::None;
	bBuildPlacementPending = false;
}

void ARiseRTSPlayerController::UpdateBuildGhostIfNeeded()
{
	if (!IsBuildModeActive())
	{
		return;
	}

	AActor* Ghost = BuildGhost.Get();
	UStaticMeshComponent* MeshComponent = Ghost ? Ghost->FindComponentByClass<UStaticMeshComponent>() : nullptr;
	if (!Ghost || !MeshComponent)
	{
		return;
	}

	FVector FloorPoint;
	if (!GetFloorPointUnderCursor(FloorPoint))
	{
		Ghost->SetActorHiddenInGame(true);
		ShowBuildModeHint();
		return;
	}

	Ghost->SetActorHiddenInGame(false);
	Ghost->SetActorLocation(FloorPoint);

	const bool bValid = IsValidBuildLocation(FloorPoint);
	if (UMaterialInstanceDynamic* Dyn = Cast<UMaterialInstanceDynamic>(MeshComponent->GetMaterial(0)))
	{
		const FLinearColor GhostColor = bValid ? FLinearColor(0.25f, 1.0f, 0.35f) : FLinearColor(1.0f, 0.20f, 0.20f);
		Dyn->SetVectorParameterValue(TEXT("GhostColor"), GhostColor);
		Dyn->SetScalarParameterValue(TEXT("GhostOpacity"), 0.5f);
	}

	ShowBuildModeHint();
}

void ARiseRTSPlayerController::ShowBuildModeHint()
{
	if (!IsBuildModeActive())
	{
		return;
	}

	FVector FloorPoint;
	if (!GetFloorPointUnderCursor(FloorPoint))
	{
		return;
	}

	const bool bValid = IsValidBuildLocation(FloorPoint);
	const FRiseBuildingStats Stats = GetBuildingStats(GetBuildTypeForMode());
	const FString BuildName = RiseBuildingTypeToString(GetBuildTypeForMode());

	FString CostText = FString::Printf(TEXT("custo: $%.0f, comida %.0f, metal %.0f, oil %.0f, chips %.0f"),
		Stats.BuildCost.Dollars, Stats.BuildCost.Food, Stats.BuildCost.RareMetal, Stats.BuildCost.Petrol, Stats.BuildCost.MicroChips);

	const FString Hint = bValid
		? FString::Printf(TEXT("Modo construcao | %s | %s | OK para construir"), *BuildName, *CostText)
		: FString::Printf(TEXT("Modo construcao | %s | %s | LOCAL INVALIDO"), *BuildName, *CostText);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(42, 0.15f, bValid ? FColor::Green : FColor::Red, Hint);
	}
}

void ARiseRTSPlayerController::PlaceBuildAtCursor()
{
	if (!IsBuildModeActive())
	{
		return;
	}

	FVector FloorPoint;
	if (!GetFloorPointUnderCursor(FloorPoint))
	{
		return;
	}

	if (!IsValidBuildLocation(FloorPoint))
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, TEXT("Local invalido para construir!"));
		}
		return;
	}

	const FRiseBuildingStats Stats = GetBuildingStats(GetBuildTypeForMode());
	const ERiseBuildingType BuildType = GetBuildTypeForMode();
	const FString BuildName = RiseBuildingTypeToString(BuildType);

	FRiseResourceContainer PlayerResources;
	RiseFactionGetResources(this, 0, PlayerResources);
	UE_LOG(LogTemp, Warning,
		TEXT("[Construcao] custo: $%.0f comida %.0f metal %.0f oil %.0f | saldo: $%.0f comida %.0f metal %.0f oil %.0f"),
		Stats.BuildCost.Dollars, Stats.BuildCost.Food, Stats.BuildCost.RareMetal, Stats.BuildCost.Petrol,
		PlayerResources.Dollars, PlayerResources.Food, PlayerResources.RareMetal, PlayerResources.Petrol);

	if (!RiseFactionConsumeResources(this, 0, Stats.BuildCost))
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red,
				FString::Printf(TEXT("Recursos insuficientes para o %s!"), *BuildName));
		}
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	ARiseBuilding* Building = World->SpawnActor<ARiseBuilding>(ARiseBuilding::StaticClass(), FloorPoint, FRotator::ZeroRotator);
	if (Building)
	{
		Building->ConfigureBuilding(BuildType, 0);
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Cyan,
				FString::Printf(TEXT("%s em obra! Selecione coletoras e clique direito nele para acelerar."), *BuildName));
		}
	}
}

bool ARiseRTSPlayerController::GetFloorPointUnderCursor(FVector& Out) const
{
	FVector WorldOrigin;
	FVector WorldDir;
	if (!DeprojectMousePositionToWorld(WorldOrigin, WorldDir))
	{
		return false;
	}

	// X/Y do cursor no plano do chao (mapa plano) - posicao exata do clique
	const float T = -WorldOrigin.Z / WorldDir.Z;
	if (T < 0.0f)
	{
		return false;
	}

	const FVector CursorGround = WorldOrigin + WorldDir * T;

	// Trace VERTICAL descendo na coluna do cursor: encontra a superficie real
	// do terreno sem ser perturbado por paredes/telhados de predios vizinhos.
	UWorld* World = GetWorld();
	if (World)
	{
		FCollisionQueryParams Params(SCENE_QUERY_STAT(GetFloorPointUnderCursor), false);
		Params.bTraceComplex = false;
		FHitResult Hit;
		const FVector Top = FVector(CursorGround.X, CursorGround.Y, 20000.0f);
		const FVector Bottom = FVector(CursorGround.X, CursorGround.Y, -5000.0f);
		if (World->LineTraceSingleByChannel(Hit, Top, Bottom, ECC_WorldStatic, Params))
		{
			Out = Hit.ImpactPoint;
			return true;
		}
	}

	Out = CursorGround;
	return true;
}

bool ARiseRTSPlayerController::IsValidBuildLocation(const FVector& Location) const
{
	if (FMath::Abs(Location.X) > 11500.0f || FMath::Abs(Location.Y) > 11500.0f)
	{
		return false;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	for (TActorIterator<ARiseBase> BaseIt(World); BaseIt; ++BaseIt)
	{
		ARiseBase* Base = *BaseIt;
		if (IsValid(Base) && FVector::DistSquared2D(Base->GetActorLocation(), Location) < FMath::Square(2200.0f))
		{
			return false;
		}
	}

	for (TActorIterator<ARiseBuilding> BuildingIt(World); BuildingIt; ++BuildingIt)
	{
		const FRiseBuildingStats Stats = GetBuildingStats(BuildingIt->GetBuildingType());
		const float Clearance = FMath::Max(1300.0f, Stats.PlacementRadius);
		if (FVector::DistSquared2D(BuildingIt->GetActorLocation(), Location) < FMath::Square(Clearance))
		{
			return false;
		}
	}

	return true;
}

void ARiseRTSPlayerController::IssueBuildOrder(ARiseBuilding* Building)
{
	if (!Building || Building->IsActive())
	{
		return;
	}

	bool bAnyWorker = false;
	for (const TWeakObjectPtr<AActor>& WeakActor : SelectedActors)
	{
		if (AActor* Actor = WeakActor.Get())
		{
			if (ARiseUnit* Unit = Cast<ARiseUnit>(Actor))
			{
				if (Unit->IsWorker())
				{
					Unit->OrderBuild(Building);
					bAnyWorker = true;
				}
			}
		}
	}

	if (!bAnyWorker)
	{
		IssueMoveOrder(Building->GetActorLocation());
	}
}