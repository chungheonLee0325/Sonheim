// Fill out your copyright notice in the Description page of Project Settings.

#include "SonheimPlayerState.h"
#include "SonheimPlayer.h"
#include "Sonheim/AreaObject/Attribute/StatBonusComponent.h"
#include "Sonheim/GameManager/SonheimGameInstance.h"
#include "Sonheim/Utilities/LogMacro.h"
#include "Utility/InventoryComponent.h"
#include "Net/UnrealNetwork.h"
#include "Utility/PalInventoryComponent.h"

ASonheimPlayerState::ASonheimPlayerState()
{
	m_InventoryComponent = CreateDefaultSubobject<UInventoryComponent>(TEXT("Inventory"));
	m_StatBonusComponent = CreateDefaultSubobject<UStatBonusComponent>(TEXT("StatBonus"));
	m_PalInventoryComponent = CreateDefaultSubobject<UPalInventoryComponent>(TEXT("PalInventory"));
	
	// 컴포넌트 복제 설정
	m_InventoryComponent->SetIsReplicated(true);
	m_StatBonusComponent->SetIsReplicated(true);
}

void ASonheimPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(ASonheimPlayerState, Level);
	DOREPLIFETIME(ASonheimPlayerState, ReplicatedStats);
	// DOREPLIFETIME(ASonheimPlayerState, m_InventoryComponent);
	// DOREPLIFETIME(ASonheimPlayerState, m_StatBonusComponent);
}

void ASonheimPlayerState::BeginPlay()
{
	Super::BeginPlay();

	m_GameInstance = Cast<USonheimGameInstance>(GetGameInstance());

	// 서버에서만 기본 스탯 초기화
	if (HasAuthority())
	{
		// 기본 스탯 초기화
		BaseStat.Add(EAreaObjectStatType::HP, 300.0f);
		BaseStat.Add(EAreaObjectStatType::Attack, 10.0f);
		BaseStat.Add(EAreaObjectStatType::Defense, 5.0f);
		BaseStat.Add(EAreaObjectStatType::WorkSpeed, 300.0f);
		BaseStat.Add(EAreaObjectStatType::RunSpeed, 600.0f);
		BaseStat.Add(EAreaObjectStatType::JumpHeight, 700.0f);
		BaseStat.Add(EAreaObjectStatType::Stamina, 100.0f);
		BaseStat.Add(EAreaObjectStatType::MaxWeight, 100.0f);

		// 초기 스탯 업데이트
		UpdateStats();
	}

	// 이벤트 바인딩
	if (m_InventoryComponent)
	{
		m_InventoryComponent->OnWeaponChanged.AddDynamic(this, &ASonheimPlayerState::OnWeaponSlotChanged);
	}
	
	if (m_StatBonusComponent)
	{
		m_StatBonusComponent->OnStatChanged.AddDynamic(this, &ASonheimPlayerState::UpdateStat);
		
		// StatBonusComponent에 델리게이트 연결
		m_StatBonusComponent->OnStatBonusChangedDelegate.BindLambda([this](EAreaObjectStatType StatType)
		{
			if (HasAuthority())
			{
				UpdateStat(StatType);
			}
		});
	}
}

void ASonheimPlayerState::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 델리게이트 언바인드 (메모리 누수 방지)
	if (m_InventoryComponent)
	{
		m_InventoryComponent->OnWeaponChanged.RemoveDynamic(this, &ASonheimPlayerState::OnWeaponSlotChanged);
	}
	
	if (m_StatBonusComponent)
	{
		m_StatBonusComponent->OnStatChanged.RemoveDynamic(this, &ASonheimPlayerState::UpdateStat);
		m_StatBonusComponent->OnStatBonusChangedDelegate.Unbind();
	}
	
	// 참조 정리
	m_Player = nullptr;
	m_GameInstance = nullptr;
	
	Super::EndPlay(EndPlayReason);
}

void ASonheimPlayerState::InitPlayerState()
{
	m_Player = Cast<ASonheimPlayer>(GetPawn());
	m_PalInventoryComponent->InitializeWithPlayer(m_Player);
}

float ASonheimPlayerState::GetStatValue(EAreaObjectStatType StatType) const
{
	if (const float* Value = ModifiedStat.Find(StatType))
	{
		return *Value;
	}
	return 0.0f;
}

void ASonheimPlayerState::SetBaseStat(EAreaObjectStatType StatType, float Value)
{
	if (HasAuthority())
	{
		BaseStat.Add(StatType, Value);
		UpdateStats();
	}
}

void ASonheimPlayerState::UpdateStats()
{
	if (!HasAuthority())
		return;

	// 모든 기본 스탯에 대해 수정된 값을 계산
	for (const auto& StatPair : BaseStat)
	{
		float ModifiedValue = m_StatBonusComponent->GetModifiedStatValue(StatPair.Key, StatPair.Value);
		ModifiedStat.Add(StatPair.Key, ModifiedValue);
		OnPlayerStatsChanged.Broadcast(StatPair.Key, ModifiedValue);
	}
	
	// Replicated 배열로 변환
	ConvertStatsToReplicatedArray();
}

void ASonheimPlayerState::UpdateStat(EAreaObjectStatType StatType)
{
	if (!HasAuthority())
		return;
		
	if (const float* BaseValue = BaseStat.Find(StatType))
	{
		float ModifiedValue = m_StatBonusComponent->GetModifiedStatValue(StatType, *BaseValue);
		ModifiedStat.Add(StatType, ModifiedValue);
		OnPlayerStatsChanged.Broadcast(StatType, ModifiedValue);
		
		// Replicated 배열 업데이트
		ConvertStatsToReplicatedArray();
	}
}

void ASonheimPlayerState::ServerUpdateStats_Implementation()
{
	UpdateStats();
}

void ASonheimPlayerState::ConvertStatsToReplicatedArray()
{
	ReplicatedStats.Empty();
	
	for (const auto& StatPair : ModifiedStat)
	{
		ReplicatedStats.Add(FReplicatedStat(StatPair.Key, StatPair.Value));
	}
}

void ASonheimPlayerState::OnRep_ReplicatedStats()
{
	// 클라이언트에서 Replicated 배열을 로컬 맵으로 변환
	ModifiedStat.Empty();
	
	for (const FReplicatedStat& Stat : ReplicatedStats)
	{
		ModifiedStat.Add(Stat.StatType, Stat.Value);
		OnPlayerStatsChanged.Broadcast(Stat.StatType, Stat.Value);
	}
}

ASonheimPlayer* ASonheimPlayerState::GetSonheimPlayer()
{
	if (!m_Player)
	{
		InitPlayerState();
	}
	return m_Player;
}

void ASonheimPlayerState::OnWeaponSlotChanged(EEquipmentSlotType Slot, int ItemID)
{
	if (HasAuthority())
	{
		UpdateStats();
	}
}