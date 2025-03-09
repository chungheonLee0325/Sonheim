// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectileAA.h"

#include "Sonheim/AreaObject/Base/AreaObject.h"
#include "Sonheim/Element/Derived/SandBlast.h"
#include "Sonheim/Utilities/LogMacro.h"

UProjectileAA::UProjectileAA()
{
	static ConstructorHelpers::FClassFinder<ASandBlast> SandBlastClass
		(TEXT("/Script/Engine.Blueprint'/Game/_BluePrint/Element/SandBlast/BP_SandBlast.BP_SandBlast_C'"));
	if (SandBlastClass.Succeeded())
	{
		SandBlastFactory = SandBlastClass.Class;
	}
}

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

	FLog::Log("UProjectileAA::OnCastFire");

	GetWorld()->SpawnActor<ASandBlast>(SandBlastFactory, m_Caster->GetActorLocation(), m_Caster->GetActorRotation());
}
