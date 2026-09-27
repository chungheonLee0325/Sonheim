#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DungeonViewData.h"
#include "DungeonViewWidget.generated.h"
class UImage;
class UPanelWidget;
class UTextBlock;
class UProgressBar;
class UWidget;
/** One reward line on the result screen: the item's icon and its label. */
UCLASS(Abstract)
class SONHEIM_API UDungeonRewardEntryWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	void SetReward(const FDungeonRewardViewData& Reward);
protected:
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UImage> Icon;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> Label;
};
UCLASS()
class SONHEIM_API UDungeonViewWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	void ApplyViewData(const FDungeonStageViewData& Data);
	UPROPERTY(BlueprintReadOnly, Category="Dungeon") FDungeonStageViewData ViewData;
	/** Seconds the screen of a finished run stays up before it folds away; 0 keeps it until the next run. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon") float FinishedSeconds = 10.f;
	/** The countdown turns TimeWarningColor once this many seconds are left. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon") float TimeWarningSeconds = 30.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon") FSlateColor TimeWarningColor = FSlateColor(FLinearColor(1.f, 0.3f, 0.22f));
	/** Entry made for each reward in RewardList. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon") TSubclassOf<UDungeonRewardEntryWidget> RewardEntryClass;
protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& Geometry, float DeltaTime) override;
	void RefreshTime();
	/** Shown to participants; a finished run's screen folds away once FinishedSeconds pass. */
	void RefreshShown();
	FTimerHandle FinishedTimer;
	int32 FinishedRevision = -1;
	bool bFinishedExpired = false;
	FSlateColor TimeColor;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> TitleText;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> ObjectiveText;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> CountText;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> BranchText;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UProgressBar> ObjectiveProgress;
	// Frame around BranchText, such as a badge; it hides with an empty branch. Without it, BranchText hides alone.
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UWidget> BranchBadge;
	// Result screen only. The HUD leaves them out and keeps working.
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> RewardText;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UPanelWidget> RewardList;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> SummaryText;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UWidget> NewBestBadge;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> NewBestText;
	// Counts the stage's time limit down. Screens without a limit never show it.
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> TimeText;
	// The dungeon's name and the step of the way through.
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> DungeonTitleText;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> StepText;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UProgressBar> StepProgress;
	// Shown while the boss lives.
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UWidget> BossPanel;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> BossNameText;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UProgressBar> BossHealthBar;
};
