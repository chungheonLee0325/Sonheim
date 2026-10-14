#include "WorldMapWidget.h"
#include "EngineUtils.h"
#include "EnhancedInputSubsystems.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/DynamicEntryBox.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Sonheim/AreaObject/Player/SonheimPlayerState.h"
#include "Sonheim/AreaObject/Player/Utility/QuestComponent.h"
#include "Sonheim/AreaObject/Player/Utility/WorldMapComponent.h"
#include "Sonheim/GameObject/Map/FastTravelPoint.h"
#include "Sonheim/Map/WorldMapDefinition.h"
#include "Sonheim/Map/WorldMapSubsystem.h"
#include "Sonheim/UI/Input/InputHintSubsystem.h"
#include "Sonheim/UI/System/UIStackSubsystem.h"

namespace
{
	const UWorldMapSubsystem* SubsystemOf(const UUserWidget& Widget)
	{
		const UGameInstance* Game = Widget.GetGameInstance();
		return Game ? Game->GetSubsystem<UWorldMapSubsystem>() : nullptr;
	}

	/** The legend's order. */
	const EMapMarkerKind LegendKinds[] = {EMapMarkerKind::QuestTarget, EMapMarkerKind::FastTravel, EMapMarkerKind::QuestGiver,
	                                      EMapMarkerKind::Dungeon, EMapMarkerKind::Player};
}

void UMapTravelEntryWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	EntryButton->OnClicked.AddDynamic(this, &UMapTravelEntryWidget::HandleClicked);
	EntryButton->OnHovered.AddDynamic(this, &UMapTravelEntryWidget::HandleHovered);
}

void UMapTravelEntryWidget::Show(UWorldMapWidget* InMap, const FName InPointId, const FText& Name, const FText& Region, const bool bSelected)
{
	Map = InMap;
	PointId = InPointId;
	NameText->SetText(Name);
	RegionText->SetText(Region);
	SelectedMark->SetVisibility(bSelected ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
}

void UMapTravelEntryWidget::HandleClicked()
{
	if (Map.IsValid()) Map->Travel(PointId);
}

void UMapTravelEntryWidget::HandleHovered()
{
	if (Map.IsValid()) Map->Select(PointId);
}

void UMapLegendEntryWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	EntryButton->OnClicked.AddDynamic(this, &UMapLegendEntryWidget::HandleClicked);
}

void UMapLegendEntryWidget::Show(UWorldMapWidget* InMap, const EMapMarkerKind InKind, UTexture2D* Icon, const FLinearColor& Color, const FText& Label,
                                 const bool bOn)
{
	Map = InMap;
	Kind = InKind;
	IconImage->SetBrushFromTexture(Icon);
	IconImage->SetColorAndOpacity(Color);
	LabelText->SetText(Label);
	CheckMark->SetVisibility(bOn ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	SetRenderOpacity(bOn ? 1.f : 0.5f);
}

void UMapLegendEntryWidget::HandleClicked()
{
	if (Map.IsValid()) Map->ToggleKind(Kind);
}

void UWorldMapWidget::NativeConstruct()
{
	Super::NativeConstruct();
	const UWorldMapSubsystem* Subsystem = SubsystemOf(*this);
	const UWorldMapDefinition* Map = Subsystem ? Subsystem->GetDefinition() : nullptr;
	MapMaterial = MapImage->GetDynamicMaterial();
	Regions = SonheimMap::CollectRegions(GetWorld());
	if (Map && MapMaterial)
	{
		MapMaterial->SetTextureParameterValue(TEXT("Art"), Map->Art);
		MapMaterial->SetTextureParameterValue(TEXT("Regions"), Map->RegionMask);
		MapMaterial->SetScalarParameterValue(TEXT("Round"), 0.f);
	}
	for (const FMapRegion& Region : Regions)
	{
		UTextBlock* Label = NewObject<UTextBlock>(MarkerLayer);
		Label->SetFont(RegionLabelStyle->GetFont());
		Label->SetColorAndOpacity(RegionLabelStyle->GetColorAndOpacity());
		Label->SetShadowOffset(RegionLabelStyle->GetShadowOffset());
		Label->SetShadowColorAndOpacity(RegionLabelStyle->GetShadowColorAndOpacity());
		Label->SetJustification(ETextJustify::Center);
		UCanvasPanelSlot* Placed = MarkerLayer->AddChildToCanvas(Label);
		Placed->SetAlignment(FVector2D(0.5f, 0.5f));
		Placed->SetAutoSize(true);
		RegionLabels.Add(Label);
	}
	RegionLabelStyle->SetVisibility(ESlateVisibility::Collapsed);
	// The banner shows with its plate, the panel it sits on.
	(HabitatBanner->GetParent() ? static_cast<UWidget*>(HabitatBanner->GetParent()) : HabitatBanner.Get())->SetVisibility(ESlateVisibility::Collapsed);

	// The map opens on the player, at a middle zoom.
	Span = FMath::Clamp(0.6f, MinSpan, MaxSpan);
	if (const APawn* Pawn = GetOwningPlayerPawn(); Pawn && Map && Map->Shows(Pawn->GetActorLocation())) Center = Map->ToUV(Pawn->GetActorLocation());
	ApplyView();

	const auto* State = GetOwningPlayerState<ASonheimPlayerState>();
	Places = State ? State->m_WorldMapComponent : nullptr;
	Quests = State ? State->m_QuestComponent : nullptr;
	if (Places.IsValid()) Places->OnMapChanged.AddUniqueDynamic(this, &UWorldMapWidget::Redraw);
	if (Quests.IsValid()) Quests->OnQuestsChanged.AddUniqueDynamic(this, &UWorldMapWidget::Redraw);
	Redraw();
}

void UWorldMapWidget::Redraw()
{
	const UWorldMapSubsystem* Subsystem = SubsystemOf(*this);
	const UWorldMapDefinition* Map = Subsystem ? Subsystem->GetDefinition() : nullptr;
	if (!Map) return;
	if (MapMaterial)
	{
		const FLinearColor Found = Map->FoundFog(Places.IsValid() ? Places->GetDiscovered() : TArray<FName>());
		for (int32 Channel = 0; Channel < 4; ++Channel)
			MapMaterial->SetScalarParameterValue(*FString::Printf(TEXT("Found%d"), Channel + 1), Found.Component(Channel));
	}
	QuestPlaces = SonheimMap::TrackedQuestPlaces(GetWorld(), Regions, Quests.Get());

	// The found points, in the world's order; the first is selected until the player picks one.
	TravelBox->Reset(true);
	TArray<const AFastTravelPoint*> Found;
	for (TActorIterator<AFastTravelPoint> It(GetWorld()); It; ++It)
		if (Places.IsValid() && Places->IsDiscovered(It->PointId)) Found.Add(*It);
	if (!Found.ContainsByPredicate([this](const AFastTravelPoint* Point) { return Point->PointId == Selected; }))
		Selected = Found.Num() ? Found[0]->PointId : NAME_None;
	for (const AFastTravelPoint* Point : Found)
	{
		const FMapRegion* Region = SonheimMap::RegionAt(Regions, Point->GetActorLocation());
		TravelBox->CreateEntry<UMapTravelEntryWidget>()->Show(this, Point->PointId, Point->DisplayName, Region ? Region->Name : FText::GetEmpty(),
		                                                     Point->PointId == Selected);
	}
	TravelEmpty->SetVisibility(Found.Num() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);

	LegendBox->Reset(true);
	for (const EMapMarkerKind Kind : LegendKinds)
	{
		const FMapMarkerStyle* Style = Map->MarkerStyles.Find(Kind);
		if (Style && Style->Icon) LegendBox->CreateEntry<UMapLegendEntryWidget>()->Show(this, Kind, Style->Icon, Style->Color, KindNames.FindRef(Kind), IsKindShown(Kind));
	}
	bShownRegionSet = false;
}

void UWorldMapWidget::ShowHabitat(const TArray<FName>& Habitats, const FText& Species)
{
	const UWorldMapSubsystem* Subsystem = SubsystemOf(*this);
	const UWorldMapDefinition* Map = Subsystem ? Subsystem->GetDefinition() : nullptr;
	if (!Map) return;
	HabitatPlaces.Reset();
	for (const FName& Habitat : Habitats)
	{
		if (const FMapRegion* Region = Regions.FindByPredicate([&Habitat](const FMapRegion& R) { return R.Id == Habitat; }))
			HabitatPlaces.Add(Region->Center);
		else if (Habitat == TEXT("Dungeon"))
			for (const TWeakObjectPtr<UMapMarkerComponent>& Marker : Subsystem->GetMarkers())
				if (Marker.IsValid() && Marker->Kind == EMapMarkerKind::Dungeon && Map->Shows(Marker->GetMarkerLocation())) HabitatPlaces.Add(Marker->GetMarkerLocation());
	}
	HabitatBanner->SetText(FText::Format(HabitatFormat, Species));
	(HabitatBanner->GetParent() ? static_cast<UWidget*>(HabitatBanner->GetParent()) : HabitatBanner.Get())->SetVisibility(ESlateVisibility::HitTestInvisible);
	if (HabitatPlaces.IsEmpty()) return;
	// The view takes in every place with room around them.
	FBox2D Fit(ForceInit);
	for (const FVector& Place : HabitatPlaces) Fit += Map->ToUV(Place);
	Center = Fit.GetCenter();
	Span = FMath::Max(Fit.GetSize().GetMax() * 1.8f, 0.45f);
	ApplyView();
}

void UWorldMapWidget::Select(const FName PointId)
{
	if (Selected == PointId) return;
	Selected = PointId;
	Redraw();
}

void UWorldMapWidget::ToggleKind(const EMapMarkerKind Kind)
{
	if (HiddenKinds.Remove(Kind) == 0) HiddenKinds.Add(Kind);
	Redraw();
}

void UWorldMapWidget::Travel(const FName PointId)
{
	if (!Places.IsValid() || !Places->IsDiscovered(PointId)) return;
	Places->ServerFastTravel(PointId);
	UUIStackSubsystem::CloseWidget(this);
}

void UWorldMapWidget::NativeTick(const FGeometry& MyGeometry, const float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	// The pad pans with the left stick and zooms with the triggers (IMC_Menu); the mouse drags and wheels instead.
	const UInputHintSubsystem* Hints = GetOwningLocalPlayer() ? GetOwningLocalPlayer()->GetSubsystem<UInputHintSubsystem>() : nullptr;
	const UEnhancedInputLocalPlayerSubsystem* Input = GetOwningLocalPlayer() ? GetOwningLocalPlayer()->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr;
	const bool bPad = Hints && Hints->GetDevice() == EInputHintDevice::Gamepad;
	if (bPad && Input && Input->GetPlayerInput())
	{
		const FVector2D Stick = PanAction ? Input->GetPlayerInput()->GetActionValue(PanAction).Get<FVector2D>() : FVector2D::ZeroVector;
		// Stick up is north, the map's up (smaller V).
		Center += FVector2D(Stick.X, -Stick.Y) * Span * StickSpeed * InDeltaTime;
		const float Pull = ZoomAction ? Input->GetPlayerInput()->GetActionValue(ZoomAction).Get<float>() : 0.f;
		if (!FMath::IsNearlyZero(Pull)) Zoom(FMath::Pow(0.35f, Pull * InDeltaTime), Center);
		ApplyView();
	}
	PlaceMarkers();

	const UWorldMapSubsystem* Subsystem = SubsystemOf(*this);
	const UWorldMapDefinition* Map = Subsystem ? Subsystem->GetDefinition() : nullptr;
	const APawn* Pawn = GetOwningPlayerPawn();
	// The panel names the region under the cursor, under the view's middle on a pad, or else the player's.
	FVector Looked = Pawn ? Pawn->GetActorLocation() : FVector::ZeroVector;
	if (Map && Hovered.X >= 0.0) Looked = Map->ToWorld(ImageToUV(Hovered), Looked.Z);
	else if (Map && bPad) Looked = Map->ToWorld(Center, Looked.Z);
	const FMapRegion* Region = SonheimMap::RegionAt(Regions, Looked);
	if (!bShownRegionSet || Region != ShownRegion) DrawSidePanel(Region);
}

void UWorldMapWidget::DrawSidePanel(const FMapRegion* Region)
{
	ShownRegion = Region;
	bShownRegionSet = true;
	const bool bFound = Region && Places.IsValid() && Places->IsDiscovered(Region->Id);
	RegionNameText->SetText(!Region ? FText::GetEmpty() : bFound ? Region->Name : UnknownName);
	RegionSubtitleText->SetText(bFound ? Region->Subtitle : FText::GetEmpty());
	RegionStateText->SetText(!Region ? FText::GetEmpty() : bFound ? FoundText : UnknownText);
	RegionStateText->SetColorAndOpacity(bFound ? FoundColor : UnknownColor);
}

void UWorldMapWidget::ApplyView()
{
	Span = FMath::Clamp(Span, MinSpan, MaxSpan);
	const double Half = FMath::Min(Span * 0.5, 0.5);
	Center.X = FMath::Clamp(Center.X, Half, 1.0 - Half);
	Center.Y = FMath::Clamp(Center.Y, Half, 1.0 - Half);
	if (!MapMaterial) return;
	MapMaterial->SetVectorParameterValue(TEXT("Center"), FLinearColor(Center.X, Center.Y, 0.f, 0.f));
	MapMaterial->SetScalarParameterValue(TEXT("Span"), Span);
}

void UWorldMapWidget::Zoom(const float Factor, const FVector2D& AroundUV)
{
	const float Next = FMath::Clamp(Span * Factor, MinSpan, MaxSpan);
	// The point under the cursor stays under it.
	Center = AroundUV + (Center - AroundUV) * (Next / Span);
	Span = Next;
	ApplyView();
}

FVector2D UWorldMapWidget::ImageToUV(const FVector2D& Local) const
{
	const FVector2D Size = MapImage->GetCachedGeometry().GetLocalSize();
	return Size.X > 0.0 ? Center + (Local / Size - FVector2D(0.5)) * Span : Center;
}

FReply UWorldMapWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	const FGeometry& Image = MapImage->GetCachedGeometry();
	if (InMouseEvent.GetEffectingButton() != EKeys::LeftMouseButton || !Image.IsUnderLocation(InMouseEvent.GetScreenSpacePosition()))
		return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
	bDragging = true;
	LastMouse = InMouseEvent.GetScreenSpacePosition();
	return FReply::Handled().CaptureMouse(TakeWidget());
}

FReply UWorldMapWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (!bDragging) return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
	bDragging = false;
	return FReply::Handled().ReleaseMouseCapture();
}

FReply UWorldMapWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	const FGeometry& Image = MapImage->GetCachedGeometry();
	Hovered = Image.IsUnderLocation(InMouseEvent.GetScreenSpacePosition()) ? Image.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition()) : FVector2D(-1.0, -1.0);
	if (!bDragging) return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
	const FVector2D Size = Image.GetLocalSize();
	const FVector2D Delta = Image.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition()) - Image.AbsoluteToLocal(LastMouse);
	LastMouse = InMouseEvent.GetScreenSpacePosition();
	if (Size.X > 0.0) Center -= Delta / Size * Span;
	ApplyView();
	return FReply::Handled();
}

FReply UWorldMapWidget::NativeOnMouseWheel(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	const FGeometry& Image = MapImage->GetCachedGeometry();
	if (!Image.IsUnderLocation(InMouseEvent.GetScreenSpacePosition())) return Super::NativeOnMouseWheel(InGeometry, InMouseEvent);
	Zoom(InMouseEvent.GetWheelDelta() > 0.f ? 0.8f : 1.25f, ImageToUV(Image.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition())));
	return FReply::Handled();
}

void UWorldMapWidget::PlaceMarkers()
{
	const UWorldMapSubsystem* Subsystem = SubsystemOf(*this);
	const UWorldMapDefinition* Map = Subsystem ? Subsystem->GetDefinition() : nullptr;
	const FVector2D Size = MapImage->GetCachedGeometry().GetLocalSize();
	if (!Map || Size.X <= 0.0) return;
	auto At = [&](const FVector& World) { return ((Map->ToUV(World) - Center) / Span + FVector2D(0.5)) * Size; };
	auto InView = [&](const FVector2D& Point) { return Point.X >= 0.0 && Point.Y >= 0.0 && Point.X <= Size.X && Point.Y <= Size.Y; };

	for (int32 Index = 0; Index < Regions.Num(); ++Index)
	{
		const FMapRegion& Region = Regions[Index];
		const bool bFound = Places.IsValid() && Places->IsDiscovered(Region.Id);
		const FVector2D Point = At(Region.Center);
		// A fog region not found yet shows as unknown; a smaller place inside one shows only once found.
		const bool bShown = InView(Point) && (bFound || Map->FogRegions.Contains(Region.Id));
		RegionLabels[Index]->SetVisibility(bShown ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		if (!bShown) continue;
		RegionLabels[Index]->SetText(bFound ? Region.Name : UnknownName);
		Cast<UCanvasPanelSlot>(RegionLabels[Index]->Slot)->SetPosition(Point);
	}

	const APawn* Pawn = GetOwningPlayerPawn();
	Icons.Begin();
	for (const TWeakObjectPtr<UMapMarkerComponent>& Weak : Subsystem->GetMarkers())
	{
		const UMapMarkerComponent* Marker = Weak.Get();
		if (!Marker || Marker->GetOwner() == Pawn || !IsKindShown(Marker->Kind)) continue;
		if (!Marker->DiscoveryId.IsNone() && !(Places.IsValid() && Places->IsDiscovered(Marker->DiscoveryId))) continue;
		const FMapMarkerStyle* Style = Map->MarkerStyles.Find(Marker->Kind);
		const FVector2D Point = At(Marker->GetMarkerLocation());
		if (!Style || !Style->Icon || !InView(Point)) continue;
		const bool bPicked = Marker->Kind == EMapMarkerKind::FastTravel && Marker->DiscoveryId == Selected;
		Icons.Add(MarkerLayer, Style->Icon, Style->Color, Style->Size * (bPicked ? 1.6f : 1.2f), Point,
		          Marker->Kind == EMapMarkerKind::Player ? Marker->GetOwner()->GetActorRotation().Yaw : 0.f);
	}
	if (const FMapMarkerStyle* Style = Map->MarkerStyles.Find(EMapMarkerKind::QuestTarget); Style && Style->Icon && IsKindShown(EMapMarkerKind::QuestTarget))
	{
		for (const FVector& Place : QuestPlaces)
			if (InView(At(Place))) Icons.Add(MarkerLayer, Style->Icon, Style->Color, Style->Size * 1.3f, At(Place));
	}
	if (HabitatIcon)
		for (const FVector& Place : HabitatPlaces)
			// Above the region's name, which stands on the same spot.
			if (InView(At(Place))) Icons.Add(MarkerLayer, HabitatIcon, HabitatColor, 40.f, At(Place) - FVector2D(0.0, 38.0));
	if (Pawn && SelfIcon && InView(At(Pawn->GetActorLocation())))
	{
		const APlayerController* Controller = GetOwningPlayer();
		Icons.Add(MarkerLayer, SelfIcon, SelfColor, 30.f, At(Pawn->GetActorLocation()), Controller ? Controller->GetControlRotation().Yaw : 0.f);
	}
	Icons.End();
}
