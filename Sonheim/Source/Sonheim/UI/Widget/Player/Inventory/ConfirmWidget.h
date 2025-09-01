#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ConfirmWidget.generated.h"

UDELEGATE()
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnConfirmItemAction, int32, Count, bool, bDiscardMode);

UCLASS()
class SONHEIM_API UConfirmWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeOnInitialized() override;

	// 팝업 초기화: ItemID(아이콘/이름 표시용), MaxCount(스택 수량), bDiscardMode(true=폐기/삭제, false=버리기/바닥)
	UFUNCTION(BlueprintCallable)
	void Setup(int32 InItemID, int32 InMaxCount, bool bInDiscardMode);

	UPROPERTY(BlueprintAssignable, Category="Event")
	FOnConfirmItemAction OnConfirm;

protected:
	UPROPERTY(meta=(BindWidget))
	class UImage* ImgIcon = nullptr;
	UPROPERTY(meta=(BindWidget))
	class UTextBlock* TxtName = nullptr;
	UPROPERTY(meta=(BindWidget))
	class UTextBlock* TxtCaption = nullptr;
	UPROPERTY(meta=(BindWidget))
	class USpinBox* SpinCount = nullptr;
	UPROPERTY(meta=(BindWidget))
	class UButton* BtnConfirm = nullptr;
	UPROPERTY(meta=(BindWidget))
	class UButton* BtnCancel = nullptr;

	int32 ItemID = 0;
	int32 MaxCount = 1;
	bool bDiscardMode = false;


	UFUNCTION()
	void SetItemVisual(int32 InItemID, int32 InMaxCount) const;

	UFUNCTION()
	void OnClickedConfirm();
	UFUNCTION()
	void OnClickedCancel();
	virtual FReply NativeOnKeyDown(const FGeometry& InGeo, const FKeyEvent& InKeyEvent) override;
};
