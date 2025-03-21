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
}