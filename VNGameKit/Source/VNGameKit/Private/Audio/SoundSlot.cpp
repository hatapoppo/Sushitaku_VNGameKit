
#include "Audio/SoundSlot.h"
#include "Kismet/GameplayStatics.h"


USoundSlot::USoundSlot()
{
	bIsLoop = false;
	bIsImporting = false;

}


void USoundSlot::Init(UObject* InWorldContext)
{
	WorldContextObject = InWorldContext;

	SetImmediaterySoundData(FString(), 0.0f, 0.0f, false);

	Importer = NewObject<URuntimeAudioImporterLibrary>(this);
	Importer->OnResultNative.AddUObject(this, &USoundSlot::OnImportFinishedNative);
}


void USoundSlot::PlaySound(const FString InSoundFileName, const float InVolume, const float InFadeTime, const bool bRestart)
{

	// ロード未完了
	if (!CacheMap.Find(InSoundFileName))
	{
		//// ロード終了次第鳴らすようにしてもらう
		SetImmediaterySoundData(InSoundFileName, InVolume, InFadeTime, bRestart);

		// 優先Queueに差し込み
		EnqueueLoadRequest(InSoundFileName, true);

		return;
	}

	// SoundWave取得
	UImportedSoundWave* SoundWave = CacheMap.Find(InSoundFileName)->Get();
	SoundWave->SetLooping(bIsLoop);

	float FadeTime = FMath::Max(0.01f, InFadeTime);

	// 再生、または音声差し替え
	if (!IsValid(AudioComponent))
	{
		SoundWave->SetNumOfPlayedFrames(0);
		CreateAudioComponent(SoundWave, InVolume);

		// フェードイン
		AudioComponent->FadeIn(FadeTime, 1.0f);

	}
	// 再生中のサウンド　かつ　鳴らし直さない設定なら音量変更以外をスキップ
	else if (!CurrentSoundFileName.Equals(InSoundFileName) || bRestart)
	{

		// フェードアウト
		AudioComponent->FadeOut(0.1f,0.0f);
		
		// タイマーで待つ
		FTimerHandle TimerHandle;
		GetWorld()->GetTimerManager().SetTimer(
			TimerHandle,
			[this, SoundWave, InVolume, FadeTime]()
			{
				AudioComponent->Stop();

				// 音声差し替え
				SoundWave->SetNumOfPlayedFrames(0);
				AudioComponent->SetSound(SoundWave);
				AudioComponent->SetVolumeMultiplier(InVolume);
				// フェードイン
				AudioComponent->FadeIn(FadeTime, 1.0f,0.0f);
			},
			0.1f, // FadeOutと同じ時間
			false
		);



	}
	else
	{
		AudioComponent->SetVolumeMultiplier(InVolume);
	}



	CurrentSoundFileName = InSoundFileName;

	
}


void USoundSlot::StopSound(const float FadeTime)
{
	if (IsValid(AudioComponent))
	{
		AudioComponent->FadeOut(FadeTime, 0.0f);
		CurrentSoundFileName = FString();
	}
}


void USoundSlot::EnqueueLoadRequest(const FString LoadFileName, const bool bIsImmediately)
{

	// 優先キューに入れるか判定
	if (bIsImmediately)
	{
		ImmediateLoadQueue.Enqueue(LoadFileName);
	}
	else
	{
		PendingLoadQueue.Enqueue(LoadFileName);
	}

	// ロード予約リストに追加
	PendingLoadSoundSet.Add(LoadFileName);


	TryLoadNextAudio();

}

void USoundSlot::UpdatePendingLoadQueue(const TArray<FString> SoundFileNameList)
{
	for (const auto& FileName : SoundFileNameList)
	{
		// 予約済みならスキップ
		if (PendingLoadSoundSet.Contains(FileName))
		{
			continue;
		}

		// ロード済みならスキップ
		if (CacheMap.Contains(FileName))
		{
			continue;
		}

		// 通常キューに追加
		EnqueueLoadRequest(FileName);
	}
}



void USoundSlot::OnImportFinishedNative(URuntimeAudioImporterLibrary* InImporter, UImportedSoundWave* ImportedSoundWave, ERuntimeImportStatus Status)
{
	bIsImporting = false;

	// 成功
	if (Status == ERuntimeImportStatus::SuccessfulImport)
	{
		// キャッシュリストに追加する
		CacheMap.Add(CurrentLoadingSoundFileName, ImportedSoundWave);

		// 読み込み次第鳴らしたいサウンドデータだったら再生する。
		if (ImmediatelySoundName.Equals(CurrentLoadingSoundFileName))
		{
			PlaySound(ImmediatelySoundName,ImmediatelySoundVolume,ImmediatelySoundFadeTime,bImmediateryIsRestart);
			
			// 一時データのリセット
			SetImmediaterySoundData(FString(), 0.0f, 0.0f, false);
		}

	}
	else
	{
		// エラー ログを出してください（TODO）

	}

	// ロード予約リストから削除
	PendingLoadSoundSet.Remove(CurrentLoadingSoundFileName);

	// 次のサウンドをロードする
	TryLoadNextAudio();

}


bool USoundSlot::TryLoadNextAudio()
{
	// インポート中はスキップ
	if (bIsImporting) 
	{
		return false;
	}

	if (!ImmediateLoadQueue.IsEmpty())
	{
		// Queueから取り出し
		FString NextLoadID;
		ImmediateLoadQueue.Dequeue(NextLoadID);

		// ロード開始
		LoadAudio(NextLoadID);

		return true;

	}

	// 次のロード待ちがなければロードIDを空にする
	if (PendingLoadQueue.IsEmpty())
	{
		CurrentLoadingSoundFileName = FString();
		return false;
	}
	else
	{
		// Queueから取り出し
		FString NextLoadID;
		PendingLoadQueue.Dequeue(NextLoadID);

		// ロード開始
		LoadAudio(NextLoadID);
		
		return true;
	}
}


void USoundSlot::LoadAudio(FString LoadFileName)
{

	// 既に読み込み済 or ロード中だったらスキップ
	if (CacheMap.Contains(LoadFileName) || CurrentLoadingSoundFileName.Equals(LoadFileName))
	{
		return;
	}

	// インポート開始
	bIsImporting = true;
	CurrentLoadingSoundFileName = LoadFileName;
	Importer->ImportAudioFromFile(LoadFileName, ERuntimeAudioFormat::Auto);

}


void USoundSlot::SetImmediaterySoundData(const FString InSoundName, const float InVolume, const float InFadeTime, const bool bInIsRestart)
{
	ImmediatelySoundName = InSoundName;
	ImmediatelySoundVolume = InVolume;
	ImmediatelySoundFadeTime = InFadeTime;
	bImmediateryIsRestart = bInIsRestart;
}


void USoundSlot::CreateAudioComponent(UImportedSoundWave* InSoundWave, float InVolume)
{
	DestroyAudioComponent();

	AudioComponent = UGameplayStatics::CreateSound2D(WorldContextObject, InSoundWave, InVolume,1.0f,0.0f,nullptr,false,false);

}


void USoundSlot::DestroyAudioComponent()
{
	if (!AudioComponent)
	{
		return;
	}

	// 止めてから破棄
	AudioComponent->Stop();

	AudioComponent->DestroyComponent();

	AudioComponent = nullptr;

}

