// Fill out your copyright notice in the Description page of Project Settings.


#include "Utility.h"
#include "Misc/FileHelper.h"
#include "HAL/PlatformFilemanager.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "Modules/ModuleManager.h"
#include "Engine/Texture2D.h"
#include "Misc/Paths.h" // 追加: 拡張子判定に使用
#include "HighResScreenshot.h"
#include "Engine/Engine.h"
#include "Blueprint/UserWidget.h"
#include "Slate/WidgetRenderer.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Engine/TextureRenderTarget2D.h"
#include "ImageUtils.h"
#include "RenderUtils.h"


// Sets default values
AUtility::AUtility()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AUtility::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AUtility::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

bool AUtility::LoadCSVToString(const FString& FilePath, FString& OutString)
{
	// プラットフォームファイルを取得
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

	// ファイル存在チェック
	if (!PlatformFile.FileExists(*FilePath))
	{
		UE_LOG(LogTemp, Warning, TEXT("File not found: %s"), *FilePath);
		return false;
	}

	// ファイル読み込み
	if (!FFileHelper::LoadFileToString(OutString, *FilePath))
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to read file: %s"), *FilePath);
		return false;
	}

	// UTF-8 BOM または BOM として扱われる U+FEFF が先頭にある場合は取り除く
	if (OutString.StartsWith(TEXT("\uFEFF")))
	{
		OutString = OutString.Mid(1);
	}

	return true;
}


TArray<FString> AUtility::SepalateToLines(const FString& InString)
{
	TArray<FString> Lines;
	InString.ParseIntoArrayLines(Lines);
	return Lines;
}

UTexture2D* AUtility::LoadTextureFromFile(const FString& FilePath)
{
	FString Path = FilePath;

	// 拡張子がない場合は検索
	if (FPaths::GetExtension(FilePath).IsEmpty())
	{
		FString Directory = FPaths::GetPath(FilePath);
		FString BaseName = FPaths::GetCleanFilename(FilePath);

		static const TArray<FString> Extensions =
		{
			TEXT("png"),
			TEXT("jpg"),
			TEXT("jpeg"),
			TEXT("bmp"),
			TEXT("exr")
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

	if (!FPaths::FileExists(Path))
	{
		UE_LOG(LogTemp, Warning, TEXT("File not found: %s"), *Path);
		return nullptr;
	}

	// UE標準ロード
	UTexture2D* Texture = FImageUtils::ImportFileAsTexture2D(Path);

	if (!Texture)
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to load texture: %s"), *Path);
		return nullptr;
	}

	Texture->SRGB = true;
	Texture->UpdateResource();

	return Texture;
}

FString AUtility::Normalize(const FString& Input)
{
	FString Result = Input;

	// ① Trim（前後空白削除）
	Result = Result.TrimStartAndEnd();

	// ② 全角英数字 → 半角変換
	for (int32 i = 0; i < Result.Len(); i++)
	{
		TCHAR& Char = Result[i];

		// 全角英数字のUnicode範囲
		if (Char >= 0xFF01 && Char <= 0xFF5E)
		{
			Char -= 0xFEE0;
		}
		// 全角スペース
		else if (Char == 0x3000)
		{
			Char = 0x0020;
		}
	}

	// ③ 小文字化
	Result = Result.ToLower();

	return Result;
}

TArray<FString> AUtility::GetFolderNames(const FString& DirectoryPath)
{
	TArray<FString> FolderNames;

	if (!FPaths::DirectoryExists(DirectoryPath))
	{
		UE_LOG(LogTemp, Warning, TEXT("Directory not found: %s"), *DirectoryPath);
		return FolderNames;
	}

	IFileManager& FileManager = IFileManager::Get();

	FString SearchPath = DirectoryPath + TEXT("*");

	FileManager.FindFiles(
		FolderNames,
		*SearchPath,
		false,
		true
	);

	return FolderNames;
}

TArray<FString> AUtility::GetCsvFileNames(const FString& DirectoryPath)
{
	TArray<FString> FileNames;

	if (!FPaths::DirectoryExists(DirectoryPath))
	{
		UE_LOG(LogTemp, Warning, TEXT("Directory not found: %s"), *DirectoryPath);
		return FileNames;
	}

	IFileManager& FileManager = IFileManager::Get();

	// パス末尾にスラッシュを保証
	FString NormalizedPath = DirectoryPath;
	if (!NormalizedPath.EndsWith(TEXT("/")))
	{
		NormalizedPath += TEXT("/");
	}

	// *.csv のみ取得（直下のみ）
	FString SearchPath = NormalizedPath + TEXT("*.csv");

	FileManager.FindFiles(
		FileNames,
		*SearchPath,
		true,   // Files
		false   // Directories
	);

	return FileNames;
}

UTextureRenderTarget2D* AUtility::RenderWidgetToTexture(
	UUserWidget* Widget,
	int32 Width,
	int32 Height)
{
	if (!Widget) return nullptr;

	UWorld* World = Widget->GetWorld();
	if (!World) return nullptr;

	UTextureRenderTarget2D* RenderTarget =
		UKismetRenderingLibrary::CreateRenderTarget2D(
			World,
			Width,
			Height
		);

	TSharedRef<SWidget> SlateWidget = Widget->TakeWidget();

	SlateWidget->SlatePrepass();

	FWidgetRenderer Renderer(true);

	Renderer.DrawWidget(
		RenderTarget,
		SlateWidget,
		FVector2D(Width, Height),
		0
	);

	return RenderTarget;
}

bool AUtility::SaveWidgetToPNG(UUserWidget* Widget, FVector2D Size, const FString& FilePath)
{
	if (!Widget) return false;

	FString Directory = FPaths::GetPath(FilePath);

	IPlatformFile& PlatformFile =
		FPlatformFileManager::Get().GetPlatformFile();

	if (!PlatformFile.DirectoryExists(*Directory))
	{
		PlatformFile.CreateDirectoryTree(*Directory);
	}

	TSharedRef<SWidget> SlateWidget = Widget->TakeWidget();

	FWidgetRenderer Renderer(true);

	UTextureRenderTarget2D* RenderTarget =
		Renderer.CreateTargetFor(Size, TF_Bilinear, false);

	Renderer.DrawWidget(RenderTarget, SlateWidget, Size, 0.f);

	FTextureRenderTargetResource* RTResource =
		RenderTarget->GameThread_GetRenderTargetResource();

	TArray<FColor> Pixels;

	RTResource->ReadPixels(Pixels);

	TArray<uint8> PNGData;

	FImageUtils::CompressImageArray(Size.X, Size.Y, Pixels, PNGData);

	return FFileHelper::SaveArrayToFile(PNGData, *FilePath);
}