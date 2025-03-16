// Fill out your copyright notice in the Description page of Project Settings.


#include "Rocket.h"

URocket::URocket()
{}

void URocket::OnCastStart(class AAreaObject* Caster, AAreaObject* Target)
{
	Super::OnCastStart(Caster, Target);
}

void URocket::OnCastTick(float DeltaTime)
{
	Super::OnCastTick(DeltaTime);
}

void URocket::OnCastFire()
{
	Super::OnCastFire();
}

void URocket::Launch()
{}
