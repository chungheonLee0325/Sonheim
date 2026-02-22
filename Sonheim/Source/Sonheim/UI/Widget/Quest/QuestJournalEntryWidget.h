#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Sonheim/Quest/QuestData.h"

#include "QuestJournalEntryWidget.generated.h"

USTRUCT(BlueprintType)
struct FQuestJournalEntryData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	int32 QuestID = 0;

	UPROPERTY(BlueprintReadOnly)
	FText Title;

	UPROPERTY(BlueprintReadOnly)
	EQuestState State = EQuestState::None;

	UPROPERTY(BlueprintReadOnly)
	int32 CurrentStepIndex = 0;

	UPROPERTY(BlueprintReadOnly)
	bool bTracked = false;
};

UDELEGATE()
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnQuestEntryClicked, int32, QuestID);

class UButton;
class UTextBlock;

UCLASS()
class SONHEIM_API UQuestJournalEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeOnInitialized() override;

	UFUNCTION(BlueprintCallable, Category="Quest")
	void Setup(const FQuestJournalEntryData& InData);

	UPROPERTY(BlueprintAssignable, Category="Quest")
	FOnQuestEntryClicked OnClicked;

protected:
	UPROPERTY(meta=(BindWidgetOptional))
	UButton* BtnSelect = nullptr;

	UPROPERTY(meta=(BindWidgetOptional))
	UTextBlock* TxtTitle = nullptr;

	UPROPERTY(meta=(BindWidgetOptional))
	UTextBlock* TxtState = nullptr;

	UPROPERTY(meta=(BindWidgetOptional))
	UTextBlock* TxtStep = nullptr;

	UPROPERTY(meta=(BindWidgetOptional))
	UTextBlock* TxtTracked = nullptr;

	UFUNCTION(BlueprintImplementableEvent, Category="Quest")
	void ApplyEntryData(const FQuestJournalEntryData& InData);

private:
	UFUNCTION()
	void HandleClicked();

	int32 QuestID = 0;
};
