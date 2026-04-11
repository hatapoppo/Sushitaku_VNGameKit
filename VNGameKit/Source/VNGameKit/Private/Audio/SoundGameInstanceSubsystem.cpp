
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

			if (Type == EVNSoundType::BGM1 || Type == EVNSoundType::BGM2)
			{
				Slot->bIsLoop = true;
			}
		}
	}
}

void USoundGameInstanceSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

void USoundGameInstanceSubsystem::SetOptionVolumeData(float InBGMVolume, float InSEVolume)
{
	OptionVolumeBGM = InBGMVolume;
	OptionVolumeSE = InSEVolume;

	for (const auto& Pair : SoundSlotMap)
	{
		switch (Pair.Key)
		{
		case EVNSoundType::BGM1:
		case EVNSoundType::BGM2:
			Pair.Value->ChangeVolumeMultipiler(OptionVolumeBGM / 100.0f);
			break;

		case EVNSoundType::SE:
			Pair.Value->ChangeVolumeMultipiler(OptionVolumeSE / 100.0f);
			break;

		default:
			break;

		}
	}

}

void USoundGameInstanceSubsystem::VNPlaySound(const EVNSoundType InSoundType, const FString InSoundFileName, const float InVolume, const float InFadeTime, const bool bInIsRestart)
{

	FString Path = InSoundFileName;

	// 拡張子が見つからなかった場合、検索
	if (FPaths::GetExtension(InSoundFileName).IsEmpty())
	{
		FString Directory = FPaths::GetPath(InSoundFileName);
		FString BaseName = FPaths::GetCleanFilename(InSoundFileName);

		static const TArray<FString> Extensions =
		{
			TEXT("wav"),
			TEXT("mp3"),
			TEXT("ogg"),
			TEXT("flac"),
			TEXT("m4a")
		};

		for (const FString& Ext : Extensions)
		{
			FString TestPath = Directory / (BaseName + TEXT(".") + Ext);
			if (FPaths::FileExists(TestPath))
			{
				Path = TestPath;
				break;
			}
		}
	}

	USoundSlot* SoundSlot = GetSoundSlot(InSoundType);
	if (!SoundSlot)
	{
		return;
	}

	float OptionVolumeMultiplier = 1.0f;

	// サウンドタイプによって参照するオプションを切り替える
	switch (InSoundType)
	{
	case EVNSoundType::BGM1:
	case EVNSoundType::BGM2:
		OptionVolumeMultiplier = OptionVolumeBGM / 100.0f;
		break;

	case EVNSoundType::SE:
		OptionVolumeMultiplier = OptionVolumeSE / 100.0f;
		break;

	default:
		break;

	}

	SoundSlot->PlaySound(Path, InVolume ,OptionVolumeMultiplier, InFadeTime, bInIsRestart);

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

void USoundGameInstanceSubsystem::StopAllSound(const float InFadeTime)
{

	for (const auto& Pair : SoundSlotMap)
	{
		USoundSlot* SoundSlot = Pair.Value;

		if (!SoundSlot)
		{
			return;
		}

		SoundSlot->StopSound(InFadeTime);

	}

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
