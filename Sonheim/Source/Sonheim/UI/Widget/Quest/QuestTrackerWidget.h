#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"

#include "QuestTrackerWidget.generated.h"

class UQuestComponent;
class UTextBlock;

UCLASS()
class SONHEIM_API UQuestTrackerWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="Quest")
	void BindQuestComponent(UQuestComponent* InQuestComponent);

	UFUNCTION(BlueprintPure, Category="Quest")
	UQuestComponent* GetQuestComponent() const { return QuestComponent.Get(); }

protected:
	UPROPERTY(meta=(BindWidgetOptional))
	UTextBlock* TxtTrackerTitle = nullptr;

	UPROPERTY(meta=(BindWidgetOptional))
	UTextBlock* TxtTrackerObjectives = nullptr;

	UFUNCTION(BlueprintImplementableEvent, Category="Quest")
	void OnQuestDataChanged();

private:
	UFUNCTION()
	void HandleQuestListChanged();

	UPROPERTY()
	TWeakObjectPtr<UQuestComponent> QuestComponent;
};
