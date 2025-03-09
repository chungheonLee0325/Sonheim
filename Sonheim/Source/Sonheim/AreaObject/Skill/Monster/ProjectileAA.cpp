// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectileAA.h"

#include "Sonheim/Utilities/LogMacro.h"


void UProjectileAA::OnCastStart(class AAreaObject* Caster, AAreaObject* Target)
{
	CurrentTime = 0.f;

	Super::OnCastStart(Caster, Target);
}

void UProjectileAA::OnCastTick(float DeltaTime)
{
	Super::OnCastTick(DeltaTime);

	CurrentTime += DeltaTime;
	if (CurrentTime > DelayTime)
	{
		CurrentTime = 0.f;
		OnCastFire();
	}
}

void UProjectileAA::OnCastFire()
{
	Super::OnCastFire();

	//LOG_SCREEN_MY(5.f, FColor::Magenta, "UProjectileAA::OnCastFire");
	LogUtils::Log("UProjectileAA::OnCastFire");
}
