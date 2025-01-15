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

#include "binaryninjaapi.h"
#include "lowlevelilinstruction.h"
#include "mediumlevelilinstruction.h"
#include "highlevelilinstruction.h"
#include "debuggercontroller.h"
#include "debuggercommon.h"
#include "../api/ffi.h"

using namespace BinaryNinjaDebugger;


char* BNDebuggerAllocString(const char* contents)
{
	return BNAllocString(contents);
}


void BNDebuggerFreeString(char* str)
{
	BNFreeString(str);
}


char** BNDebuggerAllocStringList(const char** contents, size_t size)
{
	return BNAllocStringList(contents, size);
}


void BNDebuggerFreeStringList(char** strs, size_t count)
{
	BNFreeStringList(strs, count);
}


BNDebuggerController* BNGetDebuggerController(BNBinaryView* data)
{
	if (!data)
		return nullptr;

	Ref<BinaryView> view = new BinaryView(BNNewViewReference(data));
	DebuggerController* controller = DebuggerController::GetController(view);
	if (!controller)
		return nullptr;

	return DBG_API_OBJECT_REF(controller);
}


void BNDebuggerDestroyController(BNDebuggerController* controller)
{
	controller->object->Destroy();
}


bool BNDebuggerControllerExists(BNBinaryView* data)
{
	if (!data)
		return false;

	Ref<BinaryView> view = new BinaryView(BNNewViewReference(data));
	return DebuggerController::ControllerExists(view);
}


BNDebuggerController* BNGetDebuggerControllerFromFile(BNFileMetadata* file)
{
	if (!file)
		return nullptr;

	Ref<FileMetadata> fileObject = new FileMetadata(BNNewFileReference(file));
	DebuggerController* controller = DebuggerController::GetController(fileObject);
	if (!controller)
		return nullptr;

	return DBG_API_OBJECT_REF(controller);
}


bool BNDebuggerControllerExistsFromFile(BNFileMetadata* file)
{
	if (!file)
		return false;

	Ref<FileMetadata> fileObject = new FileMetadata(BNNewFileReference(file));
	return DebuggerController::ControllerExists(fileObject);
}


BNBinaryView* BNDebuggerGetData(BNDebuggerController* controller)
{
	BinaryViewRef result = controller->object->GetData();
	if (result)
		return BNNewViewReference(result->GetObject());
	return nullptr;
}


void BNDebuggerSetData(BNDebuggerController* controller, BNBinaryView* data)
{
	Ref<BinaryView> view = new BinaryView(BNNewViewReference(data));
	controller->object->SetData(view);
}


BNArchitecture* BNDebuggerGetRemoteArchitecture(BNDebuggerController* controller)
{
	ArchitectureRef result = controller->object->GetRemoteArchitecture();
	if (result)
		return result->GetObject();
	return nullptr;
}


bool BNDebuggerIsConnected(BNDebuggerController* controller)
{
	return controller->object->GetState()->IsConnected();
}


bool BNDebuggerIsConnectedToDebugServer(BNDebuggerController* controller)
{
	return controller->object->GetState()->IsConnectedToDebugServer();
}


bool BNDebuggerIsRunning(BNDebuggerController* controller)
{
	return controller->object->GetState()->IsRunning();
}


BNDebuggerController* BNDebuggerNewControllerReference(BNDebuggerController* controller)
{
	return DBG_API_OBJECT_NEW_REF(controller);
}


void BNDebuggerFreeController(BNDebuggerController* view)
{
	DBG_API_OBJECT_FREE(view);
}


uint64_t BNDebuggerGetStackPointer(BNDebuggerController* controller)
{
	return controller->object->GetState()->StackPointer();
}


BNDataBuffer* BNDebuggerReadMemory(BNDebuggerController* controller, uint64_t address, size_t size)
{
	DataBuffer* data = new DataBuffer(controller->object->ReadMemory(address, size));
	return data->GetBufferObject();
}


bool BNDebuggerWriteMemory(BNDebuggerController* controller, uint64_t address, BNDataBuffer* buffer)
{
	// Hacky way of getting a BinaryNinj::DataBuffer out of a BNDataBuffer, without causing a segfault
	DataBuffer buf;
	BNAppendDataBuffer(buf.GetBufferObject(), buffer);
	return controller->object->WriteMemory(address, buf);
}


BNDebugProcess* BNDebuggerGetProcessList(BNDebuggerController* controller, size_t* size)
{
	std::vector<DebugProcess> processes = controller->object->GetProcessList();

	*size = processes.size();
	BNDebugProcess* results = new BNDebugProcess[processes.size()];

	for (size_t i = 0; i < processes.size(); i++)
	{
		results[i].m_pid = processes[i].m_pid;
		results[i].m_processName = BNDebuggerAllocString(processes[i].m_processName.c_str());
	}

	return results;
}


void BNDebuggerFreeProcessList(BNDebugProcess* processes, size_t count)
{
	for (size_t i = 0; i < count; i++)
	{
		BNDebuggerFreeString(processes[i].m_processName);
	}

	delete[] processes;
}


BNDebugThread* BNDebuggerGetThreads(BNDebuggerController* controller, size_t* size)
{
	std::vector<DebugThread> threads = controller->object->GetAllThreads();

	*size = threads.size();
	BNDebugThread* results = new BNDebugThread[threads.size()];

	for (size_t i = 0; i < threads.size(); i++)
	{
		results[i].m_tid = threads[i].m_tid;
		results[i].m_rip = threads[i].m_rip;
		results[i].m_isFrozen = threads[i].m_isFrozen;
	}

	return results;
}


void BNDebuggerFreeThreads(BNDebugThread* threads, size_t count)
{
	delete[] threads;
}


BNDebugThread BNDebuggerGetActiveThread(BNDebuggerController* controller)
{
	DebugThread thread = controller->object->GetActiveThread();
	BNDebugThread result;
	result.m_tid = thread.m_tid;
	result.m_rip = thread.m_rip;
	return result;
}


void BNDebuggerSetActiveThread(BNDebuggerController* controller, BNDebugThread thread)
{
	DebugThread activeThread;
	activeThread.m_rip = thread.m_rip;
	activeThread.m_tid = thread.m_tid;

	controller->object->SetActiveThread(activeThread);
}


bool BNDebuggerSuspendThread(BNDebuggerController* controller, uint32_t tid)
{
	return controller->object->SuspendThread(tid);
}


bool BNDebuggerResumeThread(BNDebuggerController* controller, uint32_t tid)
{
	return controller->object->ResumeThread(tid);
}


BNDebugFrame* BNDebuggerGetFramesOfThread(BNDebuggerController* controller, uint32_t tid, size_t* count)
{
	std::vector<DebugFrame> frames = controller->object->GetFramesOfThread(tid);
	*count = frames.size();

	BNDebugFrame* results = new BNDebugFrame[frames.size()];

	for (size_t i = 0; i < frames.size(); i++)
	{
		results[i].m_index = frames[i].m_index;
		results[i].m_pc = frames[i].m_pc;
		results[i].m_sp = frames[i].m_sp;
		results[i].m_fp = frames[i].m_fp;
		results[i].m_functionName = BNDebuggerAllocString(frames[i].m_functionName.c_str());
		results[i].m_functionStart = frames[i].m_functionStart;
		results[i].m_module = BNDebuggerAllocString(frames[i].m_module.c_str());
	}

	return results;
}


void BNDebuggerFreeFrames(BNDebugFrame* frames, size_t count)
{
	for (size_t i = 0; i < count; i++)
	{
		BNDebuggerFreeString(frames[i].m_functionName);
		BNDebuggerFreeString(frames[i].m_module);
	}

	delete[] frames;
}


BNDebugModule* BNDebuggerGetModules(BNDebuggerController* controller, size_t* size)
{
	std::vector<DebugModule> modules = controller->object->GetAllModules();

	*size = modules.size();
	BNDebugModule* results = new BNDebugModule[modules.size()];

	for (size_t i = 0; i < modules.size(); i++)
	{
		results[i].m_address = modules[i].m_address;
		results[i].m_name = BNDebuggerAllocString(modules[i].m_name.c_str());
		results[i].m_short_name = BNDebuggerAllocString(modules[i].m_short_name.c_str());
		results[i].m_size = modules[i].m_size;
		results[i].m_loaded = modules[i].m_loaded;
	}

	return results;
}


void BNDebuggerFreeModules(BNDebugModule* modules, size_t count)
{
	for (size_t i = 0; i < count; i++)
	{
		BNDebuggerFreeString(modules[i].m_name);
		BNDebuggerFreeString(modules[i].m_short_name);
	}
	delete[] modules;
}


BNDebugRegister* BNDebuggerGetRegisters(BNDebuggerController* controller, size_t* size)
{
	std::vector<DebugRegister> registers = controller->object->GetAllRegisters();

	*size = registers.size();
	BNDebugRegister* results = new BNDebugRegister[registers.size()];

	for (size_t i = 0; i < registers.size(); i++)
	{
		results[i].m_name = BNDebuggerAllocString(registers[i].m_name.c_str());
		results[i].m_value = registers[i].m_value;
		results[i].m_width = registers[i].m_width;
		results[i].m_registerIndex = registers[i].m_registerIndex;
		results[i].m_hint = BNDebuggerAllocString(registers[i].m_hint.c_str());
	}

	return results;
}


void BNDebuggerFreeRegisters(BNDebugRegister* registers, size_t count)
{
	for (size_t i = 0; i < count; i++)
	{
		BNDebuggerFreeString(registers[i].m_name);
		BNDebuggerFreeString(registers[i].m_hint);
	}
	delete[] registers;
}


bool BNDebuggerSetRegisterValue(BNDebuggerController* controller, const char* name, uint64_t value)
{
	return controller->object->SetRegisterValue(std::string(name), value);
}


uint64_t BNDebuggerGetRegisterValue(BNDebuggerController* controller, const char* name)
{
	return controller->object->GetRegisterValue(std::string(name));
}


// target control
bool BNDebuggerLaunch(BNDebuggerController* controller)
{
	return controller->object->Launch();
}


BNDebugStopReason BNDebuggerLaunchAndWait(BNDebuggerController* controller)
{
	return controller->object->LaunchAndWait();
}


bool BNDebuggerExecute(BNDebuggerController* controller)
{
	return controller->object->Execute();
}


// TODO: Maybe this should return bool?
void BNDebuggerRestart(BNDebuggerController* controller)
{
	controller->object->Restart();
}


void BNDebuggerQuit(BNDebuggerController* controller)
{
	controller->object->Quit();
}


void BNDebuggerQuitAndWait(BNDebuggerController* controller)
{
	controller->object->QuitAndWait();
}


bool BNDebuggerConnect(BNDebuggerController* controller)
{
	return controller->object->Connect();
}


BNDebugStopReason BNDebuggerConnectAndWait(BNDebuggerController* controller)
{
	return controller->object->ConnectAndWait();
}


bool BNDebuggerConnectToDebugServer(BNDebuggerController* controller)
{
	return controller->object->ConnectToDebugServer();
}


bool BNDebuggerDisconnectDebugServer(BNDebuggerController* controller)
{
	return controller->object->DisconnectDebugServer();
}


void BNDebuggerDetach(BNDebuggerController* controller)
{
	controller->object->Detach();
}


void BNDebuggerPause(BNDebuggerController* controller)
{
	controller->object->Pause();
}


// Convenience function, either launch the target process or connect to a remote, depending on the selected adapter
void BNDebuggerLaunchOrConnect(BNDebuggerController* controller)
{
	controller->object->LaunchOrConnect();
}


bool BNDebuggerAttach(BNDebuggerController* controller)
{
	return controller->object->Attach();
}


BNDebugStopReason BNDebuggerAttachAndWait(BNDebuggerController* controller)
{
	return controller->object->AttachAndWait();
}


bool BNDebuggerGo(BNDebuggerController* controller)
{
	return controller->object->Go();
}


bool BNDebuggerGoReverse(BNDebuggerController* controller)
{
	return controller->object->GoReverse();
}


bool BNDebuggerStepInto(BNDebuggerController* controller, BNFunctionGraphType il)
{
	return controller->object->StepInto(il);
}

bool BNDebuggerStepIntoReverse(BNDebuggerController* controller, BNFunctionGraphType il)
 {
 	return controller->object->StepIntoReverse(il);
 }


bool BNDebuggerStepOver(BNDebuggerController* controller, BNFunctionGraphType il)
{
	return controller->object->StepOver(il);
}


bool BNDebuggerStepOverReverse(BNDebuggerController* controller, BNFunctionGraphType il)
{
	return controller->object->StepOverReverse(il);
}


bool BNDebuggerStepReturn(BNDebuggerController* controller)
{
	return controller->object->StepReturn();
}


bool BNDebuggerStepReturnReverse(BNDebuggerController* controller)
{
	return controller->object->StepReturnReverse();
}


bool BNDebuggerRunTo(BNDebuggerController* controller, const uint64_t* remoteAddresses, size_t count)
{
	std::vector<uint64_t> addresses;
	addresses.reserve(count);
	for (size_t i = 0; i < count; i++)
	{
		addresses.push_back(remoteAddresses[i]);
	}
	return controller->object->RunTo(addresses);
}


BNDebugStopReason BNDebuggerGoAndWait(BNDebuggerController* controller)
{
	return controller->object->GoAndWait();
}


BNDebugStopReason BNDebuggerGoReverseAndWait(BNDebuggerController* controller)
{
	return controller->object->GoReverseAndWait();
}


BNDebugStopReason BNDebuggerStepIntoAndWait(BNDebuggerController* controller, BNFunctionGraphType il)
{
	return controller->object->StepIntoAndWait(il);
}


BNDebugStopReason BNDebuggerStepIntoReverseAndWait(BNDebuggerController* controller, BNFunctionGraphType il)
{
	return controller->object->StepIntoReverseAndWait(il);
}


BNDebugStopReason BNDebuggerStepOverAndWait(BNDebuggerController* controller, BNFunctionGraphType il)
{
	return controller->object->StepOverAndWait(il);
}

BNDebugStopReason BNDebuggerStepOverReverseAndWait(BNDebuggerController* controller, BNFunctionGraphType il)
{
	return controller->object->StepOverReverseAndWait(il);
}


BNDebugStopReason BNDebuggerStepReturnAndWait(BNDebuggerController* controller)
{
	return controller->object->StepReturnAndWait();
}


BNDebugStopReason BNDebuggerStepReturnReverseAndWait(BNDebuggerController* controller)
{
	return controller->object->StepReturnReverseAndWait();
}


BNDebugStopReason BNDebuggerRunToAndWait(
	BNDebuggerController* controller, const uint64_t* remoteAddresses, size_t count)
{
	std::vector<uint64_t> addresses;
	addresses.reserve(count);
	for (size_t i = 0; i < count; i++)
	{
		addresses.push_back(remoteAddresses[i]);
	}
	return controller->object->RunToAndWait(addresses);
}


DebugStopReason BNDebuggerPauseAndWait(BNDebuggerController* controller)
{
	return controller->object->PauseAndWait();
}


DebugStopReason BNDebuggerRestartAndWait(BNDebuggerController* controller)
{
	return controller->object->RestartAndWait();
}


char* BNDebuggerGetAdapterType(BNDebuggerController* controller)
{
	if (!controller->object->GetState())
		return nullptr;

	return BNDebuggerAllocString(controller->object->GetState()->GetAdapterType().c_str());
}


void BNDebuggerSetAdapterType(BNDebuggerController* controller, const char* adapter)
{
	controller->object->GetState()->SetAdapterType(adapter);
}


BNDebugAdapterType* BNGetDebugAdapterTypeByName(const char* name)
{
	DebugAdapterType* type = DebugAdapterType::GetByName(name);
 	if (!type)
		return nullptr;

	return type->GetAPIObject();
}


bool BNDebugAdapterTypeCanExecute(BNDebugAdapterType* adapter, BNBinaryView* data)
{
	Ref<BinaryView> view = new BinaryView(BNNewViewReference(data));
	return adapter->object->CanExecute(view);
}


bool BNDebugAdapterTypeCanConnect(BNDebugAdapterType* adapter, BNBinaryView* data)
{
	Ref<BinaryView> view = new BinaryView(BNNewViewReference(data));
	return adapter->object->CanConnect(view);
}


BNSettings* BNDebugAdapterGetLaunchSettingsForData(BNDebugAdapterType* adapter, BNBinaryView* data)
{
	Ref<BinaryView> view = new BinaryView(BNNewViewReference(data));
	auto settings = adapter->object->GetLaunchSettingsForData(view);
	if (!settings)
		return nullptr;
	return BNNewSettingsReference(settings->GetObject());
}


BNSettings* BNDebugAdapterGetDefaultLaunchSettingsForData(BNDebugAdapterType* adapter, BNBinaryView* data)
{
	Ref<BinaryView> view = new BinaryView(BNNewViewReference(data));
	auto settings = adapter->object->GetDefaultLaunchSettingsForData(view);
	if (!settings)
		return nullptr;
	return BNNewSettingsReference(settings->GetObject());
}


BNDebugAdapterConnectionStatus BNDebuggerGetConnectionStatus(BNDebuggerController* controller)
{
	return controller->object->GetConnectionStatus();
}


BNDebugAdapterTargetStatus BNDebuggerGetTargetStatus(BNDebuggerController* controller)
{
	return controller->object->GetExecutionStatus();
}


char** BNGetAvailableDebugAdapterTypes(BNBinaryView* data, size_t* count)
{
	Ref<BinaryView> view = new BinaryView(BNNewViewReference(data));
	std::vector<std::string> adapters = DebugAdapterType::GetAvailableAdapters(view);
	*count = adapters.size();

	std::vector<const char*> cstrings;
	cstrings.reserve(adapters.size());
	for (auto& str : adapters)
	{
		cstrings.push_back(str.c_str());
	}
	*count = adapters.size();
	return BNDebuggerAllocStringList(cstrings.data(), *count);
}


char* BNDebuggerGetRemoteHost(BNDebuggerController* controller)
{
	return BNDebuggerAllocString(controller->object->GetState()->GetRemoteHost().c_str());
}


uint32_t BNDebuggerGetRemotePort(BNDebuggerController* controller)
{
	return controller->object->GetState()->GetRemotePort();
}


int32_t BNDebuggerGetPIDAttach(BNDebuggerController* controller)
{
	return controller->object->GetState()->GetPIDAttach();
}


char* BNDebuggerGetInputFile(BNDebuggerController* controller)
{
	return BNDebuggerAllocString(controller->object->GetState()->GetInputFile().c_str());
}


char* BNDebuggerGetExecutablePath(BNDebuggerController* controller)
{
	return BNDebuggerAllocString(controller->object->GetState()->GetExecutablePath().c_str());
}


char* BNDebuggerGetWorkingDirectory(BNDebuggerController* controller)
{
	return BNDebuggerAllocString(controller->object->GetState()->GetWorkingDirectory().c_str());
}


bool BNDebuggerGetRequestTerminalEmulator(BNDebuggerController* controller)
{
	return controller->object->GetState()->GetRequestTerminalEmulator();
}


char* BNDebuggerGetCommandLineArguments(BNDebuggerController* controller)
{
	return BNDebuggerAllocString(controller->object->GetState()->GetCommandLineArguments().c_str());
}


void BNDebuggerSetRemoteHost(BNDebuggerController* controller, const char* host)
{
	controller->object->GetState()->SetRemoteHost(host);
}


void BNDebuggerSetRemotePort(BNDebuggerController* controller, uint32_t port)
{
	controller->object->GetState()->SetRemotePort(port);
}


void BNDebuggerSetPIDAttach(BNDebuggerController* controller, int32_t pid)
{
	controller->object->GetState()->SetPIDAttach(pid);
}


void BNDebuggerSetInputFile(BNDebuggerController* controller, const char* path)
{
	controller->object->GetState()->SetInputFile(path);
}


void BNDebuggerSetExecutablePath(BNDebuggerController* controller, const char* path)
{
	controller->object->GetState()->SetExecutablePath(path);
}


void BNDebuggerSetWorkingDirectory(BNDebuggerController* controller, const char* path)
{
	controller->object->GetState()->SetWorkingDirectory(path);
}


void BNDebuggerSetRequestTerminalEmulator(BNDebuggerController* controller, bool requestEmulator)
{
	controller->object->GetState()->SetRequestTerminalEmulator(requestEmulator);
}


void BNDebuggerSetCommandLineArguments(BNDebuggerController* controller, const char* args)
{
	controller->object->GetState()->SetCommandLineArguments(args);
}


// TODO: the structures to hold information about the breakpoints are different in the API and the core, so we need to
// convert it here. Better unify them later.
BNDebugBreakpoint* BNDebuggerGetBreakpoints(BNDebuggerController* controller, size_t* count)
{
	DebuggerState* state = controller->object->GetState();
	std::vector<ModuleNameAndOffset> breakpoints = state->GetBreakpoints()->GetBreakpointList();
	*count = breakpoints.size();

	//std::vector<DebugBreakpoint> remoteList;
	//if (state->IsConnected() && state->GetAdapter())
	//	remoteList = state->GetAdapter()->GetBreakpointList();

	BNDebugBreakpoint* result = new BNDebugBreakpoint[breakpoints.size()];
	for (size_t i = 0; i < breakpoints.size(); i++)
	{
		uint64_t remoteAddress = state->GetModules()->RelativeAddressToAbsolute(breakpoints[i]);
		bool enabled = false;
		result[i].module = BNDebuggerAllocString(breakpoints[i].module.c_str());
		result[i].offset = breakpoints[i].offset;
		result[i].address = remoteAddress;
		result[i].enabled = enabled;
	}
	return result;
}


void BNDebuggerFreeBreakpoints(BNDebugBreakpoint* breakpoints, size_t count)
{
	for (size_t i = 0; i < count; i++)
	{
		BNDebuggerFreeString(breakpoints[i].module);
	}
	delete[] breakpoints;
}


void BNDebuggerDeleteAbsoluteBreakpoint(BNDebuggerController* controller, uint64_t address)
{
	controller->object->DeleteBreakpoint(address);
}


void BNDebuggerDeleteRelativeBreakpoint(BNDebuggerController* controller, const char* module, uint64_t offset)
{
	controller->object->DeleteBreakpoint(ModuleNameAndOffset(module, offset));
}


void BNDebuggerAddAbsoluteBreakpoint(BNDebuggerController* controller, uint64_t address)
{
	controller->object->AddBreakpoint(address);
}


void BNDebuggerAddRelativeBreakpoint(BNDebuggerController* controller, const char* module, uint64_t offset)
{
	controller->object->AddBreakpoint(ModuleNameAndOffset(module, offset));
}


uint64_t BNDebuggerGetIP(BNDebuggerController* controller)
{
	return controller->object->GetCurrentIP();
}


uint64_t BNDebuggerGetLastIP(BNDebuggerController* controller)
{
	return controller->object->GetLastIP();
}


bool BNDebuggerSetIP(BNDebuggerController* controller, uint64_t address)
{
	return controller->object->SetIP(address);
}


bool BNDebuggerContainsAbsoluteBreakpoint(BNDebuggerController* controller, uint64_t address)
{
	DebuggerState* state = controller->object->GetState();
	if (!state)
		return false;

	DebuggerBreakpoints* breakpoints = state->GetBreakpoints();
	if (!breakpoints)
		return false;

	return breakpoints->ContainsAbsolute(address);
}


bool BNDebuggerContainsRelativeBreakpoint(BNDebuggerController* controller, const char* module, uint64_t offset)
{
	DebuggerState* state = controller->object->GetState();
	if (!state)
		return false;

	DebuggerBreakpoints* breakpoints = state->GetBreakpoints();
	if (!breakpoints)
		return false;

	return breakpoints->ContainsOffset(ModuleNameAndOffset(module, offset));
}


uint64_t BNDebuggerRelativeAddressToAbsolute(BNDebuggerController* controller, const char* module, uint64_t offset)
{
	DebuggerState* state = controller->object->GetState();
	if (!state)
		return 0;

	DebuggerModules* modules = state->GetModules();
	if (!modules)
		return 0;

	return modules->RelativeAddressToAbsolute(ModuleNameAndOffset(module, offset));
}


BNModuleNameAndOffset BNDebuggerAbsoluteAddressToRelative(BNDebuggerController* controller, uint64_t address)
{
	BNModuleNameAndOffset result;
	result.offset = 0;
	result.module = nullptr;

	DebuggerState* state = controller->object->GetState();
	if (!state)
		return result;

	DebuggerModules* modules = state->GetModules();
	if (!modules)
		return result;

	ModuleNameAndOffset addr = modules->AbsoluteAddressToRelative(address);
	result.module = BNDebuggerAllocString(addr.module.c_str());
	result.offset = addr.offset;
	return result;
}


bool BNDebuggerIsSameBaseModule(const char* module1, const char* module2)
{
	return DebugModule::IsSameBaseModule(module1, module2);
}


size_t BNDebuggerRegisterEventCallback(
	BNDebuggerController* controller, void (*callback)(void* ctx, BNDebuggerEvent* event), const char* name, void* ctx)
{
	return controller->object->RegisterEventCallback(
		[=](const DebuggerEvent& event) {
			BNDebuggerEvent* evt = new BNDebuggerEvent;

			evt->type = event.type;
			evt->data.targetStoppedData.reason = event.data.targetStoppedData.reason;
			evt->data.targetStoppedData.exitCode = event.data.targetStoppedData.exitCode;
			evt->data.targetStoppedData.lastActiveThread = event.data.targetStoppedData.lastActiveThread;
			evt->data.targetStoppedData.data = event.data.targetStoppedData.data;

			evt->data.errorData.error = BNDebuggerAllocString(event.data.errorData.error.c_str());
			evt->data.errorData.shortError = BNDebuggerAllocString(event.data.errorData.shortError.c_str());
			evt->data.errorData.data = event.data.errorData.data;

			evt->data.exitData.exitCode = event.data.exitData.exitCode;

			evt->data.relativeAddress.module = BNDebuggerAllocString(event.data.relativeAddress.module.c_str());
			evt->data.relativeAddress.offset = event.data.relativeAddress.offset;

			evt->data.absoluteAddress = event.data.absoluteAddress;

			evt->data.messageData.message = BNDebuggerAllocString(event.data.messageData.message.c_str());

			callback(ctx, evt);

			BNDebuggerFreeString(evt->data.errorData.error);
			BNDebuggerFreeString(evt->data.errorData.shortError);
			BNDebuggerFreeString(evt->data.relativeAddress.module);
			BNDebuggerFreeString(evt->data.messageData.message);
			delete evt;
		},
		name);
}


void BNDebuggerRemoveEventCallback(BNDebuggerController* controller, size_t index)
{
	controller->object->RemoveEventCallback(index);
}


uint32_t BNDebuggerGetExitCode(BNDebuggerController* controller)
{
	return controller->object->GetExitCode();
}


void BNDebuggerWriteStdin(BNDebuggerController* controller, const char* data, size_t len)
{
	controller->object->WriteStdIn(std::string(data, len));
}


DEBUGGER_FFI_API char* BNDebuggerInvokeBackendCommand(BNDebuggerController* controller, const char* cmd)
{
	std::string output = controller->object->InvokeBackendCommand(std::string(cmd));
	char* result = BNDebuggerAllocString(output.c_str());
	return result;
}


DEBUGGER_FFI_API char* BNDebuggerGetStopReasonString(BNDebugStopReason reason)
{
	std::string str = DebuggerController::GetStopReasonString(reason);
	return BNDebuggerAllocString(str.c_str());
}


DEBUGGER_FFI_API DebugStopReason BNDebuggerGetStopReason(BNDebuggerController* controller)
{
	return controller->object->StopReason();
}


DEBUGGER_FFI_API BNMetadata* BNDebuggerGetAdapterProperty(BNDebuggerController* controller, const char* name)
{
	Ref<Metadata> result = controller->object->GetAdapterProperty(name);
	if (result)
		return BNNewMetadataReference(result->GetObject());
	return nullptr;
}


DEBUGGER_FFI_API bool BNDebuggerSetAdapterProperty(
	BNDebuggerController* controller, const char* name, BNMetadata* value)
{
	return controller->object->SetAdapterProperty(name, new Metadata(BNNewMetadataReference(value)));
}


bool BNDebuggerActivateDebugAdapter(BNDebuggerController* controller)
{
	return controller->object->ActivateDebugAdapter();
}


char* BNDebuggerGetAddressInformation(BNDebuggerController* controller, uint64_t address)
{
	return BNDebuggerAllocString(controller->object->GetAddressInformation(address).c_str());
}


bool BNDebuggerIsFirstLaunch(BNDebuggerController* controller)
{
	return controller->object->IsFirstLaunch();
}


bool BNDebuggerIsTTD(BNDebuggerController* controller)
{
	return controller->object->IsTTD();
}


void BNDebuggerPostDebuggerEvent(BNDebuggerController* controller, BNDebuggerEvent* event)
{
	DebuggerEvent evt;
	evt.type = event->type;
	evt.data.targetStoppedData.reason = event->data.targetStoppedData.reason;
	evt.data.targetStoppedData.exitCode = event->data.targetStoppedData.exitCode;
	evt.data.targetStoppedData.lastActiveThread = event->data.targetStoppedData.lastActiveThread;
	evt.data.targetStoppedData.data = event->data.targetStoppedData.data;

	evt.data.errorData.error = event->data.errorData.error;
	evt.data.errorData.shortError = event->data.errorData.shortError;
	evt.data.errorData.data = event->data.errorData.data;

	evt.data.exitData.exitCode = event->data.exitData.exitCode;

	evt.data.relativeAddress.module = event->data.relativeAddress.module;
	evt.data.relativeAddress.offset = event->data.relativeAddress.offset;

	evt.data.absoluteAddress = event->data.absoluteAddress;

	evt.data.messageData.message = event->data.messageData.message;

	controller->object->PostDebuggerEvent(evt);
}


bool BNDebuggerRemoveMemoryRegion(BNDebuggerController* controller)
{
	return controller->object->RemoveDebuggerMemoryRegion();
}


bool BNDebuggerReAddMemoryRegion(BNDebuggerController* controller)
{
	return controller->object->ReAddDebuggerMemoryRegion();
}


uint64_t BNDebuggerGetViewFileSegmentsStart(BNDebuggerController* controller)
{
	return controller->object->GetViewFileSegmentsStart();
}


bool BNDebuggerComputeLLILExprValue(BNDebuggerController* controller, BNLowLevelILFunction* function, size_t expr,
	uint64_t& value)
{
	Ref<LowLevelILFunction> llil = new LowLevelILFunction(BNNewLowLevelILFunctionReference(function));
	auto instr = llil->GetExpr(expr);
	return controller->object->ComputeExprValueAPI(instr, value);
}


bool BNDebuggerComputeMLILExprValue(BNDebuggerController* controller, BNMediumLevelILFunction* function, size_t expr,
	uint64_t& value)
{
	Ref<MediumLevelILFunction> mlil = new MediumLevelILFunction(BNNewMediumLevelILFunctionReference(function));
	auto instr = mlil->GetExpr(expr);
	return controller->object->ComputeExprValueAPI(instr, value);
}


bool BNDebuggerComputeHLILExprValue(BNDebuggerController* controller, BNHighLevelILFunction* function, size_t expr,
	uint64_t& value)
{
	Ref<HighLevelILFunction> hlil = new HighLevelILFunction(BNNewHighLevelILFunctionReference(function));
	auto instr = hlil->GetExpr(expr);
	return controller->object->ComputeExprValueAPI(instr, value);
}


bool BNDebuggerGetVariableValue(BNDebuggerController* controller, BNVariable* variable, uint64_t address, size_t size,
	uint64_t& value)
{
	return controller->object->GetVariableValue(*variable, address, size, value);
}
