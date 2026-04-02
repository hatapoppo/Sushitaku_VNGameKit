// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Utility.generated.h"

UCLASS()
class VNGAMEKIT_API AUtility : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AUtility();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// BlueprintCallable
	UFUNCTION(BlueprintCallable, Category = "FileIO")
	static bool LoadCSVToString(const FString& FilePath, FString& OutString);

	UFUNCTION(BlueprintCallable, Category = "FileIO")
	static TArray<FString> SepalateToLines(const FString& InString);

	UFUNCTION(BlueprintCallable, Category = "Mod")
	static UTexture2D* LoadTextureFromFile(const FString& FilePath);

	UFUNCTION(BlueprintPure, Category = "String")
	static FString Normalize(const FString& Input);

	UFUNCTION(BlueprintCallable, Category = "File")
	static TArray<FString> GetFolderNames(const FString& DirectoryPath);

	UFUNCTION(BlueprintCallable, Category = "File")
	static TArray<FString> GetCsvFileNames(const FString& DirectoryPath);

	UFUNCTION(BlueprintCallable)
	static UTextureRenderTarget2D* RenderWidgetToTexture(
		UUserWidget* Widget,
		int32 Width,
		int32 Height
	);

	UFUNCTION(BlueprintCallable)
	static bool SaveWidgetToPNG(
		UUserWidget* Widget,
		FVector2D Size,
		const FString& Path);
	
};
