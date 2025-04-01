// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Sonheim/Utilities/SessionUtil.h"
#include "FindRoomUI.generated.h"

DECLARE_DELEGATE(FOnBackButtonClicked);

/**
 * 
 */
UCLASS()
class SONHEIM_API UFindRoomUI : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	
	UPROPERTY(meta = (BindWidget))
	class UButton* Btn_BackFromFind;
	UPROPERTY(meta = (BindWidget))
	class UButton* Btn_Find;

public:
	UFUNCTION()
	void OnClickedBackFromFind();
	UFUNCTION()
	void OnClickedFind();

	UPROPERTY(meta = (BindWidget))
	class UScrollBox* Scroll_RoomList;
	UPROPERTY(EditAnywhere)
	TSubclassOf<class URoomInfoUI> RoomInfoUIFactory;

public:
	FOnBackButtonClicked OnBackButtonClicked;
	
	FSessionSearchData SessionSearchData;

	void OnCompleteSearch(bool bIsSuccess);
};
