#include "GameUIScreenWidget.h"
#include "GameUIManagerSubsystem.h"

void UGameUIScreenWidget::CloseSelf()
{
	if (UGameUIManagerSubsystem* Manager = UGameUIManagerSubsystem::Get(this)) Manager->CloseScreen(this);
}
