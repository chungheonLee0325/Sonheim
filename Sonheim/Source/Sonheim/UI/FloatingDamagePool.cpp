#include "FloatingDamagePool.h"
#include "FloatingDamageActor.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Sonheim/Utilities/LogMacro.h"

AFloatingDamagePool* AFloatingDamagePool::Instance = nullptr;

AFloatingDamagePool::AFloatingDamagePool()
{
	PrimaryActorTick.bCanEverTick = false;

	// 네트워크 리플리케이션 비활성화 (로컬 전용)
	bReplicates = false;
}

AFloatingDamagePool* AFloatingDamagePool::GetInstance(UWorld* World)
{
	if (!World) return nullptr;

	if (!Instance || !IsValid(Instance) || Instance->GetWorld() != World)
	{
		// 새 인스턴스 생성 또는 기존 인스턴스 찾기
		TArray<AActor*> FoundActors;
		UGameplayStatics::GetAllActorsOfClass(World, AFloatingDamagePool::StaticClass(), FoundActors);

		if (FoundActors.Num() > 0)
		{
			Instance = Cast<AFloatingDamagePool>(FoundActors[0]);
		}
		else
		{
			// 새로 생성
			FActorSpawnParameters SpawnParams;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

			Instance = World->SpawnActor<AFloatingDamagePool>(AFloatingDamagePool::StaticClass(),
			                                                  FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
		}
	}

	return Instance;
}

void AFloatingDamagePool::BeginPlay()
{
	Super::BeginPlay();

	InitializePool();
}

void AFloatingDamagePool::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 풀 정리
	for (AFloatingDamageActor* Actor : Pool)
	{
		if (IsValid(Actor))
		{
			Actor->Destroy();
		}
	}
	Pool.Empty();
	ActiveActors.Empty();

	// 싱글톤 참조 제거
	if (Instance == this)
	{
		Instance = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}

void AFloatingDamagePool::InitializePool()
{
	if (!FloatingDamageActorClass)
	{
		FloatingDamageActorClass = AFloatingDamageActor::StaticClass();
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	for (int32 i = 0; i < InitialPoolSize; i++)
	{
		AFloatingDamageActor* NewActor = GetWorld()->SpawnActor<AFloatingDamageActor>(
			FloatingDamageActorClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);

		if (NewActor)
		{
			NewActor->SetActorHiddenInGame(true);
			NewActor->SetActorTickEnabled(false);
			NewActor->SetPool(this);
			Pool.Add(NewActor);
		}
	}
}

AFloatingDamageActor* AFloatingDamagePool::GetPooledActor()
{
	// 사용 가능한 액터 찾기
	if (Pool.Num() > 0)
	{
		AFloatingDamageActor* Actor = Pool.Pop();
		Actor->Reset();
		ActiveActors.Add(Actor);
		return Actor;
	}

	// 풀이 비었고 최대 크기에 도달하지 않았다면 새로 생성
	if (ActiveActors.Num() < MaxPoolSize)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		AFloatingDamageActor* NewActor = GetWorld()->SpawnActor<AFloatingDamageActor>(
			FloatingDamageActorClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);

		if (NewActor)
		{
			NewActor->SetPool(this);
			NewActor->Reset();
			ActiveActors.Add(NewActor);
			return NewActor;
		}
	}

	// 최대 크기에 도달했다면 가장 오래된 액터 재사용
	if (ActiveActors.Num() > 0)
	{
		AFloatingDamageActor* OldestActor = ActiveActors[0];
		OldestActor->Reset();
		return OldestActor;
	}

	return nullptr;
}

void AFloatingDamagePool::ReturnToPool(AFloatingDamageActor* Actor)
{
	if (!Actor) return;

	ActiveActors.Remove(Actor);

	// 액터 초기화
	Actor->Reset();
	Actor->SetActorHiddenInGame(true);
	Actor->SetActorTickEnabled(false);
	Actor->SetActorLocation(FVector::ZeroVector);

	Pool.Add(Actor);
}

bool AFloatingDamagePool::ShouldShowDamageForPlayer(const FVector& DamageLocation,
                                                    APlayerController* PlayerController) const
{
	if (!PlayerController) return false;

	APawn* PlayerPawn = PlayerController->GetPawn();
	if (!PlayerPawn) return false;

	float Distance = FVector::Dist(PlayerPawn->GetActorLocation(), DamageLocation);
	return Distance <= MaxVisibleDistance;
}

void AFloatingDamagePool::RequestDamageNumber(const FDamageNumberRequest& Request)
{
	// 로컬 플레이어 확인
	APlayerController* LocalPC = GetWorld()->GetFirstPlayerController();
	if (!LocalPC || !LocalPC->IsLocalController()) return;

	// 거리 체크
	if (!ShouldShowDamageForPlayer(Request.WorldLocation, LocalPC)) return;

	AFloatingDamageActor* DamageActor = GetPooledActor();
	if (DamageActor)
	{
		DamageActor->SetActorLocation(Request.WorldLocation);
		DamageActor->SetActorHiddenInGame(false);
		DamageActor->SetActorTickEnabled(true);
		DamageActor->Initialize(Request.Damage, Request.WeakPointType,
		                        Request.ElementAttributeType);
	}
}