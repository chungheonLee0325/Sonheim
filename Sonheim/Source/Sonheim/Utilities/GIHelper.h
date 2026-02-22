#pragma once

#include "CoreMinimal.h"
#include "Engine/Engine.h"
#include "Sonheim/GameManager/SonheimGameInstance.h"

// 간결하고 안전한 GameInstance 접근 헬퍼
inline USonheimGameInstance* GetGI(const UObject* WorldContextObject)
{
    if (!WorldContextObject)
    {
        return nullptr;
    }
    UWorld* World = GEngine->GetWorldFromContextObject(const_cast<UObject*>(WorldContextObject), EGetWorldErrorMode::LogAndReturnNull);
    return World ? World->GetGameInstance<USonheimGameInstance>() : nullptr;
}

inline USonheimGameInstance* GetGI(UWorld* World)
{
    return World ? World->GetGameInstance<USonheimGameInstance>() : nullptr;
}

// 선택적 매크로 (원치 않으면 사용하지 않아도 됨)
#ifndef GI_CTX
#define GI_CTX(WorldContextObject) GetGI(WorldContextObject)
#endif

#ifndef GI_WORLD
#define GI_WORLD(World) GetGI(World)
#endif

