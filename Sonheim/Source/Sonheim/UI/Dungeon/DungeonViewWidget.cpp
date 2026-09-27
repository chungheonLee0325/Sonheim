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
	/** One entry of the box's entry class per item; the box folds away while there is none. */
	template <typename EntryType, typename ItemType>
	void Fill(UDynamicEntryBox* Box, const TArray<ItemType>& Items, void (EntryType::*Apply)(const ItemType&))
	{
		if (!Box) return;
		Box->Reset();
		for (const ItemType& Item : Items)
			if (EntryType* Entry = Box->CreateEntry<EntryType>()) (Entry->*Apply)(Item);
		Box->SetVisibility(Items.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
}
void UDungeonObjectiveRowWidget::SetObjective(const FDungeonObjectiveViewData& Objective)
{
	const FSlateColor& KindColor = Objective.Kind == EDungeonObjectiveKind::Final ? FinalColor
		: Objective.Kind == EDungeonObjectiveKind::Optional ? OptionalColor : MainColor;
	StatusText->SetText(Objective.bDone ? DoneMark : OpenMark);
	StatusText->SetColorAndOpacity(Objective.bDone ? DoneColor : KindColor);
	LabelText->SetText(Objective.Label);
	ShowText(CountText, nullptr, Objective.Count);
	CountText->SetColorAndOpacity(Objective.bDone ? DoneColor : KindColor);
	ShowText(KindText, KindBadge, Objective.KindLabel);
	if (KindText) KindText->SetColorAndOpacity(KindColor);
}
void UDungeonStatTileWidget::SetStat(const FDungeonStatViewData& Stat)
{
	LabelText->SetText(Stat.Label);
	ValueText->SetText(Stat.Value);
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
	ApplyViewData(ViewData);
}
void UDungeonViewWidget::NativeTick(const FGeometry& Geometry, float DeltaTime)
{
	Super::NativeTick(Geometry, DeltaTime);
	if (TimeText && ViewData.DeadlineServerTime > 0) RefreshTime();
}
void UDungeonViewWidget::RefreshTime()
{
	if (!TimeText) return;
	const AGameStateBase* GameState = GetWorld() ? GetWorld()->GetGameState() : nullptr;
	if (ViewData.DeadlineServerTime <= 0 || !GameState)
	{
		TimeText->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}
	const int32 Remaining = FMath::Max(0, FMath::CeilToInt(ViewData.DeadlineServerTime - GameState->GetServerWorldTimeSeconds()));
	const FText Clock = FText::FromString(FString::Printf(TEXT("%d:%02d"), Remaining / 60, Remaining % 60));
	TimeText->SetText(ViewData.TimeFormat.IsEmpty() ? Clock : FText::Format(ViewData.TimeFormat, Clock));
	TimeText->SetColorAndOpacity(Remaining <= TimeWarningSeconds ? TimeWarningColor : TimeColor);
	TimeText->SetVisibility(ESlateVisibility::HitTestInvisible);
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
	ShowText(StepText, nullptr, Data.StepText);
	if (StepProgress)
	{
		StepProgress->SetPercent(Data.StepProgress);
		StepProgress->SetVisibility(Data.StepText.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
	Fill(ObjectiveRows, Data.Objectives, &UDungeonObjectiveRowWidget::SetObjective);
	ShowText(ObjectivesDoneText, nullptr, Data.ObjectivesDone);
	if (BossNameText) BossNameText->SetText(Data.BossName);
	if (BossHealthBar) BossHealthBar->SetPercent(Data.BossHealth);
	if (BossPanel) BossPanel->SetVisibility(Data.BossHealth > 0.f ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	Fill(StatTiles, Data.Stats, &UDungeonStatTileWidget::SetStat);
	Fill(RewardSlots, Data.Rewards, &UDungeonRewardEntryWidget::SetReward);
	ShowText(RewardText, nullptr, Data.RewardText);
	ShowText(SummaryText, nullptr, Data.SummaryText);
	ShowText(NewBestText, NewBestBadge, Data.NewBestText);
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
