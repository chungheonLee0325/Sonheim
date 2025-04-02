// Fill out your copyright notice in the Description page of Project Settings.


#include "FindRoomUI.h"

#include "OnlineSessionSettings.h"
#include "Components/Button.h"
#include "Components/ScrollBox.h"
#include "RoomInfoUI.h"
#include "Components/VerticalBoxSlot.h"
#include "Sonheim/Utilities/SessionUtil.h"

void UFindRoomUI::NativeConstruct()
{
	Super::NativeConstruct();

	Btn_BackFromFind->OnClicked.AddDynamic(this, &UFindRoomUI::OnClickedBackFromFind);
	Btn_Find->OnClicked.AddDynamic(this, &UFindRoomUI::OnClickedFind);
}

void UFindRoomUI::OnClickedBackFromFind()
{
	OnBackButtonClicked.ExecuteIfBound();
}

void UFindRoomUI::OnClickedFind()
{
	// Scroll_RoomList 자식들 다 지우고
	Scroll_RoomList->ClearChildren();
	
	// 검색 버튼 비활성화
	Btn_Find->SetIsEnabled(false);

	// 검색
	SessionSearchData.SessionSearch = MakeShareable(new FOnlineSessionSearch());
	SessionSearchData.OnFindSessionsCompleteDelegate = FOnFindSessionsCompleteDelegate::CreateUObject(
	this, &UFindRoomUI::OnCompleteSearch);
	
	FSessionUtil::SearchSession(SessionSearchData);
}

void UFindRoomUI::OnCompleteSearch(bool bIsSuccess)
{
	Btn_Find->SetIsEnabled(true);
	
	if (!bIsSuccess)
	{
		UE_LOG(LogTemp, Error, TEXT("방을 검색하는 것에 실패하였습니다."));
		return;
	}
	
	for (int i = 0; i < SessionSearchData.SessionSearch->SearchResults.Num(); i++)
	{
		URoomInfoUI* Item{CreateWidget<URoomInfoUI>(GetWorld(), RoomInfoUIFactory)};
		Item->SetPadding(FMargin(0, 0, 0, 5));
		Scroll_RoomList->AddChild(Item);
		
		Item->SetInfo(SessionSearchData.SessionSearch->SearchResults[i], i + 1000, "0000");
	}
}
