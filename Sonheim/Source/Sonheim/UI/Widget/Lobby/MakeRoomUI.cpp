// Fill out your copyright notice in the Description page of Project Settings.


#include "MakeRoomUI.h"

#include "Components/Button.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Sonheim/Utilities/LogMacro.h"
#include "Sonheim/Utilities/SessionUtil.h"

void UMakeRoomUI::NativeConstruct()
{
	Super::NativeConstruct();
	
	Btn_BackFromCreate->OnClicked.AddDynamic(this, &UMakeRoomUI::OnClickedBackFromCreate);
	Btn_CreateRoom->OnClicked.AddDynamic(this, &UMakeRoomUI::OnClickedCreateRoom);

	Slider_PlayerCount->OnValueChanged.AddDynamic(this, &UMakeRoomUI::OnValueChanged);
}

void UMakeRoomUI::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

}

void UMakeRoomUI::OnClickedBackFromCreate()
{
	OnBackButtonClicked.ExecuteIfBound();
}

void UMakeRoomUI::OnClickedCreateRoom()
{
	FSessionCreateData CreateData;
	CreateData.IsPublic = true;
	CreateData.MaxPlayer = 2;
	CreateData.RoomName = "111";
	
	FSessionUtil::CreateSession(CreateData);
	
	GetWorld()->ServerTravel(FString("/Game/_Maps/GameMap?listen"));
}

void UMakeRoomUI::OnValueChanged(float Value)
{
	Text_PlayerCount->SetText(FText::AsNumber(Value));
}
