// ContainerInteractionWidget.cpp
#include "ContainerInteractionWidget.h"
#include "ContainerWidget.h"
#include "Components/Button.h"
#include "Sonheim/UI/Widget/Player/Inventory/InventoryWidget.h"
#include "Sonheim/AreaObject/Player/SonheimPlayerController.h"
#include "Sonheim/AreaObject/Player/SonheimPlayerState.h"
#include "Sonheim/AreaObject/Player/Utility/InventoryComponent.h"
#include "Sonheim/GameObject/Buildings/Storage/BaseContainer.h"
#include "GameUIManagerSubsystem.h"
#include "Sonheim/UI/System/UITags.h"

void UContainerInteractionWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (CloseButton) CloseButton->OnClicked.AddDynamic(this, &UContainerInteractionWidget::OnCloseButtonClicked);
}

void UContainerInteractionWidget::NativeOnClosed()
{
	// Tab 등으로 스택이 먼저 닫아도 서버의 상자를 놓아준다.
	ReleaseContainer();
	// 닫힌 동안은 가방이 바뀌어도 다시 그리지 않는다. 다시 열 때 OpenContainer가 묶고 새로 채운다.
	if (InventoryComponent.IsValid() && PlayerInventoryWidget)
	{
		InventoryComponent->OnInventoryChanged.RemoveDynamic(PlayerInventoryWidget, &UInventoryWidget::UpdateInventoryFromData);
		InventoryComponent->OnEquipmentChanged.RemoveDynamic(PlayerInventoryWidget, &UInventoryWidget::UpdateEquipmentFromData);
	}
	Super::NativeOnClosed();
}

void UContainerInteractionWidget::OpenContainer(ABaseContainer* Container)
{
    if (!Container || !Container->GetContainerComponent())
        return;
    
    CurrentContainer = Container;

	// 플레이어 인벤토리 설정
	if (PlayerInventoryWidget)
	{
		if (ASonheimPlayerController* PC = Cast<ASonheimPlayerController>(GetOwningPlayer()))
		{
			if (ASonheimPlayerState* PlayerState = PC->GetPlayerState<ASonheimPlayerState>())
			{
				InventoryComponent = PlayerState->m_InventoryComponent;
				PlayerInventoryWidget->SetInventoryComponent(PlayerState->m_InventoryComponent);
				PlayerInventoryWidget->SetContainerMode(true, Container, PC);
				PlayerState->m_InventoryComponent->OnInventoryChanged.AddUniqueDynamic(PlayerInventoryWidget, &UInventoryWidget::UpdateInventoryFromData);
				PlayerState->m_InventoryComponent->OnEquipmentChanged.AddUniqueDynamic(PlayerInventoryWidget, &UInventoryWidget::UpdateEquipmentFromData);
			}
		}
	}
    
	// 상자 인벤토리 설정
	if (ContainerInventoryWidget)
	{
		ContainerInventoryWidget->SetContainerComponent(Container->GetContainerComponent());
        
		// ContainerWidget에 Container 참조 전달
		ContainerInventoryWidget->SetOwningContainer(Container);
	}
}

void UContainerInteractionWidget::CloseContainer()
{
	ReleaseContainer();
	// 입력 모드와 커서는 UI 스택이 남은 화면에 맞춘다.
	CloseSelf();
}

void UContainerInteractionWidget::ReleaseContainer()
{
    if (CurrentContainer)
    {
        // 서버에 닫기 요청
        if (ASonheimPlayerController* PC = Cast<ASonheimPlayerController>(GetOwningPlayer()))
        {
            PC->Server_ContainerOperation(CurrentContainer, EContainerOperation::Close);
            PlayerInventoryWidget->SetContainerMode(false);
		}
		CurrentContainer = nullptr;
	}
}

void UContainerInteractionWidget::OnCloseButtonClicked()
{
	CloseContainer();
}
