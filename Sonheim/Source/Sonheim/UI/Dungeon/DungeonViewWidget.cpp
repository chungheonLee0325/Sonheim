#include "DungeonViewWidget.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/DynamicEntryBox.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/Pawn.h"
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
void UDungeonViewWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (TimeText) TimeColor = TimeText->GetColorAndOpacity();
	TitleColor = TitleText->GetColorAndOpacity();
	Show(AreaTitlePanel, false);
	ApplyViewData(ViewData);
}
void UDungeonViewWidget::NativeTick(const FGeometry& Geometry, float DeltaTime)
{
	Super::NativeTick(Geometry, DeltaTime);
	if (TimeText && ViewData.DeadlineServerTime > 0) RefreshTime();
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
void UDungeonViewWidget::ShowAreaTitle(const FText& Title, const FText& Subtitle)
{
	if (!AreaTitlePanel || Title.IsEmpty() || !GetWorld()) return;
	ShowText(AreaTitleText, nullptr, Title);
	ShowText(AreaSubtitleText, nullptr, Subtitle);
	Show(AreaTitlePanel, true);
	GetWorld()->GetTimerManager().SetTimer(AreaTitleTimer, FTimerDelegate::CreateWeakLambda(this, [this]() { Show(AreaTitlePanel, false); }), AreaTitleSeconds, false);
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
