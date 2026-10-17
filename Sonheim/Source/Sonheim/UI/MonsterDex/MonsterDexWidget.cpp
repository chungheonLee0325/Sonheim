#include "MonsterDexWidget.h"
#include "Sonheim/UI/Widget/Player/Inventory/SlotWidget.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/DynamicEntryBox.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "MonsterDexStage.h"
#include "Sonheim/AreaObject/Player/SonheimPlayerState.h"
#include "Sonheim/AreaObject/Player/Utility/MonsterDexComponent.h"
#include "Sonheim/Utilities/StringTableIds.h"
#include "Sonheim/AreaObject/Player/SonheimPlayerController.h"
#include "Sonheim/AreaObject/Player/Utility/WorldMapComponent.h"
#include "Sonheim/Map/MapView.h"
#include "Sonheim/UI/Map/WorldMapWidget.h"
#include "Sonheim/UI/System/UIStackSubsystem.h"

void UMonsterDexChipWidget::Show(UTexture2D* Icon, const FText& Label, const FLinearColor& Fill, const FLinearColor& Ink, const FText& Tooltip)
{
	IconImage->SetVisibility(Icon ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	if (Icon) IconImage->SetBrushFromTexture(Icon, false);
	LabelText->SetText(Label);
	LabelText->SetColorAndOpacity(Ink);
	IconImage->SetColorAndOpacity(Ink);
	Plate->SetBrushColor(Fill);
	SetToolTipText(Tooltip);
}

void UMonsterDexCellWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	PickButton->OnClicked.AddDynamic(this, &UMonsterDexCellWidget::HandleClicked);
}

void UMonsterDexCellWidget::ClearPicture()
{
	PortraitImage->SetBrushFromTexture(nullptr);
}

void UMonsterDexCellWidget::Show(const FMonsterDexPageView& Page, const bool bSelected, UMaterialInterface* Picture)
{
	AreaObjectID = Page.AreaObjectID;
	NumberText->SetText(MonsterDexView::NumberText(Page.Number));
	NameText->SetText(Page.Name);
	if (Picture) PortraitImage->SetBrushFromMaterial(Picture);
	else PortraitImage->SetBrushFromTexture(Page.Portrait, false);
	PortraitImage->SetVisibility(Picture || Page.Portrait ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	PortraitImage->SetColorAndOpacity(Page.bDiscovered ? FLinearColor::White : SilhouetteColor);
	Frame->SetBrushColor(bSelected ? SelectedColor : FrameColor);
	CapturedMark->SetVisibility(Page.bCaptured ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	ReadyMark->SetVisibility(Page.bRewardReady ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}

void UMonsterDexCellWidget::HandleClicked()
{
	OnPicked.ExecuteIfBound(AreaObjectID);
}

void UMonsterDexTierWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	ClaimButton->OnClicked.AddDynamic(this, &UMonsterDexTierWidget::HandleClaim);
}

void UMonsterDexTierWidget::Show(const FMonsterDexTierView& Tier)
{
	TierIndex = Tier.Tier;
	ConditionIcon->SetBrushFromTexture(Tier.bCapture ? CaptureIcon : KillIcon, false);
	ConditionText->SetText(Tier.Condition);
	ProgressText->SetText(FText::Format(INVTEXT("{0} / {1}"), Tier.Current, Tier.Required));
	ProgressBar->SetPercent(static_cast<float>(Tier.Current) / Tier.Required);
	const bool bReady = Tier.State == EMonsterDexTierState::Ready;
	const bool bClaimed = Tier.State == EMonsterDexTierState::Claimed;
	ProgressBar->SetFillColorAndOpacity(bClaimed ? DoneColor : bReady ? ReadyColor : ProgressColor);

	RewardBox->Reset(true);
	for (const FRewardChipView& Reward : Tier.Rewards)
	{
		if (Reward.ItemId && USlotWidget::AddReadOnly(RewardBox, Reward.ItemId, Reward.Count, RewardSlotSize)) continue;
		RewardBox->CreateEntry<UMonsterDexChipWidget>()->Show(Reward.Icon, Reward.Label, RewardColor, FLinearColor::White, Reward.Name);
	}
	ClaimButton->SetIsEnabled(bReady);
	ClaimText->SetText(bClaimed ? LOCTABLE(SONHEIM_ST_DEX, "Tier.Claimed") : bReady ? LOCTABLE(SONHEIM_ST_DEX, "Tier.Claim") : LOCTABLE(SONHEIM_ST_DEX, "Tier.InProgress"));
	ClaimText->SetColorAndOpacity(bReady ? FSlateColor(FLinearColor(0.01f, 0.03f, 0.07f)) : FSlateColor(DoneColor));
	SetRenderOpacity(bClaimed ? 0.6f : 1.f);
}

void UMonsterDexTierWidget::HandleClaim()
{
	OnClaim.ExecuteIfBound(TierIndex);
}

UMonsterDexWidget::UMonsterDexWidget(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	// Element plates, in the spirit of the usual element colors; the screen tints a plate's rim and label with them.
	ElementColors = {
		{EElementalAttribute::Neutral, FLinearColor(0.62f, 0.6f, 0.55f)},
		{EElementalAttribute::Grass, FLinearColor(0.3f, 0.75f, 0.25f)},
		{EElementalAttribute::Fire, FLinearColor(1.f, 0.38f, 0.12f)},
		{EElementalAttribute::Water, FLinearColor(0.15f, 0.5f, 1.f)},
		{EElementalAttribute::Electric, FLinearColor(1.f, 0.85f, 0.1f)},
		{EElementalAttribute::Ground, FLinearColor(0.7f, 0.48f, 0.25f)},
		{EElementalAttribute::Ice, FLinearColor(0.55f, 0.88f, 1.f)},
		{EElementalAttribute::Dragon, FLinearColor(0.6f, 0.3f, 0.95f)},
		{EElementalAttribute::Dark, FLinearColor(0.55f, 0.2f, 0.45f)},
	};
}

UMonsterDexComponent* UMonsterDexWidget::Dex() const
{
	const ASonheimPlayerState* PS = GetOwningPlayerState<ASonheimPlayerState>();
	return PS ? PS->m_MonsterDexComponent : nullptr;
}

void UMonsterDexWidget::NativeConstruct()
{
	Super::NativeConstruct();
	HabitatButton->OnClicked.AddUniqueDynamic(this, &UMonsterDexWidget::ShowHabitat);
	if (UMonsterDexComponent* Component = Dex())
	{
		Bound = Component;
		Component->OnDexChanged.AddUniqueDynamic(this, &UMonsterDexWidget::Redraw);
	}
	// The stage lives on this machine only, as long as the book is open.
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Params.ObjectFlags |= RF_Transient;
	Stage = GetWorld()->SpawnActor<AMonsterDexStage>(AMonsterDexStage::Spot, FRotator::ZeroRotator, Params);
	SnapStage = GetWorld()->SpawnActor<AMonsterDexStage>(AMonsterDexStage::Spot + FVector(0.f, 20000.f, 0.f), FRotator::ZeroRotator, Params);
	Redraw();
}

void UMonsterDexWidget::NativeDestruct()
{
	if (Bound.IsValid()) Bound->OnDexChanged.RemoveDynamic(this, &UMonsterDexWidget::Redraw);
	if (Stage) Stage->Destroy();
	if (SnapStage) SnapStage->Destroy();
	Stage = nullptr;
	SnapStage = nullptr;
	// The book is kept to open again (UIStack), with new stages: the brushes let go of the old stages' render targets, so a
	// closed book holds none, and the next opening takes up the new ones.
	StageDrawn = nullptr;
	PortraitImage->SetBrushFromTexture(nullptr);
	for (UUserWidget* Cell : PageGrid->GetAllEntries()) Cast<UMonsterDexCellWidget>(Cell)->ClearPicture();
	CellPictures.Reset();
	Super::NativeDestruct();
}

void UMonsterDexWidget::Redraw()
{
	const UMonsterDexComponent* Component = Dex();
	if (!Component) return;
	Book = MonsterDexView::Build(*Component);
	const int32 Total = Book.Pages.Num();
	CompletionText->SetText(FText::Format(LOCTABLE(SONHEIM_ST_DEX, "Book.CompletionFormat"), Book.Discovered, Total));
	CompletionBar->SetPercent(Total ? static_cast<float>(Book.Discovered) / Total : 0.f);
	CapturedText->SetText(FText::Format(LOCTABLE(SONHEIM_ST_DEX, "Book.CapturedFormat"), Book.Captured));
	// The first open picks the first page; a picked page stays picked while the book changes.
	if (!Book.Pages.ContainsByPredicate([this](const FMonsterDexPageView& P) { return P.AreaObjectID == Picked; }))
	{
		Picked = Total ? Book.Pages[0].AreaObjectID : 0;
	}
	// The grid's pictures are stills of the same stage as the detail, taken once per opening of the book.
	if (CellPictures.IsEmpty() && SnapStage && StageMaterial)
	{
		TArray<TSubclassOf<ACharacter>> Classes;
		for (const FMonsterDexPageView& Page : Book.Pages) Classes.Add(Page.DisplayClass.LoadSynchronous());
		SnapStage->Snap(Classes, 256);
		for (int32 i = 0; i < Classes.Num(); ++i)
		{
			UMaterialInstanceDynamic* Picture = UMaterialInstanceDynamic::Create(StageMaterial, this);
			Picture->SetTextureParameterValue(TEXT("Capture"), SnapStage->GetSnap(i));
			CellPictures.Add(Classes[i] ? Picture : nullptr);
		}
	}
	PageGrid->Reset(true);
	for (int32 i = 0; i < Book.Pages.Num(); ++i)
	{
		const FMonsterDexPageView& Page = Book.Pages[i];
		UMonsterDexCellWidget* Cell = PageGrid->CreateEntry<UMonsterDexCellWidget>();
		Cell->Show(Page, Page.AreaObjectID == Picked, CellPictures.IsValidIndex(i) ? CellPictures[i].Get() : nullptr);
		Cell->OnPicked.BindUObject(this, &UMonsterDexWidget::Pick);
	}
	DrawPicked();
}

void UMonsterDexWidget::Pick(const int32 AreaObjectID)
{
	Picked = AreaObjectID;
	// Only the cells' frames and the right side change.
	for (int32 i = 0; i < Book.Pages.Num() && i < PageGrid->GetNumEntries(); ++i)
	{
		Cast<UMonsterDexCellWidget>(PageGrid->GetAllEntries()[i])->Show(Book.Pages[i], Book.Pages[i].AreaObjectID == Picked,
		                                                                CellPictures.IsValidIndex(i) ? CellPictures[i].Get() : nullptr);
	}
	DrawPicked();
}

void UMonsterDexWidget::DrawPicked()
{
	const FMonsterDexPageView* Page = Book.Pages.FindByPredicate([this](const FMonsterDexPageView& P) { return P.AreaObjectID == Picked; });
	if (!Page) return;
	NumberText->SetText(MonsterDexView::NumberText(Page->Number));
	NameText->SetText(Page->Name);
	if (Stage)
	{
		Stage->Show(Page->DisplayClass.LoadSynchronous());
		if (StageMaterial && Stage->GetTarget() && !StageDrawn)
		{
			StageDrawn = UMaterialInstanceDynamic::Create(StageMaterial, this);
			StageDrawn->SetTextureParameterValue(TEXT("Capture"), Stage->GetTarget());
			PortraitImage->SetBrushFromMaterial(StageDrawn);
		}
	}
	// Visible, not hit test invisible: a drag over the picture turns the species.
	PortraitImage->SetVisibility(ESlateVisibility::Visible);
	PortraitImage->SetColorAndOpacity(Page->bDiscovered ? FLinearColor::White : SilhouetteColor);
	DescriptionText->SetText(Page->Description);
	KillCountText->SetText(FText::AsNumber(Page->Kills));
	CaptureCountText->SetText(FText::AsNumber(Page->Captures));

	ElementBox->Reset(true);
	for (const EElementalAttribute Element : Page->Elements)
	{
		// A badge of the element's colour with its glyph and name in white, as the reference's element badges read.
		const FLinearColor Color = ElementColors.FindRef(Element, FLinearColor::White);
		ElementBox->CreateEntry<UMonsterDexChipWidget>()->Show(MonsterDexView::ElementIcon(Element), MonsterDexView::ElementName(Element),
		                                                       Color.CopyWithNewOpacity(0.9f), FLinearColor::White);
	}
	// The habitat by region name; a region the player has not found is ???, and a species not met has no habitat yet.
	const auto* State = GetOwningPlayerState<ASonheimPlayerState>();
	const UWorldMapComponent* Places = State ? State->m_WorldMapComponent : nullptr;
	const TArray<FMapRegion> Regions = SonheimMap::CollectRegions(GetWorld());
	TArray<FString> Names;
	for (const FName& Habitat : Page->bDiscovered ? Page->Habitats : TArray<FName>())
	{
		const FMapRegion* Region = Regions.FindByPredicate([&Habitat](const FMapRegion& R) { return R.Id == Habitat; });
		if (Habitat == TEXT("Dungeon")) Names.Add(LOCTABLE(SONHEIM_ST_DEX, "Habitat.Dungeon").ToString());
		else if (Region) Names.Add((Places && Places->IsDiscovered(Habitat) ? Region->Name : LOCTABLE(SONHEIM_ST_DEX, "Habitat.Unknown")).ToString());
	}
	HabitatText->SetText(Names.Num() ? FText::FromString(FString::Join(Names, TEXT("  ·  "))) : LOCTABLE(SONHEIM_ST_DEX, "Habitat.Unknown"));
	HabitatButton->SetIsEnabled(Names.Num() > 0);

	TierBox->Reset(true);
	for (const FMonsterDexTierView& Tier : Page->Tiers)
	{
		UMonsterDexTierWidget* Row = TierBox->CreateEntry<UMonsterDexTierWidget>();
		Row->Show(Tier);
		Row->OnClaim.BindUObject(this, &UMonsterDexWidget::Claim);
	}
}

void UMonsterDexWidget::ShowHabitat()
{
	const FMonsterDexPageView* Page = Book.Pages.FindByPredicate([this](const FMonsterDexPageView& P) { return P.AreaObjectID == Picked; });
	auto* Controller = Cast<ASonheimPlayerController>(GetOwningPlayer());
	const UUIStackSubsystem* UIStack = GetOwningLocalPlayer() ? GetOwningLocalPlayer()->GetSubsystem<UUIStackSubsystem>() : nullptr;
	if (!Page || !Page->bDiscovered || !Controller || !UIStack) return;
	// The map takes the book's place as the showing tab (closing this screen), then shows where the species lives.
	const TArray<FName> Habitats = Page->Habitats;
	const FText Species = Page->Name;
	Controller->OpenTab(UIScreen::WorldMap);
	if (auto* Map = Cast<UWorldMapWidget>(UIStack->GetWidget(UIScreen::WorldMap))) Map->ShowHabitat(Habitats, Species);
}

FReply UMonsterDexWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && PortraitImage->GetCachedGeometry().IsUnderLocation(InMouseEvent.GetScreenSpacePosition()))
	{
		bDragging = true;
		return FReply::Handled().CaptureMouse(TakeWidget());
	}
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UMonsterDexWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bDragging && Stage)
	{
		// Dragging right turns the species' front to the right, the way a turntable follows the hand.
		Stage->Turn(-InMouseEvent.GetCursorDelta().X * DragDegreesPerUnit);
		return FReply::Handled();
	}
	return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}

FReply UMonsterDexWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bDragging)
	{
		bDragging = false;
		return FReply::Handled().ReleaseMouseCapture();
	}
	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

void UMonsterDexWidget::Claim(const int32 Tier)
{
	if (UMonsterDexComponent* Component = Dex())
	{
		if (Component->CanClaimReward(Picked, Tier)) Component->ServerClaimReward(Picked, Tier);
	}
}
