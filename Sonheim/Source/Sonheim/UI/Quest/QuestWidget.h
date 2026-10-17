#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameUIScreenWidget.h"
#include "QuestView.h"
#include "QuestWidget.generated.h"

class UBorder;
class UButton;
class UDynamicEntryBox;
class UImage;
class UTextBlock;
class UQuestComponent;

/** One objective: a mark, its line and its count; the mark turns to a check and the line dims once it is done. */
UCLASS(Abstract)
class SONHEIM_API UQuestObjectiveRowWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	void Show(const FQuestObjectiveView& Objective);
protected:
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UImage> MarkImage;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> LabelText;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> CountText;
	UPROPERTY(EditAnywhere, Category="Style") TObjectPtr<UTexture2D> OpenIcon;
	UPROPERTY(EditAnywhere, Category="Style") TObjectPtr<UTexture2D> DoneIcon;
	UPROPERTY(EditAnywhere, Category="Style") FLinearColor OpenColor = FLinearColor(1.f, 0.7f, 0.2f, 1.f);
	UPROPERTY(EditAnywhere, Category="Style") FLinearColor DoneColor = FLinearColor(0.55f, 0.64f, 0.75f, 1.f);
};

/** One quest in a list: its title, where it stands, and a mark while it is tracked. */
UCLASS(Abstract)
class SONHEIM_API UQuestListEntryWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	void Show(const FQuestView& Quest, bool bSelected);
	TDelegate<void(int32 QuestID)> OnPicked;
protected:
	virtual void NativeOnInitialized() override;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton> PickButton;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UBorder> Frame;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> TitleText;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> StateText;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UImage> TrackedMark;
	UPROPERTY(EditAnywhere, Category="Style") FLinearColor FrameColor = FLinearColor(1.f, 1.f, 1.f, 0.06f);
	UPROPERTY(EditAnywhere, Category="Style") FLinearColor SelectedColor = FLinearColor(0.05f, 0.6f, 1.f, 0.35f);
	UPROPERTY(EditAnywhere, Category="Style") FLinearColor ReadyColor = FLinearColor(1.f, 0.7f, 0.2f, 1.f);
	UPROPERTY(EditAnywhere, Category="Style") FLinearColor MutedColor = FLinearColor(0.55f, 0.64f, 0.75f, 1.f);
private:
	UFUNCTION() void HandleClicked();
	int32 QuestID = 0;
};

/**
 * A screen with a quest list on the left and the picked quest's detail on the right: the quest dialog and the journal. The
 * detail's widgets are bound here, so both screens draw a quest the same way. The screen redraws whenever the local player's
 * quests change.
 */
UCLASS(Abstract)
class SONHEIM_API UQuestScreenWidget : public UGameUIScreenWidget
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category="Quest") void Pick(int32 QuestID);
	UFUNCTION(BlueprintPure, Category="Quest") int32 GetPicked() const { return Picked; }
protected:
	virtual void NativeOnOpened() override;
	virtual void NativeOnClosed() override;
	/** The quests the list shows, in order. */
	virtual TArray<FQuestView> ListQuests(const UQuestComponent& Quests) const PURE_VIRTUAL(UQuestScreenWidget::ListQuests, return {};);
	/** After the list and the detail are drawn: the screen's own buttons. */
	virtual void DrawActions(const FQuestView* Quest) {}
	UFUNCTION() void Redraw();
	UQuestComponent* Quests() const;
	const FQuestView* PickedView() const;

	UPROPERTY(meta=(BindWidget)) TObjectPtr<UDynamicEntryBox> QuestList;
	/** Shown in place of the list and the detail when the list is empty. */
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> EmptyText;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UWidget> DetailPanel;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> TitleText;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> StateText;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> DescriptionText;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> StepText;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UDynamicEntryBox> ObjectiveBox;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UDynamicEntryBox> RewardBox;
	UPROPERTY(EditAnywhere, Category="Style") FLinearColor RewardColor = FLinearColor(1.f, 1.f, 1.f, 0.1f);
	/** The square of a reward's item slot. */
	UPROPERTY(EditAnywhere, Category="Style") float RewardSlotSize = 56.f;

	TArray<FQuestView> Shown;
	int32 Picked = 0;
	TWeakObjectPtr<UQuestComponent> Bound;
};

/** What a quest giver has for the player, opened by F on them: a quest to take with 수락, or one to report with 보고. */
UCLASS(Abstract)
class SONHEIM_API UQuestDialogWidget : public UQuestScreenWidget
{
	GENERATED_BODY()
public:
	/** The person spoken to; the server checks the player stands at them when a button asks it something. */
	void SetTarget(AActor* InTarget);
	/** Takes or reports the picked quest, as the action button does, then closes the dialog. */
	UFUNCTION(BlueprintCallable, Category="Quest") void Act();
protected:
	virtual void NativeOnInitialized() override;
	virtual TArray<FQuestView> ListQuests(const UQuestComponent& Quests) const override;
	virtual void DrawActions(const FQuestView* Quest) override;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> SpeakerText;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton> ActionButton;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> ActionText;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton> CloseButton;
private:
	UFUNCTION() void HandleClose();
	TWeakObjectPtr<AActor> Target;
	FName TargetName;
};

/** The player's quests, opened with J: each one's steps and objectives, and buttons to track or abandon it. */
UCLASS(Abstract)
class SONHEIM_API UQuestJournalWidget : public UQuestScreenWidget
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category="Quest") void ToggleTracked();
	UFUNCTION(BlueprintCallable, Category="Quest") void Abandon();
	/** Which quests the list shows (ref 09's filter tabs): 0 the open ones, 1 the finished ones, 2 all. */
	UFUNCTION(BlueprintCallable, Category="Quest") void SetFilter(int32 InFilter);
protected:
	UFUNCTION() void ShowOpen() { SetFilter(0); }
	UFUNCTION() void ShowDone() { SetFilter(1); }
	UFUNCTION() void ShowAll() { SetFilter(2); }
	void DrawFilters();
	int32 Filter = 0;
	/** The filter tabs and their plates, lit on the one showing. */
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UButton> FilterOpenButton;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UButton> FilterDoneButton;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UButton> FilterAllButton;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UBorder> FilterOpenPlate;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UBorder> FilterDonePlate;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UBorder> FilterAllPlate;
	UPROPERTY(EditAnywhere, Category="Style") FLinearColor FilterOn = FLinearColor(0.0252f, 0.5776f, 1.f, 1.f);
	UPROPERTY(EditAnywhere, Category="Style") FLinearColor FilterOff = FLinearColor(1.f, 1.f, 1.f, 0.06f);
	virtual void NativeOnInitialized() override;
	virtual TArray<FQuestView> ListQuests(const UQuestComponent& Quests) const override;
	virtual void DrawActions(const FQuestView* Quest) override;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton> TrackButton;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> TrackText;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton> AbandonButton;
};

/** One tracked quest on the HUD: its title over its open objectives. */
UCLASS(Abstract)
class SONHEIM_API UQuestTrackerEntryWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	void Show(const FQuestView& Quest);
protected:
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> TitleText;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UDynamicEntryBox> ObjectiveBox;
};

/** The HUD's quest card: the tracked quests that are not finished, a few at most, as they change. The card folds away while
 * there are none; the widget itself stays, so it keeps ticking until the player's quests arrive. */
UCLASS(Abstract)
class SONHEIM_API UQuestTrackerWidget : public UUserWidget
{
	GENERATED_BODY()
protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	/** The player's quests can arrive after the HUD; until they do, the card looks again each second. */
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	UFUNCTION() void Redraw();
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UWidget> Card;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UDynamicEntryBox> EntryBox;
	UPROPERTY(EditAnywhere, Category="Quest") int32 MaxShown = 3;
private:
	void Bind();
	TWeakObjectPtr<UQuestComponent> Bound;
	float SinceLook = 0.f;
};
