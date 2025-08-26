// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ParabolaElement.h"
#include "Sonheim/AreaObject/Player/Utility/PalCaptureComponent.h"
#include "PalSphere.generated.h"

class UCaptureProgressWidget;
class UWidgetComponent;
class ABaseMonster;
class ASonheimPlayer;

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
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void OnBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
							UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
							const FHitResult& SweepResult) override;

	virtual void OnComponentHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
								FVector NormalImpulse, const FHitResult& Hit) override;
	

	UFUNCTION()
	void HandleCaptureReveal(class ABaseMonster* Pal, const FPalCaptureRevealParams& Params, APalSphere* SourceSphere);

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	USkeletalMeshComponent* SkeletalMesh = nullptr;
	
	UPROPERTY(VisibleAnywhere)
	UWidgetComponent* CaptureWidget = nullptr;

	UPROPERTY(EditDefaultsOnly, Category="UI")
	TSubclassOf<UUserWidget> CaptureProgressWidgetClass;

	/** ── BP 이벤트: 연출 시작/종료, 세그먼트 완료 */
	UFUNCTION(BlueprintImplementableEvent, Category="Capture|FX")
	void BP_OnCaptureRevealStart();

	UFUNCTION(BlueprintImplementableEvent, Category="Capture|FX")
	void BP_OnCaptureRevealEnd(bool bSuccess);

	UFUNCTION(BlueprintImplementableEvent, Category="Capture|FX")
	void BP_OnSegmentFilled(int32 SegmentIndex);
	
public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	virtual void InitElement(AAreaObject* Caster, AAreaObject* Target, const FVector& TargetLocation, FAttackData* AttackData) override;

	virtual FVector Fire(AAreaObject* Caster, AAreaObject* Target, FVector TargetLocation, float ArcValue) override;

	virtual void OnRep_Owner() override;

private:
	bool bCanHit = true;

	void CheckPalCatch(ASonheimPlayer* Caster, ABaseMonster* Target);

	TWeakObjectPtr<class UPalCaptureComponent> CaptureComp;
	TWeakObjectPtr<class ABaseMonster> LastHitPal;
	TWeakObjectPtr<UCaptureProgressWidget> BoundRevealWidget;

	void BindRevealDelegates(UCaptureProgressWidget* W);
	void UnbindRevealDelegates();

	// 헬퍼 함수
	/** 진행 위젯 호출 편의 */
	void StartCaptureProgressReveal(float Guess01, bool bSuccess, int32 Segments=3,
							 float SegmentTime=0.55f, float InterDelay=0.2f,
							 float StartDelay=0.0f, int32 FailStageOverride=-1);
	/** 소유자 클라에서만 델리게이트 연결 */
	void TryBindToCaptureComp();
	/** 연출 동안 흔들림 방지 */
	void FreezeSphereWhileShowing();

	/** 위젯 델리게이트 핸들러(세그먼트/종료) */
	UFUNCTION()
	void OnWidgetSegmentFilled(int32 SegmentIndex);

	UFUNCTION()
	void OnWidgetRevealFinished(bool bSuccess);

	/** 회전 연출 파라미터 */
	UPROPERTY(EditDefaultsOnly, Category="Capture|FX")
	float NodAngleDeg = 18.f;

	UPROPERTY(EditDefaultsOnly, Category="Capture|FX")
	float NodReturnDelay = 0.1f;

	FTimerHandle NodTimerHandle;
	void NodOnce();
	void NodReturn();
};
