#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Sonheim/GameObject/Buildings/Storage/BaseContainer.h"
#include "Sonheim/ResourceManager/SonheimGameType.h"
#include "ContainerWidget.generated.h"

class USlotWidget;
class UUniformGridPanel;
class UContainerComponent;
class UTextBlock;

UCLASS()
class SONHEIM_API UContainerWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// UI 컴포넌트
	UPROPERTY(meta = (BindWidget))
	UTextBlock* ContainerNameText;

	UPROPERTY(meta = (BindWidget))
	UUniformGridPanel* SlotGrid;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid Settings")
	TSubclassOf<USlotWidget> SlotWidgetClass;

	UPROPERTY()
	TArray<USlotWidget*> SlotWidgets;

	// 컨테이너 컴포넌트
	UPROPERTY(BlueprintReadWrite)
	UContainerComponent* ContainerComponent;

	// 동적 그리드 크기
	int32 GridColumns = 5;
	int32 GridRows = 4;

public:
	// 컨테이너 설정
	UFUNCTION(BlueprintCallable, Category = "Container")
	void SetContainerComponent(UContainerComponent* InContainerComponent);

	// 컨테이너 이름 설정
	UFUNCTION(BlueprintCallable, Category = "Container")
	void SetContainerName(const FText& Name);

	// 그리드 크기 설정
	UFUNCTION(BlueprintCallable, Category = "Container")
	void SetGridSize(int32 Columns, int32 Rows);

	// 인벤토리 업데이트
	UFUNCTION()
	void UpdateContainerInventory(const TArray<FInventoryItem>& Items);
	
	// 슬롯 이벤트 핸들러
	UFUNCTION()
	void OnSlotClicked(USlotWidget* SlotWidget, bool bIsRightClick);

	UFUNCTION()
	void OnSlotDragStarted(USlotWidget* SlotWidget);

	UFUNCTION()
	void OnSlotDropped(USlotWidget* FromSlot, USlotWidget* ToSlot);

	// 외부에서 드롭 받기 위한 퍼블릭 함수
	void HandleExternalDrop(USlotWidget* FromSlot, int32 ToIndex);

	UContainerComponent* GetContainerComponent() const {return ContainerComponent;};

	void SetOwningContainer(class ABaseContainer* Container) { OwningContainer = Container; }
	
	class ABaseContainer* GetOwningContainer() const { return OwningContainer; }

private:
	UPROPERTY()
	class USonheimGameInstance* GameInstance;
	UPROPERTY()
	class ABaseContainer* OwningContainer;
	
	void CreateSlotGrid();
	void BindSlotEvents(USlotWidget* SlotWidget);
	void ClearAllSlots();
};