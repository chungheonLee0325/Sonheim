// Fill out your copyright notice in the Description page of Project Settings.


#include "Rocket.h"

URocket::URocket()
{
}

void URocket::Activate(class AAreaObject* Caster, AAreaObject* Target)
{
	Super::Activate(Caster, Target);
}

void URocket::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void URocket::Fire()
{
	Super::Fire();
}

void URocket::Launch()
{
}
