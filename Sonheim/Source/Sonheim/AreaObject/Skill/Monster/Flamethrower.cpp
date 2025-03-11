// Fill out your copyright notice in the Description page of Project Settings.


#include "Flamethrower.h"

#include "Kismet/KismetMathLibrary.h"
#include "Sonheim/AreaObject/Base/AreaObject.h"
#include "Sonheim/AreaObject/Player/SonheimPlayer.h"
#include "Sonheim/Utilities/LogMacro.h"


UFlamethrower::UFlamethrower()
{}

void UFlamethrower::OnCastStart(class AAreaObject* Caster, AAreaObject* Target)
{
	Super::OnCastStart(Caster, Target);

	OnCastFire();
}

void UFlamethrower::OnCastTick(float DeltaTime)
{
	Super::OnCastTick(DeltaTime);
}

void UFlamethrower::OnCastFire()
{
	Super::OnCastFire();

	GetWorld()->GetTimerManager().SetTimer(FireTimer, this, &UFlamethrower::FireFlame, 0.2f, true);
}

void UFlamethrower::FireFlame()
{
	FVector StartPos{m_Caster->GetActorLocation()};
	FVector EndPos{
		// StartPos + UKismetMathLibrary::RandomUnitVectorInEllipticalConeInDegrees(
		// 	m_Caster->GetActorForwardVector(), SpreadYaw, SpreadPitch) * Range
		StartPos + m_Caster->GetActorForwardVector() * Range
	};

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(m_Caster);
	TArray<FHitResult> HitInfos;

	// ECC_GameTraceChannel7 : Flamethrower
	bool bHit{
		GetWorld()->LineTraceMultiByChannel(HitInfos, StartPos, EndPos, ECollisionChannel::ECC_GameTraceChannel7,
		                                    Params)
	};

	for (FHitResult& HitInfo : HitInfos)
	{
	DrawDebugLine(GetWorld(), StartPos, HitInfo.ImpactPoint, FColor::Red, false, 1.f, 0, 1.f);
		ASonheimPlayer* Player{Cast<ASonheimPlayer>(HitInfo.GetActor())};
		if (Player)
		{
			FAttackData* AttackData = GetAttackDataByIndex(0);
			m_Caster->CalcDamage(*AttackData, m_Caster, Player, HitInfo);
		}
	}
}
