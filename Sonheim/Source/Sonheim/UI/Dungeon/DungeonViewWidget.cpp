#include "DungeonViewWidget.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"
void UDungeonViewWidget::NativeConstruct()
{
	Super::NativeConstruct();
	ApplyViewData(ViewData);
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
	if (ObjectiveProgress) ObjectiveProgress->SetPercent(Data.Progress);
}
