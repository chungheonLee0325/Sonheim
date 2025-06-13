#pragma once

#include "CoreMinimal.h"
#include "ParabolaElement.h"
#include "PalSphere.generated.h"

class ABaseMonster;
class ASonheimPlayer;
class UPalCaptureComponent;

UCLASS()
class SONHEIM_API APalSphere : public AParabolaElement
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	APalSphere();

protected: 
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual void OnBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
							UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
							const FHitResult& SweepResult) override;

	virtual void OnComponentHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
								FVector NormalImpulse, const FHitResult& Hit) override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	virtual void InitElement(AAreaObject* Caster, AAreaObject* Target, const FVector& TargetLocation, FAttackData* AttackData) override;

	virtual FVector Fire(AAreaObject* Caster, AAreaObject* Target, FVector TargetLocation, float ArcValue) override;

	// 스피어 타입 설정 (아이템 ID)
	UFUNCTION(BlueprintCallable, Category = "Pal Sphere")
	void SetSphereType(int32 ItemID) { SphereItemID = ItemID; }

private:
	bool bCanHit = true;
	
	// 포획 시도
	void AttemptCapture(ASonheimPlayer* Caster, ABaseMonster* Target);
	
	// 포획 결과 처리
	UFUNCTION()
	void OnCaptureResult(ABaseMonster* Target, float CaptureRate, bool bSuccess);
	
	// 포획 애니메이션
	void PlayCaptureAnimation(bool bSuccess);

	// 스피어 아이템 ID (포획률 계산에 사용)
	UPROPERTY(EditDefaultsOnly, Category = "Pal Sphere")
	int32 SphereItemID = 0;
	
	// 포획 시도 후 처리 시간
	UPROPERTY(EditDefaultsOnly, Category = "Pal Sphere")
	float CaptureProcessTime = 2.0f;
	
	// 포획 성공/실패 이펙트
	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	UParticleSystem* CaptureSuccessEffect;
	
	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	UParticleSystem* CaptureFailEffect;
	
	// 포획 성공/실패 사운드
	UPROPERTY(EditDefaultsOnly, Category = "Sound")
	USoundBase* CaptureSuccessSound;
	
	UPROPERTY(EditDefaultsOnly, Category = "Sound")
	USoundBase* CaptureFailSound;
	
	// 포획 중 흔들림 애니메이션
	FTimerHandle ShakeTimerHandle;
	int32 ShakeCount = 0;
	
	UPROPERTY()
	ABaseMonster* TargetMonster;
};