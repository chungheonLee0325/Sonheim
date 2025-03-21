// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "Sonheim/ResourceManager/SonheimGameType.h"
#include "Sonheim/AreaObject/Player/SonheimPlayerState.h"
#include "PlayerStatWidget.generated.h"

/**
 * 
 */
UCLASS()
class SONHEIM_API UPlayerStatWidget : public UUserWidget
{
	GENERATED_BODY()

	public:
	virtual void NativeConstruct() override;

	void UpdatePlayerStat(EAreaObjectStatType StatType, float StatValue);

	UFUNCTION(BlueprintCallable, Category="PlayerStats")
	void InitializePlayerStatWidget(ASonheimPlayerState* PlayerState);

protected:
	UPROPERTY(meta = (BindWidget))
	UTextBlock* HPText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* StaminaText;
	
	UPROPERTY(meta = (BindWidget))
	UTextBlock* AttackText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* DefenceText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* WorkSpeedText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* TotalWeightText;

private:
	// 델리게이트 바인딩 해제를 위한 참조 저장
	UPROPERTY()
	ASonheimPlayerState* CurrentPlayerState;
	
	// 델리게이트 콜백 함수
	UFUNCTION()
	void OnPlayerAllStatsUpdated(const TMap<EAreaObjectStatType, float>& NewStats);
	UFUNCTION()
	void OnPlayerStatsUpdated(EAreaObjectStatType StatType, float StatValue);
	void NativeDestruct();
};
