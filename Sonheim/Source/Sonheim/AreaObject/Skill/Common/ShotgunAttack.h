#pragma once

#include "CoreMinimal.h"
#include "Sonheim/AreaObject/Skill/Base/BaseSkill.h"
#include "ShotgunAttack.generated.h"

/**
 * 샷건 공격 스킬 클래스
 * 여러 개의 산탄을 발사하여 근거리에서 강력한 대미지를 입힘
 */

UCLASS()
class SONHEIM_API UShotgunAttack : public UBaseSkill
{
	GENERATED_BODY()

public:
	UShotgunAttack();

	// BaseSkill 인터페이스 구현
	virtual void InitSkill(FSkillData* SkillData) override;
	virtual void Server_OnCastStart(class AAreaObject* Caster, AAreaObject* Target) override;
	virtual void Client_OnCastStart(class AAreaObject* Caster, AAreaObject* Target) override;
	virtual void Server_OnCastFire() override;
	virtual void Client_OnCastFire() override;

protected:
	// 샷건 특성 설정
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Shotgun")
	int32 PelletCount = 8; // 산탄 개수

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Shotgun")
	float SpreadAngle = 15.0f; // 산탄 퍼짐 각도 (도)

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Shotgun")
	float MaxRange = 1500.0f; // 최대 사거리

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Shotgun")
	float DamageFalloffStart = 500.0f; // 데미지 감소 시작 거리

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Shotgun")
	float MinDamagePercent = 0.3f; // 최소 데미지 비율 (30%)

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Debug")
	float DebugTraceDuration = 2.0f;

private:
	// 단일 산탄 발사
	void FirePellet(const FVector& StartLocation, const FVector& Direction);
	
	// 거리에 따른 데미지 계산
	float CalculateDamageWithFalloff(float BaseDamage, float Distance) const;
	
	// 산탄 방향 계산 (스프레드 적용)
	FVector CalculatePelletDirection(const FVector& BaseDirection) const;

	// 발사 위치 계산
	FVector GetFireStartLocation() const;

	// 캐싱된 공격 데이터
	FAttackData* CachedAttackData = nullptr;
};