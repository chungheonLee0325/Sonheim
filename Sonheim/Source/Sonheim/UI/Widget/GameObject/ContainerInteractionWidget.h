#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ContainerInteractionWidget.generated.h"

class UInventoryWidget;
class UContainerWidget;
class ABaseContainer;
class UButton;

UCLASS()
class SONHEIM_API UContainerInteractionWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// UI 컴포넌트
	UPROPERTY(meta = (BindWidget))
	UInventoryWidget* PlayerInventoryWidget;

	UPROPERTY(meta = (BindWidget))
	UContainerWidget* ContainerInventoryWidget;

	UPROPERTY(meta = (BindWidget))
	UButton* CloseButton;

	// 현재 열린 상자
	UPROPERTY()
	ABaseContainer* CurrentContainer;

public:
	// 상자 UI 열기
	UFUNCTION(BlueprintCallable, Category = "Container")
	void OpenContainer(ABaseContainer* Container);

	// 상자 UI 닫기
	UFUNCTION(BlueprintCallable, Category = "Container")
	void CloseContainer();

	UInventoryWidget* GetPlayerInventoryWidget() const {return PlayerInventoryWidget;};

private:
	UFUNCTION()
	void OnCloseButtonClicked();
};