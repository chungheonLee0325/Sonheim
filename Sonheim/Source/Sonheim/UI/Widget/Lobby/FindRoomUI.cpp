// Fill out your copyright notice in the Description page of Project Settings.


#include "FindRoomUI.h"

#include "OnlineSessionSettings.h"
#include "Components/Button.h"
#include "Sonheim/Utilities/CommonUtil.h"
#include "Sonheim/Utilities/SessionUtil.h"

void UFindRoomUI::NativeConstruct()
{
	Super::NativeConstruct();

	Btn_BackFromFind->OnClicked.AddDynamic(this, &UFindRoomUI::OnClickedBackFromFind);
	Btn_Find->OnClicked.AddDynamic(this, &UFindRoomUI::OnClickedFind);

	OnJoinSessionCompleteDelegate = FOnJoinSessionCompleteDelegate::CreateUObject(this, &UFindRoomUI::OnJoinSession);
}

void UFindRoomUI::OnClickedBackFromFind()
{
	OnBackButtonClicked.ExecuteIfBound();
}

void UFindRoomUI::OnClickedFind()
{
	
	SessionSearchData.SessionSearch = MakeShareable(new FOnlineSessionSearch());
	SessionSearchData.OnFindSessionsCompleteDelegate = FOnFindSessionsCompleteDelegate::CreateUObject(
	this, &UFindRoomUI::OnCompleteSearch);
	
	FSessionUtil::SearchSession(SessionSearchData);


}

void UFindRoomUI::OnJoinSession(FName SessionName, EOnJoinSessionCompleteResult::Type Type)
{
	FString Address;
	if (FSessionUtil::OnlineSessionInterface->GetResolvedConnectString(NAME_GameSession, Address))
	{
		GetOwningPlayer()->ClientTravel(Address, TRAVEL_Absolute);
	}
}

void UFindRoomUI::OnCompleteSearch(bool bIsSuccess)
{
	if (!bIsSuccess)
	{
		UE_LOG(LogTemp, Error, TEXT("방을 검색하는 것에 실패하였습니다."));
		return;
	}

	for (int i = 0; i < SessionSearchData.SessionSearch->SearchResults.Num(); i++)
	{
		FSessionUtil::JoinSession(GetWorld(), SessionSearchData.SessionSearch->SearchResults[i],
			OnJoinSessionCompleteDelegate);
	}
}
