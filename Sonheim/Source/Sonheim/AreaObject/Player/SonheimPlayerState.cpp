// Fill out your copyright notice in the Description page of Project Settings.


#include "SonheimPlayerState.h"

#include "SonheimPlayer.h"
#include "Sonheim/GameManager/SonheimGameInstance.h"
#include "Sonheim/Utilities/LogMacro.h"

void ASonheimPlayerState::BeginPlay()
{
	Super::BeginPlay();

	m_GameInstance = Cast<USonheimGameInstance>(GetGameInstance());
	m_Player = Cast<ASonheimPlayer>(GetOwner());
}

void ASonheimPlayerState::LevelUp()
{
}

void ASonheimPlayerState::AddItem(int ItemID, int ItemValue)
{
	if (Inventory.Find(ItemID))
	{
		Inventory[ItemID] += ItemValue;
		FLog::Log("<Item ID, Item Value> : ", ItemID, Inventory[ItemID]);
	}
	else
	{
		Inventory.Add(ItemID, ItemValue);
		FLog::Log("<Item ID, Item Value> : ", ItemID, ItemValue);
	}
}

void ASonheimPlayerState::RemoveItem(int ItemID, int ItemValue)
{
	// ToDo : 
}

void ASonheimPlayerState::EquipItem(int ItemID, int ItemValue)
{
	// ToDo : 
}
