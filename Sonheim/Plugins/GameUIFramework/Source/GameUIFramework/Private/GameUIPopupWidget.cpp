#include "GameUIPopupWidget.h"
#include "GameUISettings.h"

UGameUIPopupWidget::UGameUIPopupWidget()
{
	Back = EGameUIBack::Custom;
	bKeepWhenClosed = false;
}

FGameplayTag UGameUIPopupWidget::GetDefaultLayer() const
{
	return GetDefault<UGameUISettings>()->DefaultPopupLayer;
}

void UGameUIPopupWidget::NativeOnOpened()
{
	bAnswered = false;
	Super::NativeOnOpened();
}

void UGameUIPopupWidget::Finish(const bool bConfirmed)
{
	if (bAnswered) return;
	bAnswered = true;
	// Unbound first, so an opener that opens another popup from its answer gets a clean delegate on it.
	const FOnGameUIPopupResult Answer = OnResult;
	OnResult.Unbind();
	Answer.ExecuteIfBound(*this, bConfirmed);
	CloseSelf();
}

void UGameUIPopupWidget::NativeOnClosed()
{
	// Closed without an answer: the opener still hears back, as a cancel. CloseSelf finds the popup already out of its layer.
	Finish(false);
	Super::NativeOnClosed();
}

bool UGameUIPopupWidget::NativeOnBack()
{
	Finish(false);
	return true;
}
