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

void UProjectileAA::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

bool UProjectileAA::Fire()
{
	if (!Super::Fire()) return false;

	FireSandBlast();

	return true;
}

void UProjectileAA::FireSandBlast()
{
	// Data Table에서 셋팅했으면 셋팅 하기
	if (m_SkillData->ElementClass != nullptr)
	{
		SandBlastFactory = m_SkillData->ElementClass;
	}

	ASandBlast* SpawnedSandBlast{
		GetWorld()->SpawnActor<ASandBlast>(SandBlastFactory, m_Caster->GetActorLocation(), m_Caster->GetActorRotation())
	};

	// ToDo : Notify에서 Index 주입
	FAttackData* AttackData = GetAttackDataByIndex(0);

	m_TargetPos = m_Target->GetActorLocation();

	if (SpawnedSandBlast)
	{
		SpawnedSandBlast->InitElement(m_Caster, m_Target, m_TargetPos, AttackData);
	}
	else
	{
		FLog::Log("No SpawnedSandBlast");
	}
}

void UProjectileAA::SeverRPC_FireSandBlast_Implementation()
{
	FLog::Log("UProjectileAA::OnCastFire");

	ASandBlast* SpawnedSandBlast{
		GetWorld()->SpawnActor<ASandBlast>(SandBlastFactory, m_Caster->GetActorLocation(), m_Caster->GetActorRotation())
	};

	// ToDo : Notify에서 Index 주입
	FAttackData* AttackData = GetAttackDataByIndex(0);

	m_TargetPos = m_Target->GetActorLocation();

	if (SpawnedSandBlast)
	{
		SpawnedSandBlast->InitElement(m_Caster, m_Target, m_TargetPos, AttackData);
	}
	else
	{
		FLog::Log("No SpawnedSandBlast");
	}
}
