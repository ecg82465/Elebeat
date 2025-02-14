// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "BlueprintUtilityBPLibrary.h"
#include "BlueprintUtility.h"

#include "Runtime/Engine/Classes/Engine/Engine.h"

#include "ImageUtils.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"

//#include "ImageLoader.h"

#include "Engine/Texture2D.h"



UBlueprintUtilityBPLibrary::UBlueprintUtilityBPLibrary(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{

}

//Use ContentDir()
FString UBlueprintUtilityBPLibrary::GetFullPath(FString FilePath)
{
	//Check Relative Or absolute path
	FString FullFilePath;
	if (FilePath.StartsWith(".", ESearchCase::CaseSensitive))
	{

		FullFilePath = *FPaths::Combine(FPaths::ProjectContentDir(), FilePath);

		FullFilePath = *FPaths::ConvertRelativePathToFull(FullFilePath);

	}
	else
	{

		FullFilePath = FilePath;
	}

	return FullFilePath;
}

float UBlueprintUtilityBPLibrary::BlueprintUtilitySampleFunction(float Param)
{
	return -1;
}

//Discern Texture Type
static TSharedPtr<IImageWrapper> GetImageWrapperByExtention(const FString InImagePath)
{
	IImageWrapperModule& ImageWrapperModule = FModuleManager::GetModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
	


	if (InImagePath.EndsWith(".png"))
	{
		return ImageWrapperModule.CreateImageWrapper(EImageFormat::PNG);
	}
	else if (InImagePath.EndsWith(".jpg") || InImagePath.EndsWith(".jpeg"))
	{
		return ImageWrapperModule.CreateImageWrapper(EImageFormat::JPEG);
	}
	else if (InImagePath.EndsWith(".bmp"))
	{
		return ImageWrapperModule.CreateImageWrapper(EImageFormat::BMP);
	}
	else if (InImagePath.EndsWith(".ico"))
	{
		return ImageWrapperModule.CreateImageWrapper(EImageFormat::ICO);
	}
	else if (InImagePath.EndsWith(".exr"))
	{
		return ImageWrapperModule.CreateImageWrapper(EImageFormat::EXR);
	}
	else if (InImagePath.EndsWith(".icns"))
	{
		return ImageWrapperModule.CreateImageWrapper(EImageFormat::ICNS);
	}
	return nullptr;
}


UTexture2D* UBlueprintUtilityBPLibrary::LoadTexture2DFromFile(const FString& FilePath,
	bool& IsValid, int32& Width, int32& Height)
{

	FString FullFilePath = GetFullPath(FilePath);

	if (!IsVaildPath(FullFilePath))
	{
		return NULL;
	}
	

	IsValid = false;
	UTexture2D* LoadedT2D = NULL;

	//UE_LOG(LogTemp, Warning, TEXT("FinalPath %s"), *FullFilePath);

	//IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
	TSharedPtr<IImageWrapper> ImageWrapper = GetImageWrapperByExtention(FullFilePath);
	
	//Load From File
	TArray<uint8> RawFileData;
	if (!FFileHelper::LoadFileToArray(RawFileData, *FullFilePath, 0)) return NULL ;

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	//Create T2D!
	if (ImageWrapper.IsValid() && ImageWrapper->SetCompressed(RawFileData.GetData(), RawFileData.Num()))
	{
		const TArray<uint8>* UncompressedBGRA = NULL;
		if (ImageWrapper->GetRaw(ERGBFormat::BGRA, 8, UncompressedBGRA))
		{

			LoadedT2D = UTexture2D::CreateTransient(ImageWrapper->GetWidth(), ImageWrapper->GetHeight(), PF_B8G8R8A8);

			//Valid?
			if (!LoadedT2D) return NULL;
			//~~~~~~~~~~~~~~

			//Out!
			Width = ImageWrapper->GetWidth();
			Height = ImageWrapper->GetHeight();

			//Copy!
			void* TextureData = LoadedT2D->PlatformData->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
			FMemory::Memcpy(TextureData, UncompressedBGRA->GetData(), UncompressedBGRA->Num());
			LoadedT2D->PlatformData->Mips[0].BulkData.Unlock();

			//Update!
			LoadedT2D->UpdateResource();
		}
	}

	// Success!
	IsValid = true;
	return LoadedT2D;
	//return NULL;
}


UImageLoader* UBlueprintUtilityBPLibrary::LoadTexture2DFromFile_Async(UObject* Outer, const FString& FilePath)
{
	UImageLoader* Loader = NewObject<UImageLoader>();
	Loader->LoadImageAsync(Outer,FilePath);

	return Loader;
}



bool UBlueprintUtilityBPLibrary::ReadOggWaveData(class USoundWave* sw, TArray<uint8>* rawFile)
{
	FSoundQualityInfo info;
	FVorbisAudioInfo vorbis_obj;
	if (!vorbis_obj.ReadCompressedInfo(rawFile->GetData(), rawFile->Num(), &info))
	{
		//Debug("Can't load header");
		return true;
	}

	if (!sw) return true;
	sw->SoundGroup = ESoundGroup::SOUNDGROUP_Default;
	sw->NumChannels = info.NumChannels;
	sw->Duration = info.Duration;
	sw->RawPCMDataSize = info.SampleDataSize;
	sw->SetSampleRate(info.SampleRate);

	return false;
}

bool UBlueprintUtilityBPLibrary::ReadWavWaveData(class USoundWave* sw, TArray<uint8>* rawFile)
{

	return false;
}


class USoundWave* UBlueprintUtilityBPLibrary::LoadSoundWaveFromFile(const FString& FilePath)
{

	USoundWave* sw = NewObject<USoundWave>(USoundWave::StaticClass());

	if (!sw)
		return NULL;

	//* If true the song was successfully loaded
	bool loaded = false;
	
	FString FullPath = GetFullPath(FilePath);


	//* loaded song file (binary, encoded)
	TArray < uint8 > rawFile;

	loaded = FFileHelper::LoadFileToArray(rawFile, FullPath.GetCharArray().GetData());

	if (loaded)
	{
		FByteBulkData* bulkData = &sw->CompressedFormatData.GetFormat(TEXT("OGG"));

		bulkData->Lock(LOCK_READ_WRITE);
		FMemory::Memcpy(bulkData->Realloc(rawFile.Num()), rawFile.GetData(), rawFile.Num());
		bulkData->Unlock();

		loaded = ReadOggWaveData(sw, &rawFile) == 0 ? true : false;
	}

	if (!loaded)
		return NULL;

	return sw;

}


void UBlueprintUtilityBPLibrary::ReadConfig(const FString& SectionName, const FString& ValueName, FString &ReturnValue)
{

	//GConfig->Flush(true, GGameIni);

	bool succeed = false;

	 succeed = GConfig->GetString(
		*SectionName,
		*ValueName,
		ReturnValue,
		GGameIni
	);


	UE_LOG(LogTemp, Warning, TEXT("Read Config %s "),succeed ? TEXT("Succeed") : TEXT("Fail"));
	
}


void UBlueprintUtilityBPLibrary::WriteConfig(const FString& SectionName, const FString& ValueName, const FString &ReturnValue)
{

	//FString newSection = "/Script/CommunicationSetting";
	//FString TA = "DefaultMyConfig";
	GConfig->SetString(
		*SectionName,
		*ValueName,
		*ReturnValue,
		GGameIni
	);

	GConfig->Flush(false, GGameIni);
	/*
		FString log;
		ReadConfig(ReturnValue, ValueName, log);

		UE_LOG(LogTemp, Warning, TEXT("Set Config As %s "), *log);*/

}


bool UBlueprintUtilityBPLibrary::ReadCustomPathConfig(const FString&FilePath, const FString& SectionName, const FString& ValueName, FString &ReturnString)
{
	FString FullPath = GetFullPath(FilePath);

	GConfig->Flush(true, FullPath);
	bool succeed = GConfig->GetString(*SectionName, *ValueName, ReturnString, FullPath);
	return succeed;

}


 void UBlueprintUtilityBPLibrary::WriteCustomPathConfig(const FString&FilePath, const FString& SectionName, const FString& ValueName, const FString &WriteString)
{
	FString FullPath 
= GetFullPath(FilePath);

	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

	// Does the file exist?
	if (!PlatformFile.FileExists(*FullPath))
	{
		// File doesn't exist; (Attempt to) create a new one.
		FFileHelper::SaveStringToFile(TEXT(""), *FullPath);
	}


	GConfig->SetString(*SectionName, *ValueName, *WriteString, FullPath);

	GConfig->Flush(false, FullPath);

}


 void UBlueprintUtilityBPLibrary::RefrashAllSkeletallAnimation()
 {
	 for (TObjectIterator<AActor> iterator; iterator; ++iterator)
	 {
		 if (iterator)
		 {
			 for (auto actor_Component : iterator->GetComponents())
			 {
				 if (actor_Component->IsA(USkeletalMeshComponent::StaticClass()))
				 {
					 (Cast<USkeletalMeshComponent>(actor_Component))->TickAnimation(0.0f, false);
					 (Cast<USkeletalMeshComponent>(actor_Component))->RefreshBoneTransforms();
				 }
			 }
		 }
	 }
 };




 bool UBlueprintUtilityBPLibrary::ReadFile(const FString FilePath, FString& ReturnString)
 {
	 FString FullPath = GetFullPath(FilePath);

	 FString Cache = "";
	 bool Sucess = false;
	 Sucess = FFileHelper::LoadFileToString(Cache, FullPath.GetCharArray().GetData());
	 ReturnString = Cache;
	 return Sucess;
 }

 bool UBlueprintUtilityBPLibrary::WriteFile(const FString FilePath, const FString ReturnString)
 {
	 FString FullPath = GetFullPath(FilePath);

	 bool Sucess;
	 Sucess = FFileHelper::SaveStringToFile(ReturnString, *FullPath);
	 return Sucess;
 }


 FString UBlueprintUtilityBPLibrary::GetGamePath(DirType E)
 {
	 if (E == DirType::GameDir)
	 {
		 return FPaths::ProjectDir();
	 }
	 return FPaths::ProjectContentDir();

 }


 bool UBlueprintUtilityBPLibrary::IsVaildPath(const FString ImagePath)
 {

	 if (!FPaths::FileExists(ImagePath))
	 {
		 UE_LOG(LogTemp, Warning, TEXT("File not found: %s"), *ImagePath);
		 return false;
	 }

	 // Load the compressed byte data from the file
	 TArray<uint8> FileData;
	 if (!FFileHelper::LoadFileToArray(FileData, *ImagePath))
	 {
		 UE_LOG(LogTemp, Warning, TEXT("Failed to load file: %s"), *ImagePath);
		 return false;
	 }

	 return true;
 }


 void UBlueprintUtilityBPLibrary::String__ExplodeString(TArray<FString>& OutputStrings, FString InputString, FString Separator, int32 limit, bool bTrimElements)
 {
	 OutputStrings.Empty();
	 //~~~~~~~~~~~

	 if (InputString.Len() > 0 && Separator.Len() > 0) {
		 int32 StringIndex = 0;
		 int32 SeparatorIndex = 0;

		 FString Section = "";
		 FString Extra = "";

		 int32 PartialMatchStart = -1;

		 while (StringIndex < InputString.Len()) {

			 if (InputString[StringIndex] == Separator[SeparatorIndex]) {
				 if (SeparatorIndex == 0) {
					 //A new partial match has started.
					 PartialMatchStart = StringIndex;
				 }
				 Extra.AppendChar(InputString[StringIndex]);
				 if (SeparatorIndex == (Separator.Len() - 1)) {
					 //We have matched the entire separator.
					 SeparatorIndex = 0;
					 PartialMatchStart = -1;
					 if (bTrimElements == true) {
						 OutputStrings.Add(FString(Section).Trim().TrimTrailing());
					 }
					 else {
						 OutputStrings.Add(FString(Section));
					 }

					 //if we have reached the limit, stop.
					 if (limit > 0 && OutputStrings.Num() >= limit)
					 {
						 return;
						 //~~~~
					 }

					 Extra.Empty();
					 Section.Empty();
				 }
				 else {
					 ++SeparatorIndex;
				 }
			 }
			 else {
				 //Not matched.
				 //We should revert back to PartialMatchStart+1 (if there was a partial match) and clear away extra.
				 if (PartialMatchStart >= 0) {
					 StringIndex = PartialMatchStart;
					 PartialMatchStart = -1;
					 Extra.Empty();
					 SeparatorIndex = 0;
				 }
				 Section.AppendChar(InputString[StringIndex]);
			 }

			 ++StringIndex;
		 }

		 //If there is anything left in Section or Extra. They should be added as a new entry.
		 if (bTrimElements == true) {
			 OutputStrings.Add(FString(Section + Extra).Trim().TrimTrailing());
		 }
		 else {
			 OutputStrings.Add(FString(Section + Extra));
		 }

		 Section.Empty();
		 Extra.Empty();
	 }
 }