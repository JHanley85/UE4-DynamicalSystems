#pragma once

#include "CoreMinimal.h"
#include "NetClient.h"
#include "Runtime/Core/Public/Serialization/Archive.h"
#include "Runtime/Core/Public/Serialization/BufferArchive.h"
#include "Runtime/Core/Public/Serialization/MemoryReader.h"
#include "Runtime/Core/Public/Serialization/ArchiveSaveCompressedProxy.h"
#include "Runtime/Core/Public/Serialization/ArchiveLoadCompressedProxy.h"
#include "Runtime/Engine/Classes/Components/ActorComponent.h"
#include "NetAvatar.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(NetAvatar, Log, All);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FNetAvatarUpdate);

UCLASS( ClassGroup=(DynamicalSystems), meta=(BlueprintSpawnableComponent) )
class DYNAMICALSYSTEMS_API UNetAvatar : public UActorComponent
{
	GENERATED_BODY()

public:

	UNetAvatar();

	virtual void BeginPlay() override;

	virtual void TickComponent( float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction ) override;

	float LastUpdateTime;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NetAvatar")
	ANetClient* NetClient = NULL;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "NetAvatar")
	int NetID = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NetAvatar")
	FVector Location;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NetAvatar")
	FRotator Rotation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NetAvatar")
	FVector LocationHMD;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NetAvatar")
	FRotator RotationHMD;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NetAvatar")
	FVector LocationHandL;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NetAvatar")
	FRotator RotationHandL;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NetAvatar")
	FVector LocationHandR;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NetAvatar")
	FRotator RotationHandR;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NetAvatar")
	float Height = 160;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NetAvatar")
	float Floor = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NetClient")
	bool IsNetProxy = false;

	UPROPERTY(BlueprintAssignable, Category = "NetClient")
		FNetAvatarUpdate OnAvatarUpdate;
	FPoseSnapshot LatestPose;
	FPoseSnapshot PreviousPose;
	FPoseSnapshot PredictedPose;
	UFUNCTION(BlueprintCallable, Category = "Experimental")
		bool SavePose(FPoseSnapshot Pose, FString Path, uint8 meshid) {
		FBufferArchive Ar = FBufferArchive(false, "temp");
		bool result= PoseSerializeCompressed(Ar, Pose, Path,  meshid);
		PreviousPose = LatestPose;
		LatestPose = Pose;
		return result;
	}

	UFUNCTION(BlueprintCallable, Category = "Experimental")
		static bool LoadPose(FPoseSnapshot& Pose, FString Path, uint8& meshid) {
		return PoseDeserializeCompressed(Path, Pose, meshid);
	}
	static bool PoseSerializeCompressed(FBufferArchive& Ar, FPoseSnapshot Pose, FString& FullFilePath, uint8 meshid)
	{
		SaveLoadPoseData(Ar, Pose,meshid);
		//Pre Compressed Size
		UE_LOG(LogTemp, Log, TEXT("~ PreCompressed Size ~ %i"),Ar.Num());
		TArray<uint8> CompressedData;
		FArchiveSaveCompressedProxy Compressor =
			FArchiveSaveCompressedProxy(CompressedData, ECompressionFlags::COMPRESS_ZLIB);
		//Send entire binary array/archive to compressor
		Compressor << Ar;
		//send archive serialized data to binary array
		Compressor.Flush();
		//Compressed Size
		UE_LOG(LogTemp, Log, TEXT(" Compressed Size ~ %i"),CompressedData.Num());

		if (FFileHelper::SaveArrayToFile(CompressedData, *FullFilePath))
		{
			// Free Binary Array 	
			Ar.FlushCache();
			Ar.Empty();
			return true;
		}
		// Free Binary Array 	
		Ar.FlushCache();
		Ar.Empty();
		return false;
	}
	static void SaveLoadPoseData(FArchive& Ar, FPoseSnapshot& Pose, uint8& meshid) {
//		Ar << uuid; //sender
//		Ar << guid; //objectid
		Ar << meshid;
		Ar << Pose.LocalTransforms;
	}
	static bool  PoseDeserializeCompressed(const FString& FullFilePath, FPoseSnapshot& Pose, uint8& MeshId) {
		TArray<uint8> CompressedData;
		if (!FFileHelper::LoadFileToArray(CompressedData, *FullFilePath))
		{
			UE_LOG(LogTemp, Warning, TEXT("FFILEHELPER:>> Invalid File"));
			return false;
			//~~
		}
		// Decompress File 
		FArchiveLoadCompressedProxy Decompressor =
			FArchiveLoadCompressedProxy(CompressedData, ECompressionFlags::COMPRESS_ZLIB);
		//Decompression Error?
		if (Decompressor.GetError())
		{
			UE_LOG(LogTemp,Warning,TEXT("FArchiveLoadCompressedProxy>> ERROR : File Was Not Compressed "));
			return false;
			//
		}
		//Decompress
		FBufferArchive DecompressedBinaryArray;
		Decompressor << DecompressedBinaryArray;
		//~
		//		  Read the Data Retrieved by GFileManager
		//~
		if (DecompressedBinaryArray.Num() <= 0) return false;
		FMemoryReader FromBinary = FMemoryReader(DecompressedBinaryArray, true); //true, free data after done
		FromBinary.Seek(0);
		SaveLoadPoseData(FromBinary, Pose,MeshId);
		FromBinary.FlushCache();
		// Empty & Close Buffer 
		DecompressedBinaryArray.Empty();
		FromBinary.Close();
		return true;

	}
	
};
