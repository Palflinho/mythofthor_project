#include "UI/MythOptions.h"

#include "Misc/ConfigCacheIni.h"

namespace
{
	const TCHAR* OptionsSection = TEXT("Myth.Options");

	TMap<FName, bool>& OptionValues()
	{
		static TMap<FName, bool> Values;
		return Values;
	}
}

bool FMythOptions::DefaultOf(const FName& Key)
{
	// Desligados de fabrica no cliente LATAM (print 61002xxx): Hide Summons, Hide Players, Monster/Skill/Item Snap,
	// No Ctrl, No Shift, Zoom Out, Mouse Lock, Mini Slot, Equipment Compare, /notrade, /notalkmsg, /notalkmsg2
	static const TSet<FName> Off = {
		TEXT("HideSummons"), TEXT("HidePlayers"), TEXT("MonsterSnap"), TEXT("SkillSnap"), TEXT("ItemSnap"), TEXT("NoCtrl"),
		TEXT("NoShift"), TEXT("ZoomOut"), TEXT("MouseLock"), TEXT("MiniSlot"), TEXT("EquipCompare"), TEXT("NoTrade"),
		TEXT("NoTalkMsg"), TEXT("NoTalkMsg2"), TEXT("Window"), TEXT("Camera"), TEXT("Font"), TEXT("Call"), TEXT("HoAi"), TEXT("MerAi"),
		TEXT("ShowShop"), TEXT("ShowName"), TEXT("Stateinfo"), TEXT("Footprint"), TEXT("AutoLoot"), TEXT("ShowExp"), TEXT("NoAsk")
	};
	return !Off.Contains(Key);
}

bool FMythOptions::Get(const FName& Key)
{
	TMap<FName, bool>& Values = OptionValues();
	if (const bool* Found = Values.Find(Key))
	{
		return *Found;
	}
	int32 Stored = INDEX_NONE;
	bool bValue = DefaultOf(Key);
	if (GConfig && GConfig->GetInt(OptionsSection, *Key.ToString(), Stored, GGameUserSettingsIni) && (Stored == 0 || Stored == 1))
	{
		bValue = Stored == 1;
	}
	Values.Add(Key, bValue);
	return bValue;
}

void FMythOptions::Set(const FName& Key, bool bValue)
{
	OptionValues().Add(Key, bValue);
	if (GConfig)
	{
		GConfig->SetInt(OptionsSection, *Key.ToString(), bValue ? 1 : 0, GGameUserSettingsIni);
		GConfig->Flush(false, GGameUserSettingsIni);
	}
}
