// Fill out your copyright notice in the Description page of Project Settings.


#include "MakeRoomUI.h"

#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"
#include "Sonheim/GameManager/SonheimGameInstance.h"
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
	Btn_BackFromCreate->SetIsEnabled(false);
	Btn_CreateRoom->SetIsEnabled(false);
	Slider_PlayerCount->SetIsEnabled(false);
	Edit_RoomName->SetIsEnabled(false);
	
	auto GS{Cast<USonheimGameInstance>(GetGameInstance())};

	GS->MaxPlayer = Slider_PlayerCount->GetValue();
	GS->RoomName = Edit_RoomName->GetText().ToString();

	UGameplayStatics::OpenLevel(GetWorld(), FName("/Game/_Maps/CineMap"));
	
	// FSessionCreateData CreateData;
	// CreateData.IsPublic = true;
	// CreateData.MaxPlayer = Slider_PlayerCount->GetValue();
	// CreateData.RoomName = Edit_RoomName->GetText().ToString();
	
	//FSessionUtil::CreateSession(CreateData);
	
	//GetWorld()->ServerTravel(FString("/Game/_Maps/GameMap?listen"));
	//GetWorld()->ServerTravel(FString("/Game/_Maps/CineMap?listen"));
}

void UMakeRoomUI::OnValueChanged(float Value)
{
	Text_PlayerCount->SetText(FText::AsNumber(Value));
}
