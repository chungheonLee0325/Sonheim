// ConfirmWidget.cpp
#include "ConfirmWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/SpinBox.h"
#include "Components/Button.h"
#include "InputCoreTypes.h"
#include "Sonheim/GameManager/SonheimGameInstance.h"

void UConfirmWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (BtnConfirm) BtnConfirm->OnClicked.AddDynamic(this, &UConfirmWidget::OnClickedConfirm);
	if (BtnCancel) BtnCancel->OnClicked.AddDynamic(this, &UConfirmWidget::OnClickedCancel);
}

void UConfirmWidget::Setup(int32 InItemID, int32 InMaxCount, bool bInDiscardMode)
{
	ItemID = InItemID;
	MaxCount = FMath::Max(1, InMaxCount);
	bDiscardMode = bInDiscardMode;

	const FText Caption = bDiscardMode
		                      ? FText::FromString(TEXT("정말로 폐기하시겠습니까?\n(아이템이 영구 삭제됩니다)"))
		                      : FText::FromString(TEXT("정말로 버리시겠습니까?\n(아이템이 바닥에 떨어집니다)"));
	if (TxtCaption) TxtCaption->SetText(Caption);

	const bool bAllowCount = (MaxCount > 1);
	if (SpinCount)
	{
		SpinCount->SetMinValue(1.0);
		SpinCount->SetMaxValue((double)MaxCount);
		SpinCount->SetValue(1.0);
		SpinCount->SetVisibility(bAllowCount ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}

	SetItemVisual(ItemID, MaxCount);
}

void UConfirmWidget::SetItemVisual(int32 InItemID, int32 InMaxCount) const
{
	auto ItemData = USonheimGameInstance::Get(GetWorld())->GetDataItem(InItemID);
	if (ItemData)
	{
		ImgIcon->SetBrushFromTexture(ItemData->ItemIcon);
		TxtName->SetText(ItemData->ItemName);
	}
}

void UConfirmWidget::OnClickedConfirm()
{
	int32 FinalCount = 1;
	if (SpinCount && SpinCount->GetVisibility() == ESlateVisibility::Visible)
	{
		FinalCount = FMath::Clamp((int32)FMath::RoundHalfFromZero(SpinCount->GetValue()), 1, MaxCount);
	}
	OnConfirm.Broadcast(FinalCount, bDiscardMode);
	RemoveFromParent();
}

void UConfirmWidget::OnClickedCancel()
{
	RemoveFromParent();
}

FReply UConfirmWidget::NativeOnKeyDown(const FGeometry& InGeo, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	if (Key == EKeys::Enter || Key == EKeys::Virtual_Accept)
	{
		OnClickedConfirm();
		return FReply::Handled();
	}
	if (Key == EKeys::Escape || Key == EKeys::Virtual_Back)
	{
		OnClickedCancel();
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeo, InKeyEvent);
}
