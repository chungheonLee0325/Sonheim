#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"

#include "QuestTrackerWidget.generated.h"

class UQuestComponent;

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
	UFUNCTION(BlueprintImplementableEvent, Category="Quest")
	void OnQuestDataChanged();

private:
	UFUNCTION()
	void HandleQuestListChanged();

	UPROPERTY()
	TWeakObjectPtr<UQuestComponent> QuestComponent;
};
