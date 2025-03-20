// Fill out your copyright notice in the Description page of Project Settings.


#include "SlotWidget.h"
#include "ToolTipWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Sonheim/GameManager/SonheimGameInstance.h"
#include "Sonheim/ResourceManager/SonheimGameType.h"

void USlotWidget::NativeConstruct()
{
	Super::NativeConstruct();
	m_GameInstance = Cast<USonheimGameInstance>(GetWorld()->GetGameInstance());
}

void USlotWidget::Init(int Index)
{
	this->SlotIndex = Index;
}

void USlotWidget::SetItemData(const FItemData* ItemData, int32 NewQuantity)
{
	// ItemID = NewItemID;
	if (NewQuantity != 0)
	{
		TXT_Quantity->SetText(FText::FromString(FString::Printf(TEXT("%d"), NewQuantity)));
		TXT_Quantity->SetVisibility(ESlateVisibility::Visible);
	}
	if (!FMath::IsNearlyZero(ItemData->Weight))
	{
		float totalWeight = static_cast<float>(NewQuantity) * ItemData->Weight;
		TXT_Weight->SetText(FText::FromString(FString::Printf(TEXT("%.1f"), totalWeight)));
		TXT_Weight->SetVisibility(ESlateVisibility::Visible);
	}
	IMG_Item->SetBrushFromTexture(ItemData->ItemIcon);
	IMG_Item->SetVisibility(ESlateVisibility::Visible);

	ItemID = ItemData->ItemID;
	Quantity = NewQuantity;
	//ToDo : Durability 등 묶어서 구조체로 관리해야할듯.. FInventoryItem 의 확장?
	
	// ToolTip 초기화
	if (ItemData != nullptr && ToolTipInstance != nullptr)
	{
		ToolTipInstance->InitToolTip(ItemData, Quantity);
		SetToolTip(ToolTipInstance);
	}
}

void USlotWidget::ClearSlot()
{
	TXT_Quantity->SetVisibility(ESlateVisibility::Hidden);
	TXT_Weight->SetVisibility(ESlateVisibility::Hidden);
	IMG_Item->SetVisibility(ESlateVisibility::Hidden);
	ItemID = 0;
	Quantity = 0;
	
	// ToolTip 초기화
	SetToolTip(nullptr);
}

bool USlotWidget::IsEmpty() const
{
	return ItemID == 0 || Quantity == 0;
}

void USlotWidget::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);

	// 슬롯이 비어있지 않고, 툴팁 클래스가 지정되어 있다면
	if (!IsEmpty() && ToolTipWidgetClass)
	{
		// 툴팁 위젯 생성
		if (!ToolTipInstance)
		{
			ToolTipInstance = CreateWidget<UToolTipWidget>(this, ToolTipWidgetClass);

			// 아이템 데이터 전달 및 초기화 함수 호출
			if (ToolTipInstance && GetWorld())
			{
				if (m_GameInstance)
				{
					const FItemData* ItemData = m_GameInstance->GetDataItem(ItemID);
					if (ItemData)
					{
						// ToolTipWidget에 새로 추가할 InitToolTip 함수 사용
						ToolTipInstance->InitToolTip(ItemData, Quantity);
						SetToolTip(ToolTipInstance);
					}
				}
			}
		}
	}

	// 하이라이트 효과
	if (IMG_BG)
	{
		IMG_BG->SetColorAndOpacity(FLinearColor::Blue);
	}
}

void USlotWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);

	// 하이라이트 효과 원복
	if (IMG_BG)
	{
		IMG_BG->SetColorAndOpacity(FLinearColor(1.0f, 1.0f, 1.0f, 1.0f));
	}
}

FReply USlotWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	FReply Reply = Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);

	if (IsEmpty())
		return Reply;

	// 좌/우 클릭 확인
	bool bIsRightClick = InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton;

	// 클릭 이벤트 발생
	OnItemClicked.Broadcast(this, bIsRightClick);

	// 좌클릭 및 드래그 드롭 지원하는 경우 드래그 감지 시작
	if (!bIsRightClick && bSupportsDragDrop)
	{
		return Reply.DetectDrag(this->TakeWidget(), EKeys::LeftMouseButton);
	}

	return Reply;
}

FReply USlotWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

void USlotWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent,
                                       UDragDropOperation*& OutOperation)
{
	Super::NativeOnDragDetected(InGeometry, InMouseEvent, OutOperation);

	if (IsEmpty() || !bSupportsDragDrop)
		return;

	// 드래그 드롭 오퍼레이션 생성
	UDragDropSlotOperation* DragDropOp = NewObject<UDragDropSlotOperation>();
	DragDropOp->DraggedSlotWidget = this;

	// 드래그 위젯 생성 (아이템 이미지 표시)
	UUserWidget* DragVisual = CreateWidget(this, GetClass());
	UImage* DragImage = Cast<UImage>(DragVisual->GetWidgetFromName(TEXT("IMG_Item")));
	if (DragImage && IMG_Item)
	{
		DragImage->SetBrushFromTexture(Cast<UTexture2D>(IMG_Item->GetBrush().GetResourceObject()));
		DragImage->SetRenderOpacity(0.7f);
	}

	DragDropOp->DefaultDragVisual = DragVisual;
	DragDropOp->Pivot = EDragPivot::CenterCenter;

	OutOperation = DragDropOp;

	// 드래그 시작 이벤트 발생
	OnItemDragStarted.Broadcast(this);
}

bool USlotWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent,
                               UDragDropOperation* InOperation)
{
	UDragDropSlotOperation* DragDropOp = Cast<UDragDropSlotOperation>(InOperation);
	if (!DragDropOp || !DragDropOp->DraggedSlotWidget)
		return false;

	// 자기 자신에게 드롭한 경우는 무시
	if (DragDropOp->DraggedSlotWidget == this)
		return false;

	// 드롭 이벤트 발생
	OnItemDropped.Broadcast(DragDropOp->DraggedSlotWidget, this);

	return true;
}
