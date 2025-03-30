// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RoomInfoUI.generated.h"

/**
 * 
 */
UCLASS()
class SONHEIM_API URoomInfoUI : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	
	UPROPERTY(meta = (BindWidget))
	class UTextBlock* Text_RoomName;
	UPROPERTY(meta = (BindWidget))
	class UButton* Button_JoinRoom;

	int32 RoomIdx;

public:
	UFUNCTION()
	void OnClickedJoinRoom();
	
	void SetInfo(int32 Idx, FString Info);
};
