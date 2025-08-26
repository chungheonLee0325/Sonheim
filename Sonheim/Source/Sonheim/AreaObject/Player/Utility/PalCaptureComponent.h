#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PalCaptureComponent.generated.h"

class APalSphere;
class ASonheimPlayerState;
class ABaseMonster;
class ASonheimPlayer;
class UPalInventoryComponent;

// 포획 UI 정보 전달을 위한 구조체
USTRUCT(BlueprintType)
struct FCaptureUIInfo
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    ABaseMonster* TargetMonster = nullptr;

    UPROPERTY(BlueprintReadOnly)
    float CaptureRate = 0.0f;
};

// 연출 파라미터를 담는 패킷
USTRUCT(BlueprintType)
struct FPalCaptureRevealParams
{
    GENERATED_BODY()

    // 시작 진행도(확률 0~1)
    UPROPERTY(BlueprintReadOnly) float Guess = 0.f;
    // 세그먼트 수
    UPROPERTY(BlueprintReadOnly) int32 Segments = 3;       
    // 세그먼트 당 시간
    UPROPERTY(BlueprintReadOnly) float SegmentTime = 0.45f;
    // 시작 멈칫
    UPROPERTY(BlueprintReadOnly) float StartDelay = 0.15f; 
    // 세그 사이 멈칫
    UPROPERTY(BlueprintReadOnly) float InterStageDelay = 0.20f; 
    // 끝나고 유지
    UPROPERTY(BlueprintReadOnly) float EndDelay = 0.35f;   
    // 실패 정지 스테이지(0..Seg-1), 성공 시 -1
    UPROPERTY(BlueprintReadOnly) int32 FailStageOverride = -1;  
    // 최종 판정
    UPROPERTY(BlueprintReadOnly) bool bSuccess = false;    
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnThrowPalSphere);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCaptureUIDataUpdated, const FCaptureUIInfo&, UIData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnThrowingStateChanged, bool, bIsPreparing);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnCaptureReveal, ABaseMonster*, TargetPal, const FPalCaptureRevealParams&, Params, APalSphere*, SourceSphere);

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
    void AttemptCapture(ABaseMonster* TargetPal, APalSphere* SourceSphere);

    // 포획 확률 계산
    UFUNCTION(BlueprintCallable, Category = "Pal Capture")
    float CalculateCaptureRate(ABaseMonster* TargetPal) const;

    UFUNCTION(BlueprintCallable, Category = "Pal Capture")
    bool IsThrowingPalSphere() const { return bIsThrowingPalSphere; }

    // 연출 파라미터 전송
    UFUNCTION(NetMulticast, Reliable)
    void Multicast_BeginCaptureReveal(ABaseMonster* TargetPal, const FPalCaptureRevealParams& Params, APalSphere* SourceSphere);

    // 연출 끝난 뒤 실제로 결과 반영(서버 전용)
    UFUNCTION(Server, Reliable)
    void Server_ApplyCaptureOutcome(ABaseMonster* TargetPal, bool bSuccess);
    
    // 델리게이트
    UPROPERTY(BlueprintAssignable)
    FOnThrowPalSphere OnThrowPalSphere;

    UPROPERTY(BlueprintAssignable, Category = "UI")
    FOnCaptureUIDataUpdated OnCaptureUIDataUpdated;

    UPROPERTY(BlueprintAssignable, Category="UI")
    FOnCaptureReveal OnCaptureReveal;

    UPROPERTY(BlueprintAssignable, Category = "State")
    FOnThrowingStateChanged OnThrowingStateChanged;
    
    UPROPERTY(EditDefaultsOnly, Category = "Pal Capture")
    int32 PalSphereSkillID = 15;

protected:
    virtual void BeginPlay() override;

    UFUNCTION()
    void OnRep_IsThrowingPalSphere();

    UFUNCTION()
    void OnMonsterDetected(ABaseMonster* DetectedMonster);

    // 확률→세그먼트 수
    UPROPERTY(EditDefaultsOnly, Category="Capture|Reveal", meta=(ClampMin="0.0", ClampMax="1.0"))
    float Threshold_1Seg = 0.75f;
    UPROPERTY(EditDefaultsOnly, Category="Capture|Reveal", meta=(ClampMin="0.0", ClampMax="1.0"))
    float Threshold_2Seg = 0.50f;
    UPROPERTY(EditDefaultsOnly, Category="Capture|Reveal", meta=(ClampMin="0.0", ClampMax="1.0"))
    float Threshold_3Seg = 0.25f;

    // 시간정책
    UPROPERTY(EditDefaultsOnly, Category="Capture|Reveal", meta=(ClampMin="0.05"))
    float PerSegmentTime = 0.8f;

    // 딜레이
    UPROPERTY(EditDefaultsOnly, Category="Capture|Reveal", meta=(ClampMin="0.0"))
    float StartDelaySec = 1.9f;
    UPROPERTY(EditDefaultsOnly, Category="Capture|Reveal", meta=(ClampMin="0.0"))
    float InterStageDelaySec = 0.50f;
    UPROPERTY(EditDefaultsOnly, Category="Capture|Reveal", meta=(ClampMin="0.0"))
    float EndDelaySec = 1.9f;

    // 실패 정지 스테이지 결정용 변수
    UPROPERTY(EditDefaultsOnly, Category="Capture|Reveal", meta=(ClampMin="0.0", ClampMax="1.0"))
    float FailContinueProbMin = 0.25f;
    UPROPERTY(EditDefaultsOnly, Category="Capture|Reveal", meta=(ClampMin="0.0", ClampMax="1.0"))
    float FailContinueProbMax = 0.90f;

    int32 PickFailStage(float Guess, int32 Segments) const;
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
    
    void ApplyThrowingState(bool bThrowing);

    UFUNCTION(Server, Reliable)
    void Server_AttemptCapture(ABaseMonster* TargetPal, APalSphere* SourceSphere);
};