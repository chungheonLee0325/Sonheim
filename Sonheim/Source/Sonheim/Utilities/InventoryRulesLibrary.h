// InventoryRulesLibrary.h

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Sonheim/ResourceManager/SonheimGameType.h"
#include "InventoryRulesLibrary.generated.h"

UCLASS()
class SONHEIM_API UInventoryRulesLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// 슬롯 종류와 장비 종류의 적합성만 판정 (순수 로직)
	UFUNCTION(BlueprintPure, Category="Inventory|Rules")
	static bool AcceptsSlotByKind(EEquipmentSlotType SlotType, EEquipmentKindType EquipKind);

	// ItemID 기반으로 슬롯 적합성 판정 (GameInstance 사용)
	UFUNCTION(BlueprintPure, Category="Inventory|Rules", meta=(WorldContext="WorldContextObject"))
	static bool IsItemCompatibleWithSlot(const UObject* WorldContextObject, int32 ItemID, EEquipmentSlotType SlotType);
};
