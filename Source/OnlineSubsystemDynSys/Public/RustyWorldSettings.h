// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/WorldSettings.h"
#include "PoseSnapshot.h"
#include "OSSFunctionLibrary.h"
#include "Networking.h"
#include "Runtime/Core/Public/Serialization/BufferArchive.h"
#include "RustyNetConnection.h"

#include "RustyWorldSettings.generated.h"
DECLARE_DELEGATE(FInvokeProxy);
class URustyNetConnection;
class URustyIpNetDriver;
USTRUCT(BlueprintType)
struct FURLAddress {
	GENERATED_USTRUCT_BODY()
public:
	UPROPERTY(BlueprintReadWrite,VisibleAnywhere)
		FString Host;
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
		int32 Port;
	FURLAddress() {};
	FURLAddress(FString url) {
		TArray<FString> Out;
		url.ParseIntoArray(Out, TEXT(":"), true);
		if (Out.Num() == 2) {
			Host = Out[0];
			Port = FCString::Atoi(*Out[1]);
		}
		else {
			UE_LOG(LogTemp, Warning, TEXT("Invalid formatting: %s"), *url);
		}
	}
};
/**
 * 
 */
UCLASS()
class ONLINESUBSYSTEMDYNSYS_API ARustyWorldSettings : public AWorldSettings, public FNetworkNotify
{
	GENERATED_BODY()

public:
	ARustyWorldSettings(const FObjectInitializer& ObjectInitializer);


	//~ Begin FNetworkNotify Interface

	virtual EAcceptConnection::Type NotifyAcceptingConnection();
	virtual void NotifyAcceptedConnection(class UNetConnection* Connection);
	virtual bool NotifyAcceptingChannel(class UChannel* Channel);
	virtual void NotifyControlMessage(UNetConnection* Connection, uint8 MessageType, class FInBunch& Bunch);
	//~ End FNetworkNotify Interface

	UPROPERTY(EditAnywhere, Category = Network)
		FURLAddress Server;
	UPROPERTY(EditAnywhere, Category = Network)
		FURLAddress Mumble;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Network)
		uint8 LevelId;
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = Network)
		int32 UserId = 0;

	virtual void BeginPlay() override;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = Network)
		TMap<int32, UObject*> ObjectMap;
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = Network)
		int32 CallbacksRegistered;
	UPROPERTY(VisibleAnywhere, Category = Network)
		int32 PropertiesRegistered;
	TMap<int32, UFunction*> CallbackMap;
	TMap<int32, UProperty*> PropertyMap;
	TMap<int32, UObject*> PropContainerMap;
	TMap<int32, FBufferArchive> SerializedProp;
	bool TriggerPropertyReplication(int32 PropId, NetBytes Value) {
		UProperty* Property=PropertyMap.FindRef(PropId);
		UObject* Obj = PropContainerMap.FindRef(PropId);
		bool v=SetPropValue(Value, Property, Obj);
		UFunction* Func = Obj->FindFunction(Property->RepNotifyFunc);
		FInvokeProxy Deleg;
		Deleg.BindUFunction(Obj, Property->RepNotifyFunc);
		Deleg.ExecuteIfBound();
		return v;
	}

	UFUNCTION(BlueprintCallable, Category = Network)
		int32 GetObjectId(UObject* obj) {
			const int32* tempId =ObjectMap.FindKey(obj);
			if (tempId == nullptr) {
				return 0;
			}
			else {
				return *tempId;
			}
		}
	bool RegisterFunctions(AActor* a, TArray<UFunction*> NetFunctions) {
		int32 id=GetObjectId(a);
		TArray<int32> Keys;
		CallbackMap.GetKeys(Keys);
		int32 fid = generateObjectGuid(Keys);
		for (UFunction* func : NetFunctions) {
			const int32* temp=CallbackMap.FindKey(func);
			if (temp == nullptr) {
				CallbackMap.Add(fid, func);
			}
		}
		CallbacksRegistered = CallbackMap.Num();
		return true;
	}
	bool SetPropValue(NetBytes In, UProperty* Property, UObject* Obj) {
		bool Success = false;
		FMemoryReader Ar = FMemoryReader(In, true); //true, free data after done
		Ar.Seek(0);
		if (UFloatProperty* FloatProperty = Cast<UFloatProperty>(Property))
		{
			float Value;// = FloatProperty->GetFloatingPointPropertyValue(Property->ContainerPtrToValuePtr<float>(Obj));
			Ar << Value;
			FloatProperty->SetPropertyValue_InContainer(Obj, Value);
			Success = true;
		}
		else if (UIntProperty* IntProperty = Cast<UIntProperty>(Property))
		{
			int32 Value;// = IntProperty->GetSignedIntPropertyValue(Property->ContainerPtrToValuePtr<int32>(Obj));
			Ar << Value;
			IntProperty->SetPropertyValue_InContainer(Obj, Value);
			Success = true;
		}
		else if (UBoolProperty* BoolProperty = Cast<UBoolProperty>(Property))
		{
			bool Value;// = BoolProperty->GetPropertyValue(Property->ContainerPtrToValuePtr<void>(Obj));
			Ar << Value;
			BoolProperty->SetPropertyValue_InContainer(Obj, Value);
			Success = true;
		}
		else if (UObjectProperty* ObjectProperty = Cast<UObjectProperty>(Property))
		{
			UObject* Value;// = ObjectProperty->GetObjectPropertyValue(Property->ContainerPtrToValuePtr<UObject*>(Obj));
			Ar << Value;
			Success = false;
		}
		else if (UStructProperty* StructProperty = Cast<UStructProperty>(Property)) {

			if (StructProperty->Struct->GetName() == (FNetPoseSnapshot::StaticStruct()->GetName())) {
				FNetPoseSnapshot Value;
				Ar << Value;
				FNetPoseSnapshot* V=StructProperty->ContainerPtrToValuePtr<FNetPoseSnapshot>(Obj);
				V->LocalTransforms = Value.LocalTransforms;
				Success = true;
			}

		}
		else if (UStrProperty* StringProperty = Cast<UStrProperty>(Property))
		{
			FString Value = StringProperty->GetPropertyValue(Property->ContainerPtrToValuePtr<UObject*>(Obj));
			Ar << Value;
			Success = true;
		}


		Ar.FlushCache();
		// Empty & Close Buffer 
		In.Empty();
		Ar.Close();
		return Success;
	}
	bool SerializePropValue(FBufferArchive& Ar, UProperty* Property,UObject* Obj)
	{
		if (UFloatProperty* FloatProperty = Cast<UFloatProperty>(Property))
		{
			float Value = FloatProperty->GetFloatingPointPropertyValue(Property->ContainerPtrToValuePtr<float>(Obj));
			Ar << Value;
			return true;
		}
		else if (UIntProperty* IntProperty = Cast<UIntProperty>(Property))
		{
			int32 Value = IntProperty->GetSignedIntPropertyValue(Property->ContainerPtrToValuePtr<int32>(Obj));
			Ar << Value;
			return true;
		}
		else if (UBoolProperty* BoolProperty = Cast<UBoolProperty>(Property))
		{
			bool Value = BoolProperty->GetPropertyValue(Property->ContainerPtrToValuePtr<void>(Obj));
			Ar << Value;
			return true;
		}
		else if (UObjectProperty* ObjectProperty = Cast<UObjectProperty>(Property))
		{
			UObject* Value = ObjectProperty->GetObjectPropertyValue(Property->ContainerPtrToValuePtr<UObject*>(Obj));
			//Ar << Value;
			return false;
		}
		else if (UStructProperty* StructProperty = Cast<UStructProperty>(Property)) {
				
				if (StructProperty->Struct->GetName()==(FNetPoseSnapshot::StaticStruct()->GetName()) ){

					void * StructAddress = StructProperty->ContainerPtrToValuePtr<void>(Obj);
					UScriptStruct * ScriptStruct = StructProperty->Struct;
					FNetPoseSnapshot* pose= Property->ContainerPtrToValuePtr<FNetPoseSnapshot>(Obj);
					bool success;
					pose->NetSerialize(Ar, nullptr, success);
					/*StructProperty->Struct->SerializeSuperStruct(Ar);
					FNetSnapshotPose* Pose = StructProperty->ContainerPtrToValuePtr<FNetSnapshotPose>(Obj);
					UOSSFunctionLibrary::PreparePose(Ar, *Pose, 0);*/
					return success;
				}
				
		}
		else if (UStrProperty* StringProperty = Cast<UStrProperty>(Property))
		{
			FString Value = StringProperty->GetPropertyValue(Property->ContainerPtrToValuePtr<UObject*>(Obj));
			Ar << Value;
			return true;
		}
		return false;
	}
	void OnActorChannelOpen(FInBunch & InBunch,
		UNetConnection * Connection) override;

	UNetConnection* GetConnection();
	
	URustyNetConnection* NetConnection;

	URustyIpNetDriver* NetDriver;
	void Setup();
	void CheckProperties();
	bool PropChanged(int32 PropId,FBufferArchive& bytes) {
		UProperty* Property = PropertyMap.FindRef(PropId);
		UObject* Obj = PropContainerMap.FindRef(PropId);
		if (SerializePropValue(bytes, Property, Obj)) {
			FBufferArchive Ar = *SerializedProp.Find(PropId);
			return bytes == Ar;
		}
		else {
			return false;
		}
	}
	bool RegisterProperties(AActor* a, TArray<UProperty*> NetProperties) {
		int32 id = GetObjectId(a);
		TArray<int32> Keys;
		PropertyMap.GetKeys(Keys);
		int32 fid = generateObjectGuid(Keys);
		for (UProperty* prop : NetProperties) {
			const int32* temp = PropertyMap.FindKey(prop);
			if (temp == nullptr) {
				PropertyMap.Add(fid, prop);
				temp = &fid;
			
			}
			FBufferArchive bytes;
			if (SerializePropValue(bytes, prop, a)) {
				SerializedProp.Add(*temp, bytes);
			}
			PropContainerMap.Add(*temp, a);
			
		}
		PropertiesRegistered = PropertyMap.Num();
		
		return true;
	}
	
	bool RegisterObject(UObject* obj, int32& Id) {
			bool toAdd = true;
			TArray<uint32> ToRemove;
			for (auto pair : ObjectMap) {
				if (pair.Value==nullptr) {
					ToRemove.Push(pair.Key);
				}
				else {
					if (pair.Value == obj) {
						toAdd = false;
						Id = pair.Key;
					}
				}
			}
			for (auto i : ToRemove) {
				ObjectMap.Remove(i);
			}
			if (toAdd) {
				TArray<int32> Keys;
				ObjectMap.GetKeys(Keys);
				int32 newId= generateObjectGuid(Keys);
				ObjectMap.Add(newId, obj);
				Id = newId;
			}
			for (auto pair : ObjectMap) {
				if (pair.Value == nullptr) {
					ToRemove.Push(pair.Key);
				}
			}
			return toAdd;
	}
	int32 generateObjectGuid(TArray<int32> Keys) {
		int32 Min = MIN_uint32;//TNumericLimits<int32>::Min();
		int32 Max = MAX_int32; TNumericLimits<int32>::Max();
		
		int64 secs = FDateTime::Now().GetTicks();
		int32 seed = (int32)(((int64)secs) % MAX_int32);
		FMath::RandInit(seed++);
		FNetworkGUID nguid = FNetworkGUID((uint32)FMath::Rand());
		while (Keys.Contains((int32)nguid.Value)) {
			nguid = FNetworkGUID(nguid.Value+1);
		}
		return (int32)nguid.Value;
	}
		
};
