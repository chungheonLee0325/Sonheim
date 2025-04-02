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
	//CurrentTime = 0.f;

	Super::OnCastStart(Caster, Target);
}

void UProjectileBlade::OnCastTick(float DeltaTime)
{
	Super::OnCastTick(DeltaTime);

	// CurrentTime += DeltaTime;
	// if (CurrentTime > DelayTime)
	// {
	// 	CurrentTime = 0.f;
	// 	OnCastFire();
	// }
}

void UProjectileBlade::OnCastFire()
{
	Super::OnCastFire();

	FireBladeWind();
}

void UProjectileBlade::OnCastEnd()
{
	Super::OnCastEnd();
}

void UProjectileBlade::FireBladeWind()
{
	//FLog::Log("UProjectileBlade::OnCastFire");

	// Data Table에서 셋팅했으면 셋팅 하기
	if (m_SkillData->ElementClass != nullptr)
	{
		BladeWindFactory = m_SkillData->ElementClass;
	}
	
	ABladeWind* SpawnedBladeWind{
		GetWorld()->SpawnActor<ABladeWind>(BladeWindFactory, m_Caster->GetActorLocation(), m_Caster->GetActorRotation())
	};

	// ToDo : Notify에서 Index 주입
	FAttackData* AttackData = GetAttackDataByIndex(0);
	
	m_TargetPos = m_Target->GetActorLocation();

	if (SpawnedBladeWind)
	{
		SpawnedBladeWind->InitElement(m_Caster, m_Target, m_TargetPos, AttackData);
	}
	else
	{
		FLog::Log("No SpawnedBladeWind");
	}
}
