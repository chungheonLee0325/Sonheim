#pragma once

#include "CoreMinimal.h"
#include "ParabolaElement.h"
#include "PalSphere.generated.h"

class ABaseMonster;
class ASonheimPlayer;

USTRUCT()
struct FPalCaptureState
{
    GENERATED_BODY()
    
    FVector OriginalLocation;
    FRotator OriginalRotation;
    bool bOriginalPhysicsEnabled;
    bool bOriginalCollisionEnabled;
    
    FPalCaptureState()
    {
        OriginalLocation = FVector::ZeroVector;
        OriginalRotation = FRotator::ZeroRotator;
        bOriginalPhysicsEnabled = false;
        bOriginalCollisionEnabled = true;
    }
};

UCLASS()
class SONHEIM_API APalSphere : public AParabolaElement
{
    GENERATED_BODY()

public:
    APalSphere();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    class USkeletalMeshComponent* SkeletalMesh;

protected: 
    virtual void BeginPlay() override;
    virtual void OnBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                            UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
                            const FHitResult& SweepResult) override;
    virtual void OnComponentHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
                                FVector NormalImpulse, const FHitResult& Hit) override;

public:
    virtual void InitElement(AAreaObject* Caster, AAreaObject* Target, const FVector& TargetLocation, FAttackData* AttackData) override;
    virtual FVector Fire(AAreaObject* Caster, AAreaObject* Target, FVector TargetLocation, float ArcValue) override;
    
    UFUNCTION(BlueprintCallable, Category = "Pal Sphere")
    void SetSphereType(int32 ItemID) { SphereItemID = ItemID; }

    UFUNCTION(BlueprintImplementableEvent)
    void HandlePalSphereAnimation();

private:
    bool bCanHit = true;
    bool bIsProcessingCapture = false; // 포획 처리 중 플래그
    
    // 포획 프로세스 함수들
    void CheckPalCatch(ASonheimPlayer* Caster, ABaseMonster* Target);
    void StartShakeAnimation(ASonheimPlayer* Caster, ABaseMonster* Target);
    void AttemptCapture(ASonheimPlayer* Caster, ABaseMonster* Target);
    void ProcessCaptureSuccess(ASonheimPlayer* Caster, ABaseMonster* Target);
    void ProcessCaptureFailed(ABaseMonster* Target);
    
    // 팰 상태 저장/복원
    void SavePalState(ABaseMonster* Target);
    void RestorePalState(ABaseMonster* Target);
    
    // 스피어 아이템 ID
    UPROPERTY(EditDefaultsOnly, Category = "Pal Sphere")
    int32 SphereItemID = 0;
    
    UPROPERTY(EditDefaultsOnly, Category = "Pal Sphere")
    float CaptureProcessTime = 2.0f;
    
    // 이펙트
    UPROPERTY(EditDefaultsOnly, Category = "Effects")
    UParticleSystem* CaptureSuccessEffect;
    
    UPROPERTY(EditDefaultsOnly, Category = "Effects")
    UParticleSystem* CaptureFailEffect;
    
    UPROPERTY(EditDefaultsOnly, Category = "Effects")
    UParticleSystem* CaptureAttemptEffect; // 포획 시도 중 이펙트
    
    // 사운드
    UPROPERTY(EditDefaultsOnly, Category = "Sound")
    USoundBase* CaptureSuccessSound;
    
    UPROPERTY(EditDefaultsOnly, Category = "Sound")
    USoundBase* CaptureFailSound;
    
    UPROPERTY(EditDefaultsOnly, Category = "Sound")
    USoundBase* ShakeSound;
    
    // 애니메이션
    FTimerHandle ShakeTimerHandle;
    FTimerHandle CaptureTimeoutHandle;
    int32 ShakeCount = 0;
    
    UPROPERTY()
    ABaseMonster* TargetMonster;
    
    // 포획 상태 저장
    FPalCaptureState SavedPalState;
    
    // 스피어 원래 위치 (땅에 떨어진 후)
    FVector SphereGroundLocation;
};