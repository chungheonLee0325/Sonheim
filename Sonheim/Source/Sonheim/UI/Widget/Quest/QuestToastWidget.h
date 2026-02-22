#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"

#include "QuestToastWidget.generated.h"

class UTextBlock;

UCLASS()
class SONHEIM_API UQuestToastWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="Quest")
	void Setup(const FText& InText);

protected:
	UFUNCTION(BlueprintImplementableEvent, Category="Quest")
	void ApplyText(const FText& InText);

	UPROPERTY(meta=(BindWidgetOptional))
	UTextBlock* TxtToast = nullptr;
};
