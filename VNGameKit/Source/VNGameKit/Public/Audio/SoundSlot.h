// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Containers/Queue.h"
#include "RuntimeAudioImporterLibrary.h"
#include "Components/AudioComponent.h"
#include "SoundSlot.generated.h"

UCLASS()
class VNGAMEKIT_API USoundSlot : public UObject
{
    GENERATED_BODY()

public:

    USoundSlot();

    void Init(UObject* InWorldContext);

    // 再生
    void PlaySound(const FString InSoundFileName, const float InVolume, const float InVolumeMultipiler, const float InFadeTime, const bool bRestart = false);

    // 停止
    void StopSound(const float FadeTime);

    // 音声ロードを予約する
    void EnqueueLoadRequest(const FString LoadFileName, const bool bIsImmediately = false);

    // 走査情報からスロット内のロード予約を更新する
    void UpdatePendingLoadQueue(const TArray<FString> SoundFileNameList);

    // 再生中のサウンドの音量を変更する
    void ChangeVolumeMultipiler(const float InVolumeMultipiler);


public:


    // キャッシュされている音源リスト
    UPROPERTY()
    TMap<FString, TObjectPtr<UImportedSoundWave>> CacheMap;

    // 優先ロード待ち音源名リスト
    TQueue<FString> ImmediateLoadQueue;

    // ロード待ち音源名リスト
    TQueue<FString> PendingLoadQueue;

    // ロード待ち音源名TSet（重複防止）
    TSet<FString> PendingLoadSoundSet;

    // 再生中のサウンドID
    FString CurrentSoundFileName;

    // ロード中のサウンドID
    FString CurrentLoadingSoundFileName;

    // 再生中のサウンド音量
    float CurrentVolume;

    // 現在のオプション音量
    float OptionVolumeMultiPiler;

    // ループするか
    bool bIsLoop;

    



// RuntimeAudioImpoterのDelegate
private:
    void OnImportFinishedNative(URuntimeAudioImporterLibrary* InImporter, UImportedSoundWave* ImportedSoundWave, ERuntimeImportStatus Status);
    

private:

    // Queueを見て次のサウンドをロードする
    bool TryLoadNextAudio();


    // 音声ロード開始
    void LoadAudio(const FString LoadFileName);

private:

    // 読み込み次第鳴らしたいサウンドのデータ

    FString ImmediatelySoundName;
    float ImmediatelySoundVolume;
    float ImmediatelySoundFadeTime;
    bool bImmediateryIsRestart;

    void SetImmediaterySoundData(const FString InSoundName, const float InVolume, const float InFadeTime, const bool bInIsRestart);


private:

    // AudioComponent生成
    void CreateAudioComponent(UImportedSoundWave* SoundWave, float Volume);

    // AudioComponent破棄
    void DestroyAudioComponent();

private:

    bool bIsImporting;

    UPROPERTY()
    UObject* WorldContextObject;

    // RuntimeAudioImporter
    UPROPERTY()
    TObjectPtr<URuntimeAudioImporterLibrary> Importer = nullptr;

    // AudioComponent
    UPROPERTY()
    TObjectPtr<UAudioComponent> AudioComponent = nullptr;

};