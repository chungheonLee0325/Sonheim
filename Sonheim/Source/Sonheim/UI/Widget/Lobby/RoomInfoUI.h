// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "OnlineSessionSettings.h"
#include "Blueprint/UserWidget.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Sonheim/Utilities/SessionUtil.h"
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
	UPROPERTY(meta = (BindWidget))
	class UTextBlock* Text_RoomIdx;
	UPROPERTY(meta = (BindWidget))
	class UTextBlock* Text_CurrentPlayer;
	UPROPERTY(meta = (BindWidget))
	class UTextBlock* Text_MaxPlayer;
public:
	UFUNCTION()
	void OnClickedJoinRoom();
	
	void SetInfo(const FOnlineSessionSearchResult& SearchResult, int32 Idx, FString Info);

	FOnJoinSessionCompleteDelegate OnJoinSessionCompleteDelegate;

	FSessionSearchData SessionSearchData;
	
	FOnlineSessionSearchResult SessionSearchResult;

	void OnJoinSession(FName SessionName, EOnJoinSessionCompleteResult::Type Type);

};
