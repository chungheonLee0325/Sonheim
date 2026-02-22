#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"

#include "Sonheim/UI/System/UIPayloads.h"

#include "CommonToastWidget.generated.h"

UCLASS()
class SONHEIM_API UCommonToastWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="UI")
	void Setup(const FToastPayload& InPayload);

protected:
	UFUNCTION(BlueprintImplementableEvent, Category="UI")
	void ApplyPayload(const FToastPayload& Payload);
};
