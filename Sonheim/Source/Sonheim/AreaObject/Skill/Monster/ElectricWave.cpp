// Fill out your copyright notice in the Description page of Project Settings.


#include "ElectricWave.h"

UElectricWave::UElectricWave()
{}

void UElectricWave::OnCastStart(class AAreaObject* Caster, AAreaObject* Target)
{
	Super::OnCastStart(Caster, Target);

	CurrentTime = 0.f;
	
	OnCastFire();
}

void UElectricWave::OnCastTick(float DeltaTime)
{
	Super::OnCastTick(DeltaTime);

	CurrentTime += DeltaTime;
	if (CurrentTime > DelayTime)
	{
		CurrentTime = 0.f;
		OnCastFire();
	}
}

void UElectricWave::OnCastFire()
{
	Super::OnCastFire();

	ShockWave();
}

void UElectricWave::ShockWave()
{
	// sphere trace, 데미지 3번 줌
}
