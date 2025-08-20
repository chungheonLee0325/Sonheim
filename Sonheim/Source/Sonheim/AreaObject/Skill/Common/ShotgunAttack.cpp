// ShotgunAttack.cpp
#include "ShotgunAttack.h"
#include "Sonheim/AreaObject/Base/AreaObject.h"
#include "Sonheim/AreaObject/Player/SonheimPlayer.h"
#include "Sonheim/GameObject/ResourceObject/BaseResourceObject.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "NiagaraFunctionLibrary.h"
#include "DrawDebugHelpers.h"
#include "Camera/CameraComponent.h"
#include "Kismet/KismetMathLibrary.h"

UShotgunAttack::UShotgunAttack()
{
    // 기본값 설정
    PelletCount = 8;
    SpreadAngle = 15.0f;
    MaxRange = 1500.0f;
    DamageFalloffStart = 500.0f;
    MinDamagePercent = 0.3f;
}

void UShotgunAttack::InitSkill(FSkillData* SkillData)
{
    Super::InitSkill(SkillData);
}

void UShotgunAttack::Server_OnCastStart(AAreaObject* Caster, AAreaObject* Target)
{
    Super::Server_OnCastStart(Caster, Target);

    CachedAttackData = GetAttackDataByIndex(0);
}

void UShotgunAttack::Client_OnCastStart(AAreaObject* Caster, AAreaObject* Target)
{
    Super::Client_OnCastStart(Caster, Target);
    
    // 첫 번째 공격 데이터 캐싱
    CachedAttackData = GetAttackDataByIndex(0);
}

void UShotgunAttack::Server_OnCastFire()
{
    Super::Server_OnCastFire();
    
    if (!m_Caster || !CachedAttackData)
        return;
    
    // 발사 시작 위치 계산
    FVector StartLocation = GetFireStartLocation();
    
    // 기본 발사 방향 계산
    FVector BaseDirection;
    if (ASonheimPlayer* Player = Cast<ASonheimPlayer>(m_Caster))
    {
        // 플레이어는 카메라 방향으로 발사
        if (Player->IsLockOn())
        {
            BaseDirection = Player->GetFollowCamera()->GetForwardVector();
        }
        else
        {
            BaseDirection = Player->GetWeaponMesh()->GetSocketRotation(FName("FireSocket")).Vector();
        }

        Player->Multicast_PlayWeaponMontage(nullptr);
    }
    else
    {
        // 몬스터는 타겟 방향으로 발사
        if (m_Target)
        {
            BaseDirection = (m_Target->GetActorLocation() - StartLocation).GetSafeNormal();
        }
        else
        {
            BaseDirection = m_Caster->GetActorForwardVector();
        }
    }
    
    // 각 산탄 발사
    for (int32 i = 0; i < PelletCount; i++)
    {
        FVector PelletDirection = CalculatePelletDirection(BaseDirection);
        FirePellet(StartLocation, PelletDirection);
    }

    // APlayerController에 내장된 반동 함수들
    float RecoilAmount = FMath::RandRange(0.5f, 1.f);
    APlayerController* PlayerController = Cast<APlayerController>(m_Caster->GetController());
    PlayerController->AddPitchInput(-RecoilAmount);
    PlayerController->AddYawInput(FMath::RandRange(-0.5f, 0.5f) * RecoilAmount);
    
    // 발사 효과 재생
    if (m_SkillData->AttackData[0].FireSFX)
    {
        m_Caster->Multicast_PlaySoundAtLocation(StartLocation, m_SkillData->AttackData[0].FireSFX);
    }
    
    if (m_SkillData->AttackData[0].FireVFX_N)
    {
        m_Caster->Multicast_PlayNiagaraEffectAtLocation(StartLocation, m_SkillData->AttackData[0].FireVFX_N, BaseDirection.Rotation());
    }
}

void UShotgunAttack::Client_OnCastFire()
{
    Super::Client_OnCastFire();
}

void UShotgunAttack::FirePellet(const FVector& StartLocation, const FVector& Direction)
{
    if (!GetWorld() || !m_Caster || !CachedAttackData)
        return;
    
    FVector EndLocation = StartLocation + (Direction * MaxRange);
    
    // 히트 검사를 위한 파라미터 설정
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(m_Caster);
    QueryParams.bTraceComplex = false;
    QueryParams.bReturnPhysicalMaterial = true;
    
    FHitResult HitResult;
    bool bHit = false;
    
    // 라인 트레이스 또는 스피어 트레이스 수행
    if (CachedAttackData->HitBoxData.DetectionType == EHitDetectionType::Sphere)
    {
        // 스피어 트레이스 (더 넓은 히트 박스)
        bHit = GetWorld()->SweepSingleByChannel(
            HitResult,
            StartLocation,
            EndLocation,
            FQuat::Identity,
            ECC_GameTraceChannel1,
            FCollisionShape::MakeSphere(CachedAttackData->HitBoxData.Radius),
            QueryParams
        );
    }
    else
    {
        // 기본 라인 트레이스
        bHit = GetWorld()->LineTraceSingleByChannel(
            HitResult,
            StartLocation,
            EndLocation,
            ECC_GameTraceChannel1,
            QueryParams
        );
    }
    
    // 트레일 효과 생성
    if (m_SkillData->AttackData[0].FireVFX2_N)
    {
        FVector TrailEnd = bHit ? HitResult.Location : EndLocation;
        m_Caster->Multicast_PlayNiagaraEffectAtLocation(StartLocation, m_SkillData->AttackData[0].FireVFX2_N, (TrailEnd - StartLocation).Rotation());

    }
    
    // 디버그 그리기
    if (m_Caster->bShowDebug)
    {
        DrawDebugLine(
            GetWorld(),
            StartLocation,
            bHit ? HitResult.Location : EndLocation,
            bHit ? FColor::Red : FColor::White,
            false,
            DebugTraceDuration,
            0,
            1.0f
        );
        
        if (bHit)
        {
            DrawDebugSphere(
                GetWorld(),
                HitResult.Location,
                10.0f,
                12,
                FColor::Red,
                false,
                DebugTraceDuration
            );
        }
    }
    
    // 히트 처리
    if (bHit && HitResult.GetActor())
    {
        // 거리 계산
        float Distance = FVector::Dist(StartLocation, HitResult.Location);
        
        // 거리에 따른 데미지 계산
        FAttackData ModifiedAttackData = *CachedAttackData;
        float BaseDamage = FMath::RandRange(
            ModifiedAttackData.HealthDamageAmountMin,
            ModifiedAttackData.HealthDamageAmountMax
        );
        float FinalDamage = CalculateDamageWithFalloff(BaseDamage, Distance);
        
        // 계산된 데미지로 AttackData 수정
        ModifiedAttackData.HealthDamageAmountMin = FinalDamage;
        ModifiedAttackData.HealthDamageAmountMax = FinalDamage;
        
        // 데미지 적용
        m_Caster->CalcDamage(ModifiedAttackData, m_Caster, HitResult.GetActor(), HitResult);
    }
}

float UShotgunAttack::CalculateDamageWithFalloff(float BaseDamage, float Distance) const
{
    if (Distance <= DamageFalloffStart)
    {
        // 감소 시작 거리 이내: 최대 데미지
        return BaseDamage;
    }
    else if (Distance >= MaxRange)
    {
        // 최대 사거리 밖: 데미지 없음
        return 0.0f;
    }
    else
    {
        // 거리에 따른 선형 감소
        float FalloffPercent = (Distance - DamageFalloffStart) / (MaxRange - DamageFalloffStart);
        float DamageMultiplier = FMath::Lerp(1.0f, MinDamagePercent, FalloffPercent);
        return BaseDamage * DamageMultiplier;
    }
}

FVector UShotgunAttack::CalculatePelletDirection(const FVector& BaseDirection) const
{
    // 원뿔 내에서 랜덤 방향 벡터 생성
    // 이동속도를 반영하여 탄퍼짐 적용
    float MovementRecoil = m_Caster->GetCurrentSpeedRatio();
    return UKismetMathLibrary::RandomUnitVectorInConeInDegrees(BaseDirection, SpreadAngle * (MovementRecoil + 1.f));
}

FVector UShotgunAttack::GetFireStartLocation() const
{
    if (!m_Caster)
        return FVector::ZeroVector;
    
    // 플레이어인 경우 카메라 위치에서 발사
    if (ASonheimPlayer* Player = Cast<ASonheimPlayer>(m_Caster))
    {
        return Player->GetWeaponMesh()->GetSocketLocation(FName("FireSocket"));
    }
    
    // 그 외의 경우 캐릭터 위치 + 높이 오프셋
    FVector StartLocation = m_Caster->GetActorLocation();
    StartLocation.Z += 50.0f; // 약간의 높이 오프셋
    
    return StartLocation;
}