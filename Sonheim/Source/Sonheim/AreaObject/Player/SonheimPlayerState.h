// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "Sonheim/ResourceManager/SonheimGameType.h"
#include "SonheimPlayerState.generated.h"

class UInventoryComponent;
class ASonheimPlayer;
class USonheimGameInstance;
/**
 * 
 */

UCLASS()
class SONHEIM_API ASonheimPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	ASonheimPlayerState();

protected:
	virtual void BeginPlay() override;

	// 캐릭터 스탯
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Stats")
	int32 Level = 1;
    
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Stats")
	int32 Strength = 10;
    
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Stats")
	int32 Dexterity = 10;
    
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Stats")
	int32 Vitality = 10;

public:
	UPROPERTY(BlueprintReadWrite, Category="Inventory")
	UInventoryComponent* InventoryComponent;
	
private:
	UPROPERTY()
	USonheimGameInstance* m_GameInstance;
	UPROPERTY()
	ASonheimPlayer* m_Player;
};
