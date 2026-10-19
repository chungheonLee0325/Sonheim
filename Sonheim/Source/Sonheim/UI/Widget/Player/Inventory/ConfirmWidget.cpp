// ConfirmWidget.cpp
#include "ConfirmWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/SpinBox.h"
#include "Components/Button.h"
#include "InputCoreTypes.h"

void UConfirmWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (BtnConfirm) BtnConfirm->OnClicked.AddDynamic(this, &UConfirmWidget::OnClickedConfirm);
	if (BtnCancel) BtnCancel->OnClicked.AddDynamic(this, &UConfirmWidget::OnClickedCancel);
}

void UConfirmWidget::Setup(const FItemQuantityView& View)
{
	MaxCount = FMath::Max(1, View.MaxCount);

	const FText Caption = View.bDiscard
		                      ? FText::FromString(TEXT("정말로 폐기하시겠습니까?\n(아이템이 영구 삭제됩니다)"))
		                      : FText::FromString(TEXT("정말로 버리시겠습니까?\n(아이템이 바닥에 떨어집니다)"));
	if (TxtCaption) TxtCaption->SetText(Caption);

	if (SpinCount)
	{
		SpinCount->SetMinValue(1.0);
		SpinCount->SetMaxValue((double)MaxCount);
		SpinCount->SetValue(1.0);
		SpinCount->SetVisibility(MaxCount > 1 ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	if (View.Icon) ImgIcon->SetBrushFromTexture(View.Icon);
	TxtName->SetText(View.Name);
}

int32 UConfirmWidget::GetCount() const
{
	if (!SpinCount || SpinCount->GetVisibility() != ESlateVisibility::Visible) return 1;
	return FMath::Clamp((int32)FMath::RoundHalfFromZero(SpinCount->GetValue()), 1, MaxCount);
}

void UConfirmWidget::OnClickedConfirm()
{
	Finish(true);
}

void UConfirmWidget::OnClickedCancel()
{
	Finish(false);
}

FReply UConfirmWidget::NativeOnKeyDown(const FGeometry& InGeo, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	if (Key == EKeys::Enter || Key == EKeys::Virtual_Accept)
	{
		Finish(true);
		return FReply::Handled();
	}
	if (Key == EKeys::Escape || Key == EKeys::Virtual_Back)
	{
		Finish(false);
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeo, InKeyEvent);
}
