#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PalCaptureComponent.generated.h"

class ASonheimPlayerState;
class ABaseMonster;
class ASonheimPlayer;
class UPalInventoryComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPalCaptured, ABaseMonster*, CapturedPal, bool, bSuccess);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnThrowPalSphere);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SONHEIM_API UPalCaptureComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UPalCaptureComponent();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    void InitializeWithPlayerState(ASonheimPlayerState* PlayerState);

    // PalSphere 던지기 입력 처리
    UFUNCTION(BlueprintCallable, Category = "Pal Capture")
    void StartThrowPalSphere();
    
    UFUNCTION(BlueprintCallable, Category = "Pal Capture")
    void ThrowPalSphere();
    
    UFUNCTION(BlueprintCallable, Category = "Pal Capture")
    void CancelThrowPalSphere();

    // 포획 시도 (PalSphere에서 호출)
    UFUNCTION(BlueprintCallable, Category = "Pal Capture")
    void AttemptCapture(ABaseMonster* TargetPal);

    // 포획 확률 계산
    UFUNCTION(BlueprintCallable, Category = "Pal Capture")
    float CalculateCaptureRate(ABaseMonster* TargetPal) const;

    UFUNCTION(BlueprintCallable, Category = "Pal Capture")
    bool IsThrowingPalSphere() const { return bIsThrowingPalSphere; }

    // 델리게이트
    UPROPERTY(BlueprintAssignable)
    FOnPalCaptured OnPalCaptured;

    UPROPERTY(BlueprintAssignable)
    FOnThrowPalSphere OnThrowPalSphere;

    // 포획 관련 설정
    UPROPERTY(EditDefaultsOnly, Category = "Pal Capture")
    float BaseCaptureProbability = 0.5f;

    UPROPERTY(EditDefaultsOnly, Category = "Pal Capture")
    float LowHealthThreshold = 0.3f;

    UPROPERTY(EditDefaultsOnly, Category = "Pal Capture")
    int32 PalSphereSkillID = 15;

protected:
    virtual void BeginPlay() override;

    UFUNCTION()
    void OnRep_IsThrowingPalSphere();

private:
    UPROPERTY()
    ASonheimPlayer* OwnerPlayer;

    UPROPERTY()
    UPalInventoryComponent* PalInventory;

    UPROPERTY(ReplicatedUsing = OnRep_IsThrowingPalSphere)
    bool bIsThrowingPalSphere = false;

    UFUNCTION(Server, Reliable)
    void Server_StartThrowPalSphere();
    
    UFUNCTION(Server, Reliable)
    void Server_ThrowPalSphere();
    
    UFUNCTION(Server, Reliable)
    void Server_CancelThrowPalSphere();

    UFUNCTION(Server, Reliable)
    void Server_AttemptCapture(ABaseMonster* TargetPal);

    UFUNCTION(NetMulticast, Reliable)
    void MultiCast_OnCaptureResult(ABaseMonster* TargetPal, bool bSuccess);
    
    UFUNCTION(NetMulticast, Reliable)
    void MultiCast_SetThrowingState(bool bThrowing);
};