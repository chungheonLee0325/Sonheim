// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LobbyUI.generated.h"

/**
 * 
 */
UCLASS()
class SONHEIM_API ULobbyUI : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	class UWidgetSwitcher* WidgetSwitcher;
	UPROPERTY(meta = (BindWidget))
	class UButton* Btn_GoMakeRoom;
	UPROPERTY(meta = (BindWidget))
	class UButton* Btn_GoFindRoom;
	UPROPERTY(meta = (BindWidget))
	class UMakeRoomUI* WBP_MakeRoomUI;
	UPROPERTY(meta = (BindWidget))
	class UFindRoomUI* WBP_FindRoomUI;

public:
	UFUNCTION()
	void OnClickedGoMakeRoom();
	UFUNCTION()
	void OnClickedGoFindRoom();

public:
	void OnBackButtonClicked();
};
