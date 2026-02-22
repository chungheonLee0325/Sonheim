#include "QuestAcceptWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Sonheim/AreaObject/Player/SonheimPlayerController.h"
#include "Sonheim/UI/System/UIStackSubsystem.h"

void UQuestAcceptWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (BtnAccept) BtnAccept->OnClicked.AddDynamic(this, &UQuestAcceptWidget::OnClickedAccept);
	if (BtnDecline) BtnDecline->OnClicked.AddDynamic(this, &UQuestAcceptWidget::OnClickedDecline);
}

void UQuestAcceptWidget::Setup(int32 InQuestID, const FText& InTitle, const FText& InDescription)
{
	QuestID = InQuestID;
	if (TxtTitle) TxtTitle->SetText(InTitle);
	if (TxtDescription) TxtDescription->SetText(InDescription);
	ApplyQuestData(InQuestID, InTitle, InDescription);
}

void UQuestAcceptWidget::OnClickedAccept()
{
	if (APlayerController* PC = GetOwningPlayer())
	{
		if (ASonheimPlayerController* SPC = Cast<ASonheimPlayerController>(PC))
		{
			SPC->RequestAcceptQuest(QuestID);
		}
	}

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

void UQuestAcceptWidget::OnClickedDecline()
{
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
