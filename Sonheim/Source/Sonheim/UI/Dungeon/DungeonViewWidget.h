#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DungeonViewData.h"
#include "DungeonViewWidget.generated.h"
class UTextBlock;
class UProgressBar;
class UWidget;
UCLASS()
class SONHEIM_API UDungeonViewWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	void ApplyViewData(const FDungeonStageViewData& Data);
	UPROPERTY(BlueprintReadOnly, Category="Dungeon") FDungeonStageViewData ViewData;
protected:
	virtual void NativeConstruct() override;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> TitleText;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> ObjectiveText;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> CountText;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> BranchText;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UProgressBar> ObjectiveProgress;
	// Frame around BranchText, such as a badge; it hides with an empty branch. Without it, BranchText hides alone.
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UWidget> BranchBadge;
	// Result screen only. The HUD leaves both out and keeps working.
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> RewardText;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> SummaryText;
};
