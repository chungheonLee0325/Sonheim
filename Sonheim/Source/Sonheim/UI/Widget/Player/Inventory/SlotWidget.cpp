// Fill out your copyright notice in the Description page of Project Settings.


#include "SlotWidget.h"

#include "InventoryWidget.h"
#include "ToolTipWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Sonheim/GameManager/SonheimGameInstance.h"
#include "Sonheim/ResourceManager/SonheimGameType.h"
#include "Sonheim/UI/Widget/GameObject/ContainerWidget.h"
#include "Sonheim/Utilities/SonheimUtility.h"

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
	// 등급에 따른 배경색 변경
	if (IMG_BackGround)
	{
		IMG_BackGround->SetColorAndOpacity(USonheimUtility::GetRarityColor(ItemData->ItemRarity, 0.2f));
		IMG_BackGround->SetVisibility(ESlateVisibility::Visible);
	}
	
	IMG_Item->SetBrushFromTexture(ItemData->ItemIcon);
	IMG_Item->SetVisibility(ESlateVisibility::Visible);

	ItemID = ItemData->ItemID;
	Quantity = NewQuantity;
	//ToDo : Durability 등 묶어서 구조체로 관리해야할듯.. FInventoryItem 의 확장?

	// 툴팁 위젯이 없으면 생성
	if (!ToolTipInstance && ToolTipWidgetClass)
	{
		ToolTipInstance = CreateWidget<UToolTipWidget>(this, ToolTipWidgetClass);
	}
	
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
	IMG_BackGround->SetVisibility(ESlateVisibility::Hidden);
	ItemID = 0;
	Quantity = 0;
	
	// ToolTip 초기화
	SetToolTip(nullptr);
}

void USlotWidget::SetCraftingMatMode(bool Enable)
{
	TXT_Quantity->SetVisibility(Enable ? ESlateVisibility::Hidden : ESlateVisibility::HitTestInvisible);
	TXT_Weight->SetVisibility(Enable ? ESlateVisibility::Hidden : ESlateVisibility::HitTestInvisible);
	IMG_BackGround->SetVisibility(Enable ? ESlateVisibility::Hidden : ESlateVisibility::HitTestInvisible);
	IMG_Border->SetVisibility(Enable ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	bOnlyView = Enable;
}

bool USlotWidget::IsEmpty() const
{
	return ItemID == 0 || Quantity == 0;
}

void USlotWidget::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);

	if (bOnlyView) return;
	
	// 슬롯이 비어있지 않고, 툴팁 클래스가 지정되어 있다면
	if (!IsEmpty() && ToolTipWidgetClass)
	{
		Border_MouseOver->SetVisibility(ESlateVisibility::Visible);
		PlayMouseEnterAnimation();
	}

	// 하이라이트 효과
	if (IMG_Border)
	{
		IMG_Border->SetColorAndOpacity(FLinearColor::Blue);
	}
}

void USlotWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);

	if (bOnlyView) return;
	
	// 하이라이트 효과 원복
	if (IMG_Border)
	{
		IMG_Border->SetColorAndOpacity(FLinearColor(1.0f, 1.0f, 1.0f, 1.0f));
	}
		Border_MouseOver->SetVisibility(ESlateVisibility::Hidden);
	
}

FReply USlotWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	FReply Reply = Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);

	if (bOnlyView) return Reply;
	
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

	if (bOnlyView) return;
	
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

	if (bOnlyView) return false;
	
	UDragDropSlotOperation* DragDropOp = Cast<UDragDropSlotOperation>(InOperation);
	if (!DragDropOp || !DragDropOp->DraggedSlotWidget)
		return false;

	// 자기 자신에게 드롭한 경우는 무시
	if (DragDropOp->DraggedSlotWidget == this)
		return false;

	// 부모 위젯 확인하여 크로스 드롭 처리
	UInventoryWidget* MyInventoryWidget = GetTypedOuter<UInventoryWidget>();
	UContainerWidget* MyContainerWidget = GetTypedOuter<UContainerWidget>();
    
	UInventoryWidget* FromInventoryWidget = DragDropOp->DraggedSlotWidget->GetTypedOuter<UInventoryWidget>();
	UContainerWidget* FromContainerWidget = DragDropOp->DraggedSlotWidget->GetTypedOuter<UContainerWidget>();

	// 플레이어 인벤토리 → 상자
	if (FromInventoryWidget && MyContainerWidget)
	{
		MyContainerWidget->HandleExternalDrop(DragDropOp->DraggedSlotWidget, this->SlotIndex);
		return true;
	}
	// 상자 → 플레이어 인벤토리
	else if (FromContainerWidget && MyInventoryWidget)
	{
		MyInventoryWidget->HandleExternalDrop(DragDropOp->DraggedSlotWidget, this->SlotIndex);
		return true;
	}
	// 같은 위젯 내에서의 드롭
	else
	{
		OnItemDropped.Broadcast(DragDropOp->DraggedSlotWidget, this);
		return true;
	}
}

void USlotWidget::PlayMouseEnterAnimation()
{
    if (ClickAnimation)
    {
        PlayAnimation(ClickAnimation);
    }
    else
    {
        // 기본 클릭 효과
        SetRenderScale(FVector2D(0.95f, 0.95f));
        
        FTimerHandle TimerHandle;
        GetWorld()->GetTimerManager().SetTimer(TimerHandle, [this]()
        {
            SetRenderScale(FVector2D(1.0f, 1.0f));
        }, 0.1f, false);
    }
}
