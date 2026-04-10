// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RuntimeAudioImporterLibrary.h"
#include "SoundSlot.h"
#include "SoundGameInstanceSubsystem.generated.h"



/* 
* サウンド種別
*/ 
UENUM(BlueprintType)
enum class EVNSoundType : uint8
{
    BGM1,
    BGM2,
    SE,

    MAX UMETA(Hidden)
};

ENUM_RANGE_BY_COUNT(EVNSoundType, EVNSoundType::MAX)



/*
* サウンド管理用GameInstanceSubSystem
*/
UCLASS(BlueprintType, Blueprintable)
class VNGAMEKIT_API USoundGameInstanceSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
    void Initialize(FSubsystemCollectionBase& Collection) override;
    void Deinitialize() override;

    // オプションの音量設定をセットする
    UFUNCTION(BlueprintCallable)
    void SetOptionVolumeData(TMap<EVNSoundType, float> InOptionVolumeData);



    // メモ　設定のボリュームを見て音量を入れる
    // 再生
    UFUNCTION(BlueprintCallable)
    void VNPlaySound(const EVNSoundType InSoundType, const FString InSoundFileName, const float InVolume, const float InFadeTime, const bool bInIsRestart = false);

    // 停止
    UFUNCTION(BlueprintCallable)
    void VNStopSound(const EVNSoundType InSoundType, const float FadeTime = 0.0f);
    
    // 走査情報を渡してスロット内のロード予約を更新する
    UFUNCTION(BlueprintCallable)
    void UpdatePendingLoadQueue(const EVNSoundType InSoundType, const TArray<FString> SoundFileNameList);

    // 全てのサウンドを停止する
    UFUNCTION(BlueprintCallable)
    void StopAllSound(const float InFadeTime = 0.0f);


private:
    USoundSlot* GetSoundSlot(const EVNSoundType) const;

private:
    // SoundSlot
    UPROPERTY()
    TMap<EVNSoundType, TObjectPtr<USoundSlot>> SoundSlotMap;

    TMap<EVNSoundType, float> OptionVolumeData;

};