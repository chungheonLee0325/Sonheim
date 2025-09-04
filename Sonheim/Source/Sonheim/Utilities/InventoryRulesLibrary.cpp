// InventoryRulesLibrary.cpp

#include "InventoryRulesLibrary.h"
#include "Sonheim/GameManager/SonheimGameInstance.h"

bool UInventoryRulesLibrary::AcceptsSlotByKind(EEquipmentSlotType SlotType, EEquipmentKindType EquipKind)
{
	switch (SlotType)
	{
	case EEquipmentSlotType::Head: return EquipKind == EEquipmentKindType::Head;
	case EEquipmentSlotType::Body: return EquipKind == EEquipmentKindType::Body;
	case EEquipmentSlotType::Shield: return EquipKind == EEquipmentKindType::Shield;
	case EEquipmentSlotType::Glider: return EquipKind == EEquipmentKindType::Glider;
	case EEquipmentSlotType::SphereModule: return EquipKind == EEquipmentKindType::SphereModule;
	case EEquipmentSlotType::Accessory1:
	case EEquipmentSlotType::Accessory2: return EquipKind == EEquipmentKindType::Accessory;
	case EEquipmentSlotType::Weapon1:
	case EEquipmentSlotType::Weapon2:
	case EEquipmentSlotType::Weapon3:
	case EEquipmentSlotType::Weapon4: return EquipKind == EEquipmentKindType::Weapon;
	default: return false;
	}
}

bool UInventoryRulesLibrary::IsItemCompatibleWithSlot(const UObject* WorldContextObject, int32 ItemID,
                                                      EEquipmentSlotType SlotType)
{
	if (!WorldContextObject || ItemID <= 0 || SlotType == EEquipmentSlotType::None)
		return false;

	if (UWorld* World = WorldContextObject->GetWorld())
	{
		if (USonheimGameInstance* GI = World->GetGameInstance<USonheimGameInstance>())
		{
			if (FItemData* ItemData = GI->GetDataItem(ItemID))
			{
				if (!(ItemData->ItemCategory == EItemCategory::Equipment || ItemData->ItemCategory ==
					EItemCategory::Weapon))
					return false;
				return AcceptsSlotByKind(SlotType, ItemData->EquipmentData.EquipKind);
			}
		}
	}
	return false;
}
