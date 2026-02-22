#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"

#include "Sonheim/UI/System/UIPayloads.h"

#include "CommonModalWidget.generated.h"

UDELEGATE()
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCommonModalResult, bool, bAccepted);

UCLASS()
class SONHEIM_API UCommonModalWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeOnInitialized() override;

	UFUNCTION(BlueprintCallable, Category="UI")
	void Setup(const FModalPayload& InPayload);

	UPROPERTY(BlueprintAssignable, Category="UI")
	FOnCommonModalResult OnResult;

protected:
	UPROPERTY(meta=(BindWidgetOptional))
	class UButton* BtnPrimary = nullptr;

	UPROPERTY(meta=(BindWidgetOptional))
	class UButton* BtnSecondary = nullptr;

	UFUNCTION(BlueprintImplementableEvent, Category="UI")
	void ApplyPayload(const FModalPayload& Payload);

private:
	UFUNCTION()
	void OnClickedPrimary();
	UFUNCTION()
	void OnClickedSecondary();
};
