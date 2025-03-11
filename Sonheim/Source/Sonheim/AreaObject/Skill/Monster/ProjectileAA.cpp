// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectileAA.h"

#include "Components/SphereComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sonheim/AreaObject/Base/AreaObject.h"
#include "Sonheim/AreaObject/Player/SonheimPlayer.h"
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
	// 
	ASandBlast* SpawnedSandBlast{
		GetWorld()->SpawnActor<ASandBlast>(SandBlastFactory, m_Caster->GetActorLocation(), m_Caster->GetActorRotation())
	};

	// ToDo : Notify에서 Index 주입
	FAttackData* AttackData = GetAttackDataByIndex(0);
	// ToDo : TempTarget -> m_Target으로 수정
	ASonheimPlayer* TempTarget{Cast<ASonheimPlayer>(GetWorld()->GetFirstPlayerController()->GetPawn())};
	//SpawnedSandBlast->InitElement(m_Caster, m_Target, m_Target->GetActorLocation(), AttackData);
	
	if (SpawnedSandBlast)
	{
		SpawnedSandBlast->InitElement(m_Caster, TempTarget, TempTarget->GetActorLocation(), AttackData);
	}
	else
	{
		FLog::Log("No SpawnedSandBlast");
	}
}
