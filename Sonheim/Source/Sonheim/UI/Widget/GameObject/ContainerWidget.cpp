#include "ContainerWidget.h"
#include "Components/UniformGridPanel.h"
#include "Components/TextBlock.h"
#include "Sonheim/UI/Widget/Player/Inventory/SlotWidget.h"
#include "Sonheim/GameManager/SonheimGameInstance.h"
#include "Sonheim/AreaObject/Player/SonheimPlayer.h"
#include "Sonheim/AreaObject/Player/SonheimPlayerController.h"
#include "Sonheim/AreaObject/Player/SonheimPlayerState.h"
#include "Sonheim/AreaObject/Player/Utility/InventoryComponent.h"
#include "Sonheim/GameObject/Buildings/Utility/ContainerComponent.h"

void UContainerWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	
	// 에디터에서 미리보기용 그리드 생성
	if (SlotGrid && SlotWidgetClass)
	{
		CreateSlotGrid();
	}
}

void UContainerWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	GameInstance = Cast<USonheimGameInstance>(GetGameInstance());
	
	// 슬롯 이벤트 바인딩
	for (USlotWidget* SlotWidget : SlotWidgets)
	{
		if (SlotWidget)
		{
			BindSlotEvents(SlotWidget);
		}
	}
}

void UContainerWidget::NativeDestruct()
{
	// 델리게이트 정리
	if (ContainerComponent)
	{
		ContainerComponent->OnContainerInventoryChanged.RemoveDynamic(this, &UContainerWidget::UpdateContainerInventory);
	}
	
	Super::NativeDestruct();
}

void UContainerWidget::SetContainerComponent(UContainerComponent* InContainerComponent)
{
	// 기존 바인딩 제거
	if (ContainerComponent)
	{
		ContainerComponent->OnContainerInventoryChanged.RemoveDynamic(this, &UContainerWidget::UpdateContainerInventory);
	}
	
	ContainerComponent = InContainerComponent;
	
	if (ContainerComponent)
	{
		// 새 바인딩
		ContainerComponent->OnContainerInventoryChanged.AddDynamic(this, &UContainerWidget::UpdateContainerInventory);
		
		// 초기 데이터로 업데이트
		// Replicate 될 시간 타이머로 지연
		FTimerHandle InitTimerHandle;
		GetWorld()->GetTimerManager().SetTimer(InitTimerHandle, [this]()
		{
			if (ContainerComponent)
			{
				UpdateContainerInventory(ContainerComponent->GetContainerInventory());
			}
		}, 0.1f, false);
		
		// 그리드 크기 조정 (필요시)
		int32 MaxSlots = ContainerComponent->GetMaxSlots();
		int32 RequiredRows = FMath::CeilToInt((float)MaxSlots / GridColumns);
		if (RequiredRows != GridRows)
		{
			SetGridSize(GridColumns, RequiredRows);
		}
	}
}

void UContainerWidget::SetContainerName(const FText& Name)
{
	if (ContainerNameText)
	{
		ContainerNameText->SetText(Name);
	}
}

void UContainerWidget::SetGridSize(int32 Columns, int32 Rows)
{
	GridColumns = Columns;
	GridRows = Rows;
	CreateSlotGrid();
}

void UContainerWidget::CreateSlotGrid()
{
	if (!SlotGrid || !SlotWidgetClass)
		return;
	
	// 기존 슬롯 제거
	SlotGrid->ClearChildren();
	SlotWidgets.Empty();
	
	// 새 슬롯 생성
	for (int32 Row = 0; Row < GridRows; Row++)
	{
		for (int32 Column = 0; Column < GridColumns; Column++)
		{
			USlotWidget* SlotWidget = CreateWidget<USlotWidget>(this, SlotWidgetClass);
			if (SlotWidget)
			{
				SlotGrid->AddChildToUniformGrid(SlotWidget, Row, Column);
				SlotWidgets.Add(SlotWidget);
				
				// 슬롯 초기화
				SlotWidget->ClearSlot();
				SlotWidget->Init(SlotWidgets.Num() - 1);
				
				// 이벤트 바인딩 (NativeConstruct 이후에만)
				if (IsConstructed())
				{
					BindSlotEvents(SlotWidget);
				}
			}
		}
	}
}

void UContainerWidget::UpdateContainerInventory(const TArray<FInventoryItem>& Items)
{
	ClearAllSlots();
	
	// 아이템 데이터로 슬롯 업데이트
	for (int32 i = 0; i < Items.Num() && i < SlotWidgets.Num(); i++)
	{
		const FInventoryItem& Item = Items[i];
		if (Item.ItemID > 0)
		{
			FItemData* ItemData = GameInstance->GetDataItem(Item.ItemID);
			if (ItemData && SlotWidgets[i])
			{
				SlotWidgets[i]->SetItemData(ItemData, Item.Count);
			}
		}
	}
}

void UContainerWidget::OnSlotClicked(USlotWidget* SlotWidget, bool bIsRightClick)
{
	if (!SlotWidget || !ContainerComponent)
		return;
    
	// 우클릭: 아이템을 플레이어 인벤토리로 이동
	if (bIsRightClick && SlotWidget->ItemID != 0)
	{
		if (ASonheimPlayerController* PC = Cast<ASonheimPlayerController>(GetOwningPlayer()))
		{
			// PlayerController의 중계 함수 사용
			PC->Server_PlayerContainerTransfer(
				GetOwningContainer(), 
				true,
				SlotWidget->ItemID,
				SlotWidget->Quantity,
				SlotWidget->SlotIndex
			);
		}
	}
}

void UContainerWidget::OnSlotDragStarted(USlotWidget* SlotWidget)
{
	// 드래그 시작 처리
}

void UContainerWidget::OnSlotDropped(USlotWidget* FromSlot, USlotWidget* ToSlot)
{
	if (!FromSlot || !ToSlot || !ContainerComponent)
		return;
    
	if (ASonheimPlayerController* PC = Cast<ASonheimPlayerController>(GetOwningPlayer()))
	{
		// 컨테이너 내에서 아이템 위치 교환
		PC->Server_ContainerOperation(
			GetOwningContainer(),
			EContainerOperation::SwapItems,
			FromSlot->SlotIndex,
			ToSlot->SlotIndex
		);
	}
}

void UContainerWidget::HandleExternalDrop(USlotWidget* FromSlot, int32 ToIndex)
{
	if (!FromSlot || !ContainerComponent || ToIndex < 0)
		return;
    
	if (ASonheimPlayerController* PC = Cast<ASonheimPlayerController>(GetOwningPlayer()))
	{
		// 플레이어 인벤토리에서 상자로 아이템 이동
		PC->Server_PlayerContainerTransfer(
			GetOwningContainer(),
			false,                 // bFromContainerToPlayer (false = player to container)
			FromSlot->ItemID,
			FromSlot->Quantity,
			FromSlot->SlotIndex
		);
	}
}

void UContainerWidget::BindSlotEvents(USlotWidget* SlotWidget)
{
	if (!SlotWidget)
		return;
	
	if(!SlotWidget->OnItemClicked.IsBound())
	{
		SlotWidget->OnItemClicked.AddDynamic(this, &UContainerWidget::OnSlotClicked);
	}
	if(!SlotWidget->OnItemDragStarted.IsBound())
	{
		SlotWidget->OnItemDragStarted.AddDynamic(this, &UContainerWidget::OnSlotDragStarted);
	}
	if(!SlotWidget->OnItemDropped.IsBound())
	{
		SlotWidget->OnItemDropped.AddDynamic(this, &UContainerWidget::OnSlotDropped);
	}
}

void UContainerWidget::ClearAllSlots()
{
	for (USlotWidget* SlotWidget : SlotWidgets)
	{
		if (SlotWidget)
		{
			SlotWidget->ClearSlot();
		}
	}
}