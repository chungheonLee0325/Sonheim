// ContainerInteractionWidget.cpp
#include "ContainerInteractionWidget.h"
#include "ContainerWidget.h"
#include "Components/Button.h"
#include "Sonheim/UI/Widget/Player/Inventory/InventoryWidget.h"
#include "Sonheim/UI/Widget/Player/Inventory/SlotWidget.h"
#include "Sonheim/AreaObject/Player/SonheimPlayer.h"
#include "Sonheim/AreaObject/Player/SonheimPlayerController.h"
#include "Sonheim/AreaObject/Player/SonheimPlayerState.h"
#include "Sonheim/AreaObject/Player/Utility/InventoryComponent.h"
#include "Sonheim/GameObject/Buildings/Storage/BaseContainer.h"

void UContainerInteractionWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	// 닫기 버튼 이벤트 바인딩
	if (CloseButton)
	{
		CloseButton->OnClicked.AddDynamic(this, &UContainerInteractionWidget::OnCloseButtonClicked);
	}
}

void UContainerInteractionWidget::NativeDestruct()
{
	// 상자 닫기 처리
	if (CurrentContainer)
	{
		CurrentContainer->CloseContainer();
		CurrentContainer = nullptr;
	}
	
	Super::NativeDestruct();
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
				PlayerInventoryWidget->SetInventoryComponent(PlayerState->m_InventoryComponent);
			}
		}
	}
	
	// 상자 인벤토리 설정
	if (ContainerInventoryWidget)
	{
		ContainerInventoryWidget->SetContainerComponent(Container->GetContainerComponent());
	}
	
	// 마우스 커서 표시
	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->SetShowMouseCursor(true);
		PC->SetInputMode(FInputModeGameAndUI());
	}
}

void UContainerInteractionWidget::CloseContainer()
{
	// 상자 닫기
	if (CurrentContainer)
	{
		CurrentContainer->CloseContainer();
		CurrentContainer = nullptr;
	}
	
	// UI 제거
	RemoveFromParent();
	
	// 마우스 커서 숨기기
	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->SetShowMouseCursor(false);
		PC->SetInputMode(FInputModeGameOnly());
	}
}

void UContainerInteractionWidget::OnCloseButtonClicked()
{
	CloseContainer();
}