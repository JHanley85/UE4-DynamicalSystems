// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "OnlineAsyncTaskManagerDynSys.h"

void FOnlineAsyncTaskManagerDynSys::OnlineTick()
{
	check(DynSysSubsystem);
	check(FPlatformTLS::GetCurrentThreadId() == OnlineThreadId || !FPlatformProcess::SupportsMultithreading());
}

