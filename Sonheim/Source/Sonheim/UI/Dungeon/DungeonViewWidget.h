#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DungeonViewData.h"
#include "DungeonViewWidget.generated.h"
class UDynamicEntryBox;
class UImage;
class UTextBlock;
class UProgressBar;
class UWidget;
/** One line of the objective list: a state mark, the kind's tag, the label and the count. */
UCLASS(Abstract)
class SONHEIM_API UDungeonObjectiveRowWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	void SetObjective(const FDungeonObjectiveViewData& Objective);
	/** The mark and count take the kind's color while the line is open and DoneColor once it is done. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon") FSlateColor FinalColor = FSlateColor(FLinearColor(1.f, 0.7f, 0.2f));
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon") FSlateColor MainColor = FSlateColor(FLinearColor::White);
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon") FSlateColor OptionalColor = FSlateColor(FLinearColor(0.05f, 0.6f, 1.f));
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon") FSlateColor DoneColor = FSlateColor(FLinearColor(0.35f, 0.85f, 0.45f));
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon") FText OpenMark = INVTEXT("○");
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon") FText DoneMark = INVTEXT("●");
protected:
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> StatusText;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> LabelText;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> CountText;
	// The kind's tag and the plate around it; both hide on a line whose kind has no tag.
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> KindText;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UWidget> KindBadge;
};
/** One figure of the result: a label over a value. */
UCLASS(Abstract)
class SONHEIM_API UDungeonStatTileWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	void SetStat(const FDungeonStatViewData& Stat);
protected:
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> LabelText;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> ValueText;
};
/** One reward slot of the result: the item's icon, its name and how many. */
UCLASS(Abstract)
class SONHEIM_API UDungeonRewardEntryWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	void SetReward(const FDungeonRewardViewData& Reward);
protected:
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UImage> Icon;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> NameText;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> CountText;
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
	/** The title's color on a won and on a failed run; while the run goes it keeps the color the Widget Blueprint gives it. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon") FSlateColor SucceededTitleColor = FSlateColor(FLinearColor(1.f, 0.7f, 0.2f));
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon") FSlateColor FailedTitleColor = FSlateColor(FLinearColor(1.f, 0.36f, 0.3f));
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
	FSlateColor TitleColor;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> TitleText;
	// Every other part is optional, so a screen shows only what it lays out.
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> ObjectiveText;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> CountText;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UProgressBar> ObjectiveProgress;
	// The branch's label and the plate around it; it hides with an empty branch. Without the plate, BranchText hides alone.
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> BranchText;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UWidget> BranchBadge;
	// The dungeon's name and the step of the way through.
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> DungeonTitleText;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> StepText;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UProgressBar> StepProgress;
	// Counts the stage's time limit down. Screens without a limit never show it.
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> TimeText;
	// The objective list: one entry of the box's entry class per line, and how many lines are done.
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UDynamicEntryBox> ObjectiveRows;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> ObjectivesDoneText;
	// Shown while the boss lives.
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UWidget> BossPanel;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> BossNameText;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UProgressBar> BossHealthBar;
	// The result: tiles of figures, reward slots or reward lines, the record, and the new-best badge.
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UDynamicEntryBox> StatTiles;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UDynamicEntryBox> RewardSlots;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> RewardText;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> SummaryText;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UWidget> NewBestBadge;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> NewBestText;
};
