// Fill out your copyright notice in the Description page of Project Settings.


#include "LobbyUI.h"

#include "FindRoomUI.h"
#include "MakeRoomUI.h"
#include "Components/Button.h"
#include "Components/WidgetSwitcher.h"

void ULobbyUI::NativeConstruct()
{
	Super::NativeConstruct();
	
	// 마우스 활성화
	GetWorld()->GetFirstPlayerController()->SetShowMouseCursor(true);
	
	Btn_GoMakeRoom->OnClicked.AddDynamic(this, &ULobbyUI::OnClickedGoMakeRoom);
	Btn_GoFindRoom->OnClicked.AddDynamic(this, &ULobbyUI::OnClickedGoFindRoom);

	WBP_MakeRoomUI->OnBackButtonClicked.BindUObject(this, &ULobbyUI::OnBackButtonClicked);
	WBP_FindRoomUI->OnBackButtonClicked.BindUObject(this, &ULobbyUI::OnBackButtonClicked);
}

void ULobbyUI::OnClickedGoMakeRoom()
{
	WidgetSwitcher->SetActiveWidgetIndex(1);
}

void ULobbyUI::OnClickedGoFindRoom()
{
	WidgetSwitcher->SetActiveWidgetIndex(2);
}

void ULobbyUI::OnBackButtonClicked()
{
	WidgetSwitcher->SetActiveWidgetIndex(0);
}
