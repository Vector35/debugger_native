#include "binaryninjaapi.h"
#include "debuggercontroller.h"
#include "debuggercommon.h"
#include "../api/ffi.h"

using namespace BinaryNinjaDebugger;


// Macro-like function to convert an non-referenced object to the external API reference
template <typename T>
static auto* API_OBJECT_STATIC(T* obj)
{
	if (obj == nullptr)
		return nullptr;
	return obj->GetAPIObject();
}

template <typename T>
static auto* API_OBJECT_STATIC(const BinaryNinja::Ref<T>& obj)
{
	if (!obj)
		return (decltype(obj->m_object))nullptr;
	obj->AddRef();
	return obj->GetObject();
}

// From refcountobject.h
template <typename T>
static auto* API_OBJECT_REF(T* obj)
{
	if (obj == nullptr)
		return (decltype(obj->m_object))nullptr;
	obj->AddRef();
	return obj->m_object;
}

template <typename T>
static auto* API_OBJECT_REF(const BinaryNinja::Ref<T>& obj)
{
	if (!obj)
		return (decltype(obj->m_object))nullptr;
	obj->AddRef();
	return obj->m_object;
}

template <class T>
static T* API_OBJECT_NEW_REF(T* obj)
{
	if (obj)
		obj->object->AddRef();
	return obj;
}

template <class T>
static void API_OBJECT_FREE(T* obj)
{
	if (obj)
		obj->object->ReleaseAPIRef();
}


BNDebuggerController* BNGetDebuggerController(BNBinaryView* data)
{
	return DebuggerController::GetController(new BinaryView(data))->GetAPIObject();
}


BNBinaryView* BNDebuggerGetLiveView(BNDebuggerController* controller)
{
	return API_OBJECT_REF(controller->object->GetLiveView());
}


BNBinaryView* BNDebuggerGetData(BNDebuggerController* controller)
{
	return API_OBJECT_REF(controller->object->GetData());
}


BNArchitecture* BNDebuggerGetRemoteArchitecture(BNDebuggerController* controller)
{
	return API_OBJECT_STATIC(controller->object->GetRemoteArchitecture());
}


bool BNDebuggerIsConnected(BNDebuggerController* controller)
{
	return controller->object->GetState()->IsConnected();
}


bool BNDebuggerIsRunning(BNDebuggerController* controller)
{
	return controller->object->GetState()->IsRunning();
}


uint64_t BNDebuggerGetStackPointer(BNDebuggerController* controller)
{
	return controller->object->GetState()->StackPointer();
}


BNDataBuffer* BNDebuggerReadMemory(BNDebuggerController* controller, uint64_t address, size_t size)
{
	DataBuffer data = controller->object->ReadMemory(address, size);
	return data.GetBufferObject();
}


bool BNDebuggerWriteMemory(BNDebuggerController* controller, uint64_t address, BNDataBuffer* buffer)
{
	return controller->object->WriteMemory(address, DataBuffer(buffer));
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


BNDebugModule* BNDebuggerGetModules(BNDebuggerController* controller, size_t* size)
{
	std::vector<DebugModule> modules = controller->object->GetAllModules();

	*size = modules.size();
	BNDebugModule* results = new BNDebugModule[modules.size()];

	for (size_t i = 0; i < modules.size(); i++)
	{
		results[i].m_address = modules[i].m_address;
		results[i].m_name = BNAllocString(modules[i].m_name.c_str());
		results[i].m_short_name = BNAllocString(modules[i].m_short_name.c_str());
		results[i].m_size = modules[i].m_size;
		results[i].m_loaded = modules[i].m_loaded;
	}

	return results;
}


void BNDebuggerFreeModules(BNDebugModule* modules, size_t count)
{
	for (size_t i = 0; i < count; i++)
	{
		BNFreeString(modules[i].m_name);
		BNFreeString(modules[i].m_short_name);
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
		results[i].m_name = BNAllocString(registers[i].m_name.c_str());
		results[i].m_value = registers[i].m_value;
		results[i].m_width = registers[i].m_width;
		results[i].m_registerIndex = registers[i].m_registerIndex;
		results[i].m_hint = BNAllocString(registers[i].m_hint.c_str());
	}

	return results;
}


void BNDebuggerFreeRegisters(BNDebugRegister* registers, size_t count)
{
	for (size_t i = 0; i < count; i++)
	{
		BNFreeString(registers[i].m_name);
		BNFreeString(registers[i].m_hint);
	}
	delete[] registers;
}


bool BNDebuggerSetRegisterValue(BNDebuggerController* controller, const char* name, size_t len, uint64_t value)
{
	return controller->object->SetRegisterValue(std::string(name, len), value);
}


// target control
bool BNDebuggerLaunch(BNDebuggerController* controller)
{
	return controller->object->Launch();
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


void BNDebuggerConnect(BNDebuggerController* controller)
{
	controller->object->Connect();
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


BNDebugStopReason BNDebuggerGo(BNDebuggerController* controller)
{
	return controller->object->Go();
}


BNDebugStopReason BNDebuggerStepInto(BNDebuggerController* controller, BNFunctionGraphType il)
{
	return controller->object->StepInto(il);
}


BNDebugStopReason BNDebuggerStepOver(BNDebuggerController* controller, BNFunctionGraphType il)
{
	return controller->object->StepOver(il);
}


BNDebugStopReason BNDebuggerStepReturn(BNDebuggerController* controller)
{
	return controller->object->StepReturn();
}


BNDebugStopReason BNDebuggerStepTo(BNDebuggerController* controller, const uint64_t* remoteAddresses, size_t count)
{
	std::vector<uint64_t> addresses;
	addresses.reserve(count);
	for (size_t i = 0; i < count; i++)
	{
		addresses.push_back(remoteAddresses[i]);
	}
	return controller->object->StepTo(addresses);
}


char* BNDebuggerGetAdapterType(BNDebuggerController* controller)
{
	if (!controller->object->GetState())
		return nullptr;

	return BNAllocString(controller->object->GetState()->GetAdapterType().c_str());
}


void BNDebuggerSetAdapterType(BNDebuggerController* controller, const char* adapter)
{
	controller->object->GetState()->SetAdapterType(adapter);
}


BNDebugAdapterType* BNGetDebugAdapterTypeByName(const char* name)
{
	return DebugAdapterType::GetByName(name)->GetAPIObject();
}


bool BNDebugAdapterTypeCanExecute(BNDebugAdapterType* adapter, BNBinaryView* data)
{
	return adapter->object->CanExecute(new BinaryView(data));
}


bool BNDebugAdapterTypeCanConnect(BNDebugAdapterType* adapter, BNBinaryView* data)
{
	return adapter->object->CanConnect(new BinaryView(data));
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
	std::vector<std::string> adapters = DebugAdapterType::GetAvailableAdapters(new BinaryView(data));
	*count = adapters.size();

	std::vector<const char*> cstrings;
	cstrings.reserve(adapters.size());
	for (auto& str: adapters)
	{
		cstrings.push_back(str.c_str());
	}
	*count = adapters.size();
	return BNAllocStringList(cstrings.data(), *count);
}


char* BNDebuggerGetRemoteHost(BNDebuggerController* controller)
{
	return BNAllocString(controller->object->GetState()->GetRemoteHost().c_str());
}


uint32_t BNDebuggerGetRemotePort(BNDebuggerController* controller)
{
	return controller->object->GetState()->GetRemotePort();
}


char* BNDebuggerGetExecutablePath(BNDebuggerController* controller)
{
	return BNAllocString(controller->object->GetState()->GetExecutablePath().c_str());
}


bool BNDebuggerGetRequestTerminalEmulator(BNDebuggerController* controller)
{
	return controller->object->GetState()->GetRequestTerminalEmulator();
}


char* BNDebuggerGetCommandLineArguments(BNDebuggerController* controller)
{
	return BNAllocString(controller->object->GetState()->GetCommandLineArguments().c_str());
}


void BNDebuggerSetRemoteHost(BNDebuggerController* controller, const char* host)
{
	controller->object->GetState()->SetRemoteHost(host);
}


void BNDebuggerSetRemotePort(BNDebuggerController* controller, uint32_t port)
{
	controller->object->GetState()->SetRemotePort(port);
}


void BNDebuggerSetExecutablePath(BNDebuggerController* controller, const char* path)
{
	controller->object->GetState()->SetExecutablePath(path);
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

	std::vector<DebugBreakpoint> remoteList;
	if (state->IsConnected() && state->GetAdapter())
		remoteList = state->GetAdapter()->GetBreakpointList();

	BNDebugBreakpoint* result = new BNDebugBreakpoint[breakpoints.size()];
	for (size_t i = 0; i < breakpoints.size(); i++)
	{
		uint64_t remoteAddress = state->GetModules()->RelativeAddressToAbsolute(breakpoints[i]);
		bool enabled = false;
		for (const DebugBreakpoint& bp: remoteList)
		{
			if (bp.m_address == remoteAddress)
			{
				enabled = true;
				break;
			}
		}
		result[i].module = BNAllocString(breakpoints[i].module.c_str());
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
		BNFreeString(breakpoints[i].module);
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
	result.module = BNAllocString(addr.module.c_str());
	result.offset = addr.offset;
	return result;
}


bool BNDebuggerIsSameBaseModule(const char* module1, const char* module2)
{
	return DebugModule::IsSameBaseModule(module1, module2);
}