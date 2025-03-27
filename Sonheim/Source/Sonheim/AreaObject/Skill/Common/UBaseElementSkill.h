// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Sonheim/AreaObject/Skill/Base/BaseSkill.h"
#include "Sonheim/Element/BaseElement.h"
#include "UBaseElementSkill.generated.h"

/**
 * 
 */
UCLASS()
class SONHEIM_API UUBaseElementSkill : public UBaseSkill
{
	GENERATED_BODY()

	virtual void Client_OnCastFire() override;

	virtual void FireElement();

	UPROPERTY(EditAnywhere)
	TSubclassOf<class ABaseElement> ElementClass = ABaseElement::StaticClass();
};
