#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"

#include "QuestJournalWidget.generated.h"

class UQuestComponent;
class UTextBlock;
class UVerticalBox;
class UQuestJournalEntryWidget;

UCLASS()
class SONHEIM_API UQuestJournalWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="Quest")
	void BindQuestComponent(UQuestComponent* InQuestComponent);

	UFUNCTION(BlueprintPure, Category="Quest")
	UQuestComponent* GetQuestComponent() const { return QuestComponent.Get(); }

	UFUNCTION(BlueprintCallable, Category="Quest")
	void SetSelectedQuest(int32 QuestID);

	UFUNCTION(BlueprintPure, Category="Quest")
	int32 GetSelectedQuest() const { return SelectedQuestID; }

protected:
	UFUNCTION(BlueprintImplementableEvent, Category="Quest")
	void OnQuestDataChanged();

	UFUNCTION(BlueprintImplementableEvent, Category="Quest")
	void OnQuestSelected(int32 QuestID);

	UPROPERTY(meta=(BindWidgetOptional))
	UTextBlock* TxtQuestList = nullptr;

	UPROPERTY(meta=(BindWidgetOptional))
	UVerticalBox* QuestListBox = nullptr;

	UPROPERTY(EditDefaultsOnly, Category="Quest")
	TSubclassOf<UQuestJournalEntryWidget> EntryWidgetClass;

	UPROPERTY(meta=(BindWidgetOptional))
	UTextBlock* TxtDetailTitle = nullptr;

	UPROPERTY(meta=(BindWidgetOptional))
	UTextBlock* TxtDetailDescription = nullptr;

	UPROPERTY(meta=(BindWidgetOptional))
	UTextBlock* TxtDetailObjectives = nullptr;

	UPROPERTY(meta=(BindWidgetOptional))
	UTextBlock* TxtDetailState = nullptr;

private:
	UFUNCTION()
	void HandleQuestListChanged();

	void UpdateQuestListFallback();
	void RebuildQuestList();
	void UpdateDetailPanel(int32 QuestID);
	void SelectQuestInternal(int32 QuestID);

	UFUNCTION()
	void HandleEntryClicked(int32 QuestID);

	UPROPERTY()
	TWeakObjectPtr<UQuestComponent> QuestComponent;

	int32 SelectedQuestID = 0;
};
