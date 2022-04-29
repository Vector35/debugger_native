/*
Copyright 2020-2022 Vector 35 Inc.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include <inttypes.h>
#include "processview.h"
#include "adapters/gdbadapter.h"
#include "adapters/lldbrspadapter.h"
#include "adapters/lldbadapter.h"
#ifdef WIN32
#include "adapters/dbgengadapter.h"
#endif

using namespace BinaryNinja;
using namespace BinaryNinjaDebugger;

void InitDebugAdapterTypes()
{
#ifdef WIN32
    InitDbgEngAdapterType();
#endif

//	Disable these adapters because they are not tested, and will get replaced later
//    InitGdbAdapterType();
//    InitLldbRspAdapterType();
	InitLldbAdapterType();
}


static void RegisterSettings()
{
	Ref<Settings> settings = Settings::Instance();
	settings->RegisterGroup("debugger", "Debugger");
	/*

	Removed blockPython -- we can re-add it once this debugger is enabled by
	default

	Leaving this function for migration of the settings popup.

	*/
}

extern "C"
{
	BN_DECLARE_CORE_ABI_VERSION

	BINARYNINJAPLUGIN void CorePluginDependencies()
	{
		SetCurrentPluginLoadOrder(LatePluginLoadOrder);
	}

#ifdef DEMO_VERSION
	bool DebuggerPluginInit()
#else
	BINARYNINJAPLUGIN bool CorePluginInit()
#endif
	{
		Log(BNLogLevel::DebugLog, "Native debugger loaded!" );
        InitDebugAdapterTypes();
        InitDebugProcessViewType();
		RegisterSettings();
		return true;
	}
}
