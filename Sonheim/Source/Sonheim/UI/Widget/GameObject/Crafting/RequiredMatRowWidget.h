#pragma once
#include "Blueprint/UserWidget.h"
#include "RequiredMatRowWidget.generated.h"

class USlotWidget;
class USizeBox;
class UBorder;
class UTextBlock;
class USonheimGameInstance;

UCLASS()
class SONHEIM_API URequiredMatRowWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category="Row")
	TSubclassOf<USlotWidget> SlotWidgetClass;

	void SetupOnce();
	void UpdateRow(USonheimGameInstance* GI, int32 MatID, int32 Need, int32 Have);

protected:
	virtual void NativeConstruct() override
	{
		Super::NativeConstruct();
		SetupOnce();
	}

	UPROPERTY(meta=(BindWidget))
	USizeBox* SlotBox = nullptr;
	UPROPERTY(meta=(BindWidget))
	UBorder* CountBorder = nullptr;
	UPROPERTY(meta=(BindWidget))
	UTextBlock* CountText = nullptr;

private:
	UPROPERTY()
	USlotWidget* CachedSlotWidget = nullptr;
};
