// Fill out your copyright notice in the Description page of Project Settings.


#include "Utility.h"
#include "Misc/FileHelper.h"
#include "HAL/PlatformFilemanager.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "Internationalization/Regex.h"
#include "Modules/ModuleManager.h"
#include "Engine/Texture2D.h"
#include "Misc/Paths.h"
#include "HighResScreenshot.h"
#include "Engine/Engine.h"
#include "Blueprint/UserWidget.h"
#include "Slate/WidgetRenderer.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Engine/TextureRenderTarget2D.h"
#include "ImageUtils.h"
#include "RenderUtils.h"
#include "CoreMinimal.h"
#include <string>
#include <functional>
#include <cctype>


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

/// <summary>
/// ファイルを文字列として読み込む。UTF-8 BOM または BOM として扱われる U+FEFF が先頭にある場合は取り除く。
/// </summary>
/// <param name="FilePath"></param>
/// <param name="OutString"></param>
/// <returns></returns>
bool AUtility::LoadFileToString(const FString& FilePath, FString& OutString)
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


/// <summary>
///	テキストを行ごとに分割して配列で返す。改行コードは \r, \n, \r\n のいずれも対応。空行も含む。
/// </summary>
/// <param name="InString"></param>
/// <returns></returns>
TArray<FString> AUtility::SepalateToLines(const FString& InString)
{
	TArray<FString> Lines;
	InString.ParseIntoArrayLines(Lines);
	return Lines;
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

/// <summary>
/// 指定ディレクトリ直下のCSVファイル名を取得します。
/// </summary>
/// <param name="DirectoryPath">検索対象のディレクトリのパス。末尾のスラッシュは自動で補正されます。</param>
/// <returns>見つかったCSVファイル名の配列。指定ディレクトリ直下の *.csv ファイル（非再帰、ファイル名のみ）を返します。ディレクトリが存在しない場合は空の配列を返し、警告をログに出力します。</returns>
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

TArray<FString> AUtility::GetIniFileNames(const FString& DirectoryPath)
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
	FString SearchPath = NormalizedPath + TEXT("*.ini");

	FileManager.FindFiles(
		FileNames,
		*SearchPath,
		true,   // Files
		false   // Directories
	);

	return FileNames;
}

/// <summary>
/// テクスチャをファイルから読み込む。
/// 拡張子がない場合は png/jpg/jpeg/bmp/exr を順に検索する。
/// UE標準のロード方法を使用するため、サポートされている形式であれば拡張子がなくても読み込める。SRGBは有効にして返す。
/// </summary>
/// <param name="FilePath"></param>
/// <returns></returns>
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


/// <summary>
/// ウィジェットを指定サイズのテクスチャにレンダリングして返す関数。
/// </summary>
/// <param name="Widget"></param>
/// <param name="Width"></param>
/// <param name="Height"></param>
/// <returns></returns>
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

/// <summary>
/// ウィジェットを指定サイズのPNG画像として保存する関数。
/// 保存先のディレクトリが存在しない場合は作成する。成功したら true を返す。
/// </summary>
/// <param name="Widget"></param>
/// <param name="Size"></param>
/// <param name="FilePath"></param>
/// <returns></returns>
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

/// <summary>
/// 文字列を正規化する
/// </summary>
/// <param name="Input"></param>
/// <returns></returns>
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

/// <summary>
/// テキストから正規表現でパターンを抽出する関数。
/// </summary>
/// <param name="Input"></param>
/// <param name="InPattern">正規表現のパターン</param>
/// <param name="bFullMatch">true の場合はマッチ全体を、false の場合は最初のキャプチャグループを抽出</param>
/// <returns></returns>
TArray<FString> AUtility::ExtractPattern(const FString& Input, const FString& InPattern, const bool bFullMatch)
{
	TArray<FString> Results;

	FRegexPattern Pattern(InPattern);
	FRegexMatcher Matcher(Pattern, Input);

	if (bFullMatch) 
	{
		while (Matcher.FindNext())
		{
			Results.Add(Matcher.GetCaptureGroup(0));
		}
	}
	else {
		while (Matcher.FindNext())
		{
			// キャプチャグループ1 = {}の中身
			Results.Add(Matcher.GetCaptureGroup(1));
		}
	}

	return Results;
}

// ----------------------
// Blueprint用ラッパー
// ----------------------
/// <summary>
/// 文字列で表された数式を評価する関数。四則演算と括弧に対応。例: "3 + 4 * (2 - 1)"
/// </summary>
/// <param name="Input"></param>
/// <returns></returns>
/// BlueprintPure で呼び出せるようにするため、内部で再帰下降パーサを実装して EvaluateExpression_Internal を呼び出す。
/// BlueprintPure は副作用のない関数を呼び出すためのもので、再帰下降パーサは状態を持たないため、BlueprintPure で呼び出せる。
double AUtility::EvaluateExpression(const FString& Input)
{
	int32 Index = 0;
	return EvaluateExpression_Internal(Input, Index);
}

// ----------------------
// 内部パーサ関数
// ----------------------
double AUtility::EvaluateExpression_Internal(const FString& Expr, int32& Index)
{
	auto SkipSpaces = [&](void)
		{
			while (Index < Expr.Len() && FChar::IsWhitespace(Expr[Index]))
			{
				Index++;
			}
		};

	std::function<double()> ParseExpression; // + -
	std::function<double()> ParseTerm;       // * /
	std::function<double()> ParseFactor;     // 数字 / () / 単項符号

	// 数字・()・符号
	ParseFactor = [&]() -> double
		{
			SkipSpaces();

			bool bNegative = false;

			// ★ ここをループに変更
			while (Index < Expr.Len() && (Expr[Index] == '+' || Expr[Index] == '-'))
			{
				if (Expr[Index] == '-')
				{
					bNegative = !bNegative; // マイナスが来るたびに反転
				}
				Index++;
				SkipSpaces();
			}

			double Value = 0;

			if (Index < Expr.Len() && Expr[Index] == '(')
			{
				Index++; // '('
				Value = EvaluateExpression_Internal(Expr, Index);
				if (Index < Expr.Len() && Expr[Index] == ')')
				{
					Index++; // ')'
				}
			}
			else
			{
				while (Index < Expr.Len() && FChar::IsDigit(Expr[Index]))
				{
					Value = Value * 10 + (Expr[Index++] - '0');
				}
			}

			return bNegative ? -Value : Value;
		};

	// * /
	ParseTerm = [&]() -> double
		{
			double Value = ParseFactor();

			while (true)
			{
				SkipSpaces();
				if (Index >= Expr.Len()) break;

				if (Expr[Index] == '*')
				{
					Index++;
					Value *= ParseFactor();
				}
				else if (Expr[Index] == '/')
				{
					Index++;
					Value /= ParseFactor();
				}
				else if (Expr[Index] == '%') 
				{
					Index++;
					double RHS = ParseFactor();

					if (RHS == 0)
					{
						return 0;
					}

					Value = FMath::Fmod(Value, RHS);
				}
				else break;
			}

			return Value;
		};

	// + -
	ParseExpression = [&]() -> double
		{
			double Value = ParseTerm();

			while (true)
			{
				SkipSpaces();
				if (Index >= Expr.Len()) break;

				if (Expr[Index] == '+')
				{
					Index++;
					Value += ParseTerm();
				}
				else if (Expr[Index] == '-')
				{
					Index++;
					Value -= ParseTerm();
				}
				else break;
			}

			return Value;
		};

	return ParseExpression();
}

bool AUtility::IsValidExpressionString(const FString& Expr)
{
	for (int32 i = 0; i < Expr.Len(); i++)
	{
		TCHAR C = Expr[i];

		if (FChar::IsDigit(C) ||
			C == '+' || C == '-' ||
			C == '*' || C == '/' || C == '%' ||
			C == '(' || C == ')' ||
			FChar::IsWhitespace(C))
		{
			continue;
		}

		// それ以外は全部NG
		return false;
	}

	return true;
}