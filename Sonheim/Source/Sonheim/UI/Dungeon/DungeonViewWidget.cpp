#include "DungeonViewWidget.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/DynamicEntryBox.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"
namespace
{
	void ShowText(UTextBlock* Block, UWidget* Frame, const FText& Text)
	{
		if (Block) Block->SetText(Text);
		if (UWidget* Shown = Frame ? Frame : Block) Shown->SetVisibility(Text.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
	void Show(UWidget* Widget, bool bShown)
	{
		if (Widget) Widget->SetVisibility(bShown ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	FText PaceClock(int32 Seconds)
	{
		return FText::FromString(FString::Printf(TEXT("%d:%02d"), FMath::Max(0, Seconds) / 60, FMath::Max(0, Seconds) % 60));
	}
	/** Draws the texture in the image, tinted, or hides the image while there is none. */
	void ShowIcon(UImage* Image, UTexture2D* Icon, const FSlateColor& Tint)
	{
		if (!Image) return;
		if (Icon)
		{
			Image->SetBrushFromTexture(Icon);
			Image->SetColorAndOpacity(Tint.GetSpecifiedColor());
		}
		Show(Image, Icon != nullptr);
	}
	/** One entry of the box's entry class per item; the box folds away while there is none. */
	template <typename EntryType, typename ItemType>
	void Fill(UDynamicEntryBox* Box, const TArray<ItemType>& Items, void (EntryType::*Apply)(const ItemType&))
	{
		if (!Box) return;
		Box->Reset();
		for (const ItemType& Item : Items)
			if (EntryType* Entry = Box->CreateEntry<EntryType>()) (Entry->*Apply)(Item);
		Show(Box, !Items.IsEmpty());
	}
}
void UDungeonObjectiveRowWidget::SetObjective(const FDungeonObjectiveViewData& Objective)
{
	const FSlateColor& KindColor = Objective.Kind == EDungeonObjectiveKind::Final ? FinalColor
		: Objective.Kind == EDungeonObjectiveKind::Optional ? OptionalColor : MainColor;
	const FSlateColor& StateColor = Objective.State == EDungeonObjectiveState::Done ? DoneColor
		: Objective.State == EDungeonObjectiveState::Missed ? MissedColor : KindColor;
	StatusText->SetText(Objective.State == EDungeonObjectiveState::Done ? DoneMark : Objective.State == EDungeonObjectiveState::Missed ? MissedMark : OpenMark);
	StatusText->SetColorAndOpacity(StateColor);
	// The goal's icon while the line is open, the done or missed mark after; the text mark stands in while there is no icon.
	UTexture2D* Icon = StatusIcon ? (Objective.State == EDungeonObjectiveState::Done ? DoneIcon.Get()
		: Objective.State == EDungeonObjectiveState::Missed ? MissedIcon.Get() : Objective.Icon.Get()) : nullptr;
	ShowIcon(StatusIcon, Icon, StateColor);
	Show(StatusText, Icon == nullptr);
	LabelText->SetText(Objective.Label);
	LabelText->SetColorAndOpacity(Objective.State == EDungeonObjectiveState::Missed ? MissedColor : MainColor);
	ShowText(CountText, nullptr, Objective.Count);
	CountText->SetColorAndOpacity(StateColor);
	ShowText(KindText, KindBadge, Objective.KindLabel);
	if (KindText) KindText->SetColorAndOpacity(KindColor);
	ShowText(WindowText, nullptr, Objective.Window);
	ShowText(NoteText, nullptr, Objective.Note);
}
void UDungeonStepNodeWidget::SetStep(const FDungeonStepViewData& Step)
{
	LabelText->SetText(Step.Label);
	const FSlateColor& Color = Step.State == EDungeonStepState::Current ? CurrentColor : Step.State == EDungeonStepState::Done ? DoneColor : UpcomingColor;
	LabelText->SetColorAndOpacity(Color);
	ShowIcon(StepIcon, Step.Icon, Color);
	Show(ArrowText, !Step.bFirst);
}
void UDungeonPartyMemberWidget::SetMember(const FDungeonMemberViewData& Member)
{
	NameText->SetText(Member.Name);
	HealthBar->SetPercent(Member.Health);
	HealthBar->SetFillColorAndOpacity(Member.Health <= 0.f ? DownColor : Member.Health < LowHealth ? LowColor : HealthyColor);
	Show(OwnerMark, Member.bOwner);
	Show(DownMark, Member.Health <= 0.f);
}
void UDungeonStatTileWidget::SetStat(const FDungeonStatViewData& Stat)
{
	LabelText->SetText(Stat.Label);
	ValueText->SetText(Stat.Value);
	ShowIcon(TileIcon, Stat.Icon, LabelText->GetColorAndOpacity());
}
void UDungeonRewardEntryWidget::SetReward(const FDungeonRewardViewData& Reward)
{
	NameText->SetText(Reward.Name);
	ShowText(CountText, nullptr, Reward.Count);
	Icon->SetBrushFromTexture(Reward.Icon);
	Icon->SetVisibility(Reward.Icon ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}
void UDungeonMarkerWidget::SetMarker(const FDungeonMarkerViewData& Marker)
{
	const FSlateColor& Tint = Marker.Kind == EDungeonObjectiveKind::Optional ? OptionalColor : MainColor;
	ShowIcon(Icon, Marker.Icon, Tint);
	if (Arrow) Arrow->SetColorAndOpacity(Tint);
}
void UDungeonMarkerWidget::Place(bool bEdge, const FVector2D& Direction, float Meters, const FText& Format)
{
	if (DistanceText) DistanceText->SetText(FText::Format(Format, FMath::RoundToInt(Meters)));
	Show(Arrow, bEdge);
	if (Arrow && bEdge)
		Arrow->SetRenderTransform(FWidgetTransform(Direction.GetSafeNormal() * ArrowOffset, FVector2D::UnitVector, FVector2D::ZeroVector,
			FMath::RadiansToDegrees(FMath::Atan2(Direction.Y, Direction.X))));
}
UImage* UDungeonMinimapWidget::AddMark(const FSlateBrush& Brush, const FSlateColor& Tint, int32 ZOrder, bool bArea)
{
	UImage* Mark = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
	Mark->SetBrush(Brush);
	Mark->SetColorAndOpacity(Tint.GetSpecifiedColor());
	Mark->SetVisibility(ESlateVisibility::HitTestInvisible);
	if (UCanvasPanelSlot* Placed = MapLayer->AddChildToCanvas(Mark))
	{
		Placed->SetZOrder(ZOrder);
		Placed->SetAutoSize(!bArea);
		Placed->SetAlignment(bArea ? FVector2D::ZeroVector : FVector2D(0.5f, 0.5f));
	}
	return Mark;
}
void UDungeonMinimapWidget::SetView(const FDungeonStageViewData& Data)
{
	Bounds = Data.MapBounds;
	if (Data.MapTexture) MapImage->SetBrushFromTexture(Data.MapTexture);
	for (UImage* Mark : ViewMarks) Mark->RemoveFromParent();
	ViewMarks.Reset();
	ViewAreas.Reset();
	if (!Data.MapTexture) return;
	const auto IconBrush = [this](UTexture2D* Texture)
	{
		FSlateBrush Brush;
		Brush.SetResourceObject(Texture);
		Brush.SetImageSize(FVector2D(IconSize));
		return Brush;
	};
	for (const FDungeonMapRoomViewData& Room : Data.MapRooms)
	{
		if (Room.bCurrent)
		{
			FSlateBrush Fill;
			Fill.DrawAs = ESlateBrushDrawType::RoundedBox;
			Fill.OutlineSettings = FSlateBrushOutlineSettings(3.f);
			ViewMarks.Add(AddMark(Fill, CurrentRoomColor, 0, true));
			ViewAreas.Add(Room.Area);
		}
		if (Room.Icon)
		{
			ViewMarks.Add(AddMark(IconBrush(Room.Icon), Room.bCurrent ? CurrentRoomIconColor : RoomIconColor, 1, false));
			ViewAreas.Add(FBox2D(Room.Area.GetCenter(), Room.Area.GetCenter()));
		}
	}
	// Rooms show themselves; a switch an open line leads to gets the line's icon.
	for (const FDungeonMarkerViewData& Marker : Data.Markers)
		if (!Marker.bArea && Marker.Icon)
		{
			ViewMarks.Add(AddMark(IconBrush(Marker.Icon), Marker.Kind == EDungeonObjectiveKind::Optional ? OptionalMarkColor : MainMarkColor, 2, false));
			const FVector2D Spot(Marker.Location);
			ViewAreas.Add(FBox2D(Spot, Spot));
		}
}
void UDungeonMinimapWidget::NativeTick(const FGeometry& Geometry, float DeltaTime)
{
	Super::NativeTick(Geometry, DeltaTime);
	const FVector2D Size = MapLayer->GetCachedGeometry().GetLocalSize();
	const FVector2D Span = Bounds.bIsValid ? Bounds.GetSize() : FVector2D::ZeroVector;
	if (Size.X <= 0.f || Size.Y <= 0.f || Span.X <= 0.f || Span.Y <= 0.f) return;
	const auto ToMap = [this, &Size, &Span](const FVector2D& World) { return (World - Bounds.Min) / Span * Size; };
	for (int32 Index = 0; Index < ViewMarks.Num(); ++Index)
		if (UCanvasPanelSlot* Placed = Cast<UCanvasPanelSlot>(ViewMarks[Index]->Slot))
		{
			const FBox2D& Area = ViewAreas[Index];
			Placed->SetPosition(ToMap(Area.Min));
			if (Area.Max != Area.Min) Placed->SetSize(ToMap(Area.Max) - ToMap(Area.Min));
		}
	APlayerController* Player = GetOwningPlayer();
	const APawn* Self = Player ? Player->GetPawn() : nullptr;
	// The local player, turned the way the camera looks.
	const bool bSelf = Self && Bounds.IsInside(FVector2D(Self->GetActorLocation()));
	Show(PlayerArrow, bSelf);
	if (bSelf)
	{
		FVector CameraLocation;
		FRotator CameraRotation;
		Player->GetPlayerViewPoint(CameraLocation, CameraRotation);
		if (UCanvasPanelSlot* Placed = Cast<UCanvasPanelSlot>(PlayerArrow->Slot)) Placed->SetPosition(ToMap(FVector2D(Self->GetActorLocation())));
		PlayerArrow->SetRenderTransformAngle(CameraRotation.Yaw);
	}
	// The world's other players who stand on the map.
	int32 Dots = 0;
	if (const AGameStateBase* State = GetWorld() ? GetWorld()->GetGameState() : nullptr)
		for (const APlayerState* Member : State->PlayerArray)
		{
			const APawn* Pawn = Member ? Member->GetPawn() : nullptr;
			if (!Pawn || Pawn == Self || !Bounds.IsInside(FVector2D(Pawn->GetActorLocation()))) continue;
			if (Dots == MemberDots.Num())
			{
				FSlateBrush Round;
				Round.DrawAs = ESlateBrushDrawType::RoundedBox;
				Round.SetImageSize(FVector2D(MemberSize));
				Round.OutlineSettings.RoundingType = ESlateBrushRoundingType::HalfHeightRadius;
				MemberDots.Add(AddMark(Round, MemberColor, 3, false));
			}
			UImage* Dot = MemberDots[Dots++];
			Show(Dot, true);
			if (UCanvasPanelSlot* Placed = Cast<UCanvasPanelSlot>(Dot->Slot)) Placed->SetPosition(ToMap(FVector2D(Pawn->GetActorLocation())));
		}
	for (int32 Index = Dots; Index < MemberDots.Num(); ++Index) Show(MemberDots[Index], false);
}
void UDungeonViewWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (TimeText) TimeColor = TimeText->GetColorAndOpacity();
	if (RunBestText) RunBestColor = RunBestText->GetColorAndOpacity();
	TitleColor = TitleText->GetColorAndOpacity();
	ApplyViewData(ViewData);
}
void UDungeonViewWidget::NativeTick(const FGeometry& Geometry, float DeltaTime)
{
	Super::NativeTick(Geometry, DeltaTime);
	if (TimeText && ViewData.DeadlineServerTime > 0) RefreshTime();
	if (ElapsedText && ViewData.RunStartServerTime > 0) RefreshPace();
	if (BossActionBar && ViewData.BossActionEndServerTime > 0) RefreshBossAction();
	if (!MarkerWidgets.IsEmpty()) PlaceMarkers(Geometry);
}
void UDungeonViewWidget::SyncMarkers()
{
	if (!MarkerLayer || !MarkerClass) return;
	while (MarkerWidgets.Num() > ViewData.Markers.Num()) MarkerWidgets.Pop()->RemoveFromParent();
	while (MarkerWidgets.Num() < ViewData.Markers.Num())
	{
		UDungeonMarkerWidget* Marker = CreateWidget<UDungeonMarkerWidget>(this, MarkerClass);
		if (!Marker) return;
		if (UCanvasPanelSlot* Placed = MarkerLayer->AddChildToCanvas(Marker))
		{
			Placed->SetAutoSize(true);
			Placed->SetAlignment(FVector2D(0.5f, 0.5f));
		}
		// Hidden until the next frame places it.
		Show(Marker, false);
		MarkerWidgets.Add(Marker);
	}
	for (int32 Index = 0; Index < MarkerWidgets.Num(); ++Index) MarkerWidgets[Index]->SetMarker(ViewData.Markers[Index]);
}
void UDungeonViewWidget::PlaceMarkers(const FGeometry& Geometry)
{
	APlayerController* Player = GetOwningPlayer();
	const APawn* Pawn = Player ? Player->GetPawn() : nullptr;
	const FVector2D Size = Geometry.GetLocalSize();
	const FVector2D Center = Size * 0.5f;
	const FVector2D Half = Center - FVector2D(MarkerEdgeInset);
	if (!Pawn || Half.X <= 0.f || Half.Y <= 0.f)
	{
		for (UDungeonMarkerWidget* Marker : MarkerWidgets) Show(Marker, false);
		return;
	}
	FVector CameraLocation;
	FRotator CameraRotation;
	Player->GetPlayerViewPoint(CameraLocation, CameraRotation);
	// The panels an edge marker must not sit behind, as laid out now, in the layer's space and grown by the clearance.
	TArray<FBox2D> Blocked;
	const FGeometry& Layer = MarkerLayer->GetCachedGeometry();
	for (const FName& Name : MarkerBlockers)
	{
		const UWidget* Blocker = GetWidgetFromName(Name);
		if (!Blocker || !Blocker->IsVisible()) continue;
		const FGeometry& Area = Blocker->GetCachedGeometry();
		const FVector2D AreaSize = Area.GetLocalSize();
		const FVector2D TopLeft = Area.LocalToAbsolute(FVector2D::ZeroVector);
		const FVector2D BottomRight = Area.LocalToAbsolute(AreaSize);
		const FVector2D Min = Layer.AbsoluteToLocal(TopLeft);
		const FVector2D Max = Layer.AbsoluteToLocal(BottomRight);
		if (Max.X > Min.X && Max.Y > Min.Y) Blocked.Add(FBox2D(Min - FVector2D(MarkerClearance), Max + FVector2D(MarkerClearance)));
	}
	const FBox2D Screen(Center - Half, Center + Half);
	for (int32 Index = 0; Index < MarkerWidgets.Num() && Index < ViewData.Markers.Num(); ++Index)
	{
		const FDungeonMarkerViewData& Place = ViewData.Markers[Index];
		UDungeonMarkerWidget* Marker = MarkerWidgets[Index];
		if (Place.Arrival.IsValid && Place.Arrival.IsInsideOrOn(Pawn->GetActorLocation()))
		{
			Show(Marker, false);
			continue;
		}
		// In camera space X looks ahead, Y goes right and Z up, while the screen's y goes down.
		const FVector Local = CameraRotation.UnrotateVector(Place.Location - CameraLocation);
		FVector2D Position = FVector2D::ZeroVector;
		const bool bProjected = Local.X > 0.f && UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(Player, Place.Location, Position, true);
		FVector2D Direction = bProjected ? Position - Center : FVector2D(Local.Y, -Local.Z);
		const bool bEdge = !bProjected || FMath::Abs(Direction.X) > Half.X || FMath::Abs(Direction.Y) > Half.Y;
		if (bEdge)
		{
			// Beside the screen or behind the camera: on the inset edge, in the place's direction.
			if (Direction.IsNearlyZero()) Direction = FVector2D(0.f, 1.f);
			Position = Center + Direction * FMath::Min(Half.X / FMath::Max(FMath::Abs(Direction.X), UE_KINDA_SMALL_NUMBER),
				Half.Y / FMath::Max(FMath::Abs(Direction.Y), UE_KINDA_SMALL_NUMBER));
			// Behind a panel: to the nearest side of it that is still on the inset screen, along the edge where there is room.
			for (int32 Pass = 0; Pass < Blocked.Num(); ++Pass)
			{
				const FBox2D* Hit = Blocked.FindByPredicate([&Position](const FBox2D& Box) { return Box.IsInside(Position); });
				if (!Hit) break;
				const FVector2D Sides[] = {{Hit->Min.X, Position.Y}, {Hit->Max.X, Position.Y}, {Position.X, Hit->Min.Y}, {Position.X, Hit->Max.Y}};
				FVector2D Next = Position;
				double Nearest = TNumericLimits<double>::Max();
				for (const FVector2D& Side : Sides)
					if (Screen.IsInsideOrOn(Side) && FVector2D::DistSquared(Side, Position) < Nearest)
					{
						Nearest = FVector2D::DistSquared(Side, Position);
						Next = Side;
					}
				if (Next == Position) break;
				Position = Next;
			}
		}
		if (UCanvasPanelSlot* Placed = Cast<UCanvasPanelSlot>(Marker->Slot)) Placed->SetPosition(Position);
		Marker->Place(bEdge, Direction, FVector::Dist(Pawn->GetActorLocation(), Place.Location) / 100.f, ViewData.MarkerDistanceFormat);
		Show(Marker, true);
	}
}
void UDungeonViewWidget::RefreshBossAction()
{
	if (!BossActionBar) return;
	const AGameStateBase* GameState = GetWorld() ? GetWorld()->GetGameState() : nullptr;
	const double Start = ViewData.BossActionStartServerTime;
	const double End = ViewData.BossActionEndServerTime;
	const bool bTimed = GameState && End > Start;
	Show(BossActionBar, bTimed);
	if (bTimed) BossActionBar->SetPercent(FMath::Clamp(float((GameState->GetServerWorldTimeSeconds() - Start) / (End - Start)), 0.f, 1.f));
}
void UDungeonViewWidget::RefreshTime()
{
	if (!TimeText) return;
	const AGameStateBase* GameState = GetWorld() ? GetWorld()->GetGameState() : nullptr;
	if (ViewData.DeadlineServerTime <= 0 || !GameState)
	{
		TimeText->SetVisibility(ESlateVisibility::Collapsed);
		Show(TimeIcon, false);
		return;
	}
	const int32 Remaining = FMath::Max(0, FMath::CeilToInt(ViewData.DeadlineServerTime - GameState->GetServerWorldTimeSeconds()));
	const FText Clock = FText::FromString(FString::Printf(TEXT("%d:%02d"), Remaining / 60, Remaining % 60));
	TimeText->SetText(ViewData.TimeFormat.IsEmpty() ? Clock : FText::Format(ViewData.TimeFormat, Clock));
	TimeText->SetColorAndOpacity(Remaining <= TimeWarningSeconds ? TimeWarningColor : TimeColor);
	TimeText->SetVisibility(ESlateVisibility::HitTestInvisible);
	if (TimeIcon) TimeIcon->SetColorAndOpacity(TimeText->GetColorAndOpacity().GetSpecifiedColor());
	Show(TimeIcon, true);
}
void UDungeonViewWidget::RefreshPace()
{
	const AGameStateBase* GameState = GetWorld() ? GetWorld()->GetGameState() : nullptr;
	if (!ElapsedText || !GameState || ViewData.RunStartServerTime <= 0) return;
	// The countdown's synchronized server clock; nothing is counted here.
	const double Elapsed = FMath::Max(0.0, GameState->GetServerWorldTimeSeconds() - ViewData.RunStartServerTime);
	const FText Time = PaceClock(FMath::FloorToInt32(Elapsed));
	ElapsedText->SetText(ViewData.ElapsedFormat.IsEmpty() ? Time : FText::Format(ViewData.ElapsedFormat, Time));
	const bool bBest = RunBestText && ViewData.RunStartBestSeconds > 0.f;
	Show(RunBestText, bBest);
	if (!bBest) return;
	const FText Best = PaceClock(FMath::RoundToInt32(ViewData.RunStartBestSeconds));
	RunBestText->SetText(ViewData.RunBestFormat.IsEmpty() ? Best : FText::Format(ViewData.RunBestFormat, Best));
	RunBestText->SetColorAndOpacity(Elapsed > ViewData.RunStartBestSeconds ? PaceBehindColor : RunBestColor);
}
void UDungeonViewWidget::ApplyViewData(const FDungeonStageViewData& Data)
{
	ViewData = Data;
	TitleText->SetText(Data.Title);
	TitleText->SetColorAndOpacity(Data.Status == EDungeonRunStatus::Succeeded ? SucceededTitleColor
		: Data.Status == EDungeonRunStatus::Failed ? FailedTitleColor : TitleColor);
	if (ObjectiveText) ObjectiveText->SetText(Data.Objective);
	if (CountText) CountText->SetText(Data.CountText);
	if (ObjectiveProgress) ObjectiveProgress->SetPercent(Data.Progress);
	ShowText(BranchText, BranchBadge, Data.BranchText);
	ShowText(DungeonTitleText, nullptr, Data.DungeonTitle);
	ShowText(GoalText, nullptr, Data.Goal);
	Fill(StepNodes, Data.Steps, &UDungeonStepNodeWidget::SetStep);
	ShowText(StepText, nullptr, Data.StepText);
	if (StepProgress)
	{
		StepProgress->SetPercent(Data.StepProgress);
		Show(StepProgress, !Data.StepText.IsEmpty());
	}
	Fill(ObjectiveRows, Data.Objectives, &UDungeonObjectiveRowWidget::SetObjective);
	ShowText(ObjectivesDoneText, nullptr, Data.ObjectivesDone);
	Fill(OptionalRows, Data.OptionalObjectives, &UDungeonObjectiveRowWidget::SetObjective);
	Show(OptionalPanel, !Data.OptionalObjectives.IsEmpty());
	Fill(PartyRows, Data.Members, &UDungeonPartyMemberWidget::SetMember);
	Show(PartyPanel, !Data.Members.IsEmpty());
	if (BossNameText) BossNameText->SetText(Data.BossName);
	if (BossHealthBar) BossHealthBar->SetPercent(Data.BossHealth);
	Show(BossPanel, Data.BossHealth > 0.f);
	ShowText(BossActionText, nullptr, Data.BossActionText);
	ShowIcon(BossActionIcon, Data.BossActionText.IsEmpty() ? nullptr : Data.BossActionIcon.Get(), BossActionText ? BossActionText->GetColorAndOpacity() : FSlateColor(FLinearColor::White));
	ShowText(BossPhaseText, nullptr, Data.BossPhaseText);
	Show(BossPhaseIcon, !Data.BossPhaseText.IsEmpty());
	ShowText(BossHintText, nullptr, Data.BossHintText);
	Show(BossHintIcon, !Data.BossHintText.IsEmpty());
	if (BossBreakBar) BossBreakBar->SetPercent(Data.BossBreak);
	RefreshBossAction();
	Fill(StatTiles, Data.Stats, &UDungeonStatTileWidget::SetStat);
	Fill(RewardSlots, Data.Rewards, &UDungeonRewardEntryWidget::SetReward);
	ShowText(RewardText, nullptr, Data.RewardText);
	ShowText(SummaryText, nullptr, Data.SummaryText);
	ShowText(NewBestText, NewBestBadge, Data.NewBestText);
	ShowIcon(OutcomeEmblem, Data.OutcomeEmblem, TitleText->GetColorAndOpacity());
	SyncMarkers();
	Show(PaceRow, Data.RunStartServerTime > 0);
	RefreshPace();
	if (Minimap) Minimap->SetView(Data);
	Show(MinimapPanel, Data.MapTexture != nullptr);
	RefreshTime();
	const bool bFinished = Data.Status == EDungeonRunStatus::Succeeded || Data.Status == EDungeonRunStatus::Failed;
	if (!bFinished)
	{
		FinishedRevision = -1; bFinishedExpired = false;
		if (GetWorld()) GetWorld()->GetTimerManager().ClearTimer(FinishedTimer);
	}
	else if (FinishedRevision < 0 && FinishedSeconds > 0.f && GetWorld())
	{
		// The timer starts when the run finishes; later updates of the same finished run do not restart it.
		FinishedRevision = Data.Revision;
		GetWorld()->GetTimerManager().SetTimer(FinishedTimer, FTimerDelegate::CreateWeakLambda(this, [this]() { bFinishedExpired = true; RefreshShown(); }), FinishedSeconds, false);
	}
	RefreshShown();
}
void UDungeonViewWidget::RefreshShown()
{
	SetVisibility(ViewData.bParticipant && !bFinishedExpired ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
}
