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

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

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
		bool result = PoseSerializeCompressed(Ar, Pose, Path, meshid);
		PreviousPose = LatestPose;
		LatestPose = Pose;
		PoseSerialize(Fullpose, Pose, meshid);
		return result;
	}
	UFUNCTION(BlueprintCallable, Category = "Experimental")
		void PreparePose(TArray<uint8>& Out , FPoseSnapshot Pose, uint8 MeshId) {
		FBufferArchive Ar = FBufferArchive(false, "temp");
		PoseSerializeCompress(Out, Pose, MeshId);
	}

	UFUNCTION(BlueprintCallable, Category = "Experimental")
		static bool LoadPose(FPoseSnapshot& Pose, FString Path, uint8& meshid) {
		return PoseDeserializeCompressed(Path, Pose, meshid);
	}
	static void PoseSerialize(FBufferArchive& Ar, FPoseSnapshot Pose, uint8 meshid) {
		SaveLoadPoseData(Ar, Pose, meshid);
		//UE_LOG(LogTemp, Log, TEXT("~ PreCompressed Size ~ %i"), Ar.Num());
		TArray<uint8> CompressedData;
		FArchiveSaveCompressedProxy Compressor =
			FArchiveSaveCompressedProxy(CompressedData, ECompressionFlags::COMPRESS_ZLIB);
		//Send entire binary array/archive to compressor
		Compressor << Ar;
		//send archive serialized data to binary array
		Compressor.Flush();
		//UE_LOG(LogTemp, Log, TEXT(" Compressed Size ~ %i"), CompressedData.Num());
	}
	static bool PoseSerializeCompress(TArray<uint8>& Compressed, FPoseSnapshot Pose, uint8 meshid) {
		FBufferArchive Ar = FBufferArchive(false, "temp");
		SaveLoadPoseData(Ar, Pose, meshid);
		FArchiveSaveCompressedProxy Compressor =
			FArchiveSaveCompressedProxy(Compressed, ECompressionFlags::COMPRESS_ZLIB);
		//Send entire binary array/archive to compressor
		Compressor << Ar;
		//send archive serialized data to binary array
		Compressor.Flush();
		//UE_LOG(LogTemp, Log, TEXT(" Compressed Size ~ %i"), Compressed.Num());
		return true;
	}

	/// This is a placeholder - this goes to network as serialized uint8 arry
	static bool SavePoseToDisk(const FString& FullFilePath, const TArray<uint8> CompressedData) {
		FBufferArchive Ar = FBufferArchive(false, "temp");
		if (FFileHelper::SaveArrayToFile(CompressedData, *FullFilePath))
		{
			Ar.FlushCache();
			Ar.Empty();
			return true;
		}
		Ar.FlushCache();
		Ar.Empty();
		return false;
	}
	//Old.
	static bool PoseSerializeCompressed(FBufferArchive& Ar, FPoseSnapshot Pose, FString& FullFilePath, uint8 meshId)
	{
		TArray<uint8> CompressedData;
		if (PoseSerializeCompress(CompressedData, Pose, meshId)) {
			return SavePoseToDisk(FullFilePath, CompressedData);
		}
		return false;
		///

		//SaveLoadPoseData(Ar, Pose, meshId);
		////Pre Compressed Size
		//UE_LOG(LogTemp, Log, TEXT("~ PreCompressed Size ~ %i"), Ar.Num());
		//TArray<uint8> CompressedData;
		//FArchiveSaveCompressedProxy Compressor =
		//	FArchiveSaveCompressedProxy(CompressedData, ECompressionFlags::COMPRESS_ZLIB);
		////Send entire binary array/archive to compressor
		//Compressor << Ar;
		////send archive serialized data to binary array
		//Compressor.Flush();
		////Compressed Size
		//UE_LOG(LogTemp, Log, TEXT(" Compressed Size ~ %i"), CompressedData.Num());

		//if (FFileHelper::SaveArrayToFile(CompressedData, *FullFilePath))
		//{
		//	// Free Binary Array 	
		//	Ar.FlushCache();
		//	Ar.Empty();
		//	return true;
		//}
		//// Free Binary Array 	
		//Ar.FlushCache();
		//Ar.Empty();
		//return false;
	}
	static void SaveLoadPoseData(FArchive& Ar, FPoseSnapshot& Pose, uint8& meshid) {
		//		Ar << uuid; //sender
		//		Ar << guid; //objectid
		Ar << meshid;
		Ar << Pose.LocalTransforms;
	}
	// Replace with ReadNetwork;
	static bool ReadPoseFromDisk(const FString& FullFilePath, TArray<uint8>& CompressedData) {
		if (!FFileHelper::LoadFileToArray(CompressedData, *FullFilePath))
		{
			UE_LOG(LogTemp, Warning, TEXT("FFILEHELPER:>> Invalid File"));
			return false;
		}
		return true;
	}
	static bool PoseDeserializeDecompress(TArray<uint8> CompressedData, FPoseSnapshot& Pose, uint8& MeshId) {
		FArchiveLoadCompressedProxy Decompressor =
			FArchiveLoadCompressedProxy(CompressedData, ECompressionFlags::COMPRESS_ZLIB);
		if (Decompressor.GetError())
		{
			UE_LOG(LogTemp, Warning, TEXT("FArchiveLoadCompressedProxy>> ERROR : File Was Not Compressed "));
			return false;
		}
		FBufferArchive DecompressedBinaryArray;
		Decompressor << DecompressedBinaryArray;
		if (DecompressedBinaryArray.Num() <= 0) return false;
		FMemoryReader FromBinary = FMemoryReader(DecompressedBinaryArray, true); //true, free data after done
		FromBinary.Seek(0);
		SaveLoadPoseData(FromBinary, Pose, MeshId);
		FromBinary.FlushCache();
		DecompressedBinaryArray.Empty();
		FromBinary.Close();
		return true;
	}
	static bool  PoseDeserializeCompressed(const FString& FullFilePath, FPoseSnapshot& Pose, uint8& MeshId) {
		TArray<uint8> CompressedData;;
		if (ReadPoseFromDisk(FullFilePath, CompressedData)) {
			return PoseDeserializeDecompress(CompressedData, Pose, MeshId);
		}
		return false;
		///
		///
		///
		// Decompress File 
		//FArchiveLoadCompressedProxy Decompressor =
		//	FArchiveLoadCompressedProxy(CompressedData, ECompressionFlags::COMPRESS_ZLIB);
		////Decompression Error?
		//if (Decompressor.GetError())
		//{
		//	UE_LOG(LogTemp, Warning, TEXT("FArchiveLoadCompressedProxy>> ERROR : File Was Not Compressed "));
		//	return false;
		//	//
		//}
		////Decompress
		//FBufferArchive DecompressedBinaryArray;
		//Decompressor << DecompressedBinaryArray;
		////~
		////		  Read the Data Retrieved by GFileManager
		////~
		//if (DecompressedBinaryArray.Num() <= 0) return false;
		//FMemoryReader FromBinary = FMemoryReader(DecompressedBinaryArray, true); //true, free data after done
		//FromBinary.Seek(0);
		//SaveLoadPoseData(FromBinary, Pose, MeshId);
		//FromBinary.FlushCache();
		//// Empty & Close Buffer 
		//DecompressedBinaryArray.Empty();
		//FromBinary.Close();
		//return true;

	}

	void Pose_WriteNetwork(const TArray<uint8> CompressedData) {
		UE_LOG(LogTemp, Log, TEXT("[NETWORK] Writing  Compressed Size ~ %i"), CompressedData.Num());
	}
	void Pose_ReadNetwork(const TArray<uint8> CompressedData) {
		PreviousPose = LatestPose;
		PoseDeserializeDecompress(Fullpose, LatestPose, AvatarMeshId);
		UE_LOG(LogTemp, Log, TEXT("[NETWORK] Reading  Compressed Size ~ %i"), CompressedData.Num());

	}


	FBufferArchive Fullpose;
	uint8 AvatarMeshId;
	UFUNCTION(Client, Reliable, Category = "Character Creation")
		void RPC_Client_SendFullpose();
	void RPC_Client_SendFullpose_Implementation();
	UFUNCTION(Category = "Character Creation")
		void Recieve_Fullpose(const TArray<uint8> CompressedData) {
			Pose_ReadNetwork(CompressedData);
	}
	
};
