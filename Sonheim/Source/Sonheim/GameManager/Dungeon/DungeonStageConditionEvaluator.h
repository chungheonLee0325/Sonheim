#pragma once
#include "CoreMinimal.h"
#include "Sonheim/GameObject/Dungeon/DungeonStageDefinition.h"
class FDungeonStageConditionEvaluator
{
public:
	static bool Evaluate(const TArray<FDungeonStageCondition>& Conditions, const FGameplayTagContainer& Tags, TFunctionRef<bool(FName)> GroupCompleted)
	{
		if (Conditions.IsEmpty()) return false;
		for (const auto& Condition : Conditions)
		{
			bool bMatches = false;
			switch (Condition.Type)
			{
			case EDungeonStageCondition::Always: bMatches = true; break;
			case EDungeonStageCondition::HasRunTag: bMatches = Tags.HasTagExact(Condition.RunTag); break;
			case EDungeonStageCondition::SpawnGroupCompleted: bMatches = GroupCompleted(Condition.GroupId); break;
			}
			if (Condition.bNegate) bMatches = !bMatches;
			if (!bMatches) return false;
		}
		return true;
	}
};
