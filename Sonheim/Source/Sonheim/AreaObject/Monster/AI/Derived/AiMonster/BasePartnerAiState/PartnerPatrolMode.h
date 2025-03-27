// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Sonheim/AreaObject/Monster/AI/Base/BaseAiState.h"
#include "PartnerPatrolMode.generated.h"

/**
 * 
 */
UCLASS()
class SONHEIM_API UPartnerPatrolMode : public UBaseAiState
{
	GENERATED_BODY()
	
public:
	virtual void InitState() override;
	virtual void CheckIsValid() override;
	virtual void ServerEnter() override;
	virtual void ServerExecute(float dt) override;
	virtual void ServerExit() override;

	void Patrol();
	void PatrolToPlayer();
	void TeleportToPlayer();
	void MoveToPlayer();
	void MoveCompleted(struct FAIRequestID RequestID, const struct FPathFollowingResult& Result);
	void PatrolMoveCompleted(struct FAIRequestID RequestID, const struct FPathFollowingResult& Result);

	float PatrolTime{1.f};
	float FlowTime{};
	
	float JumpTime{2.f};
	float FlowTimeForJump{};

	bool StopUpdateLocation{false};
	
	UPROPERTY()
	class UNavigationSystemV1* NavSystem;
};
