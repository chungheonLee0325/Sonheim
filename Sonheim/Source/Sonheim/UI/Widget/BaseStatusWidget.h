// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BaseStatusWidget.generated.h"

class UProgressBar;
class UTextBlock;
class UImage;
/**
 * 
 */
UCLASS()
class SONHEIM_API UBaseStatusWidget : public UUserWidget
{
	GENERATED_BODY()

	
protected:
	virtual void NativeConstruct() override;
    
public:
	UFUNCTION()
	void UpdateHealth(float CurrentHP, float Delta, float MaxHP);

protected:
	UPROPERTY(meta = (BindWidget))
	UProgressBar* HealthBar;
    
	UPROPERTY(meta = (BindWidget))
	UTextBlock* HealthText;
}; 

