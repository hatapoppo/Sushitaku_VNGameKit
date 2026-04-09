
#include "Audio/SoundGameInstanceSubsystem.h"

void  USoundGameInstanceSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// SoundSlot生成
	for (EVNSoundType Type : TEnumRange<EVNSoundType>())
	{
		if (Type != EVNSoundType::MAX)
		{
			TObjectPtr<USoundSlot> Slot = NewObject<USoundSlot>(this);
			Slot->Init(GetGameInstance());
			SoundSlotMap.Add(Type, Slot);
		}
	}
}

void USoundGameInstanceSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

void USoundGameInstanceSubsystem::SetOptionVolumeData(TMap<EVNSoundType, float> InOptionVolumeData)
{
	OptionVolumeData = InOptionVolumeData;
}

void USoundGameInstanceSubsystem::VNPlaySound(const EVNSoundType InSoundType, const FString InSoundFileName, const float InVolume, const float InFadeTime, const bool bInIsRestart)
{

	USoundSlot* SoundSlot = GetSoundSlot(InSoundType);
	if (!SoundSlot)
	{
		return;
	}

	// キャッシュしているオプションの値を取得
	float OptionVolumeMultiplyValue = 1.0f;

	if (OptionVolumeData.Contains(InSoundType))
	{
		OptionVolumeMultiplyValue = *OptionVolumeData.Find(InSoundType);
	}
	else
	{
		// ワーニング
	}

	SoundSlot->PlaySound(InSoundFileName, InVolume * OptionVolumeMultiplyValue, InFadeTime, bInIsRestart);

}

void USoundGameInstanceSubsystem::VNStopSound(const EVNSoundType InSoundType, const float InFadeTime)
{
	USoundSlot* SoundSlot = GetSoundSlot(InSoundType);
	if (!SoundSlot)
	{
		return;
	}

	SoundSlot->StopSound(InFadeTime);

}

void USoundGameInstanceSubsystem::UpdatePendingLoadQueue(const EVNSoundType InSoundType, const TArray<FString> SoundFileNameList)
{
	USoundSlot* SoundSlot = GetSoundSlot(InSoundType);
	if (!SoundSlot)
	{
		return;
	}

	SoundSlot->UpdatePendingLoadQueue(SoundFileNameList);

}


USoundSlot* USoundGameInstanceSubsystem::GetSoundSlot(const EVNSoundType InSoundType) const
{
	if (const TObjectPtr<USoundSlot>* Found = SoundSlotMap.Find(InSoundType))
	{
		return Found->Get();
	}

	return nullptr;
}
