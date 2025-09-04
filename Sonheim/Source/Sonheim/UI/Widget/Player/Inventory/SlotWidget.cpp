// Fill out your copyright notice in the Description page of Project Settings.


#include "SlotWidget.h"

#include "Sonheim/Utilities/InventoryRulesLibrary.h"
#include "InventoryWidget.h"
#include "ToolTipWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/SizeBox.h"
#include "Sonheim/GameManager/SonheimGameInstance.h"
#include "Sonheim/ResourceManager/SonheimGameType.h"
#include "Sonheim/UI/Widget/GameObject/ContainerWidget.h"
#include "Sonheim/AreaObject/Player/SonheimPlayerController.h"
#include "Sonheim/GameObject/Buildings/Utility/ContainerComponent.h"
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

	// 슬롯이 비어있지 않으면, 슬롯 테두리 효과 부여(슬롯 테두리 하늘색)
	if (Border_MouseOver && !IsEmpty())
	{
		Border_MouseOver->SetVisibility(ESlateVisibility::Visible);
		PlayMouseEnterAnimation();
	}

	// 슬롯이 비어있더라도 해당 슬롯에 마우스가 올라간 피드백으로 슬롯 색 변화 (슬롯 배경색 파란색으로)
	if (IMG_Border)
	{
		IMG_Border->SetColorAndOpacity(FLinearColor::Blue);
	}
}

void USlotWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);

	if (bOnlyView) return;

	// 슬롯이 빈것과 관계없이, 슬롯 테두리 효과 초기화(장비 해제나 아이템 장착시 슬롯이 빌수도 있음)
	if (Border_MouseOver)
	{
		Border_MouseOver->SetVisibility(ESlateVisibility::Hidden);
	}
	// 슬롯이 빈것과 관계없이, 해당 슬롯 배경색 초기화
	if (IMG_Border)
	{
		IMG_Border->SetColorAndOpacity(FLinearColor(1.0f, 1.0f, 1.0f, 1.0f));
	}
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

	IsDragging = true;

	// 드래그 드롭 오퍼레이션 생성
	UDragDropSlotOperation* DragDropOp = NewObject<UDragDropSlotOperation>();
	DragDropOp->DraggedSlotWidget = this;

	// 드래그 위젯 생성 (아이템 이미지 표시)
	USlotWidget* DragVisual = Cast<USlotWidget>(CreateWidget(this, GetClass()));

	const FVector2D SlotSize = InGeometry.GetLocalSize();
	DragVisual->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));

	if (DragVisual)
	{
		if (DragVisual->SB_Root)
		{
			DragVisual->SB_Root->SetWidthOverride(SlotSize.X);
			DragVisual->SB_Root->SetHeightOverride(SlotSize.Y);
			DragVisual->SB_Root->SetRenderOpacity(0.7f);
		}
		if (DragVisual->IMG_Item)
		{
			DragVisual->IMG_Item->SetBrushFromTexture(Cast<UTexture2D>(IMG_Item->GetBrush().GetResourceObject()));
		}
		if (DragVisual->IMG_BackGround)
		{
			DragVisual->IMG_BackGround->SetColorAndOpacity(IMG_BackGround->GetColorAndOpacity());
			DragVisual->IMG_BackGround->SetVisibility(ESlateVisibility::Visible);
			DragVisual->IMG_BackGround->SetRenderOpacity(0.4f);
		}
		if (DragVisual->TXT_Quantity)
		{
			DragVisual->TXT_Quantity->SetText(TXT_Quantity->GetText());
			DragVisual->TXT_Quantity->SetVisibility(ESlateVisibility::Visible);
		}
		if (DragVisual->TXT_Weight)
		{
			DragVisual->TXT_Weight->SetText(TXT_Weight->GetText());
			DragVisual->TXT_Weight->SetVisibility(ESlateVisibility::Visible);
		}
	}

	DragDropOp->DefaultDragVisual = DragVisual;
	DragDropOp->Pivot = EDragPivot::CenterCenter;

	OutOperation = DragDropOp;

	// 하이라이트 표시
	SetHighlighted(true);
	// 하이라이트 해제 델리게이트 바인드
	// 드래그 시작: 소스 슬롯 하이라이트 표시
	SetHighlighted(true, false);
	DragDropOp->OnDrop.AddDynamic(this, &USlotWidget::HandleDragOperationEnded);
	DragDropOp->OnDragCancelled.AddDynamic(this, &USlotWidget::HandleDragOperationEnded);

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

	// 하이라이트/드롭 하이라이트 모두 정리
	this->SetHighlighted(false, true);
	DragDropOp->DraggedSlotWidget->SetHighlighted(false, true);


	// 드롭 불가시 종료
	if (!IsDropValidFrom(DragDropOp->DraggedSlotWidget))
	{
		return false;
	}

	// 부모 위젯 확인하여 크로스 드롭 처리
	UInventoryWidget* MyInventoryWidget = GetTypedOuter<UInventoryWidget>();
	UContainerWidget* MyContainerWidget = GetTypedOuter<UContainerWidget>();

	UInventoryWidget* FromInventoryWidget = DragDropOp->DraggedSlotWidget->GetTypedOuter<UInventoryWidget>();
	UContainerWidget* FromContainerWidget = DragDropOp->DraggedSlotWidget->GetTypedOuter<UContainerWidget>();

	// 플레이어 인벤토리/장비 → 상자
	if (FromInventoryWidget && MyContainerWidget)
	{
		// 컨테이너 위젯의 헬퍼로 위임(스왑/이동 모두 처리)
		MyContainerWidget->HandleExternalDrop(DragDropOp->DraggedSlotWidget, this);
		return true;
	}
	// 상자 → 플레이어 인벤토리/장비
	else if (FromContainerWidget && MyInventoryWidget)
	{
		// 인벤토리 위젯의 헬퍼로 위임(스왑/장비장착/이동 모두 처리)
		MyInventoryWidget->HandleExternalDrop(DragDropOp->DraggedSlotWidget, this);
		return true;
	}
	// 같은 위젯 내에서의 드롭
	else
	{
		OnItemDropped.Broadcast(DragDropOp->DraggedSlotWidget, this);
		return true;
	}
}

void USlotWidget::NativeOnDragCancelled(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	Super::NativeOnDragCancelled(InDragDropEvent, InOperation);

	if (IsDragging) return;

	SetHighlighted(false, false);
	SetDragOperationVisual(InOperation, NormalColor);
	if (UDragDropSlotOperation* DragOp = Cast<UDragDropSlotOperation>(InOperation))
	{
		if (DragOp->DraggedSlotWidget)
		{
			DragOp->DraggedSlotWidget->SetHighlighted(false, true);
		}
	}
}

void USlotWidget::NativeOnDragEnter(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent,
                                    UDragDropOperation* InOperation)
{
	Super::NativeOnDragEnter(InGeometry, InDragDropEvent, InOperation);
	if (bOnlyView) return;
	if (IsDragging) return;

	UDragDropSlotOperation* DragDropOp = Cast<UDragDropSlotOperation>(InOperation);
	if (!DragDropOp || !DragDropOp->DraggedSlotWidget)
		return;

	// 자기 자신에게 드롭한 경우는 무시
	if (DragDropOp->DraggedSlotWidget == this)
		return;

	// 드롭 유효성 체크
	if (!IsDropValidFrom(DragDropOp->DraggedSlotWidget))
	{
		SetHighlighted(true, false);
		SetDragOperationVisual(InOperation, InvalidColor);
		if (USlotWidget* From = DragDropOp->DraggedSlotWidget)
		{
			From->SetHighlighted(true, false);
		}
	}
	else
	{
		SetHighlighted(true, true);
		SetDragOperationVisual(InOperation, NormalColor);
		if (USlotWidget* From = DragDropOp->DraggedSlotWidget)
		{
			From->SetHighlighted(true, true);
		}
	}
}

void USlotWidget::NativeOnDragLeave(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	Super::NativeOnDragLeave(InDragDropEvent, InOperation);

	if (IsDragging) return;

	SetHighlighted(false, true);
	SetDragOperationVisual(InOperation, NormalColor);
	if (UDragDropSlotOperation* DragOp = Cast<UDragDropSlotOperation>(InOperation))
	{
		if (DragOp->DraggedSlotWidget)
		{
			DragOp->DraggedSlotWidget->SetHighlighted(true, false);
		}
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

void USlotWidget::SetHighlighted(bool bEnable, bool bValid) const
{
	if (!Border_Highlight)
		return;

	Border_Highlight->SetBrushColor(bValid ? ValidColor : InvalidColor);
	Border_Highlight->SetVisibility(bEnable ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
}

void USlotWidget::HandleDragOperationEnded(UDragDropOperation* Operation)
{
	IsDragging = false;

	// 소스 기준으로 확실히 끄기
	SetHighlighted(false);

	// 혹시 Operation 안에 소스 포인터를 넣어뒀다면 그쪽도 안전하게
	if (auto* Op = Cast<UDragDropSlotOperation>(Operation))
	{
		if (Op->DraggedSlotWidget)
		{
			Op->DraggedSlotWidget->SetHighlighted(false);
		}
	}
}

void USlotWidget::SetDragOperationVisual(UDragDropOperation* InOperation, const FLinearColor& Tint) const
{
	if (!InOperation) return;
	if (USlotWidget* SlotWidget = Cast<USlotWidget>(InOperation->DefaultDragVisual))
	{
		if (SlotWidget->IMG_Item)
		{
			SlotWidget->IMG_Item->SetColorAndOpacity(Tint);
		}
	}
}

// 드롭 가능 여부 판단
bool USlotWidget::IsDropValidFrom(USlotWidget* FromSlot) const
{
	if (!FromSlot) return false;
	if (FromSlot == this) return false;

	// 빈 슬롯 드래그는 무시
	if (FromSlot->IsEmpty()) return false;

	// 컨텍스트 판별
	auto IsEquip = [](const USlotWidget* S) { return S && S->EquipmentSlotType != EEquipmentSlotType::None; };
	auto IsContainer = [](const USlotWidget* S) { return S && (S->GetTypedOuter<UContainerWidget>() != nullptr); };
	auto IsPlayerInv = [&](const USlotWidget* S) { return S && !IsEquip(S) && !IsContainer(S); };

	const bool FromEquip = IsEquip(FromSlot);
	const bool ToEquip = IsEquip(this);
	const bool FromCont = IsContainer(FromSlot);
	const bool ToCont = IsContainer(this);
	const bool FromInv = IsPlayerInv(FromSlot);
	const bool ToInv = IsPlayerInv(this);

	// 플레이어 인벤토리 <-> 플레이어 인벤토리: 스왑 전용(둘 다 채워져 있어야 의미)
	if (FromInv && ToInv)
	{
		return !this->IsEmpty();
	}

	// 컨테이너 <-> 컨테이너: 스왑 전용
	if (FromCont && ToCont)
	{
		return !this->IsEmpty();
	}

	// 인벤토리 -> 장비 슬롯: 아이템-슬롯 적합성 검사(InventoryComponent에 위임)
	if (FromInv && ToEquip)
	{
		return UInventoryRulesLibrary::IsItemCompatibleWithSlot(this, FromSlot->ItemID, EquipmentSlotType);
	}

	// 장비 -> 인벤토리: 허용(인벤 용량은 서버가 검증)
	if (FromEquip && ToInv)
	{
		// 비어있으면 항상 true
		if (IsEmpty()) return true;

		// 차있다면 Swap 조건 검사
		if (!m_GameInstance) return false;
		const FItemData* ItemData = m_GameInstance->GetDataItem(ItemID);
		if (!ItemData) return false;

		// if (!(ItemData->ItemCategory == EItemCategory::Equipment || ItemData->ItemCategory == EItemCategory::Weapon))
		// 	return false;

		switch (FromSlot->EquipmentSlotType)
		{
		case EEquipmentSlotType::Head: return ItemData->EquipmentData.EquipKind == EEquipmentKindType::Head;
		case EEquipmentSlotType::Body: return ItemData->EquipmentData.EquipKind == EEquipmentKindType::Body;
		case EEquipmentSlotType::Shield: return ItemData->EquipmentData.EquipKind == EEquipmentKindType::Shield;
		case EEquipmentSlotType::Glider: return ItemData->EquipmentData.EquipKind == EEquipmentKindType::Glider;
		case EEquipmentSlotType::SphereModule: return ItemData->EquipmentData.EquipKind ==
				EEquipmentKindType::SphereModule;
		case EEquipmentSlotType::Accessory1:
		case EEquipmentSlotType::Accessory2: return ItemData->EquipmentData.EquipKind == EEquipmentKindType::Accessory;
		case EEquipmentSlotType::Weapon1:
		case EEquipmentSlotType::Weapon2:
		case EEquipmentSlotType::Weapon3:
		case EEquipmentSlotType::Weapon4: return ItemData->EquipmentData.EquipKind == EEquipmentKindType::Weapon;
		default: return false;
		}
	}

	// 장비 <-> 장비: 같은 계열만 스왑 허용(무기끼리/악세끼리)
	if (FromEquip && ToEquip)
	{
		auto IsWeaponSlot = [](EEquipmentSlotType T)
		{
			return T == EEquipmentSlotType::Weapon1 || T == EEquipmentSlotType::Weapon2 ||
				T == EEquipmentSlotType::Weapon3 || T == EEquipmentSlotType::Weapon4;
		};
		auto IsAccessorySlot = [](EEquipmentSlotType T)
		{
			return T == EEquipmentSlotType::Accessory1 || T == EEquipmentSlotType::Accessory2;
		};

		if (IsWeaponSlot(FromSlot->EquipmentSlotType) && IsWeaponSlot(EquipmentSlotType)) return true;
		if (IsAccessorySlot(FromSlot->EquipmentSlotType) && IsAccessorySlot(EquipmentSlotType)) return true;
		return false;
	}

	// 플레이어 인벤토리 -> 컨테이너: 간단 용량 체크(보수적)
	if (FromInv && ToCont)
	{
		if (const UContainerWidget* CW = GetTypedOuter<UContainerWidget>())
		{
			if (const UContainerComponent* CC = CW->GetContainerComponent())
			{
				const int32 Used = CC->GetContainerInventory().Num();
				const int32 Max = CC->GetMaxSlots();
				return Used < Max;
			}
		}
		return true;
	}

	// 컨테이너 -> 플레이어 인벤토리: 허용(서버 검증)
	if (FromCont && ToInv)
	{
		return true;
	}

	// 컨테이너 -> 장비 슬롯: 아이템-슬롯 적합성 검사(InventoryComponent에 위임)
	if (FromCont && ToEquip)
	{
		return UInventoryRulesLibrary::IsItemCompatibleWithSlot(this, FromSlot->ItemID, EquipmentSlotType);
	}

	// 장비 -> 컨테이너: 허용(서버가 용량/스왑 판단)
	if (FromEquip && ToCont)
	{
		return true;
	}

	return false;
}
