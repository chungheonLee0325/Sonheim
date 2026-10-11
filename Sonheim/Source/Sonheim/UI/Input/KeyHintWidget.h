#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "KeyHintWidget.generated.h"

class UBorder;
class UInputAction;
class USizeBox;
class UTextBlock;

/**
 * The key an input action is on, for the device the player is using: a key cap for the keyboard, a face button or bumper for a
 * gamepad. It follows the player's mappings and device (UInputHintSubsystem), so no screen writes a key letter itself. Collapses
 * when the action has no key on the current device.
 */
UCLASS(Abstract)
class SONHEIM_API UKeyHintWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category="Input") void SetAction(UInputAction* InAction);
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input") TObjectPtr<UInputAction> Action;
	/** The cap's side for a key or face button; a wide key or bumper grows with its label. */
	UPROPERTY(EditAnywhere, Category="Style") float Size = 30.f;
protected:
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	void Refresh();
	UPROPERTY(meta=(BindWidget)) TObjectPtr<USizeBox> CapSize;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UBorder> Cap;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> KeyText;
private:
	FDelegateHandle Changed;
};
