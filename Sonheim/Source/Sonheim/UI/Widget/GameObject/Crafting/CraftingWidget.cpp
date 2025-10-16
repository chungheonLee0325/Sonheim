#include "CraftingWidget.h"

#include "RequiredMatRowWidget.h"
#include "Sonheim/GameObject/Buildings/Crafting/CraftingStation.h"
#include "Components/WrapBox.h"
#include "Components/VerticalBox.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/SpinBox.h"
#include "Sonheim/AreaObject/Player/SonheimPlayerController.h"
#include "Sonheim/AreaObject/Player/Utility/InventoryComponent.h"
#include "Sonheim/Utilities/InventoryResourceProvider.h"
#include "Sonheim/GameManager/SonheimGameInstance.h"
#include "Sonheim/UI/Widget/Player/Inventory/SlotWidget.h"
#include "Engine/DataTable.h"
#include "Sonheim/AreaObject/Player/SonheimPlayerState.h"

void UCraftingWidget::Initialise(ACraftingStation* InStation)
{
	Station = InStation;
	if (ASonheimPlayerController* PC = Cast<ASonheimPlayerController>(GetOwningPlayer()))
	{
		if (ASonheimPlayerState* PlayerState = PC->GetPlayerState<ASonheimPlayerState>())
		{
			InventoryComp = PlayerState->m_InventoryComponent;
		}
	}
	GameInstance = GetGameInstance<USonheimGameInstance>();
	RefreshRecipes();
	RefreshDetail();
	if (QuantitySpin) QuantitySpin->SetValue(1.f);

	if (InventoryComp)
	{
		InventoryComp->OnInventoryChanged.AddDynamic(this, &UCraftingWidget::UpdateInventoryFromData);
	}
}

void UCraftingWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (CraftButton) CraftButton->OnClicked.AddDynamic(this, &UCraftingWidget::OnClickCraft);
	if (MaxButton) MaxButton->OnClicked.AddDynamic(this, &UCraftingWidget::OnClickMax);
	if (MinButton) MinButton->OnClicked.AddDynamic(this, &UCraftingWidget::OnClickMin);
	if (AddButton) AddButton->OnClicked.AddDynamic(this, &UCraftingWidget::OnClickAdd);
	if (SubButton) SubButton->OnClicked.AddDynamic(this, &UCraftingWidget::OnClickSub);
	if (QuantitySpin) QuantitySpin->OnValueChanged.AddDynamic(this, &UCraftingWidget::OnQuantityChanged);
	if (CloseButton) CloseButton->OnClicked.AddDynamic(this, &UCraftingWidget::OnClickedClose);
}

void UCraftingWidget::NativeDestruct()
{
	// UI 닫힐 때 서버에 UI 락 해제
	if (Station)
	{
		if (ASonheimPlayerController* SPC = Cast<ASonheimPlayerController>(GetOwningPlayer()))
		{
			SPC->Server_Crafting_ReleaseUI(Station);
		}
	}
	if (InventoryComp)
	{
		InventoryComp->OnInventoryChanged.RemoveDynamic(this, &UCraftingWidget::UpdateInventoryFromData);
	}

	Super::NativeDestruct();
}

void UCraftingWidget::RefreshRecipes()
{
	if (!Station || !RecipeWrap) return;
	RecipeWrap->ClearChildren();
	RecipeSlotMap.Empty();

	UDataTable* Table = Station->GetRecipeTable();
	if (!Table) return;

	for (const auto& Pair : Table->GetRowMap())
	{
		const FName Row = Pair.Key;
		const FCraftingRecipe* Recipe = reinterpret_cast<const FCraftingRecipe*>(Pair.Value);
		if (!Recipe || Recipe->ResultItemID <= 0) continue;

		USlotWidget* slot = nullptr;
		if (SlotWidgetClass) slot = CreateWidget<USlotWidget>(this, SlotWidgetClass);
		else slot = CreateWidget<USlotWidget>(this, USlotWidget::StaticClass());
		if (!slot) continue;

		if (GameInstance)
		{
			const FItemData* ItemData = GameInstance->GetDataItem(Recipe->ResultItemID);
			if (ItemData) slot->SetItemData(ItemData, 1);
			if (slot->TXT_Quantity) slot->TXT_Quantity->SetVisibility(ESlateVisibility::Hidden);
			if (slot->TXT_Weight) slot->TXT_Weight->SetVisibility(ESlateVisibility::Hidden);
		}

		slot->bSupportsDragDrop = false;
		slot->OnItemClicked.AddDynamic(this, &UCraftingWidget::OnRecipeSlotClicked);

		RecipeWrap->AddChild(slot);
		RecipeSlotMap.Add(Row, slot);
	}
	RefreshCraftability();
}

void UCraftingWidget::RefreshDetail()
{
	if (!Station) return;
	if (SelectedRow.IsNone()) return;

	const FCraftingRecipe* R = GetRecipe(SelectedRow);
	if (!R) return;

	// 1) 레시피 변경 감지 시에만 정적 요소 재구성(아이템 이름/아이콘, 재료 행 레이아웃/아이콘)
	if (LastRecipeRow != SelectedRow)
	{
		RebuildStaticForRecipe(R);
		LastRecipeRow = SelectedRow;
	}
	// 2) 보유/필요 수량·색상·버튼 상태는 항상 동적 갱신(수량/인벤 변경에 대응)
	RefreshDynamicForRecipe(R);
}

void UCraftingWidget::RefreshCraftability()
{
	if (!Station) return;
	for (auto& Pair : RecipeSlotMap)
	{
		const FName Row = Pair.Key;
		USlotWidget* slot = Pair.Value;
		if (!slot) continue;

		const FCraftingRecipe* R = GetRecipe(Row);
		bool bCraftable = (R && InventoryComp)
			                  ? UInventoryResourceProvider::HasItems(InventoryComp, R->RequiredMaterials)
			                  : false;
		slot->SetIsEnabled(true);

		slot->SetRenderOpacity(bCraftable ? 1.0f : 0.7f);
		slot->IMG_Item->SetColorAndOpacity(bCraftable ? FLinearColor::White : FLinearColor::Red);
	}
}

void UCraftingWidget::RebuildStaticForRecipe(const struct FCraftingRecipe* R)
{
	if (!R) return;

	// 결과 아이템 이름/아이콘 — 레시피 바뀔 때만
	if (GameInstance)
	{
		if (const FItemData* RD = GameInstance->GetDataItem(R->ResultItemID))
		{
			if (ItemName) ItemName->SetText(RD->ItemName);
			if (ItemIcon) ItemIcon->SetBrushFromTexture(RD->ItemIcon);
			if (ItemResultQuantity)
				ItemResultQuantity->SetText(
					FText::FromString(FString::Printf(TEXT(" x %d"), R->ResultCount)));
		}
		else
		{
			if (ItemName) ItemName->SetText(FText::FromName(SelectedRow));
		}
	}

	// 요구 재료 레이아웃/아이콘 — 한 번만
	if (!RequiredList) return;

	// 풀/리스트는 파괴하지 않음. 필요한 개수만큼 만들고 나머지는 숨김.
	CachedMatIDs.Reset();
	CachedMatIDs.Reserve(R->RequiredMaterials.Num());
	for (const auto& KVP : R->RequiredMaterials)
		CachedMatIDs.Add(KVP.Key);

	// 부족하면 생성
	while (RequiredRowPool.Num() < CachedMatIDs.Num())
	{
		URequiredMatRowWidget* RowW = CreateWidget<URequiredMatRowWidget>(
			this, RequiredRowClass ? *RequiredRowClass : URequiredMatRowWidget::StaticClass());
		if (RowW)
		{
			// 처음 한 번만 Add
			RowW->SetupOnce();
			RequiredRowPool.Add(RowW);
			RequiredList->AddChild(RowW);
			RowW->SetPadding(3.f);
		}
	}
	// 가시성 정리 + 아이콘 1회 세팅
	for (int32 i = 0; i < RequiredRowPool.Num(); ++i)
	{
		const bool bVisible = (i < CachedMatIDs.Num());
		RequiredRowPool[i]->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);

		if (bVisible && GameInstance)
		{
			const int32 MatID = CachedMatIDs[i];
			// Need/Have는 동적에서 채우므로 여기서는 아이콘만 준비해도 됨
			if (const FItemData* MD = GameInstance->GetDataItem(MatID))
			{
				// Need=0/Have=0으로 1회 초기 채움
				RequiredRowPool[i]->UpdateRow(GameInstance, MatID, 0, 0);
			}
		}
	}
}

void UCraftingWidget::RefreshDynamicForRecipe(const struct FCraftingRecipe* R)
{
	if (!R) return;

	// 결과 아이템 보유 수량(동적)
	if (OwnedCountText)
	{
		const int32 Owned = InventoryComp ? InventoryComp->GetItemCount(R->ResultItemID) : 0;
		OwnedCountText->SetText(FText::FromString(FString::Printf(TEXT("보유 수     %d"), Owned)));
	}

	if (!GameInstance || !RequiredList) return;

	const int32 Qty = GetCurrentQuantity();
	// 인벤토리 기준 최대 제작 가능 수
	const int32 MaxUnits = (InventoryComp)
		                       ? UInventoryResourceProvider::ComputeMaxCraftable(InventoryComp, R->RequiredMaterials)
		                       : 0;
	bool bAllOK = Qty > 0 && Qty <= MaxUnits;

	for (int32 i = 0; i < CachedMatIDs.Num(); ++i)
	{
		const int32 MatID = CachedMatIDs[i];
		const int32 NeedPerOne = R->RequiredMaterials.FindRef(MatID);
		const int32 Need = NeedPerOne * FMath::Max(1, Qty);
		const int32 Have = InventoryComp ? InventoryComp->GetItemCount(MatID) : 0;

		if (RequiredRowPool.IsValidIndex(i))
		{
			// 숫자/색만 갱신 — 아이콘/레이아웃은 건드리지 않음
			RequiredRowPool[i]->UpdateRow(GameInstance, MatID, Need, Have);
		}

		if (Have < Need) bAllOK = false;
	}

	bCachedCanCraft = bAllOK;
	CachedMaxCraftable = MaxUnits;

	if (CraftButton) CraftButton->SetIsEnabled(bCachedCanCraft);
	if (MaxButton) MaxButton->SetIsEnabled(CachedMaxCraftable > 0);
}

void UCraftingWidget::UpdateInventoryFromData(const TArray<FInventoryItem>& InventoryData)
{
	RefreshDetail();
}

void UCraftingWidget::OnQuantityChanged(float NewValue)
{
	// 요구 재료/보유 텍스트 재계산
	RefreshDetail();
}

void UCraftingWidget::OnRecipeSlotClicked(USlotWidget* ClickedSlot, bool bRightClick)
{
	for (const auto& P : RecipeSlotMap)
	{
		if (P.Value == ClickedSlot)
		{
			SelectRow(P.Key);
			P.Value->SetHighlighted(true);
		}
		else
		{
			P.Value->SetHighlighted(false);
		}
	}
}

void UCraftingWidget::OnClickCraft()
{
	if (!Station || SelectedRow.IsNone()) return;
	const int32 Qty = FMath::Max(1, (int32)(QuantitySpin ? QuantitySpin->GetValue() : 1.f));
	if (APlayerController* PC = GetOwningPlayer())
	{
		if (ASonheimPlayerController* SPC = Cast<ASonheimPlayerController>(PC))
		{
			// 서버에 작업 추가 요청
			SPC->ServerStartWork(Station, SelectedRow, Qty);

			// 제작 UI 종료
			RemoveFromParent();
		}
	}
}

void UCraftingWidget::OnClickMax()
{
	if (!QuantitySpin) return;
	const FCraftingRecipe* R = GetRecipe(SelectedRow);
	if (!R || !InventoryComp)
	{
		QuantitySpin->SetValue(0);
		return;
	}
	const int32 MaxUnits = UInventoryResourceProvider::ComputeMaxCraftable(InventoryComp, R->RequiredMaterials);
	QuantitySpin->SetValue(MaxUnits);
}

void UCraftingWidget::OnClickMin()
{
	if (QuantitySpin) QuantitySpin->SetValue(0);
}

void UCraftingWidget::OnClickAdd()
{
	if (QuantitySpin) QuantitySpin->SetValue(QuantitySpin->GetValue() + 1);
}

void UCraftingWidget::OnClickSub()
{
	if (QuantitySpin) QuantitySpin->SetValue(QuantitySpin->GetValue() - 1);
}

void UCraftingWidget::OnClickedClose()
{
	RemoveFromParent();
}

const FCraftingRecipe* UCraftingWidget::GetRecipe(FName Row) const
{
	if (!Station) return nullptr;
	UDataTable* Table = Station->GetRecipeTable();
	if (!Table) return nullptr;
	return Table->FindRow<FCraftingRecipe>(Row, TEXT("CraftingWidget"));
}

void UCraftingWidget::SelectRow(FName NewRow)
{
	SelectedRow = NewRow;
	RefreshDetail();
	RefreshCraftability();
}

int32 UCraftingWidget::GetCurrentQuantity() const
{
	return FMath::Max(1, (int32)(QuantitySpin ? QuantitySpin->GetValue() : 1.f));
}
