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

	virtual bool Activate(class AAreaObject* Caster, AAreaObject* Target) override;
	virtual bool Fire() override;

	virtual void FireElement();

	UPROPERTY(EditAnywhere)
	TSubclassOf<class ABaseElement> ElementClass = ABaseElement::StaticClass();

	bool IsFired = false;
};
