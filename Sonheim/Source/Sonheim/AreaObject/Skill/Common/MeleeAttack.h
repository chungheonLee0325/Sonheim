// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Sonheim/AreaObject/Skill/Base/BaseSkill.h"
#include "MeleeAttack.generated.h"

/**
 * 
 */
USTRUCT(BlueprintType)
struct FAttackCollision
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	USkeletalMeshComponent* OwnerSourceMesh = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSet<AActor*> HitActors = {};
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool IsEnableHitDetection = false;
	FAttackData* IndexedAttackData = nullptr;

	// 이전 프레임의 시작/끝 위치 저장
	FVector PreviousStartLocation = FVector::ZeroVector;
	FVector PreviousEndLocation = FVector::ZeroVector;
	FRotator PreviousSocketRotation = FRotator::ZeroRotator;
	bool bHasPreviousPositions = false;
};

UCLASS()
class SONHEIM_API UMeleeAttack : public UBaseSkill
{
	GENERATED_BODY()
	
public:
	virtual void OnCastStart(class AAreaObject* Caster, AAreaObject* Target) override;
	virtual void OnCastEnd() override;
	virtual void CancelCast() override;
	virtual void OnCastTick(float DeltaTime) override;
	void SetCasterMesh(int AttackDataIndex);
	void ProcessHitDetection(int AttackDataIndex);
	void ResetCollisionData(int AttackDataIndex);
	bool PerformCollisionCheck(
		EHitDetectionType DetectionType,
		const FVector& StartLocation,
		const FVector& EndLocation,
		const FRotator& SocketRotation,
		const FHitBoxData& HitBoxData,
		const FCollisionQueryParams& QueryParams,
		TArray<FHitResult>& OutHitResults);
	virtual void ResetNextSkillByBHit() override;
	
	// 디버그 드로잉 옵션
	bool bDebugDraw = false;
	float DebugDrawDuration = 0.3f;
	FColor DebugColor = FColor::Red;
	bool bPersistentLines = false;

protected:
	bool bIsHitOnce = false;

private:
	void DrawDebugHitDetection(int AttackDataIndex, const FVector& Start, const FVector& End,
							   const TArray<FHitResult>& HitResults,
							   const FRotator& SocketRotation);

	UPROPERTY()
	TMap<int, FAttackCollision> NotifyStateMap;
};
