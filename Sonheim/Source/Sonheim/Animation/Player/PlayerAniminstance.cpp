// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerAniminstance.h"

#include "Sonheim/AreaObject/Player/SonheimPlayer.h"

void UPlayerAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
	
	if (nullptr == m_Owner)
	{
		return;
	}
	// walkSpeed 값을 설정
	FVector velocity = m_Owner->GetVelocity();
	// 전방벡터
	FVector forward = m_Owner->GetActorForwardVector();
	speed = FVector::DotProduct(velocity, forward);

	// 좌우방향
	FVector right = m_Owner->GetActorRightVector();
	direction = FVector::DotProduct(velocity, right);
}
