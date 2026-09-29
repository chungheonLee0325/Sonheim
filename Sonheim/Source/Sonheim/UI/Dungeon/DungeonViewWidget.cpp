#include "DungeonViewWidget.h"
#include "Components/DynamicEntryBox.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"
#include "GameFramework/GameStateBase.h"
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
