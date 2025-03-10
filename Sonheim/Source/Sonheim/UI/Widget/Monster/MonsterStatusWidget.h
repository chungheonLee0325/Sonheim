// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Sonheim/UI/Widget/BaseStatusWidget.h"
#include "MonsterStatusWidget.generated.h"

struct FAreaObjectData;
class UOverlay;
enum class EElementalAttribute : uint8;
/**
 * 
 */
class UImage;

UCLASS()
class SONHEIM_API UMonsterStatusWidget : public UBaseStatusWidget
{
public:
	GENERATED_BODY()
	
	void InitMonsterStatusWidget(FAreaObjectData* AreaObjectData,bool Gender , int Level);

	UPROPERTY(meta = (BindWidget))
	UTextBlock* MonsterNameText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* GenderText;
	
	UPROPERTY(meta = (BindWidget))
	UTextBlock* LevelText;

	// Single Element Attribute Type UI
	UPROPERTY(meta = (BindWidget))
	UOverlay* TypeSingleOverlay;
	UPROPERTY(meta = (BindWidget))
	UImage* TypeSingleImage;

	// Multi Element Attribute Type UI
	UPROPERTY(meta = (BindWidget))
	UOverlay* TypeMultiOverlay;
	UPROPERTY(meta = (BindWidget))
	UImage* TypeOneImage;
	UPROPERTY(meta = (BindWidget))
	UImage* TypeTwoImage;
	UPROPERTY(meta = (BindWidget))
	UImage* TypeThreeImage;

private:
	FLinearColor CalcImageByElementAttributes(EElementalAttribute Type);
};
