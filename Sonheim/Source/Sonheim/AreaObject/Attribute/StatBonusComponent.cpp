// Fill out your copyright notice in the Description page of Project Settings.

#include "StatBonusComponent.h"
#include "Sonheim/GameManager/SonheimGameInstance.h"
#include "Sonheim/Utilities/LogMacro.h"

UStatBonusComponent::UStatBonusComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UStatBonusComponent::AddStatBonus(EAreaObjectStatType StatType, float Value, EStatModifierType ModType, int SourceID)
{
	FStatModifier NewModifier(StatType, Value, ModType, SourceID);
	
	// 해당 스탯 유형에 대한 배열이 없으면 생성
	if (!StatBonuses.Contains(StatType))
	{
		StatBonuses.Add(StatType, TArray<FStatModifier>());
	}
	
	// 보너스 추가
	StatBonuses[StatType].Add(NewModifier);
	
	// 소스 ID가 있고 아이템이면 아이템 보너스에도 추가
	if (SourceID > 0)
	{
		if (!ItemStatBonuses.Contains(SourceID))
		{
			ItemStatBonuses.Add(SourceID, TArray<FStatModifier>());
		}
		
		ItemStatBonuses[SourceID].Add(NewModifier);
	}

	OnStatChanged.Broadcast(StatType);
}

void UStatBonusComponent::RemoveStatBonus(EAreaObjectStatType StatType, float Value, EStatModifierType ModType, int SourceID)
{
	if (!StatBonuses.Contains(StatType))
	{
		return;
	}
	
	TArray<FStatModifier>& Modifiers = StatBonuses[StatType];
	
	// 일치하는 첫 번째 수정자 찾아 제거
	for (int i = 0; i < Modifiers.Num(); i++)
	{
		if (Modifiers[i].Value == Value && 
			Modifiers[i].ModifierType == ModType && 
			Modifiers[i].SourceID == SourceID)
		{
			Modifiers.RemoveAt(i);
			break;
		}
	}
	
	OnStatChanged.Broadcast(StatType);
}

void UStatBonusComponent::ApplyItemStatBonuses(int ItemID, bool bApply)
{
	// 게임 인스턴스에서 아이템 데이터 가져오기
	USonheimGameInstance* GameInstance = Cast<USonheimGameInstance>(GetWorld()->GetGameInstance());
	if (!GameInstance)
	{
		return;
	}
	
	FItemData* ItemData = GameInstance->GetDataItem(ItemID);
	if (!ItemData)
	{
		return;
	}
	
	// 장비 종류가 무기인 경우, 별도 처리하지 않음
	// 무기는 RegisterEquippedItem에서 처리됨
	if (ItemData->EquipmentData.EquipKind == EEquipmentKindType::Weapon)
	{
		return;
	}
	
	if (bApply)
	{
		// HP 보너스 적용
		if (ItemData->EquipmentData.HPBonus != 0)
		{
			AddStatBonus(EAreaObjectStatType::HP, ItemData->EquipmentData.HPBonus, EStatModifierType::Additive, ItemID);
		}
		
		// 스태미나 보너스 적용
		if (ItemData->EquipmentData.StaminaBonus != 0)
		{
			AddStatBonus(EAreaObjectStatType::Stamina, ItemData->EquipmentData.StaminaBonus, EStatModifierType::Additive, ItemID);
		}
		
		// 공격력 보너스 적용
		if (ItemData->EquipmentData.DamageBonus != 0)
		{
			AddStatBonus(EAreaObjectStatType::Attack, ItemData->EquipmentData.DamageBonus, EStatModifierType::Additive, ItemID);
		}
		
		// 방어력 보너스 적용
		if (ItemData->EquipmentData.DefenseBonus != 0)
		{
			AddStatBonus(EAreaObjectStatType::Defense, ItemData->EquipmentData.DefenseBonus, EStatModifierType::Additive, ItemID);
		}
		
		// 추가 스탯 적용...
	}
	else
	{
		// 이 아이템과 관련된 모든 보너스 제거
		RemoveAllBonusesFromSource(ItemID);
	}
}

void UStatBonusComponent::RegisterEquippedItem(EEquipmentSlotType SlotType, int ItemID, bool bEquipping)
{
	// 무기 슬롯인지 확인
	bool bIsWeaponSlot = (SlotType == EEquipmentSlotType::Weapon1 ||
						 SlotType == EEquipmentSlotType::Weapon2 ||
						 SlotType == EEquipmentSlotType::Weapon3 ||
						 SlotType == EEquipmentSlotType::Weapon4);
	
	// 게임 인스턴스 가져오기
	USonheimGameInstance* GameInstance = Cast<USonheimGameInstance>(GetWorld()->GetGameInstance());
	if (!GameInstance)
	{
		return;
	}
	
	// 아이템 데이터 가져오기
	FItemData* ItemData = GameInstance->GetDataItem(ItemID);
	if (!ItemData)
	{
		return;
	}
	
	if (bEquipping)
	{
		// 무기 슬롯에 장착된 경우
		if (bIsWeaponSlot)
		{
			// 무기 슬롯별 보너스 저장
			TArray<FStatModifier> WeaponBonuses;
			
			// HP 보너스 추가
			if (ItemData->EquipmentData.HPBonus != 0)
			{
				WeaponBonuses.Add(FStatModifier(EAreaObjectStatType::HP, 
					ItemData->EquipmentData.HPBonus, EStatModifierType::Additive, ItemID));
			}
			
			// 스태미나 보너스 추가
			if (ItemData->EquipmentData.StaminaBonus != 0)
			{
				WeaponBonuses.Add(FStatModifier(EAreaObjectStatType::Stamina, 
					ItemData->EquipmentData.StaminaBonus, EStatModifierType::Additive, ItemID));
			}
			
			// 공격력 보너스 추가
			if (ItemData->EquipmentData.DamageBonus != 0)
			{
				WeaponBonuses.Add(FStatModifier(EAreaObjectStatType::Attack, 
					ItemData->EquipmentData.DamageBonus, EStatModifierType::Additive, ItemID));
			}
			
			// 방어력 보너스 추가
			if (ItemData->EquipmentData.DefenseBonus != 0)
			{
				WeaponBonuses.Add(FStatModifier(EAreaObjectStatType::Defense, 
					ItemData->EquipmentData.DefenseBonus, EStatModifierType::Additive, ItemID));
			}
			
			// 무기 슬롯 보너스 맵에 추가
			WeaponSlotBonuses.Add(SlotType, WeaponBonuses);
			
			// 현재 활성화된 무기 슬롯인 경우에만 실제 스탯에 적용
			if (SlotType == CurrentWeaponSlot)
			{
				for (const FStatModifier& Mod : WeaponBonuses)
				{
					AddStatBonus(Mod.StatType, Mod.Value, Mod.ModifierType, Mod.SourceID);
				}
			}
		}
		else
		{
			// 무기가 아닌 경우 일반적인 방식으로 스탯 적용
			ApplyItemStatBonuses(ItemID, true);
		}
	}
	else
	{
		// 무기 슬롯의 장비를 해제하는 경우
		if (bIsWeaponSlot)
		{
			// 현재 활성화된 무기 슬롯인 경우 스탯에서 제거
			if (SlotType == CurrentWeaponSlot)
			{
				if (WeaponSlotBonuses.Contains(SlotType))
				{
					for (const FStatModifier& Mod : WeaponSlotBonuses[SlotType])
					{
						RemoveStatBonus(Mod.StatType, Mod.Value, Mod.ModifierType, Mod.SourceID);
					}
				}
			}
			
			// 무기 슬롯 보너스 맵에서 제거
			WeaponSlotBonuses.Remove(SlotType);
		}
		else
		{
			// 무기가 아닌 경우 일반적인 방식으로 스탯 제거
			RemoveAllBonusesFromSource(ItemID);
		}
	}
}

void UStatBonusComponent::SetCurrentWeaponSlot(EEquipmentSlotType SlotType)
{
	// 이전 무기 슬롯의 보너스 제거
	if (WeaponSlotBonuses.Contains(CurrentWeaponSlot))
	{
		for (const FStatModifier& Mod : WeaponSlotBonuses[CurrentWeaponSlot])
		{
			RemoveStatBonus(Mod.StatType, Mod.Value, Mod.ModifierType, Mod.SourceID);
		}
	}
	
	// 현재 무기 슬롯 업데이트
	CurrentWeaponSlot = SlotType;
	
	// 새 무기 슬롯의 보너스 적용
	if (WeaponSlotBonuses.Contains(CurrentWeaponSlot))
	{
		for (const FStatModifier& Mod : WeaponSlotBonuses[CurrentWeaponSlot])
		{
			AddStatBonus(Mod.StatType, Mod.Value, Mod.ModifierType, Mod.SourceID);
		}
	}
}

float UStatBonusComponent::GetModifiedStatValue(EAreaObjectStatType StatType, float BaseValue)
{
	// 이 스탯 유형에 대한 보너스가 없으면 기본값 반환
	if (!StatBonuses.Contains(StatType))
	{
		return BaseValue;
	}
	
	float FinalValue = BaseValue;
	float MultiplicativeTotal = 1.0f;
	bool bHasOverride = false;
	float OverrideValue = 0.0f;
	
	// 각 수정자 적용
	for (const FStatModifier& Mod : StatBonuses[StatType])
	{
		switch (Mod.ModifierType)
		{
		case EStatModifierType::Additive:
			FinalValue += Mod.Value;
			break;
			
		case EStatModifierType::Multiplicative:
			MultiplicativeTotal += Mod.Value;
			break;
			
		case EStatModifierType::Override:
			// 덮어쓰기 수정자가 여러 개라면 마지막 값이 적용됨
			bHasOverride = true;
			OverrideValue = Mod.Value;
			break;
		}
	}
	
	// 곱하기 보너스 적용
	FinalValue *= MultiplicativeTotal;
	
	// 덮어쓰기가 있으면 모든 계산을 무시하고 그 값으로 설정
	if (bHasOverride)
	{
		return OverrideValue;
	}
	
	return FinalValue;
}

void UStatBonusComponent::ClearAllStatBonuses(EAreaObjectStatType StatType)
{
	if (StatBonuses.Contains(StatType))
	{
		StatBonuses[StatType].Empty();
	}
}

void UStatBonusComponent::ClearAllBonuses()
{
	StatBonuses.Empty();
	ItemStatBonuses.Empty();
}

void UStatBonusComponent::RemoveAllBonusesFromSource(int SourceID)
{
	// 소스 ID로 등록된 모든 보너스 찾아 제거
	if (ItemStatBonuses.Contains(SourceID))
	{
		TArray<FStatModifier>& SourceModifiers = ItemStatBonuses[SourceID];
		
		// 각 스탯 유형별 보너스에서도 제거
		for (const FStatModifier& Mod : SourceModifiers)
		{
			if (StatBonuses.Contains(Mod.StatType))
			{
				RemoveStatBonus(Mod.StatType, Mod.Value, Mod.ModifierType, Mod.SourceID);
				//TArray<FStatModifier>& TypeModifiers = StatBonuses[Mod.StatType];
				//for (int i = TypeModifiers.Num() - 1; i >= 0; i--)
				//{
				//	if (TypeModifiers[i].SourceID == SourceID)
				//	{
				//		//TypeModifiers.RemoveAt(i);
				//	}
				//}
			}
		}
		
		// 소스 목록에서 제거
		ItemStatBonuses.Remove(SourceID);
	}
} 