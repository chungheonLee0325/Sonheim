// Fill out your copyright notice in the Description page of Project Settings.


#include "RoomInfoUI.h"

#include "OnlineSessionSettings.h"
#include "Components/Button.h"
#include "Sonheim/Utilities/SessionUtil.h"


void URoomInfoUI::NativeConstruct()
{
	Super::NativeConstruct();

	Button_JoinRoom->OnClicked.AddDynamic(this, &URoomInfoUI::OnClickedJoinRoom);

	OnJoinSessionCompleteDelegate = FOnJoinSessionCompleteDelegate::CreateUObject(this, &URoomInfoUI::OnJoinSession);
}

void URoomInfoUI::SetInfo(const FOnlineSessionSearchResult& SearchResult, int32 Idx, FString Info)
{
	SessionSearchResult = SearchResult;
}

void URoomInfoUI::OnClickedJoinRoom()
{
	FSessionUtil::JoinSession(GetWorld(), SessionSearchResult,
		OnJoinSessionCompleteDelegate);
}

void URoomInfoUI::OnJoinSession(FName SessionName, EOnJoinSessionCompleteResult::Type Type)
{
	FString Address;
	if (FSessionUtil::OnlineSessionInterface->GetResolvedConnectString(NAME_GameSession, Address))
	{
		GetOwningPlayer()->ClientTravel(Address, TRAVEL_Absolute);
	}
}
