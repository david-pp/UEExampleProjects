// Copyright Epic Games, Inc. All Rights Reserved.


#include "HelloProgram.h"

#include "RequiredProgramMainCPPInclude.h"

DEFINE_LOG_CATEGORY_STATIC(LogHelloProgram, Log, All);

IMPLEMENT_APPLICATION(HelloProgram, "HelloProgram");

INT32_MAIN_INT32_ARGC_TCHAR_ARGV()
{
	GEngineLoop.PreInit(ArgC, ArgV);
	UE_LOG(LogHelloProgram, Display, TEXT("Hello World"));
	UE_LOG(LogHelloProgram, Display, TEXT("Hello -> 你好!"));
	FEngineLoop::AppExit();
	return 0;
}
