// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MakeRoomUI.generated.h"

DECLARE_DELEGATE(FOnBackButtonClicked);
/**
 * 
 */
UCLASS()
class SONHEIM_API UMakeRoomUI : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	
	UPROPERTY(meta = (BindWidget))
	class UButton* Btn_BackFromCreate;
	UPROPERTY(meta = (BindWidget))
	class UButton* Btn_CreateRoom;
	UPROPERTY(meta = (BindWidget))
	class USlider* Slider_PlayerCount;
	UPROPERTY(meta = (BindWidget))
	class UTextBlock* Text_PlayerCount;
	UPROPERTY(meta = (BindWidget))
	class UEditableTextBox* Edit_RoomName;

public:
	UFUNCTION()
	void OnClickedBackFromCreate();
	UFUNCTION()
	void OnClickedCreateRoom();
	UFUNCTION()
	void OnValueChanged(float Value);

	
public:
	FOnBackButtonClicked OnBackButtonClicked;
	
};
