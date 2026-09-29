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
/** One line of the objective list: a state mark, the kind's tag, the label, the count, and for optional lines their window and note. */
UCLASS(Abstract)
class SONHEIM_API UDungeonObjectiveRowWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	void SetObjective(const FDungeonObjectiveViewData& Objective);
	/** The mark and count take the kind's color while the line is open, DoneColor once it is done and MissedColor once it is missed. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon") FSlateColor FinalColor = FSlateColor(FLinearColor(1.f, 0.7f, 0.2f));
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon") FSlateColor MainColor = FSlateColor(FLinearColor::White);
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon") FSlateColor OptionalColor = FSlateColor(FLinearColor(1.f, 0.7f, 0.2f));
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon") FSlateColor DoneColor = FSlateColor(FLinearColor(0.35f, 0.85f, 0.45f));
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon") FSlateColor MissedColor = FSlateColor(FLinearColor(0.55f, 0.6f, 0.66f));
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon") FText OpenMark = INVTEXT("○");
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon") FText DoneMark = INVTEXT("●");
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon") FText MissedMark = INVTEXT("×");
	/** Drawn in StatusIcon once the line is done or missed; while it is open, the goal's icon. Without one, the text marks. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon") TObjectPtr<UTexture2D> DoneIcon;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon") TObjectPtr<UTexture2D> MissedIcon;
protected:
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> StatusText;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UImage> StatusIcon;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> LabelText;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> CountText;
	// The kind's tag and the plate around it; both hide on a line whose kind has no tag.
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> KindText;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UWidget> KindBadge;
	// An optional line's window and note; each hides while empty.
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> WindowText;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> NoteText;
};
/** One step of the way through the dungeon, with an arrow in front of every step but the first. */
UCLASS(Abstract)
class SONHEIM_API UDungeonStepNodeWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	void SetStep(const FDungeonStepViewData& Step);
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon") FSlateColor DoneColor = FSlateColor(FLinearColor(0.55f, 0.64f, 0.75f));
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon") FSlateColor CurrentColor = FSlateColor(FLinearColor(0.05f, 0.6f, 1.f));
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon") FSlateColor UpcomingColor = FSlateColor(FLinearColor(1.f, 1.f, 1.f, 0.45f));
protected:
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> LabelText;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> ArrowText;
	/** The room's icon, in the name's color. */
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UImage> StepIcon;
};
/** One player of the run: name, health, the crown of the player who started it, and a mark when down. */
UCLASS(Abstract)
class SONHEIM_API UDungeonPartyMemberWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	void SetMember(const FDungeonMemberViewData& Member);
	/** Health bar colors: healthy, below LowHealth, and down. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon") FLinearColor HealthyColor = FLinearColor(0.35f, 0.85f, 0.45f);
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon") FLinearColor LowColor = FLinearColor(1.f, 0.7f, 0.2f);
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon") FLinearColor DownColor = FLinearColor(1.f, 0.3f, 0.22f);
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon", meta=(ClampMin="0", ClampMax="1")) float LowHealth = 0.35f;
protected:
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> NameText;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UProgressBar> HealthBar;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UWidget> OwnerMark;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UWidget> DownMark;
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
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UImage> TileIcon;
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
	/** Shows a room's name in the middle of the screen for AreaTitleSeconds, as a player of the run walks in. */
	void ShowAreaTitle(const FText& Title, const FText& Subtitle);
	UPROPERTY(BlueprintReadOnly, Category="Dungeon") FDungeonStageViewData ViewData;
	/** Seconds the screen of a finished run stays up before it folds away; 0 keeps it until the next run. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon") float FinishedSeconds = 10.f;
	/** The countdown turns TimeWarningColor once this many seconds are left. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon") float TimeWarningSeconds = 30.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon") FSlateColor TimeWarningColor = FSlateColor(FLinearColor(1.f, 0.3f, 0.22f));
	/** The title's color on a won and on a failed run; while the run goes it keeps the color the Widget Blueprint gives it. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon") FSlateColor SucceededTitleColor = FSlateColor(FLinearColor(1.f, 0.7f, 0.2f));
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon") FSlateColor FailedTitleColor = FSlateColor(FLinearColor(1.f, 0.36f, 0.3f));
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon", meta=(ClampMin="0")) float AreaTitleSeconds = 2.5f;
protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& Geometry, float DeltaTime) override;
	void RefreshTime();
	/** Fills the boss's action bar from the server time. */
	void RefreshBossAction();
	/** Shown to participants; a finished run's screen folds away once FinishedSeconds pass. */
	void RefreshShown();
	FTimerHandle FinishedTimer;
	FTimerHandle AreaTitleTimer;
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
	// The dungeon's name, its goal, and the way through it.
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> DungeonTitleText;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> GoalText;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UDynamicEntryBox> StepNodes;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> StepText;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UProgressBar> StepProgress;
	// Counts the stage's time limit down. Screens without a limit never show it.
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> TimeText;
	/** Shown with TimeText, in its color. */
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UImage> TimeIcon;
	// The stage's objective lines and how many are done; the optional lines on their own card, which hides while there is none.
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UDynamicEntryBox> ObjectiveRows;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> ObjectivesDoneText;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UDynamicEntryBox> OptionalRows;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UWidget> OptionalPanel;
	// The players of the run, on a card that hides while the run is not going.
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UDynamicEntryBox> PartyRows;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UWidget> PartyPanel;
	// A room's name as a player walks in.
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UWidget> AreaTitlePanel;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> AreaTitleText;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> AreaSubtitleText;
	// Shown while the boss lives.
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UWidget> BossPanel;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> BossNameText;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UProgressBar> BossHealthBar;
	// What the boss does and how far along, its phase, how close it is to a knockdown, and the capture hint while it can be taken.
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> BossActionText;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UImage> BossActionIcon;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UProgressBar> BossActionBar;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> BossPhaseText;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UImage> BossPhaseIcon;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UProgressBar> BossBreakBar;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> BossHintText;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UImage> BossHintIcon;
	// The result: tiles of figures, reward slots or reward lines, the record, and the new-best badge.
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UDynamicEntryBox> StatTiles;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UDynamicEntryBox> RewardSlots;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> RewardText;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> SummaryText;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UWidget> NewBestBadge;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> NewBestText;
	/** Over the result's title, in the title's color. */
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UImage> OutcomeEmblem;
};
