#include "GameUIScreenWidget.h"
#include "GameUIManagerSubsystem.h"
#include "GameUISettings.h"

FGameplayTag UGameUIScreenWidget::GetDefaultLayer() const
{
	return GetDefault<UGameUISettings>()->DefaultScreenLayer;
}

void UGameUIScreenWidget::CloseSelf()
{
	if (UGameUIManagerSubsystem* Manager = UGameUIManagerSubsystem::Get(this)) Manager->CloseScreen(this);
}
