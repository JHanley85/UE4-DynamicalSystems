// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/NetworkGuid.h"
#include "Runtime/Engine/Classes/Kismet/GameplayStatics.h"

#include "Runtime/Core/Public/Serialization/Archive.h"
#include "Runtime/Core/Public/Serialization/BufferArchive.h"
#include "Runtime/Core/Public/Serialization/MemoryReader.h"
#include "Runtime/Core/Public/Serialization/ArchiveSaveCompressedProxy.h"
#include "Runtime/Core/Public/Serialization/ArchiveLoadCompressedProxy.h"
#include "DelegateInstanceInterface.h"
#include "DelegateInstancesImpl.h"

#include "OnlineSessionInterfaceDynSys.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "NetClient.h"
#include "PoseSnapshot.h"
#include "Delegate.h"
#include "OSSFunctionLibrary.generated.h"
#define PORT_COUNT 3

DECLARE_DYNAMIC_DELEGATE_OneParam(FSnapshotDelegate, FPoseSnapshot, snapshot);
/**
 * 
 */
UCLASS()
class ONLINESUBSYSTEMDYNSYS_API UOSSFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UOSSFunctionLibrary() {
		ANetClient::OnByteArray.BindStatic(UOSSFunctionLibrary::RouteArchive);
	}
	static void RouteArchive(TArray<uint8> Data, int32 Guid) {
		UE_LOG(LogTemp, VeryVerbose, TEXT("Recd: data %i for %i"), Data.Num(), Guid);
		FPoseSnapshot Pose;
		uint8 MeshId;
		PoseDeserializeDecompress(Data, Pose, MeshId);
		if (!_CallbackList.Contains(Guid)) {
			UE_LOG(LogTemp, VeryVerbose, TEXT("Failed Archive To"));
		}
		else {
			_CallbackList.Find(Guid)->Execute(Pose);
		}
		//UE_LOG(LogTemp, Log, TEXT("Routing Archive To:"));
	}
#pragma region PoseSerialization
	static bool PoseDeserializeDecompress(TArray<uint8> CompressedData, FPoseSnapshot& Pose, uint8& MeshId) {
		FArchiveLoadCompressedProxy Decompressor =
			FArchiveLoadCompressedProxy(CompressedData, ECompressionFlags::COMPRESS_ZLIB);
		if (Decompressor.GetError())
		{
			UE_LOG(LogTemp, VeryVerbose, TEXT("FArchiveLoadCompressedProxy>> ERROR : File Was Not Compressed "));
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
	
	static void SaveLoadPoseData(FArchive& Ar, FPoseSnapshot& Pose, uint8& meshid) {
		//		Ar << uuid; //sender
		//		Ar << guid; //objectid
		Ar << meshid;
		Ar << Pose.LocalTransforms;
	}
	static bool PoseSerializeCompress(TArray<uint8>& Compressed, FPoseSnapshot Pose, uint8 meshid) {
		FBufferArchive Ar = FBufferArchive();
		SaveLoadPoseData(Ar, Pose, meshid);
		FArchiveSaveCompressedProxy Compressor =
			FArchiveSaveCompressedProxy(Compressed, ECompressionFlags::COMPRESS_ZLIB);
		//Send entire binary array/archive to compressor
		Compressor << Ar;
		//send archive serialized data to binary array
		Compressor.Flush();
		UE_LOG(LogTemp, VeryVerbose, TEXT(" Compressed Size ~ %i"), Compressed.Num());
		UE_LOG(LogTemp, VeryVerbose, TEXT(" Uncompressed Size ~ %i"), Ar.Num());
		return true;
	}
	static void PreparePose(TArray<uint8>& Out, FPoseSnapshot Pose, uint8 MeshId) {
		FBufferArchive Ar = FBufferArchive();
		PoseSerializeCompress(Out, Pose, MeshId);
	}

	//static FSnapshotDelegate delegateTest;
	UFUNCTION(BlueprintCallable, Category = OSS)
		static void NET_SendSnapshot(FPoseSnapshot Pose, uint8 MeshId, ANetClient* client, const FSnapshotDelegate& Callback) {
		TArray<uint8> Payload;
		PreparePose(Payload, Pose, MeshId);
		const UObject* Sender=Callback.GetUObject();
		FSnapshotDelegate cb = FSnapshotDelegate(Callback);
		FName boundFunc = cb.GetFunctionName();
		//FDelegateBase del =(FDelegateBase) Callback;
		//FName boundFunc = Callback.TryGetBoundFunctionName();

		NET_SendArchive(client, Sender, Payload, boundFunc);
	}
	static TMap<int32, FSnapshotDelegate> _CallbackList;
#pragma endregion PoseSerialization
	UFUNCTION(BlueprintCallable,Category="OSS")
		static void NET_SendArchive(ANetClient* client,const UObject* Sender, TArray<uint8> Payload, FName repl_func) {
		int32 out=0;
		GetOSSUUID(Sender, repl_func, out);


		uint8 _session; uint8  _class; uint8  _object; uint8  _property;
		FOSSGuID::IntUnpack(out, _session, _class, _object, _property);

		FSnapshotDelegate cb = FSnapshotDelegate();
		cb.BindUFunction((UObject*)Sender, repl_func);
		_CallbackList.Add(out, cb);
		UE_LOG(LogTemp, VeryVerbose, TEXT("object: %s Bound function: %s"), *Sender->GetName(), *repl_func.ToString());
		UE_LOG(LogTemp, VeryVerbose, TEXT("guid=%i =  %i:%i:%i:%i"), out, _session, _class, _object, _property);
		
		client->SendByteArray(Payload, out);
	}
	UFUNCTION(BlueprintCallable, Category = "OSS")
		static void GetOSSUUID(const UObject* obj, FName funcName, int32& outid) {
		uint8 _session = -1;
		uint8 _class = -1;
		uint8 _object = -1;
		uint8 _property = -1;
		uint8 instId = -1;

		FString oN = UKismetSystemLibrary::GetObjectName(obj);
		FString oDn = UKismetSystemLibrary::GetDisplayName(obj);
		oN.RemoveFromStart(oDn, ESearchCase::CaseSensitive);
		oN.RemoveFromStart("_", ESearchCase::CaseSensitive);
		_object = FCString::Atoi(*oN);

		TArray<FLifetimeProperty> OutLifetimeProps;
		obj->GetLifetimeReplicatedProps(OutLifetimeProps);
		for (TFieldIterator<UFunction> It(obj->GetClass()); It; ++It)
		{
			UFunction* Function = *It;
				if (Function->GetName() == funcName.ToString()) {
					_property= Function->RPCId;
				}
		}
		/*for (TFieldIterator<UProperty> PropIt(obj->GetClass()); PropIt; ++PropIt)
		{

			UProperty* Property = *PropIt;
			if (Property->GetNameCPP() == funcName.ToString()) {
				_property = Property->RepIndex;
				break;
			}
		}*/
		RegisterClassNetId(obj->GetClass()->GetName(), 4, _class);
		//get session
		uint8 session = 0;
		 outid= PackOSSGuid(0, _class, _object, _property);

		 UE_LOG(LogTemp, VeryVerbose, TEXT("%i:%i:%i:%i"), _session, _class, _object, _property);
		 return;
		FString foundClass;
		if (IdToClass(_class)) {
			foundClass = IdToClass(_class)->GetName();
		}
	}
	UFUNCTION(BlueprintCallable)
		static void TriggerOnRep(UObject* context,int32 In) {
		TArray<AActor*> FoundActors;
		
		uint8 _session; uint8  _class; uint8  _object; uint8  _property;
		FOSSGuID::IntUnpack(In, _session, _class, _object, _property);
		UClass* kls =  IdToClass(_class);
		if (kls == nullptr) {
			UE_LOG(LogTemp, VeryVerbose, TEXT("Class not found ID: %i %s"), _class, *_ClassMap.FindRef(_class));
			return;
		}
		UGameplayStatics::GetAllActorsOfClass(context, (TSubclassOf<AActor>)kls, FoundActors);
		for (auto c : FoundActors) {
			UE_LOG(LogTemp, VeryVerbose, TEXT("Found %s"), *c->GetName());
		}
	}
	UFUNCTION(BlueprintCallable, Category = OSS)
		static int32 PackOSSGuid(const uint8 _session, const uint8 _class, const uint8 _object, const uint8 _property) {
		return FOSSGuID::IntPack(_session, _class, _object, _property);
	}
	UFUNCTION(BlueprintCallable, Category = OSS)
		static void UnpackOSSGuid(int32 guid, uint8& _session, uint8& _class, uint8& _object, uint8& _property) {
		FOSSGuID::IntUnpack(guid, _session, _class, _object, _property);
	}
public:
	static IOnlineSessionPtr GetSession() {

		IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
		IOnlineSessionPtr Sessions = NULL;
		Sessions = OnlineSub->GetSessionInterface();
		return Sessions;
	}
	static void AddObjectToSession(int32 id, UObject* listener) {
		auto settings=GetSession()->GetSessionSettings("default");/*
		auto setting = FOnlineSessionSetting(listener, EOnlineDataAdvertisementType::DontAdvertise, id);

		settings->Settings.Add(*FString::Printf(TEXT("%i"),id), setting);*/
		//settings->Set(FName(*listener->GetName()), TWeakObjectPtr<UObject>(listener), EOnlineDataAdvertisementType::DontAdvertise,id);
		//GetSession()->UpdateSession(FName("default"), *settings, false);
	}
	UFUNCTION(BlueprintCallable)
		static void CreateSessionManual(FName SessionName) {
		IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
		IOnlineSessionPtr Sessions = NULL;
		Sessions = OnlineSub->GetSessionInterface();
		FOnlineSessionSettings NewSessionSettings;
		NewSessionSettings.bShouldAdvertise = true;
		NewSessionSettings.bAllowInvites = true;
		NewSessionSettings.bAllowJoinInProgress = true;
		NewSessionSettings.bUsesStats = true;
		NewSessionSettings.NumPublicConnections = 460;
		Sessions->CreateSession(0, SessionName, NewSessionSettings);
		//FBlueprintSessionResult
		FNamedOnlineSession* session=Sessions->GetNamedSession(SessionName);
		//return session;
	}


	///callback references
	static TMap<int32, TWeakObjectPtr<UFunction>> _CallbackMap;

	static void RegisterCallback(UObject* object, FName replFunc) {

	}
	static void DeregisterCallback() {

	}
	///Object Registration
	static TMap<uint8, TWeakObjectPtr<UObject>> _ObjectMap;
	static void RegisterObject() {

	}
	static void DeregisterObject() {

	}
	static bool IdToObject(uint8 id, UObject*& Out ) {
		/*Out = nullptr;
		if (_ObjectMap.Contains(id)) {
			TWeakObjectPtr<UObject>* ptr = _ObjectMap.Find(id);
			if (ptr->IsValid()){
				Out = ptr->Get();
				return true;
			}
			
		}*/
		return false;
	}
	static bool ObjectToId(uint8& id, UObject* _ObjectMap) {
		return false;
	}
	/// Class Registration
	static TMap<uint8,FString > _ClassMap;
	UFUNCTION(BlueprintCallable, Category = OSS)
	static UClass* IdToClass(uint8 id) {
		if (_ClassMap.Find(id)) {
			return NameToClass(_ClassMap.FindRef(id));
		}
		else {
			return nullptr;
		}
	}
	UFUNCTION(BlueprintCallable, Category = OSS)
	static UClass*  NameToClass(FString ClassName) {
		if (UClass* Result = FindObject<UClass>((UObject*)ANY_PACKAGE, *ClassName,false))
			return Result;
		return nullptr;
	}
	UFUNCTION(BlueprintCallable, Category = OSS)
	static void RegisterClassNetId(FString ClassName, uint8 _inId, uint8& _outId) {
		auto tempId = _ClassMap.FindKey(ClassName);
		if (tempId) {
			_outId = *tempId;
			return;
		}
		TArray<uint8> keys;
		_ClassMap.GetKeys(keys);
		_outId = _inId;
		if (keys.Contains(_outId)) {
			if (!FindUniqueIndex(_outId))
				return;
		}
		 _ClassMap.Add(_outId, ClassName);
	}
	static bool FindUniqueIndex(uint8& out) {
		TArray<uint8> keys;
		_ClassMap.GetKeys(keys);
	    uint8* key=nullptr;
		for (int8 i = 1; i < 255; i++) {
			if (!keys.Contains(i)) {
				out = i;
				return true;
			}
		}
		out = -1;
		return false;
	}

#pragma region Server
	UFUNCTION(BlueprintCallable, Category="OSS|Server")
	static bool SendRPC(int32 To, FName Func) {
		return false;
	}
	UFUNCTION(BlueprintCallable, Category="OSS|Server")
	static bool GetServerChannelPort(const TMap<FName, uint8> LevelMap, FName Level, uint8& Port) {
		if (!LevelMap.Contains(Level)) {
			Port = 0;
			return false;
		}
		else {
			uint8 id = *LevelMap.Find(Level);
			Port = (*LevelMap.Find(Level) > PORT_COUNT) ? 0 : *LevelMap.Find(Level);
			return true;
		}
	}
#pragma endregion Server
};


