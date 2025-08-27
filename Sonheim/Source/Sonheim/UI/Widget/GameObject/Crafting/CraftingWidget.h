#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Sonheim/ResourceManager/SonheimGameType.h"
#include "CraftingWidget.generated.h"

class UImage;
class ACraftingStation;
class USlotWidget;
class UWrapBox;
class UVerticalBox;
class UTextBlock;
class UButton;
class UProgressBar;
class USpinBox;
class UInventoryComponent;
class USonheimGameInstance;

UCLASS()
class SONHEIM_API UCraftingWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	void Initialise(ACraftingStation* InStation);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// 레시피 목록 UI를 데이터 테이블 기준으로 다시 구성(슬롯 생성/바인딩)하고 선택 상태를 유지한다. - UI 구성할때 한번만 호출
	void RefreshRecipes();

	// 각 레시피의 제작 가능 여부를 계산해 슬롯에 ‘시각적 표시’만 반영한다(회색 처리/불투명도 조정, 클릭은 제한하지 않음).
	void RefreshCraftability();
	
	// 현재 선택된 레시피로 우측 상세 패널을 갱신한다
	//  (레시피 변경 시: 이름/아이콘/재료 레이아웃 재구성, 그 외: 보유/필요 수량·색·버튼 상태만 업데이트).
	void RefreshDetail();
	
	UFUNCTION()
	void UpdateInventoryFromData(const TArray<FInventoryItem>& InventoryData);
	
	UFUNCTION()
	void OnQuantityChanged(float NewValue);

	UFUNCTION()
	void OnRecipeSlotClicked(class USlotWidget* ClickedSlot, bool bRightClick);
	UFUNCTION()
	void OnClickCraft();
	UFUNCTION()
	void OnClickMax();
	UFUNCTION()
	void OnClickMin();
	UFUNCTION()
	void OnClickAdd();
	UFUNCTION()
	void OnClickSub();
	UFUNCTION()
	void OnClickedClose();

	int32 ComputeMaxCraftableForRow(FName Row) const;
	int32 GetOwnedCount(int32 ItemID) const;
	const struct FCraftingRecipe* GetRecipe(FName Row) const;
	void SelectRow(FName NewRow);

private:
	// 이름/아이콘/행 레이아웃(아이콘)만
	void RebuildStaticForRecipe(const struct FCraftingRecipe* R);
	// 보유/필요 수량, 색, 버튼 상태만
	void RefreshDynamicForRecipe(const struct FCraftingRecipe* R);
	
	UPROPERTY()
	ACraftingStation* Station = nullptr;
	UPROPERTY()
	USonheimGameInstance* GameInstance = nullptr;
	UPROPERTY()
	UInventoryComponent* InventoryComp = nullptr;
	UPROPERTY()
	FName SelectedRow = NAME_None;

	UPROPERTY()
	TMap<FName, USlotWidget*> RecipeSlotMap;

	UPROPERTY(meta=(BindWidget))
	UWrapBox* RecipeWrap = nullptr;
	UPROPERTY(meta=(BindWidget))
	UVerticalBox* RequiredList = nullptr;
	UPROPERTY(meta=(BindWidget))
	UImage* ItemIcon = nullptr;
	UPROPERTY(meta=(BindWidget))
	UTextBlock* ItemName = nullptr;
	UPROPERTY(meta=(BindWidget))
	UTextBlock* ItemResultQuantity = nullptr;
	UPROPERTY(meta=(BindWidget))
	UTextBlock* OwnedCountText = nullptr;
	UPROPERTY(meta=(BindWidget))
	USpinBox* QuantitySpin = nullptr;
	UPROPERTY(meta=(BindWidget))
	UButton* CraftButton = nullptr;
	UPROPERTY(meta=(BindWidget))
	UButton* MaxButton = nullptr;
	UPROPERTY(meta=(BindWidget))
	UButton* MinButton = nullptr;
	UPROPERTY(meta=(BindWidget))
	UButton* AddButton = nullptr;
	UPROPERTY(meta=(BindWidget))
	UButton* SubButton = nullptr;
	UPROPERTY(meta=(BindWidget))
	UButton* CloseButton = nullptr;

	UPROPERTY(EditAnywhere, Category="Crafting|UI")
	TSubclassOf<USlotWidget> SlotWidgetClass;

	// 요구 재료 레이아웃(행 위젯) 풀 + 현재 레시피의 재료 ID 순서 캐시
	UPROPERTY(EditAnywhere, Category="Crafting|UI")
	TSubclassOf<class URequiredMatRowWidget> RequiredRowClass;
	UPROPERTY()
	TArray<class URequiredMatRowWidget*> RequiredRowPool;
	UPROPERTY()
	TArray<int32> CachedMatIDs;
	FName LastRecipeRow;
	
	int32 GetCurrentQuantity() const;

	bool bCachedCanCraft = false;
	int32 CachedMaxCraftable = 0;
};
