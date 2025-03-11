// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectileBlade.h"

#include "Sonheim/AreaObject/Player/SonheimPlayer.h"
#include "Sonheim/Element/Derived/BladeWind.h"
#include "Sonheim/Utilities/LogMacro.h"

UProjectileBlade::UProjectileBlade()
{
	static ConstructorHelpers::FClassFinder<ABladeWind> BladeWindClass
	(TEXT("/Script/Engine.Blueprint'/Game/_BluePrint/Element/BladeWind/BP_BladeWind.BP_BladeWind_C'"));
	if (BladeWindClass.Succeeded())
	{
		BladeWindFactory = BladeWindClass.Class;
	}
}

void UProjectileBlade::OnCastStart(class AAreaObject* Caster, AAreaObject* Target)
{
	CurrentTime = 0.f;

	Super::OnCastStart(Caster, Target);
}

void UProjectileBlade::OnCastTick(float DeltaTime)
{
	Super::OnCastTick(DeltaTime);

	CurrentTime += DeltaTime;
	if (CurrentTime > DelayTime)
	{
		CurrentTime = 0.f;
		OnCastFire();
	}
}

void UProjectileBlade::OnCastFire()
{
	Super::OnCastFire();

	FLog::Log("UProjectileBlade::OnCastFire");
	// 
	ABladeWind* SpawnedBladeWind{
		GetWorld()->SpawnActor<ABladeWind>(BladeWindFactory, m_Caster->GetActorLocation(), m_Caster->GetActorRotation())
	};

	// ToDo : Notify에서 Index 주입
	FAttackData* AttackData = GetAttackDataByIndex(0);
	// ToDo : TempTarget -> m_Target으로 수정
	ASonheimPlayer* TempTarget{Cast<ASonheimPlayer>(GetWorld()->GetFirstPlayerController()->GetPawn())};
	
	if (SpawnedBladeWind)
	{
		SpawnedBladeWind->InitElement(m_Caster, TempTarget, TempTarget->GetActorLocation(), AttackData);
	}
	else
	{
		FLog::Log("No SpawnedBladeWind");
	}
}
