// Fill out your copyright notice in the Description page of Project Settings.


#include "SonheimPlayerState.h"

#include "SonheimPlayer.h"
#include "Sonheim/AreaObject/Attribute/StatBonusComponent.h"
#include "Sonheim/GameManager/SonheimGameInstance.h"
#include "Sonheim/Utilities/LogMacro.h"
#include "Utility/InventoryComponent.h"

ASonheimPlayerState::ASonheimPlayerState()
{
	m_InventoryComponent = CreateDefaultSubobject<UInventoryComponent>(TEXT("Inventory"));
	m_StatBonusComponent = CreateDefaultSubobject<UStatBonusComponent>(TEXT("StatBonus"));
}

void ASonheimPlayerState::BeginPlay()
{
	Super::BeginPlay();

	m_GameInstance = Cast<USonheimGameInstance>(GetGameInstance());
	m_Player = Cast<ASonheimPlayer>(GetOwner());

	// 기본 스탯 초기화
	BaseStat.Add(EAreaObjectStatType::HP, 100.0f);
	BaseStat.Add(EAreaObjectStatType::Attack, 10.0f);
	BaseStat.Add(EAreaObjectStatType::Defense, 5.0f);
	BaseStat.Add(EAreaObjectStatType::WorkSpeed, 300.0f);
	BaseStat.Add(EAreaObjectStatType::RunSpeed, 600.0f);
	BaseStat.Add(EAreaObjectStatType::JumpHeight, 420.0f);
	BaseStat.Add(EAreaObjectStatType::Stamina, 100.0f);
	BaseStat.Add(EAreaObjectStatType::MaxWeight, 100.0f);

	// 수정된 스탯 초기화 (기본값과 동일하게 시작)
	for (const TPair<EAreaObjectStatType, float>& StatPair : BaseStat)
	{
		ModifiedStat.Add(StatPair.Key, StatPair.Value);
	}

	// 무기 슬롯 변경 이벤트에 바인딩
	if (m_InventoryComponent)
	{
		m_InventoryComponent->OnWeaponChanged.AddDynamic(this, &ASonheimPlayerState::OnWeaponSlotChanged);
	}
	if (m_StatBonusComponent)
	{
		m_StatBonusComponent->OnStatChanged.AddDynamic(this, &ASonheimPlayerState::UpdateStat);
	}

	// 초기 스탯 업데이트
	UpdateStats();
}

float ASonheimPlayerState::GetStatValue(EAreaObjectStatType StatType) const
{
	if (ModifiedStat.Contains(StatType))
	{
		return ModifiedStat[StatType];
	}
	return 0.0f;
}

void ASonheimPlayerState::SetBaseStat(EAreaObjectStatType StatType, float Value)
{
	BaseStat.Add(StatType, Value);
	UpdateStats();
}

void ASonheimPlayerState::UpdateStats()
{
	// 모든 기본 스탯에 대해 수정된 값을 계산
	for (const TPair<EAreaObjectStatType, float>& StatPair : BaseStat)
	{
		float ModifiedValue = m_StatBonusComponent->GetModifiedStatValue(StatPair.Key, StatPair.Value);
		ModifiedStat.Add(StatPair.Key, ModifiedValue);
		OnPlayerStatsChanged.Broadcast(StatPair.Key, ModifiedValue);
	}
}

void ASonheimPlayerState::UpdateStat(EAreaObjectStatType StatType)
{
	float ModifiedValue = m_StatBonusComponent->GetModifiedStatValue(StatType, BaseStat[StatType]);
	ModifiedStat.Add(StatType, ModifiedValue);
	OnPlayerStatsChanged.Broadcast(StatType, ModifiedValue);
}

void ASonheimPlayerState::OnWeaponSlotChanged(EEquipmentSlotType Slot, int ItemID)
{
	// 무기 슬롯이 변경되었을 때 스탯 재계산
	UpdateStats();
}
