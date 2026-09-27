#include "DungeonViewWidget.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"
#include "Components/PanelWidget.h"
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
}
void UDungeonRewardEntryWidget::SetReward(const FDungeonRewardViewData& Reward)
{
	Label->SetText(Reward.Label);
	Icon->SetBrushFromTexture(Reward.Icon);
	Icon->SetVisibility(Reward.Icon ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}
void UDungeonViewWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (TimeText) TimeColor = TimeText->GetColorAndOpacity();
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
	if (TitleText) TitleText->SetText(Data.Title);
	if (ObjectiveText) ObjectiveText->SetText(Data.Objective);
	if (CountText) CountText->SetText(Data.CountText);
	ShowText(BranchText, BranchBadge, Data.BranchText);
	ShowText(RewardText, nullptr, Data.RewardText);
	ShowText(SummaryText, nullptr, Data.SummaryText);
	ShowText(NewBestText, NewBestBadge, Data.NewBestText);
	ShowText(DungeonTitleText, nullptr, Data.DungeonTitle);
	ShowText(StepText, nullptr, Data.StepText);
	if (StepProgress)
	{
		StepProgress->SetPercent(Data.StepProgress);
		StepProgress->SetVisibility(Data.StepText.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
	if (BossNameText) BossNameText->SetText(Data.BossName);
	if (BossHealthBar) BossHealthBar->SetPercent(Data.BossHealth);
	if (BossPanel) BossPanel->SetVisibility(Data.BossHealth > 0.f ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	if (RewardList)
	{
		RewardList->ClearChildren();
		if (RewardEntryClass)
			for (const FDungeonRewardViewData& Reward : Data.Rewards)
				if (auto* Entry = CreateWidget<UDungeonRewardEntryWidget>(this, RewardEntryClass))
				{
					Entry->SetReward(Reward);
					RewardList->AddChild(Entry);
				}
		RewardList->SetVisibility(Data.Rewards.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
	RefreshTime();
	if (ObjectiveProgress) ObjectiveProgress->SetPercent(Data.Progress);
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
