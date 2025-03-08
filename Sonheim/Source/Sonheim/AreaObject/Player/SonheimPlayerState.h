// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "SonheimPlayerState.generated.h"

/**
 * 
 */
UCLASS()
class SONHEIM_API ASonheimPlayerState : public APlayerState
{
	GENERATED_BODY()

protected:
	// 캐릭터 스탯
	UPROPERTY()
	int32 Level;
    
	UPROPERTY()
	int32 Strength;
    
	UPROPERTY()
	int32 Dexterity;
    
	UPROPERTY()
	int32 Vitality;
    
	// 인벤토리
	//UPROPERTY()
	//TArray<class UItemBase*> Inventory;
    
public:
	void LevelUp();
	//void AddItem(UItemBase* Item);
	//void RemoveItem(UItemBase* Item);
	//void EquipItem(UItemBase* Item);
};
