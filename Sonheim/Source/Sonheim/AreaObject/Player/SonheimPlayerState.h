// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "SonheimPlayerState.generated.h"

class ASonheimPlayer;
class USonheimGameInstance;
/**
 * 
 */

UCLASS()
class SONHEIM_API ASonheimPlayerState : public APlayerState
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;

	// 캐릭터 스탯
	UPROPERTY()
	int32 Level;
    
	UPROPERTY()
	int32 Strength;
    
	UPROPERTY()
	int32 Dexterity;
    
	UPROPERTY()
	int32 Vitality;
    
	// ToDo : 인벤토리
	TMap<int, int> Inventory;
    
public:
	void LevelUp();
	// ToDo : bool 반환으로 max 넘기는지 확인로직?
	void AddItem(int ItemID, int ItemValue);
	void RemoveItem(int ItemID, int ItemValue);
	void EquipItem(int ItemID, int ItemValue);

private:
UPROPERTY()
	USonheimGameInstance* m_GameInstance;
UPROPERTY()
	ASonheimPlayer* m_Player;
};
