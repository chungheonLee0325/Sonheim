// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Sonheim/AreaObject/Monster/AI/Base/BaseAiState.h"
#include "PartnerSkillMode.generated.h"

/**
 * 
 */
UCLASS()
class SONHEIM_API UPartnerSkillMode : public UBaseAiState
{
	GENERATED_BODY()
	
public:
	virtual void InitState() override;
	virtual void CheckIsValid() override;
	virtual void ServerEnter() override;
	virtual void ServerExecute(float dt) override;
	virtual void ServerExit() override;

	virtual void ClientEnter() override;
	virtual void ClientExecute(float dt) override;
	
	void AttachToPlayer();
	UFUNCTION(Server, Reliable)
	void Server_AttachToPlayer();
	UFUNCTION(NetMulticast, Reliable)
	void MulticastRPC_AttachToPlayer();
	UFUNCTION(Client, Reliable)
	void ClientRPC_AttachToPlayer();
	void DetachFromPlayer();
	UFUNCTION(NetMulticast, Reliable)
	void MultiCastRPC_DetachFromPlayer();
	UFUNCTION(Client, Reliable)
	void ClientRPC_DetachFromPlayer();


};
