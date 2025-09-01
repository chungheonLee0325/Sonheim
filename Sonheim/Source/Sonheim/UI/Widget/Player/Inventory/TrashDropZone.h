#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TrashDropZone.generated.h"

class UInventoryWidget;
class UDragDropSlotOperation;

UCLASS()
class SONHEIM_API UTrashDropZone : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	void BindOwner(UInventoryWidget* InOwner) { OwnerInventory = InOwner; }

	UFUNCTION(BlueprintCallable)
	void SetDiscardMode(bool bInDiscard) { bDiscardMode = bInDiscard; }

protected:
	virtual bool NativeOnDrop(const FGeometry& G, const FDragDropEvent& E, UDragDropOperation* Op) override;
	virtual void NativeOnDragEnter(const FGeometry& G, const FDragDropEvent& E, UDragDropOperation* Op) override;
	virtual void NativeOnDragLeave(const FDragDropEvent& E, UDragDropOperation* Op) override;

	// 드래그 시 시각 효과용(옵션)
	UPROPERTY(meta=(BindWidgetOptional))
	class UBorder* Border_Highlight = nullptr;

	UPROPERTY()
	TObjectPtr<UInventoryWidget> OwnerInventory;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mode")
	bool bDiscardMode = false; // true=폐기, false=버리기
};
