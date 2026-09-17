#include "DungeonViewWidget.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"
#include "GameFramework/GameStateBase.h"
void UDungeonViewWidget::NativeConstruct()
{
	Super::NativeConstruct();
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
	TimeText->SetText(FText::FromString(FString::Printf(TEXT("남은 시간 %d:%02d"), Remaining / 60, Remaining % 60)));
	TimeText->SetVisibility(ESlateVisibility::HitTestInvisible);
}
void UDungeonViewWidget::ApplyViewData(const FDungeonStageViewData& Data)
{
	ViewData = Data;
	if (TitleText) TitleText->SetText(Data.Title);
	if (ObjectiveText) ObjectiveText->SetText(Data.Objective);
	if (CountText) CountText->SetText(Data.CountText);
	if (BranchText) BranchText->SetText(Data.BranchText);
	if (UWidget* BranchRow = BranchBadge ? BranchBadge.Get() : BranchText.Get())
		BranchRow->SetVisibility(Data.BranchText.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	if (RewardText)
	{
		RewardText->SetText(Data.RewardText);
		RewardText->SetVisibility(Data.RewardText.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
	if (SummaryText)
	{
		SummaryText->SetText(Data.SummaryText);
		SummaryText->SetVisibility(Data.SummaryText.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
	RefreshTime();
	if (ObjectiveProgress) ObjectiveProgress->SetPercent(Data.Progress);
}
