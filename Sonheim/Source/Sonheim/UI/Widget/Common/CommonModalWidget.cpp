#include "CommonModalWidget.h"

#include "Components/Button.h"
#include "Sonheim/UI/System/UIStackSubsystem.h"

void UCommonModalWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (BtnPrimary) BtnPrimary->OnClicked.AddDynamic(this, &UCommonModalWidget::OnClickedPrimary);
	if (BtnSecondary) BtnSecondary->OnClicked.AddDynamic(this, &UCommonModalWidget::OnClickedSecondary);
}

void UCommonModalWidget::Setup(const FModalPayload& InPayload)
{
	ApplyPayload(InPayload);
}

void UCommonModalWidget::OnClickedPrimary()
{
	OnResult.Broadcast(true);
	if (APlayerController* PC = GetOwningPlayer())
	{
		if (ULocalPlayer* LP = PC->GetLocalPlayer())
		{
			if (UUIStackSubsystem* UI = ULocalPlayer::GetSubsystem<UUIStackSubsystem>(LP))
			{
				UI->CloseModalWidget(this);
				return;
			}
		}
	}
	RemoveFromParent();
}

void UCommonModalWidget::OnClickedSecondary()
{
	OnResult.Broadcast(false);
	if (APlayerController* PC = GetOwningPlayer())
	{
		if (ULocalPlayer* LP = PC->GetLocalPlayer())
		{
			if (UUIStackSubsystem* UI = ULocalPlayer::GetSubsystem<UUIStackSubsystem>(LP))
			{
				UI->CloseModalWidget(this);
				return;
			}
		}
	}
	RemoveFromParent();
}
