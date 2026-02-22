#pragma once

#include "CoreMinimal.h"
#include "Engine/World.h"

#include "Sonheim/GameManager/SonheimTableManagerSubsystem.h"

namespace Sonheim::TableManager
{
	inline USonheimTableManagerSubsystem* Get(const UObject* WorldContextObject)
	{
		if (!WorldContextObject)
		{
			return nullptr;
		}

		if (const UWorld* World = WorldContextObject->GetWorld())
		{
			if (UGameInstance* GI = World->GetGameInstance())
			{
				return GI->GetSubsystem<USonheimTableManagerSubsystem>();
			}
		}

		return nullptr;
	}

	inline USonheimTableManagerSubsystem* Get(UWorld* World)
	{
		if (!World)
		{
			return nullptr;
		}

		if (UGameInstance* GI = World->GetGameInstance())
		{
			return GI->GetSubsystem<USonheimTableManagerSubsystem>();
		}

		return nullptr;
	}
}
