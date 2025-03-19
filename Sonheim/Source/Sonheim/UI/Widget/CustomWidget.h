// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CustomWidget.generated.h"

class AAreaObject;
/**
 * 
 */
UCLASS()
class SONHEIM_API UCustomWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	FORCEINLINE void SetOwningActor(AAreaObject* NewOwner) { OwningActor = NewOwner; }

protected:
	// 현재 위젯을 소유하고 있는 액터 저장용 변수
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Actor")
	TObjectPtr<AAreaObject> OwningActor;
};
