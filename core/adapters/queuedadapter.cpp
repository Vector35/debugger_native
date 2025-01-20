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

#include "queuedadapter.h"
#include <memory>
#include <cstring>
#ifdef WIN32
#include <windows.h>
#undef min
#undef max
#else
#include <unistd.h>
#include <spawn.h>
#include <csignal>
#endif
#include <algorithm>
#include <string>
#include <chrono>
#include <thread>
#include <cstdio>
#include <iostream>
#include <string_view>
#include <regex>
#include <stdexcept>
#include "../../vendor/pugixml/pugixml.hpp"
#include <binaryninjacore.h>
#include <binaryninjaapi.h>
#include <lowlevelilinstruction.h>
#include <mediumlevelilinstruction.h>
#include <highlevelilinstruction.h>

using namespace BinaryNinja;
using namespace std;
using namespace BinaryNinjaDebugger;


QueuedAdapter::QueuedAdapter(DebugAdapter* adapter, BinaryView* data): DebugAdapter(data), m_adapter(adapter)
{
    std::thread worker([&](){
        Worker();
    });
    worker.detach();
}


QueuedAdapter::~QueuedAdapter()
{

}


bool QueuedAdapter::Execute(const std::string& path, const LaunchConfigurations& configs)
{
    std::unique_lock<std::mutex> lock(m_queueMutex);

    bool ret;
    Semaphore sem;
    m_queue.push([&]{
        ret = m_adapter->Execute(path, configs);
        sem.Release();
    });

    lock.unlock();
    sem.Wait();
    return ret;
}


bool QueuedAdapter::ExecuteWithArgs(const std::string &path, const std::string &args, const std::string &workingDir,
					 const LaunchConfigurations &configs)
{
    std::unique_lock<std::mutex> lock(m_queueMutex);

    bool ret;
    Semaphore sem;
    m_queue.push([&, path, args]{
        ret = m_adapter->ExecuteWithArgs(path, args, workingDir, configs);
        sem.Release();
    });

    lock.unlock();
    sem.Wait();
    return ret;
}


bool QueuedAdapter::Attach(std::uint32_t pid)
{
    std::unique_lock<std::mutex> lock(m_queueMutex);

    bool ret;
    Semaphore sem;
    m_queue.push([&, pid]{
        ret = m_adapter->Attach(pid);
        sem.Release();
    });

    lock.unlock();
    sem.Wait();
    return ret;
}


bool QueuedAdapter::Connect(const std::string& server, std::uint32_t port)
{
    std::unique_lock<std::mutex> lock(m_queueMutex);

    bool ret;
    Semaphore sem;
    m_queue.push([&, server, port]{
        ret = m_adapter->Connect(server, port);
        sem.Release();
    });

    lock.unlock();
    sem.Wait();
    return ret;
}


bool QueuedAdapter::Detach()
{
    std::unique_lock<std::mutex> lock(m_queueMutex);

	bool ret;
    Semaphore sem;
    m_queue.push([&]{
        ret = m_adapter->Detach();
        sem.Release();
    });

    lock.unlock();
    sem.Wait();
	return ret;
}


bool QueuedAdapter::Quit()
{
    std::unique_lock<std::mutex> lock(m_queueMutex);

	bool ret;
    Semaphore sem;
    m_queue.push([&]{
        ret = m_adapter->Quit();
        sem.Release();
    });

    lock.unlock();
    sem.Wait();
	return ret;
}


std::vector<DebugThread> QueuedAdapter::GetThreadList()
{
    std::unique_lock<std::mutex> lock(m_queueMutex);

    std::vector<DebugThread> threads;
    Semaphore sem;
    m_queue.push([&]{
        threads = m_adapter->GetThreadList();
        sem.Release();
    });

    lock.unlock();
    sem.Wait();
    return threads;
}


DebugThread QueuedAdapter::GetActiveThread() const
{
    std::unique_lock<std::mutex> lock(m_queueMutex);

    DebugThread thread;
    Semaphore sem;
    m_queue.push([&]{
        thread = m_adapter->GetActiveThread();
        sem.Release();
    });

    lock.unlock();
    sem.Wait();
    return thread;
}


std::uint32_t QueuedAdapter::GetActiveThreadId() const
{
    std::unique_lock<std::mutex> lock(m_queueMutex);

    std::uint32_t tid;
    Semaphore sem;
    m_queue.push([&]{
        tid = m_adapter->GetActiveThreadId();
        sem.Release();
    });

    lock.unlock();
    sem.Wait();
    return tid;
}


bool QueuedAdapter::SetActiveThread(const DebugThread& thread)
{
    std::unique_lock<std::mutex> lock(m_queueMutex);

    bool ret;
    Semaphore sem;
    m_queue.push([&]{
        ret = m_adapter->SetActiveThread(thread);
        sem.Release();
    });

    lock.unlock();
    sem.Wait();
    return ret;
}


bool QueuedAdapter::SetActiveThreadId(std::uint32_t tid)
{
    std::unique_lock<std::mutex> lock(m_queueMutex);

    bool ret;
    Semaphore sem;
    m_queue.push([&]{
        ret = m_adapter->SetActiveThreadId(tid);
        sem.Release();
    });
    lock.unlock();
    sem.Wait();
    return ret;
}


std::vector<DebugFrame> QueuedAdapter::GetFramesOfThread(std::uint32_t tid)
{
    std::unique_lock<std::mutex> lock(m_queueMutex);

    std::vector<DebugFrame> result;
    Semaphore sem;
    m_queue.push([&]{
        result = m_adapter->GetFramesOfThread(tid);
        sem.Release();
    });
    lock.unlock();
    sem.Wait();
    return result;
}


DebugBreakpoint QueuedAdapter::AddBreakpoint(std::uintptr_t address, unsigned long breakpoint_type)
{
    std::unique_lock<std::mutex> lock(m_queueMutex);

    DebugBreakpoint ret;
    Semaphore sem;
    m_queue.push([&]{
        ret = m_adapter->AddBreakpoint(address, breakpoint_type);
        sem.Release();
    });
    lock.unlock();
    sem.Wait();
    return ret;
}


bool QueuedAdapter::RemoveBreakpoint(const DebugBreakpoint& breakpoint)
{
    std::unique_lock<std::mutex> lock(m_queueMutex);

    bool ret;
    Semaphore sem;
    m_queue.push([&]{
        ret = m_adapter->RemoveBreakpoint(breakpoint);
        sem.Release();
    });
    lock.unlock();
    sem.Wait();
    return ret;
}


std::vector<DebugBreakpoint> QueuedAdapter::GetBreakpointList() const
{
    std::unique_lock<std::mutex> lock(m_queueMutex);

    std::vector<DebugBreakpoint> ret;
    Semaphore sem;
    m_queue.push([&]{
        ret = m_adapter->GetBreakpointList();
        sem.Release();
    });
    lock.unlock();
    sem.Wait();
    return ret;
}


std::unordered_map<std::string, DebugRegister> QueuedAdapter::ReadAllRegisters()
{
    std::unique_lock<std::mutex> lock(m_queueMutex);

    std::unordered_map<std::string, DebugRegister> ret;
    Semaphore sem;
    m_queue.push([&]{
        ret = m_adapter->ReadAllRegisters();
        sem.Release();
    });
    lock.unlock();
    sem.Wait();
    return ret;
}


DebugRegister QueuedAdapter::ReadRegister(const std::string& reg)
{
    std::unique_lock<std::mutex> lock(m_queueMutex);

    DebugRegister ret;
    Semaphore sem;
    m_queue.push([&]{
        ret = m_adapter->ReadRegister(reg);
        sem.Release();
    });
    lock.unlock();
    sem.Wait();
    return ret;
}


bool QueuedAdapter::WriteRegister(const std::string& reg, std::uintptr_t value)
{
    std::unique_lock<std::mutex> lock(m_queueMutex);

    bool ret;
    Semaphore sem;
    m_queue.push([&]{
        ret = m_adapter->WriteRegister(reg, value);
        sem.Release();
    });
    lock.unlock();
    sem.Wait();
    return ret;
}


DataBuffer QueuedAdapter::ReadMemory(std::uintptr_t address, std::size_t size)
{
    std::unique_lock<std::mutex> lock(m_queueMutex);

    DataBuffer ret;
    Semaphore sem;
    m_queue.push([&]{
        ret = m_adapter->ReadMemory(address, size);
        sem.Release();
    });
    lock.unlock();
    sem.Wait();
    return ret;
}


bool QueuedAdapter::WriteMemory(std::uintptr_t address, const DataBuffer& buffer)
{
    std::unique_lock<std::mutex> lock(m_queueMutex);

    bool ret;
    Semaphore sem;
    m_queue.push([&]{
        ret = m_adapter->WriteMemory(address, buffer);
        sem.Release();
    });
    lock.unlock();
    sem.Wait();
    return ret;
}


std::vector<DebugModule> QueuedAdapter::GetModuleList()
{
    std::unique_lock<std::mutex> lock(m_queueMutex);

    std::vector<DebugModule> ret;
    Semaphore sem;
    m_queue.push([&]{
        ret = m_adapter->GetModuleList();
        sem.Release();
    });
    lock.unlock();
    sem.Wait();
    return ret;
}


std::string QueuedAdapter::GetTargetArchitecture()
{
    std::unique_lock<std::mutex> lock(m_queueMutex);

    std::string ret;
    Semaphore sem;
    m_queue.push([&]{
        ret = m_adapter->GetTargetArchitecture();
        sem.Release();
    });
    lock.unlock();
    sem.Wait();
    return ret;
}


DebugStopReason QueuedAdapter::StopReason()
{
    std::unique_lock<std::mutex> lock(m_queueMutex);

    DebugStopReason ret;
    Semaphore sem;
    m_queue.push([&]{
        ret = m_adapter->StopReason();
        sem.Release();
    });
    lock.unlock();
    sem.Wait();
    return ret;
}


uint64_t QueuedAdapter::ExitCode()
{
    std::unique_lock<std::mutex> lock(m_queueMutex);

    uint64_t ret;
    Semaphore sem;
    m_queue.push([&]{
        ret = m_adapter->ExitCode();
        sem.Release();
    });
    lock.unlock();
    sem.Wait();
    return ret;
}


bool QueuedAdapter::BreakInto()
{
    std::unique_lock<std::mutex> lock(m_queueMutex);
	// BreakInto must skip the queue, otherwise it will cause deadlock
	return m_adapter->BreakInto();
}


bool QueuedAdapter::Go()
{
    std::unique_lock<std::mutex> lock(m_queueMutex);

    bool ret;
    Semaphore sem;
    m_queue.push([&]{
        ret = m_adapter->Go();
        sem.Release();
    });
    lock.unlock();
    sem.Wait();
    return ret;
}


bool QueuedAdapter::StepInto()
{
    std::unique_lock<std::mutex> lock(m_queueMutex);

	bool ret;
    Semaphore sem;
    m_queue.push([&]{
        ret = m_adapter->StepInto();
        sem.Release();
    });
    lock.unlock();
    sem.Wait();
    return ret;
}


bool QueuedAdapter::StepOver()
{
    std::unique_lock<std::mutex> lock(m_queueMutex);

	bool ret;
    Semaphore sem;
    m_queue.push([&]{
        ret = m_adapter->StepOver();
        sem.Release();
    });
    lock.unlock();
    sem.Wait();
    return ret;
}


bool QueuedAdapter::StepReturn()
{
    std::unique_lock<std::mutex> lock(m_queueMutex);

	bool ret;
    Semaphore sem;
    m_queue.push([&]{
        ret = m_adapter->StepReturn();
        sem.Release();
    });
    lock.unlock();
    sem.Wait();
    return ret;
}


std::string QueuedAdapter::InvokeBackendCommand(const std::string& command)
{
	std::string result = m_adapter->InvokeBackendCommand(command);
	return result;
}


std::uintptr_t QueuedAdapter::GetInstructionOffset()
{
    std::unique_lock<std::mutex> lock(m_queueMutex);

    std::uintptr_t ret;
    Semaphore sem;
    m_queue.push([&]{
        ret = m_adapter->GetInstructionOffset();
        sem.Release();
    });
    lock.unlock();
    sem.Wait();
    return ret;
}


uint64_t QueuedAdapter::GetStackPointer()
{
    std::unique_lock<std::mutex> lock(m_queueMutex);

    std::uintptr_t ret;
    Semaphore sem;
    m_queue.push([&]{
        ret = m_adapter->GetStackPointer();
        sem.Release();
    });
    lock.unlock();
    sem.Wait();
    return ret;
}


bool QueuedAdapter::SupportFeature(DebugAdapterCapacity feature)
{
    return m_adapter->SupportFeature(feature);
}


void QueuedAdapter::Worker()
{
    while (true)
    {
        std::unique_lock<std::mutex> lock(m_queueMutex);
        if (m_queue.size() != 0)
        {
            std::function<void()> func = m_queue.front();
            m_queue.pop();

            lock.unlock();
            func();
        }
    }
}


// The queued adapter must set the event callback on the actual adapter, not itself
void QueuedAdapter::SetEventCallback(std::function<void(const DebuggerEvent &)> function)
{
	m_adapter->SetEventCallback(function);
}


void QueuedAdapter::WriteStdin(const std::string &msg)
{
	m_adapter->WriteStdin(msg);
}


std::vector<DebugProcess> QueuedAdapter::GetProcessList()
{
	return {};
}


bool QueuedAdapter::SuspendThread(std::uint32_t tid)
{
	return false;
}


bool QueuedAdapter::ResumeThread(std::uint32_t tid)
{
	return false;
}


DebugBreakpoint QueuedAdapter::AddBreakpoint(const ModuleNameAndOffset& address, unsigned long breakpoint_type)
{
	return {};
}
