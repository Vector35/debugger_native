/*
Copyright 2020-2024 Vector 35 Inc.

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

#include "debugadaptertype.h"

#ifdef WIN32
	#include "./adapters/dbgengadapter.h"
	#include "./adapters/lldbadapter.h"
#endif

#include "debuggerexceptions.h"
#include "../api/ffi.h"

using namespace BinaryNinjaDebugger;


DebugAdapterType::DebugAdapterType(const std::string& name) : m_name(name)
{
	INIT_DEBUGGER_API_OBJECT();
}


void DebugAdapterType::Register(DebugAdapterType* type)
{
	m_types.push_back(type);
}


DebugAdapterType* DebugAdapterType::GetByName(const std::string& name)
{
	for (DebugAdapterType* adapter : m_types)
	{
		if (adapter->GetName() == name)
			return adapter;
	}
	return nullptr;
}


std::vector<std::string> DebugAdapterType::GetAvailableAdapters(BinaryNinja::BinaryView* data)
{
	std::vector<std::string> result;
	for (DebugAdapterType* adapter : m_types)
	{
		// The adapter must be:
		// 1. valid for the data
		// 2. can connect/execute on the current host system
		if (adapter->IsValidForData(data) && (adapter->CanConnect(data) || adapter->CanExecute(data)))
		{
			result.push_back(adapter->GetName());
		}
	}
	return result;
}


std::string DebugAdapterType::GetBestAdapterForCurrentSystem(BinaryNinja::BinaryView* data)
{
#ifdef WIN32
	return "DBGENG";
#else
	return "LLDB";
#endif
}


Ref<Settings> DebugAdapterType::GetDefaultLaunchSettingsForData(BinaryView* data)
{
	// Create launch Settings and generate default schema
	auto settings = Settings::Instance(GetUniqueIdentifierString());
	settings->RegisterGroup("target", "Target Options");
	settings->RegisterSetting("target.executable_path",
			R"({
			"title" : "Executable Path",
			"type" : "string",
			"default" : "",
			"description" : "Path of the executable to launch.",
			"readOnly" : false,
			})");
	settings->RegisterSetting("target.input_file",
			R"({
			"title" : "Input File",
			"type" : "string",
			"default" : "",
			"description" : "Input file to use to find the base address of the binary view",
			"readOnly" : false,
			})");
	settings->RegisterSetting("target.working_directory",
			R"({
			"title" : "Working Directory",
			"type" : "string",
			"default" : "",
			"description" : "Working directory to launch the target in.",
			"readOnly" : false,
			})");
	settings->RegisterSetting("target.command_line",
			R"({
			"title" : "Command Line Arguments",
			"type" : "string",
			"default" : "",
			"description" : "Command line arguments to pass to the target",
			"readOnly" : false,
			})");

	return settings;
}


Ref<Settings> DebugAdapterType::GetLaunchSettingsForData(BinaryView* data)
{
	return GetDefaultLaunchSettingsForData(data);
}
