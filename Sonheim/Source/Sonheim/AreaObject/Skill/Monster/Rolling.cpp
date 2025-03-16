// Fill out your copyright notice in the Description page of Project Settings.


#include "Rolling.h"

URolling::URolling()
{}

void URolling::OnCastStart(class AAreaObject* Caster, AAreaObject* Target)
{
	Super::OnCastStart(Caster, Target);
}

void URolling::OnCastTick(float DeltaTime)
{
	Super::OnCastTick(DeltaTime);
}

void URolling::OnCastFire()
{
	Super::OnCastFire();
}

void URolling::StartRoll()
{}
