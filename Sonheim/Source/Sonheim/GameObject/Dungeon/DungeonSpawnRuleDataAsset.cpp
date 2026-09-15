#include "DungeonSpawnRuleDataAsset.h"
#if WITH_EDITOR
#include "Misc/DataValidation.h"
EDataValidationResult UDungeonSpawnRuleDataAsset::IsDataValid(FDataValidationContext& Context) const
{
	Super::IsDataValid(Context);
	if (MonsterClass.IsNull() || Count < 1 || Count > 16)
	{
		Context.AddError(FText::FromString(TEXT("Spawn rule requires MonsterClass and Count between 1 and 16.")));
		return EDataValidationResult::Invalid;
	}
	return EDataValidationResult::Valid;
}
#endif
