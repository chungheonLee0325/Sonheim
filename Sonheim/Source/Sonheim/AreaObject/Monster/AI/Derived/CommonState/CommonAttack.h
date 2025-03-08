// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Sonheim/AreaObject/Monster/AI/Base/BaseAiState.h"
#include "CommonAttack.generated.h"

UCLASS()
class SONHEIM_API UCommonAttack : public UBaseAiState
{
	GENERATED_BODY()

public:
	virtual void InitState() override;
	virtual void CheckIsValid() override;
	virtual void Enter() override;
	virtual void Execute(float dt) override;
	virtual void Exit() override;

	// 스킬 완료시 실행 메서드 - Call by delegate
	UFUNCTION()
	void OnSkillCompleted();
	// 스킬 실패시 실행 메서드 - Call by delegate
	UFUNCTION()
	void OnSkillCanceled();

private:
	int SkillID = 0;
	bool bHasFailed = false;
};
