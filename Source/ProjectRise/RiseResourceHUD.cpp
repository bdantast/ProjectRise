#include "RiseResourceHUD.h"
#include "RiseUnit.h"
#include "RiseBase.h"
#include "RiseResourceNode.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/SizeBox.h"
#include "Components/ProgressBar.h"
#include "Components/WrapBox.h"
#include "Components/WrapBoxSlot.h"
#include "Components/TextBlock.h"
#include "Fonts/SlateFontInfo.h"
#include "Styling/CoreStyle.h"
#include "EngineUtils.h"

namespace
{
	const FLinearColor ColorFood(0.9f, 0.75f, 0.25f);
	const FLinearColor ColorChips(0.25f, 0.85f, 0.95f);
	const FLinearColor ColorMetal(0.6f, 0.75f, 0.95f);
	const FLinearColor ColorPetrol(0.35f, 0.85f, 0.45f);
	const FLinearColor ColorDollars(0.45f, 1.0f, 0.35f);
	const FLinearColor ColorKnowledge(0.85f, 0.45f, 1.0f);
	const FLinearColor ColorPanel(0.01f, 0.02f, 0.05f, 0.82f);

	const FLinearColor& ResourceColor(ERiseResourceType Type)
	{
		switch (Type)
		{
		case ERiseResourceType::MicroChips: return ColorChips;
		case ERiseResourceType::RareMetal:  return ColorMetal;
		case ERiseResourceType::Petrol:     return ColorPetrol;
		case ERiseResourceType::Dollars:    return ColorDollars;
		case ERiseResourceType::Knowledge:  return ColorKnowledge;
		case ERiseResourceType::Food:
		default:                            return ColorFood;
		}
	}

	UTextBlock* MakeText(UWidgetTree* Tree, const FString& Name, int32 FontSize, const FLinearColor& Color)
	{
		UTextBlock* Block = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), FName(*Name));
		if (Block)
		{
			Block->SetColorAndOpacity(FSlateColor(Color));
			Block->SetShadowColorAndOpacity(FLinearColor::Black);
			Block->SetShadowOffset(FVector2D(1.0f, 1.0f));
			Block->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), FontSize));
		}
		return Block;
	}

	UBorder* MakeBox(UWidgetTree* Tree, const FString& Name, const FLinearColor& Color, float Padding)
	{
		UBorder* Border = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), FName(*Name));
		if (Border)
		{
			Border->SetBrushColor(Color);
			Border->SetPadding(FMargin(Padding));
		}
		return Border;
	}
}

void URiseResourceHUD::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
	WidgetTree->RootWidget = Canvas;

	BuildResourcePanel(Canvas);
	BuildCommandPanel(Canvas);
	BuildMinimapPanel(Canvas);
}

void URiseResourceHUD::NativeConstruct()
{
	Super::NativeConstruct();

	if (URiseEconomyManager* Economy = GetGameInstance<URiseEconomyManager>())
	{
		UE_LOG(LogTemp, Warning, TEXT("[HUD-RTS] economia conectada | food %.0f | chips %.0f | metal %.0f | oil %.0f | $ %.0f | knowledge %.0f"),
			Economy->GetCurrentResources().Food,
			Economy->GetCurrentResources().MicroChips,
			Economy->GetCurrentResources().RareMetal,
			Economy->GetCurrentResources().Petrol,
			Economy->GetCurrentResources().Dollars,
			Economy->GetCurrentResources().Knowledge);
		Economy->OnResourcesChanged.AddDynamic(this, &URiseResourceHUD::OnResourcesChangedHandler);
		Refresh(Economy->GetCurrentResources());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[HUD-RTS] GameInstance NAO e RiseEconomyManager (widget sem economia)"));
	}
}

void URiseResourceHUD::BuildResourcePanel(UCanvasPanel* Canvas)
{
	UBorder* Root = MakeBox(WidgetTree, TEXT("ResourceRoot"), ColorPanel, 10.0f);
	if (UCanvasPanelSlot* CanvasSlot = Canvas->AddChildToCanvas(Root))
	{
		CanvasSlot->SetPosition(FVector2D(20.0f, 20.0f));
		CanvasSlot->SetAutoSize(true);
	}

	ResourceBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ResourceBox"));
	Root->SetContent(ResourceBox);

	UTextBlock* Title = MakeText(WidgetTree, TEXT("ResourcesTitle"), 16, FLinearColor(0.55f, 0.75f, 1.0f));
	Title->SetText(FText::FromString(TEXT("RECURSOS")));
	ResourceBox->AddChildToVerticalBox(Title);

	struct FResourceDef
	{
		const TCHAR* Label;
		ERiseResourceType Type;
	};

	const FResourceDef Defs[] = {
		{ TEXT("Comida"),       ERiseResourceType::Food },
		{ TEXT("Microchips"),   ERiseResourceType::MicroChips },
		{ TEXT("Metal raro"),   ERiseResourceType::RareMetal },
		{ TEXT("Petroleo"),     ERiseResourceType::Petrol },
		{ TEXT("Dolares"),      ERiseResourceType::Dollars },
		{ TEXT("Conhecimento"), ERiseResourceType::Knowledge },
	};

	for (const FResourceDef& Def : Defs)
	{
		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		UTextBlock* Label = MakeText(WidgetTree, FString::Printf(TEXT("RLabel_%s"), Def.Label), 16, ResourceColor(Def.Type));
		Label->SetText(FText::FromString(FString(Def.Label) + TEXT(":")));
		UTextBlock* Value = MakeText(WidgetTree, FString::Printf(TEXT("RValue_%s"), Def.Label), 16, FLinearColor::White);
		Value->SetText(FText::FromString(TEXT("0")));

		UHorizontalBoxSlot* LabelSlot = Row->AddChildToHorizontalBox(Label);
		LabelSlot->SetPadding(FMargin(0.0f, 0.0f, 12.0f, 0.0f));

		UHorizontalBoxSlot* ValueSlot = Row->AddChildToHorizontalBox(Value);
		ValueSlot->SetHorizontalAlignment(HAlign_Right);

		ResourceBox->AddChildToVerticalBox(Row);
		ResourceValueTexts.Add(Value);
	}

	Root->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void URiseResourceHUD::BuildCommandPanel(UCanvasPanel* Canvas)
{
	PanelBorder = MakeBox(WidgetTree, TEXT("PanelBorder"), ColorPanel, 10.0f);
	if (UCanvasPanelSlot* CanvasSlot = Canvas->AddChildToCanvas(PanelBorder))
	{
		CanvasSlot->SetAnchors(FAnchors(0.0f, 1.0f));
		CanvasSlot->SetAlignment(FVector2D(0.0f, 1.0f));
		CanvasSlot->SetPosition(FVector2D(20.0f, -20.0f));
		CanvasSlot->SetAutoSize(true);
	}

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("PanelColumn"));
	PanelBorder->SetContent(Column);

	UHorizontalBox* HeaderRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("PanelHeader"));

	USizeBox* IconSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("IconSize"));
	IconSize->SetWidthOverride(52.0f);
	IconSize->SetHeightOverride(52.0f);

	PanelIcon = MakeBox(WidgetTree, TEXT("PanelIcon"), FLinearColor(0.2f, 0.6f, 1.0f), 0.0f);
	IconSize->SetContent(PanelIcon);
	HeaderRow->AddChildToHorizontalBox(IconSize);

	UVerticalBox* PanelInfo = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("PanelInfo"));
	HeaderRow->AddChildToHorizontalBox(PanelInfo);

	PanelTitleText = MakeText(WidgetTree, TEXT("PanelTitle"), 20, FLinearColor::White);
	PanelHealthBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("PanelHealthBar"));
	PanelHealthBar->SetPercent(1.0f);
	PanelHealthBar->SetFillColorAndOpacity(FLinearColor(0.15f, 1.0f, 0.4f));
	PanelHealthText = MakeText(WidgetTree, TEXT("PanelHealth"), 15, FLinearColor(0.85f, 0.9f, 1.0f));
	PanelDetailText = MakeText(WidgetTree, TEXT("PanelDetail"), 15, FLinearColor(0.8f, 0.85f, 0.95f));

	PanelInfo->AddChildToVerticalBox(PanelTitleText);
	PanelInfo->AddChildToVerticalBox(PanelHealthBar);
	PanelInfo->AddChildToVerticalBox(PanelHealthText);
	PanelInfo->AddChildToVerticalBox(PanelDetailText);

	Column->AddChildToVerticalBox(HeaderRow);

	ActionRow = WidgetTree->ConstructWidget<UWrapBox>(UWrapBox::StaticClass(), TEXT("ActionRow"));
	ActionRow->SetInnerSlotPadding(FVector2D(6.0f, 6.0f));
	Column->AddChildToVerticalBox(ActionRow);

	// Deixa o fundo invisivel ao clique para nao bloquear a selecao, mas os botoes continuam clicaveis
	PanelBorder->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	PanelBorder->SetVisibility(ESlateVisibility::Collapsed);
}

void URiseResourceHUD::BuildMinimapPanel(UCanvasPanel* Canvas)
{
	MinimapBorder = MakeBox(WidgetTree, TEXT("MinimapBorder"), ColorPanel, 2.0f);
	if (UCanvasPanelSlot* CanvasSlot = Canvas->AddChildToCanvas(MinimapBorder))
	{
		CanvasSlot->SetAnchors(FAnchors(1.0f, 1.0f));
		CanvasSlot->SetAlignment(FVector2D(1.0f, 1.0f));
		CanvasSlot->SetPosition(FVector2D(-20.0f, -20.0f));
		CanvasSlot->SetSize(FVector2D(220.0f, 220.0f));
	}

	USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("MinimapSize"));
	Size->SetWidthOverride(216.0f);
	Size->SetHeightOverride(216.0f);

	MinimapCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("MinimapCanvas"));
	MinimapCanvas->SetVisibility(ESlateVisibility::HitTestInvisible);
	Size->SetContent(MinimapCanvas);
	MinimapBorder->SetContent(Size);
	MinimapBorder->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void URiseResourceHUD::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	HudRefreshTimer += InDeltaTime;
	if (HudRefreshTimer >= 0.25f)
	{
		HudRefreshTimer = 0.0f;
		if (URiseEconomyManager* Economy = GetGameInstance<URiseEconomyManager>())
		{
			Refresh(Economy->GetCurrentResources());
		}
	}

	MinimapTimer += InDeltaTime;
	if (MinimapTimer >= 0.25f)
	{
		MinimapTimer = 0.0f;
		RefreshMinimap();
	}

	RefreshSelection();
}

void URiseResourceHUD::OnResourcesChangedHandler(const FRiseResourceContainer& UpdatedResources)
{
	Refresh(UpdatedResources);
}

void URiseResourceHUD::Refresh(const FRiseResourceContainer& Resources)
{
	const FString Values[] = {
		FString::Printf(TEXT("%.0f"), Resources.Food),
		FString::Printf(TEXT("%.0f"), Resources.MicroChips),
		FString::Printf(TEXT("%.0f"), Resources.RareMetal),
		FString::Printf(TEXT("%.0f"), Resources.Petrol),
		FString::Printf(TEXT("%.0f"), Resources.Dollars),
		FString::Printf(TEXT("%.0f"), Resources.Knowledge),
	};

	for (int32 i = 0; i < ResourceValueTexts.Num() && i < 6; ++i)
	{
		if (UTextBlock* Value = ResourceValueTexts[i])
		{
			Value->SetText(FText::FromString(Values[i]));
		}
	}
}

void URiseResourceHUD::RefreshMinimap()
{
	if (!MinimapCanvas)
	{
		return;
	}

	MinimapCanvas->ClearChildren();

	int32 MinimapDots = 0;

	auto AddDot = [this, &MinimapDots](const FVector& WorldPos, const FLinearColor& Color, float Size)
	{
		const float U = (WorldPos.X - MinimapTopLeft.X) / MinimapWorldSize.X * 216.0f;
		const float V = (WorldPos.Y - MinimapTopLeft.Y) / MinimapWorldSize.Y * 216.0f;
		if (U < -8.0f || U > 224.0f || V < -8.0f || V > 224.0f)
		{
			return;
		}

		USizeBox* DotSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		DotSize->SetWidthOverride(Size);
		DotSize->SetHeightOverride(Size);

		UBorder* Dot = MakeBox(WidgetTree, TEXT("MinimapDot"), Color, 0.0f);
		DotSize->SetContent(Dot);
		if (UCanvasPanelSlot* DotCanvasSlot = MinimapCanvas->AddChildToCanvas(DotSize))
		{
			DotCanvasSlot->SetPosition(FVector2D(U - Size * 0.5f, V - Size * 0.5f));
			DotCanvasSlot->SetAutoSize(true);
		}
		++MinimapDots;
	};

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	for (TActorIterator<ARiseBase> It(World); It; ++It)
	{
		ARiseBase* Base = *It;
		if (IsValid(Base) && Base->IsAlive())
		{
			AddDot(Base->GetActorLocation(), Base->GetFactionID() == 0 ? FLinearColor(0.2f, 0.6f, 1.0f) : FLinearColor(1.0f, 0.25f, 0.25f), 10.0f);
		}
	}

	for (TActorIterator<ARiseBuilding> It(World); It; ++It)
	{
		ARiseBuilding* Building = *It;
		if (IsValid(Building) && Building->IsAlive())
		{
			const FLinearColor Color = Building->IsActive()
				? (Building->GetFactionID() == 0 ? FLinearColor(0.7f, 0.9f, 1.0f) : FLinearColor(1.0f, 0.5f, 0.5f))
				: FLinearColor(1.0f, 0.85f, 0.2f);
			AddDot(Building->GetActorLocation(), Color, 7.0f);
		}
	}

	for (TActorIterator<ARiseUnit> It(World); It; ++It)
	{
		ARiseUnit* Unit = *It;
		if (IsValid(Unit) && Unit->IsAlive())
		{
			AddDot(Unit->GetActorLocation(), Unit->GetFactionID() == 0 ? FLinearColor(0.3f, 1.0f, 0.4f) : FLinearColor(1.0f, 0.3f, 0.3f), 4.0f);
		}
	}

	for (TActorIterator<ARiseResourceNode> It(World); It; ++It)
	{
		ARiseResourceNode* Node = *It;
		if (IsValid(Node) && !Node->IsDepleted())
		{
			AddDot(Node->GetActorLocation(), ResourceColor(Node->GetResourceType()), 5.0f);
		}
	}

	static bool bMinimapLogged = false;
	if (!bMinimapLogged)
	{
		UE_LOG(LogTemp, Warning, TEXT("[HUD-RTS] minimap refresh: %d pontos"), MinimapDots);
		bMinimapLogged = true;
	}
}

UButton* URiseResourceHUD::MakeActionButton(const FString& Label, const FLinearColor& Color)
{
	UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), FName(*Label));
	if (!Button)
	{
		return nullptr;
	}

	Button->SetBackgroundColor(Color);
	Button->WidgetStyle.Normal.TintColor = FSlateColor(Color);
	Button->WidgetStyle.Hovered.TintColor = FSlateColor(Color * 1.25f);
	Button->WidgetStyle.Pressed.TintColor = FSlateColor(Color * 0.7f);

	UTextBlock* Text = MakeText(WidgetTree, FString::Printf(TEXT("Btn_%s"), *Label), 14, FLinearColor::White);
	Text->SetText(FText::FromString(Label));
	Button->SetContent(Text);

	if (UWrapBoxSlot* WrapSlot = ActionRow->AddChildToWrapBox(Button))
	{
		WrapSlot->SetPadding(FMargin(0.0f));
	}
	return Button;
}

FString URiseResourceHUD::BuildSelectionSignature(const TArray<AActor*>& Actors)
{
	FString Sig;
	for (AActor* Actor : Actors)
	{
		Sig += Actor->GetName();
		Sig += TEXT("|");
	}
	return Sig;
}

void URiseResourceHUD::RefreshSelection()
{
	if (!PanelBorder)
	{
		return;
	}

	static bool bHudPcLogged = false;
	ARiseRTSPlayerController* PC = Cast<ARiseRTSPlayerController>(GetOwningPlayer());
	if (!PC)
	{
		if (!bHudPcLogged)
		{
			UE_LOG(LogTemp, Warning, TEXT("[HUD-RTS] GetOwningPlayer retornou NULL/outro PC -> painel oculto"));
			bHudPcLogged = true;
		}
		PanelBorder->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}
	if (!bHudPcLogged)
	{
		UE_LOG(LogTemp, Warning, TEXT("[HUD-RTS] PC ok: %s | panel pronto: %s"),
			*PC->GetName(), PanelBorder ? TEXT("sim") : TEXT("nao"));
		bHudPcLogged = true;
	}

	TArray<AActor*> Actors;
	for (const TWeakObjectPtr<AActor>& WeakActor : PC->GetSelectedActors())
	{
		if (AActor* Actor = WeakActor.Get())
		{
			Actors.Add(Actor);
		}
	}

	if (Actors.Num() == 0)
	{
		if (!SelectionSignature.IsEmpty())
		{
			SelectionSignature.Empty();
			ActionRow->ClearChildren();
		}
		PanelBorder->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	const FString NewSig = BuildSelectionSignature(Actors);
	const bool bSelectionChanged = (NewSig != SelectionSignature);
	SelectionSignature = NewSig;

	PanelBorder->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	if (bSelectionChanged)
	{
		UE_LOG(LogTemp, Warning, TEXT("[HUD-RTS] selecao mudou: %d ator(es) | %s"),
			Actors.Num(), *Actors[0]->GetName());
	}

	if (Actors.Num() > 1)
	{
		TMap<ERiseUnitType, int32> Counts;
		int32 WorkerCount = 0;
		int32 UnitCount = 0;
		float TotalHealth = 0.0f;

		for (AActor* Actor : Actors)
		{
			if (ARiseUnit* Unit = Cast<ARiseUnit>(Actor))
			{
				Counts.FindOrAdd(Unit->GetUnitType())++;
				TotalHealth += Unit->GetHealthPercent();
				++UnitCount;
				if (Unit->IsWorker())
				{
					++WorkerCount;
				}
			}
		}

		PanelIcon->SetBrushColor(FLinearColor(0.25f, 0.5f, 0.9f));
		PanelTitleText->SetText(FText::FromString(FString::Printf(TEXT("%d selecionadas"), Actors.Num())));

		FString Breakdown;
		for (const TPair<ERiseUnitType, int32>& Pair : Counts)
		{
			if (!Breakdown.IsEmpty())
			{
				Breakdown += TEXT("  ");
			}
			Breakdown += FString::Printf(TEXT("%d %s"), Pair.Value, *RiseUnitTypeToString(Pair.Key));
		}

		PanelHealthBar->SetPercent(UnitCount > 0 ? TotalHealth / UnitCount : 1.0f);
		PanelHealthText->SetText(FText::FromString(FString::Printf(TEXT("Vida media: %.0f%%"), UnitCount > 0 ? (TotalHealth / UnitCount) * 100.0f : 0.0f)));
		PanelDetailText->SetText(FText::FromString(Breakdown));

		if (bSelectionChanged)
		{
			ActionRow->ClearChildren();

			UButton* Move = MakeActionButton(TEXT("Mover"), FLinearColor(0.15f, 0.4f, 0.7f));
			if (Move) Move->OnClicked.AddDynamic(this, &URiseResourceHUD::OnMoveClicked);

			UButton* Stop = MakeActionButton(TEXT("Parar"), FLinearColor(0.4f, 0.3f, 0.6f));
			if (Stop) Stop->OnClicked.AddDynamic(this, &URiseResourceHUD::OnStopClicked);

			if (WorkerCount > 0)
			{
				UButton* Harvest = MakeActionButton(TEXT("Colher recurso"), FLinearColor(0.7f, 0.55f, 0.2f));
				if (Harvest) Harvest->OnClicked.AddDynamic(this, &URiseResourceHUD::OnMoveClicked);

				UButton* Build = MakeActionButton(TEXT("Construir obra"), FLinearColor(0.2f, 0.6f, 0.4f));
				if (Build) Build->OnClicked.AddDynamic(this, &URiseResourceHUD::OnWorkersBuildClicked);
			}
		}
		return;
	}

	AActor* Actor = Actors[0];

	// ---- QG / base ----
	if (ARiseBase* Base = Cast<ARiseBase>(Actor))
	{
		PanelIcon->SetBrushColor(Base->GetFactionID() == 0 ? FLinearColor(0.15f, 0.55f, 1.0f) : FLinearColor(1.0f, 0.25f, 0.25f));
		PanelTitleText->SetText(FText::FromString(TEXT("QG - Quartel General")));
		PanelHealthBar->SetPercent(Base->GetHealthPercent());
		PanelHealthText->SetText(FText::FromString(FString::Printf(TEXT("Vida: %.0f%%"), Base->GetHealthPercent() * 100.0f)));
		PanelDetailText->SetText(FText::FromString(FString::Printf(TEXT("Fila: %s"), *Base->GetQueueSummary())));

		if (bSelectionChanged)
		{
			ActionRow->ClearChildren();

			struct FUnitBtn
			{
				const TCHAR* Label;
				ERiseUnitType Type;
				FLinearColor Color;
			};

			const FUnitBtn UnitBtns[] = {
				{ TEXT("Infantaria"),  ERiseUnitType::Infantry,   FLinearColor(0.2f, 0.6f, 1.0f) },
				{ TEXT("Tanque"),      ERiseUnitType::Tank,       FLinearColor(0.15f, 0.35f, 0.7f) },
				{ TEXT("Drone"),       ERiseUnitType::Drone,      FLinearColor(0.3f, 0.9f, 0.9f) },
				{ TEXT("Coletora"),    ERiseUnitType::Worker,     FLinearColor(0.95f, 0.85f, 0.2f) },
				{ TEXT("Hummer"),      ERiseUnitType::Hummer,     FLinearColor(0.55f, 0.8f, 0.45f) },
				{ TEXT("Helicoptero"), ERiseUnitType::Helicopter, FLinearColor(0.8f, 0.4f, 0.9f) },
			};

			for (const FUnitBtn& Btn : UnitBtns)
			{
				const FRiseUnitStats Stats = GetUnitStats(Btn.Type);
				const FString Cost = FString::Printf(TEXT("%s ($%.0f)"), Btn.Label, Stats.BuildCost.Dollars);
				UButton* B = MakeActionButton(Cost, Btn.Color);
				if (B)
				{
					switch (Btn.Type)
					{
					case ERiseUnitType::Infantry:   B->OnClicked.AddDynamic(this, &URiseResourceHUD::OnQueueInfantry); break;
					case ERiseUnitType::Tank:       B->OnClicked.AddDynamic(this, &URiseResourceHUD::OnQueueTank); break;
					case ERiseUnitType::Drone:      B->OnClicked.AddDynamic(this, &URiseResourceHUD::OnQueueDrone); break;
					case ERiseUnitType::Worker:     B->OnClicked.AddDynamic(this, &URiseResourceHUD::OnQueueWorker); break;
					case ERiseUnitType::Hummer:     B->OnClicked.AddDynamic(this, &URiseResourceHUD::OnQueueHummer); break;
					case ERiseUnitType::Helicopter: B->OnClicked.AddDynamic(this, &URiseResourceHUD::OnQueueHelicopter); break;
					default: break;
					}
				}
			}

			UButton* Extractor = MakeActionButton(TEXT("Construir Extrator"), FLinearColor(0.7f, 0.45f, 0.2f));
			if (Extractor) Extractor->OnClicked.AddDynamic(this, &URiseResourceHUD::OnSelectBuildExtractor);

			UButton* Refinery = MakeActionButton(TEXT("Construir Refinaria"), FLinearColor(0.45f, 0.3f, 0.75f));
			if (Refinery) Refinery->OnClicked.AddDynamic(this, &URiseResourceHUD::OnSelectBuildRefinery);
		}
		return;
	}

	// ---- Predio ----
	if (ARiseBuilding* Building = Cast<ARiseBuilding>(Actor))
	{
		PanelIcon->SetBrushColor(Building->GetFactionID() == 0 ? FLinearColor(0.2f, 0.8f, 0.4f) : FLinearColor(1.0f, 0.3f, 0.3f));
		PanelTitleText->SetText(FText::FromString(RiseBuildingTypeToString(Building->GetBuildingType())));
		PanelHealthBar->SetPercent(Building->GetHealthPercent());
		PanelHealthText->SetText(FText::FromString(FString::Printf(TEXT("Vida: %.0f%%"), Building->GetHealthPercent() * 100.0f)));

		if (Building->IsActive())
		{
			PanelDetailText->SetText(FText::FromString(TEXT("Operante")));
		}
		else
		{
			PanelDetailText->SetText(FText::FromString(FString::Printf(TEXT("Em obra: %.0f%% concluido"), Building->GetBuildProgressPercent() * 100.0f)));
		}

		if (bSelectionChanged)
		{
			ActionRow->ClearChildren();

			if (!Building->IsActive() && Building->GetFactionID() == 0)
			{
				UButton* Build = MakeActionButton(TEXT("Acelerar obra"), FLinearColor(0.2f, 0.6f, 0.4f));
				if (Build) Build->OnClicked.AddDynamic(this, &URiseResourceHUD::OnWorkersBuildClicked);
			}
		}
		return;
	}

	// ---- Unidade ----
	if (ARiseUnit* Unit = Cast<ARiseUnit>(Actor))
	{
		FLinearColor IconColor = FLinearColor(0.2f, 0.6f, 1.0f);
		switch (Unit->GetUnitType())
		{
		case ERiseUnitType::Worker:     IconColor = FLinearColor(0.95f, 0.85f, 0.2f); break;
		case ERiseUnitType::Tank:       IconColor = FLinearColor(0.15f, 0.35f, 0.7f); break;
		case ERiseUnitType::Drone:      IconColor = FLinearColor(0.3f, 0.9f, 0.9f); break;
		case ERiseUnitType::Hummer:     IconColor = FLinearColor(0.55f, 0.8f, 0.45f); break;
		case ERiseUnitType::Helicopter: IconColor = FLinearColor(0.8f, 0.4f, 0.9f); break;
		default:                        break;
		}
		PanelIcon->SetBrushColor(IconColor);

		PanelTitleText->SetText(FText::FromString(RiseUnitTypeToString(Unit->GetUnitType())));
		PanelHealthBar->SetPercent(Unit->GetHealthPercent());
		PanelHealthText->SetText(FText::FromString(FString::Printf(TEXT("Vida: %.0f / %.0f"), Unit->GetHealth(), Unit->GetMaxHealth())));

		if (Unit->IsWorker())
		{
			FString StateText = TEXT("Parada");
			switch (Unit->GetWorkerState())
			{
			case ERiseWorkerState::MovingToNode: StateText = TEXT("Indo colher"); break;
			case ERiseWorkerState::Harvesting:   StateText = TEXT("Colhendo"); break;
			case ERiseWorkerState::Returning:    StateText = TEXT("Retornando a base"); break;
			case ERiseWorkerState::MovingToBuild: StateText = TEXT("Indo construir"); break;
			case ERiseWorkerState::Building:     StateText = TEXT("Construindo"); break;
			default: break;
			}
			PanelDetailText->SetText(FText::FromString(FString::Printf(TEXT("%s | Carga: %.0f/%.0f"), *StateText, Unit->GetCarriedAmount(), Unit->GetCarryCapacity())));

			if (bSelectionChanged)
			{
				ActionRow->ClearChildren();

				UButton* Harvest = MakeActionButton(TEXT("Colher recurso"), FLinearColor(0.7f, 0.55f, 0.2f));
				if (Harvest) Harvest->OnClicked.AddDynamic(this, &URiseResourceHUD::OnMoveClicked);

				UButton* Build = MakeActionButton(TEXT("Construir obra"), FLinearColor(0.2f, 0.6f, 0.4f));
				if (Build) Build->OnClicked.AddDynamic(this, &URiseResourceHUD::OnWorkersBuildClicked);

				UButton* Move = MakeActionButton(TEXT("Mover"), FLinearColor(0.15f, 0.4f, 0.7f));
				if (Move) Move->OnClicked.AddDynamic(this, &URiseResourceHUD::OnMoveClicked);

				UButton* Stop = MakeActionButton(TEXT("Parar"), FLinearColor(0.4f, 0.3f, 0.6f));
				if (Stop) Stop->OnClicked.AddDynamic(this, &URiseResourceHUD::OnStopClicked);
			}
		}
		else
		{
			const FRiseUnitStats Stats = GetUnitStats(Unit->GetUnitType());
			PanelDetailText->SetText(FText::FromString(FString::Printf(TEXT("Dano: %.0f | Alcance: %.0f"), Stats.AttackDamage, Stats.AttackRange)));

			if (bSelectionChanged)
			{
				ActionRow->ClearChildren();

				UButton* Move = MakeActionButton(TEXT("Mover"), FLinearColor(0.15f, 0.4f, 0.7f));
				if (Move) Move->OnClicked.AddDynamic(this, &URiseResourceHUD::OnMoveClicked);

				UButton* Stop = MakeActionButton(TEXT("Parar"), FLinearColor(0.4f, 0.3f, 0.6f));
				if (Stop) Stop->OnClicked.AddDynamic(this, &URiseResourceHUD::OnStopClicked);
			}
		}
		return;
	}

	// Nada conhecido selecionado
	ActionRow->ClearChildren();
	PanelBorder->SetVisibility(ESlateVisibility::Collapsed);
}

void URiseResourceHUD::OnQueueInfantry()
{
	if (ARiseRTSPlayerController* PC = Cast<ARiseRTSPlayerController>(GetOwningPlayer())) PC->HUDQueueUnit(ERiseUnitType::Infantry);
}

void URiseResourceHUD::OnQueueTank()
{
	if (ARiseRTSPlayerController* PC = Cast<ARiseRTSPlayerController>(GetOwningPlayer())) PC->HUDQueueUnit(ERiseUnitType::Tank);
}

void URiseResourceHUD::OnQueueDrone()
{
	if (ARiseRTSPlayerController* PC = Cast<ARiseRTSPlayerController>(GetOwningPlayer())) PC->HUDQueueUnit(ERiseUnitType::Drone);
}

void URiseResourceHUD::OnQueueWorker()
{
	if (ARiseRTSPlayerController* PC = Cast<ARiseRTSPlayerController>(GetOwningPlayer())) PC->HUDQueueUnit(ERiseUnitType::Worker);
}

void URiseResourceHUD::OnQueueHummer()
{
	if (ARiseRTSPlayerController* PC = Cast<ARiseRTSPlayerController>(GetOwningPlayer())) PC->HUDQueueUnit(ERiseUnitType::Hummer);
}

void URiseResourceHUD::OnQueueHelicopter()
{
	if (ARiseRTSPlayerController* PC = Cast<ARiseRTSPlayerController>(GetOwningPlayer())) PC->HUDQueueUnit(ERiseUnitType::Helicopter);
}

void URiseResourceHUD::OnSelectBuildExtractor()
{
	if (ARiseRTSPlayerController* PC = Cast<ARiseRTSPlayerController>(GetOwningPlayer())) PC->HUDSelectBuild(ERiseBuildMode::Extractor);
}

void URiseResourceHUD::OnSelectBuildRefinery()
{
	if (ARiseRTSPlayerController* PC = Cast<ARiseRTSPlayerController>(GetOwningPlayer())) PC->HUDSelectBuild(ERiseBuildMode::Refinery);
}

void URiseResourceHUD::OnMoveClicked()
{
	if (ARiseRTSPlayerController* PC = Cast<ARiseRTSPlayerController>(GetOwningPlayer())) PC->HUDIssueMove();
}

void URiseResourceHUD::OnStopClicked()
{
	if (ARiseRTSPlayerController* PC = Cast<ARiseRTSPlayerController>(GetOwningPlayer())) PC->HUDStopSelected();
}

void URiseResourceHUD::OnWorkersBuildClicked()
{
	if (ARiseRTSPlayerController* PC = Cast<ARiseRTSPlayerController>(GetOwningPlayer())) PC->HUDWorkersBuild();
}