#include "NativeGameplayTags.h"
// The screens the UI registry maps to widgets. A dungeon's own tags (stages, groups, zones, barriers, run flags) live under
// Dungeon.<Name> in that dungeon's tag file, Config/Tags/Dungeon_<Name>.ini.
UE_DEFINE_GAMEPLAY_TAG(TAG_UIDungeonHUD, "UI.Dungeon.HUD");
UE_DEFINE_GAMEPLAY_TAG(TAG_UIDungeonResult, "UI.Dungeon.Result");
