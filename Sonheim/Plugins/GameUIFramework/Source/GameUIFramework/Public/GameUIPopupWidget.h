#pragma once

#include "CoreMinimal.h"
#include "GameUIScreenWidget.h"
#include "GameUIPopupWidget.generated.h"

class UGameUIPopupWidget;
DECLARE_DELEGATE_TwoParams(FOnGameUIPopupResult, UGameUIPopupWidget& /*Popup*/, bool /*bConfirmed*/);

/** A screen that asks the player something and answers whoever opened it, once: confirmed or not. The back input and any other
 * closing (its layer emptied, the world ending) answer "not confirmed". What else the answer holds, such as a count, the derived
 * popup keeps for the opener to read from the popup it is handed. Opens in UGameUISettings::DefaultPopupLayer unless its class
 * names a layer. */
UCLASS(Abstract)
class GAMEUIFRAMEWORK_API UGameUIPopupWidget : public UGameUIScreenWidget
{
	GENERATED_BODY()
public:
	UGameUIPopupWidget();
	virtual FGameplayTag GetDefaultLayer() const override;
	/** Set by the opener before the popup answers. */
	FOnGameUIPopupResult OnResult;
	/** Answers the opener and closes the popup; only the first answer counts. */
	UFUNCTION(BlueprintCallable, Category="Popup") void Finish(bool bConfirmed);
protected:
	virtual void NativeOnOpened() override;
	virtual void NativeOnClosed() override;
	virtual bool NativeOnBack() override;
private:
	bool bAnswered = false;
};
