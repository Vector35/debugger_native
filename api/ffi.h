#pragma once
#include "binaryninjaapi.h"

using namespace BinaryNinja;

// Define macros for defining objects exposed by the API
#define DECLARE_DEBUGGER_API_OBJECT(handle, cls) \
	namespace BinaryNinjaDebugger{ class cls; } struct handle { BinaryNinjaDebugger::cls* object; }
#define IMPLEMENT_DEBUGGER_API_OBJECT(handle) \
	private: handle m_apiObject; public: typedef handle* APIHandle; handle* GetAPIObject() { return &m_apiObject; } private:
#define INIT_DEBUGGER_API_OBJECT() \
	m_apiObject.object = this;

DECLARE_DEBUGGER_API_OBJECT(BNDebuggerController, DebuggerController);
DECLARE_DEBUGGER_API_OBJECT(BNDebugAdapterType, DebugAdapterType);
DECLARE_DEBUGGER_API_OBJECT(BNDebugAdapter, DebugAdapter);
DECLARE_DEBUGGER_API_OBJECT(BNDebuggerState, DebuggerState);

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef __GNUC__
#  ifdef DEBUGGER_LIBRARY
#    define DEBUGGER_FFI_API __attribute__((visibility("default")))
#  else // DEBUGGER_LIBRARY
#    define DEBUGGER_FFI_API
#  endif // DEBUGGER_LIBRARY
#else // __GNUC__
#ifdef _MSC_VER
#  ifdef DEBUGGER_LIBRARY
#    define DEBUGGER_FFI_API __declspec(dllexport)
#  else // DEBUGGER_LIBRARY
#    define DEBUGGER_FFI_API __declspec(dllimport)
#  endif // DEBUGGER_LIBRARY
#else // _MSC_VER
#define DEBUGGER_FFI_API
#endif // _MSC_VER
#endif // __GNUC__C

	struct BNDebugThread
	{
		uint32_t m_tid;
		uint64_t m_rip;
	};


	struct BNDebugModule
	{
		char* m_name;
		char* m_short_name;
		uint64_t m_address;
		size_t m_size;
		bool m_loaded;
	};


	struct BNDebugRegister
	{
		char* m_name;
		uint64_t m_value;
		size_t m_width;
		size_t m_registerIndex;
		char* m_hint;
	};


	struct BNDebugBreakpoint
	{
		// TODO: we should add an absolute address to this, along with a boolean telling whether it is valid
		char* module;
		uint64_t offset;
		uint64_t address;
		bool enabled;
	};


	struct BNModuleNameAndOffset
	{
		char* module;
		uint64_t offset;
	};


	enum class BNDebugStopReason {
		UnknownReason = 0,
		InitialBreakpoint,
		StdoutMessage,
		ProcessExited,
		AccessViolation,
		SingleStep,
		Calculation,
		Breakpoint,
		IllegalInstruction,
		SignalHup,
		SignalInt,
		SignalQuit,
		SignalIll,
		SignalAbrt,
		SignalEmt,
		SignalFpe,
		SignalKill,
		SignalBus,
		SignalSegv,
		SignalSys,
		SignalPipe,
		SignalAlrm,
		SignalTerm,
		SignalUrg,
		SignalStop,
		SignalTstp,
		SignalCont,
		SignalChld,
		SignalTtin,
		SignalTtou,
		SignalIo,
		SignalXcpu,
		SignalXfsz,
		SignalVtalrm,
		SignalProf,
		SignalWinch,
		SignalInfo,
		SignalUsr1,
		SignalUsr2,
		SignalStkflt,
		SignalBux,
		SignalPoll,
		ExcEmulation,
		ExcSoftware,
		ExcSyscall,
		ExcMachSyscall,
		ExcRpcAlert,
		ExcCrash,

		InternalError,
		InvalidStatusOrOperation,

		UserRequestedBreak
	};


	enum BNDebugAdapterConnectionStatus
	{
		DebugAdapterNotConnectedStatus,
		DebugAdapterConnectingStatus,
		DebugAdapterConnectedStatus,
	};


	enum BNDebugAdapterTargetStatus
	{
		// Target is not created yet, or not connected to yet
		DebugAdapterInvalidStatus,
		DebugAdapterRunningStatus,
		DebugAdapterPausedStatus,
	};


	DEBUGGER_FFI_API BNDebuggerController* BNGetDebuggerController(BNBinaryView* data);
	DEBUGGER_FFI_API BNBinaryView* BNDebuggerGetLiveView(BNDebuggerController* controller);
	DEBUGGER_FFI_API BNBinaryView* BNDebuggerGetData(BNDebuggerController* controller);
	DEBUGGER_FFI_API BNArchitecture* BNDebuggerGetRemoteArchitecture(BNDebuggerController* controller);
	DEBUGGER_FFI_API bool BNDebuggerIsConnected(BNDebuggerController* controller);
	DEBUGGER_FFI_API bool BNDebuggerIsRunning(BNDebuggerController* controller);

	DEBUGGER_FFI_API uint64_t BNDebuggerGetStackPointer(BNDebuggerController* controller);

	DEBUGGER_FFI_API BNDataBuffer* BNDebuggerReadMemory(BNDebuggerController* controller, uint64_t address, size_t size);
	DEBUGGER_FFI_API bool BNDebuggerWriteMemory(BNDebuggerController* controller, uint64_t address, BNDataBuffer* buffer);

	DEBUGGER_FFI_API BNDebugThread* BNDebuggerGetThreads(BNDebuggerController* controller, size_t* count);
	DEBUGGER_FFI_API void BNDebuggerFreeThreads(BNDebugThread* threads, size_t count);

	DEBUGGER_FFI_API BNDebugThread BNDebuggerGetActiveThread(BNDebuggerController* controller);
	DEBUGGER_FFI_API void BNDebuggerSetActiveThread(BNDebuggerController* controller, BNDebugThread thread);

	DEBUGGER_FFI_API BNDebugModule* BNDebuggerGetModules(BNDebuggerController* controller, size_t* count);
	DEBUGGER_FFI_API void BNDebuggerFreeModules(BNDebugModule* modules, size_t count);

	DEBUGGER_FFI_API BNDebugRegister* BNDebuggerGetRegisters(BNDebuggerController* controller, size_t* count);
	DEBUGGER_FFI_API void BNDebuggerFreeRegisters(BNDebugRegister* modules, size_t count);
	DEBUGGER_FFI_API bool BNDebuggerSetRegisterValue(BNDebuggerController* controller, const char* name, size_t len, uint64_t value);

	// target control
	DEBUGGER_FFI_API bool BNDebuggerLaunch(BNDebuggerController* controller);
	DEBUGGER_FFI_API bool BNDebuggerExecute(BNDebuggerController* controller);
	DEBUGGER_FFI_API void BNDebuggerRestart(BNDebuggerController* controller);
	DEBUGGER_FFI_API void BNDebuggerQuit(BNDebuggerController* controller);
	DEBUGGER_FFI_API void BNDebuggerConnect(BNDebuggerController* controller);
	DEBUGGER_FFI_API void BNDebuggerDetach(BNDebuggerController* controller);
	DEBUGGER_FFI_API void BNDebuggerPause(BNDebuggerController* controller);
	// Convenience function, either launch the target process or connect to a remote, depending on the selected adapter
	DEBUGGER_FFI_API void BNDebuggerLaunchOrConnect(BNDebuggerController* controller);

	DEBUGGER_FFI_API BNDebugStopReason BNDebuggerGo(BNDebuggerController* controller);
	DEBUGGER_FFI_API BNDebugStopReason BNDebuggerStepInto(BNDebuggerController* controller, BNFunctionGraphType il = NormalFunctionGraph);
	DEBUGGER_FFI_API BNDebugStopReason BNDebuggerStepOver(BNDebuggerController* controller, BNFunctionGraphType il = NormalFunctionGraph);
	DEBUGGER_FFI_API BNDebugStopReason BNDebuggerStepReturn(BNDebuggerController* controller);
	DEBUGGER_FFI_API BNDebugStopReason BNDebuggerStepTo(BNDebuggerController* controller, const uint64_t* remoteAddresses, size_t count);

	DEBUGGER_FFI_API char* BNDebuggerGetAdapterType(BNDebuggerController* controller);
	DEBUGGER_FFI_API void BNDebuggerSetAdapterType(BNDebuggerController* controller, const char* adapter);

	DEBUGGER_FFI_API BNDebugAdapterConnectionStatus BNDebuggerGetConnectionStatus(BNDebuggerController* controller);
	DEBUGGER_FFI_API BNDebugAdapterTargetStatus BNDebuggerGetTargetStatus(BNDebuggerController* controller);

	DEBUGGER_FFI_API char* BNDebuggerGetRemoteHost(BNDebuggerController* controller);
	DEBUGGER_FFI_API uint32_t BNDebuggerGetRemotePort(BNDebuggerController* controller);
	DEBUGGER_FFI_API char* BNDebuggerGetExecutablePath(BNDebuggerController* controller);
	DEBUGGER_FFI_API bool BNDebuggerGetRequestTerminalEmulator(BNDebuggerController* controller);
	DEBUGGER_FFI_API char* BNDebuggerGetCommandLineArguments(BNDebuggerController* controller);

	DEBUGGER_FFI_API void BNDebuggerSetRemoteHost(BNDebuggerController* controller, const char* host);
	DEBUGGER_FFI_API void BNDebuggerSetRemotePort(BNDebuggerController* controller, uint32_t port);
	DEBUGGER_FFI_API void BNDebuggerSetExecutablePath(BNDebuggerController* controller, const char* path);
	DEBUGGER_FFI_API void BNDebuggerSetRequestTerminalEmulator(BNDebuggerController* controller, bool requestEmulator);
	DEBUGGER_FFI_API void BNDebuggerSetCommandLineArguments(BNDebuggerController* controller, const char* args);

	DEBUGGER_FFI_API BNDebugBreakpoint* BNDebuggerGetBreakpoints(BNDebuggerController* controller, size_t* count);
	DEBUGGER_FFI_API void BNDebuggerFreeBreakpoints(BNDebugBreakpoint* breakpoints, size_t count);

	DEBUGGER_FFI_API void BNDebuggerDeleteAbsoluteBreakpoint(BNDebuggerController* controller, uint64_t address);
	DEBUGGER_FFI_API void BNDebuggerDeleteRelativeBreakpoint(BNDebuggerController* controller, const char* module, uint64_t offset);
	DEBUGGER_FFI_API void BNDebuggerAddAbsoluteBreakpoint(BNDebuggerController* controller, uint64_t address);
	DEBUGGER_FFI_API void BNDebuggerAddRelativeBreakpoint(BNDebuggerController* controller, const char* module, uint64_t offset);
	DEBUGGER_FFI_API bool BNDebuggerContainsAbsoluteBreakpoint(BNDebuggerController* controller, uint64_t address);
	DEBUGGER_FFI_API bool BNDebuggerContainsRelativeBreakpoint(BNDebuggerController* controller, const char* module, uint64_t offset);

	DEBUGGER_FFI_API uint64_t BNDebuggerGetIP(BNDebuggerController* controller);
	DEBUGGER_FFI_API uint64_t BNDebuggerGetLastIP(BNDebuggerController* controller);

	DEBUGGER_FFI_API uint64_t BNDebuggerRelativeAddressToAbsolute(BNDebuggerController* controller, const char* module, uint64_t offset);
	DEBUGGER_FFI_API BNModuleNameAndOffset BNDebuggerAbsoluteAddressToRelative(BNDebuggerController* controller, uint64_t address);


	// DebugAdapterType
	DEBUGGER_FFI_API BNDebugAdapterType* BNGetDebugAdapterTypeByName(const char* name);
	DEBUGGER_FFI_API bool BNDebugAdapterTypeCanExecute(BNDebugAdapterType* adapter, BNBinaryView* data);
	DEBUGGER_FFI_API bool BNDebugAdapterTypeCanConnect(BNDebugAdapterType* adapter, BNBinaryView* data);
	DEBUGGER_FFI_API char** BNGetAvailableDebugAdapterTypes(BNBinaryView* data, size_t* count);


	// DebugModule
	DEBUGGER_FFI_API bool BNDebuggerIsSameBaseModule(const char* module1, const char* module2);

#ifdef __cplusplus
}
#endif