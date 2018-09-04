// Fill out your copyright notice in the Description page of Project Settings.
#include "OSSFunctionLibrary.h"

#include "NetClient.h"



TMap<uint8,FString> UOSSFunctionLibrary::_ClassMap = TMap< uint8, FString>();
///callback references
TMap<int32, TWeakObjectPtr<UFunction>> UOSSFunctionLibrary::_CallbackMap= TMap<int32, TWeakObjectPtr<UFunction>>() ;
///Object Registration
TMap<uint8, TWeakObjectPtr<UObject>> UOSSFunctionLibrary::_ObjectMap = TMap<uint8, TWeakObjectPtr<UObject>>();
TMap<int32, FSnapshotDelegate> UOSSFunctionLibrary::_CallbackList= TMap<int32, FSnapshotDelegate>();
//ANetClient::OnByteArray.BindStatic(UOSSFunctionLibrary::RouteArchive);
