// Fill out your copyright notice in the Description page of Project Settings.


#include "GumossAnimInstance.h"

#include "Sonheim/AreaObject/Base/AreaObject.h"

void UGumossAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (m_Owner)
	{
		Speed = m_Owner->GetVelocity().Length();
	}
}
