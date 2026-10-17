#include "UITags.h"

namespace UITags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(LayerHUD, "UI.Layer.HUD", "Always on the screen: status, quest card, minimap");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(LayerMenu, "UI.Layer.Menu", "The menu's screens, one showing at a time");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(LayerPopup, "UI.Layer.Popup", "Popups over the menu, which take the input first");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(LayerToast, "UI.Layer.Toast", "Notices over everything, never taking input");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Inventory, "UI.Screen.Inventory", "The bag, the equipment and the player's stats");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(MonsterDex, "UI.Screen.MonsterDex", "The monster book");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(WorldMap, "UI.Screen.WorldMap", "The island map and fast travel");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(QuestJournal, "UI.Screen.QuestJournal", "Every quest of the player");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(QuestDialog, "UI.Screen.QuestDialog", "A quest giver's offer or report");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Container, "UI.Screen.Container", "A chest beside the player's bag");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Crafting, "UI.Screen.Crafting", "A crafting station");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(ItemQuantity, "UI.Popup.ItemQuantity", "How many of an item to drop or discard");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(ToastFeed, "UI.Toast.Feed", "The left-hand stack of progress notices");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(ToastTitle, "UI.Toast.Title", "A place's name across the top of the screen");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(ToastHeadline, "UI.Toast.Headline", "A big moment alone in the middle");
}
