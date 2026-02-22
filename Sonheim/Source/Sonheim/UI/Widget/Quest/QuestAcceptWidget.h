#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"

#include "QuestAcceptWidget.generated.h"

class UButton;
class UTextBlock;

UCLASS()
class SONHEIM_API UQuestAcceptWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeOnInitialized() override;

	UFUNCTION(BlueprintCallable, Category="Quest")
	void Setup(int32 InQuestID, const FText& InTitle, const FText& InDescription);

protected:
	UPROPERTY(meta=(BindWidgetOptional))
	class UButton* BtnAccept = nullptr;

	UPROPERTY(meta=(BindWidgetOptional))
	class UButton* BtnDecline = nullptr;

	UPROPERTY(meta=(BindWidgetOptional))
	UTextBlock* TxtTitle = nullptr;

	UPROPERTY(meta=(BindWidgetOptional))
	UTextBlock* TxtDescription = nullptr;

	UFUNCTION(BlueprintImplementableEvent, Category="Quest")
	void ApplyQuestData(int32 InQuestID, const FText& InTitle, const FText& InDescription);

private:
	UFUNCTION()
	void OnClickedAccept();

	UFUNCTION()
	void OnClickedDecline();

	int32 QuestID = 0;
};
