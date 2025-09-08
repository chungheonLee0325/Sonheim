// Fill out your copyright notice in the Description page of Project Settings.


#include "MachineGun.h"

#include "Kismet/KismetMathLibrary.h"
#include "Sonheim/AreaObject/Base/AreaObject.h"
#include "Sonheim/AreaObject/Player/SonheimPlayer.h"

UMachineGun::UMachineGun()
{}

bool UMachineGun::Activate(class AAreaObject* Caster, AAreaObject* Target)
{
	if (!Super::Activate(Caster, Target)) return false; 
	
	Fire();

	return true;
}

void UMachineGun::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

bool UMachineGun::Fire()
{
	if(!Super::Fire()) return false;

	GetWorld()->GetTimerManager().SetTimer(FireTimer, this, &UMachineGun::FireGun, 0.1f, true);

	return true;
}

void UMachineGun::FireGun()
{
	FVector StartPos{m_Caster->GetActorLocation()};
	FVector EndPos{StartPos + UKismetMathLibrary::RandomUnitVectorInConeInDegrees(m_Caster->GetActorForwardVector(), Spread) * Range};
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(m_Caster);
	FHitResult HitInfo;

	// ECC_GameTraceChannel8 : MonsterGunAttack
	bool bHit{
		GetWorld()->LineTraceSingleByChannel(HitInfo, StartPos, EndPos, ECollisionChannel::ECC_GameTraceChannel8, Params)
	};
	
	if (bHit)
	{
		if (m_Caster->bShowDebug)
		{
			DrawDebugLine(GetWorld(), StartPos, HitInfo.ImpactPoint, FColor::Emerald, false, 1.f, 0, 1.f);
		}
		
		//FLog::Log();
		ASonheimPlayer* Player{Cast<ASonheimPlayer>(HitInfo.GetActor())};
		if (Player)
		{
			FAttackData* AttackData = GetAttackDataByIndex(0);
			m_Caster->CalcDamage(*AttackData, m_Caster, Player, HitInfo);	
		}
	}
}
