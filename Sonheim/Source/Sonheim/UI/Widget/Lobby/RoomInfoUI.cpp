// Fill out your copyright notice in the Description page of Project Settings.


#include "RoomInfoUI.h"

#include "OnlineSessionSettings.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
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

	FString RoomName;
	SearchResult.Session.SessionSettings.Get(TEXT("RoomName"), RoomName);

	Text_RoomIdx->SetText(FText::AsNumber(Idx));
	Text_RoomName->SetText(FText::FromString(FSessionUtil::DecodeData(RoomName)));

	uint8 MaxPlayerCount = SearchResult.Session.SessionSettings.NumPublicConnections;
	uint8 RemainPlayerCount = SearchResult.Session.NumOpenPublicConnections;
	
	Text_CurrentPlayer->SetText(FText::FromString(FString::FromInt(MaxPlayerCount - RemainPlayerCount)));
	Text_MaxPlayer->SetText(FText::FromString(FString::FromInt(MaxPlayerCount)));
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
