#pragma once
#include "CoreMinimal.h"
#include "GameUIPopupWidget.h"
#include "ConfirmWidget.generated.h"

class UTexture2D;

/** What the drop/discard popup asks about: the item, how many of it there are, and whether it is thrown away for good. */
struct FItemQuantityView
{
	UTexture2D* Icon = nullptr;
	FText Name;
	int32 MaxCount = 1;
	/** True: discarded for good; false: dropped on the ground. */
	bool bDiscard = false;
};

/** Asks how many of an item to drop or discard (UI.Popup.ItemQuantity); the opener reads GetCount from its answer. */
UCLASS()
class SONHEIM_API UConfirmWidget : public UGameUIPopupWidget
{
	GENERATED_BODY()

public:
	void Setup(const FItemQuantityView& View);
	/** The count picked, 1 to the item's count. */
	int32 GetCount() const;

protected:
	virtual void NativeOnInitialized() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeo, const FKeyEvent& InKeyEvent) override;

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

	int32 MaxCount = 1;

	UFUNCTION()
	void OnClickedConfirm();
	UFUNCTION()
	void OnClickedCancel();
};
