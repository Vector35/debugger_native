#include "debuggerstate.h"
#include "debugadapter.h"

using namespace BinaryNinja;
using namespace std;


DebuggerRegisters::DebuggerRegisters(DebuggerState* state): m_state(state)
{
    markDirty();
}


void DebuggerRegisters::markDirty()
{
    m_cachedRgisterList.clear();
    m_registerCache.clear();
}


void DebuggerRegisters::update()
{
    DebugAdapter* adapter = m_state->getAdapter();
    if (!adapter)
        throw runtime_error("Cannot update registers when disconnected");

    // TODO: This is ineffective, especially during remote debugging.
    // We need to get all register and its values in one request
    m_cachedRgisterList = adapter->GetRegisterList();
    for (const std::string reg: m_cachedRgisterList)
    {
        m_registerCache[reg] = adapter->ReadRegister(reg);
    }
}


uint64_t DebuggerRegisters::getRegisterValue(const std::string& name)
{
    auto iter = m_registerCache.find(name);
    if (iter == m_registerCache.end())
        // TODO: we should return a boolean to indicate the call succeeds, and return the value by reference
        return 0;

    return iter->second.m_value;
}


void DebuggerRegisters::updateRegisterValue(const std::string& name, uint64_t value)
{
    DebugAdapter* adapter = m_state->getAdapter();
    if (!adapter)
        return;

    adapter->WriteRegister(name, value);
    // TODO: Do we really need to mark it dirty? How about we just update our cache
    markDirty();
}


DebuggerModules::DebuggerModules(DebuggerState* state, std::vector<DebugModule> modules):
    m_state(state), m_modules(modules)
{

}


void DebuggerModules::markDirty()
{
    m_modules.clear();
}


void DebuggerModules::update()
{
    DebugAdapter* adapter = m_state->getAdapter();
    if (!adapter)
        return;

    m_modules = adapter->GetModuleList();
}


bool DebuggerModules::GetModuleBase(const std::string& name, uint64_t& address)
{
    for (const DebugModule& module: m_modules)
    {
        if ((name == module.m_name) || (name == module.m_short_name))
        {
            address = module.m_address;
            return true;
        }
    }
    return false;
}


DebuggerState::DebuggerState(BinaryViewRef data): m_data(data)
{
    m_memoryView = new DebugProcessView(data);
    m_adapter = new DummyAdapter();
    m_registers = new DebuggerRegisters(this);

    Ref<Metadata> metadata;
    // metadata = m_data->QueryMetadata("native_debugger.command_line_args");
    // if (metadata && metadata->IsStringList())
    //     m_commandLineArgs = metadata->GetStringList();

    metadata = m_data->QueryMetadata("native_debugger.remote_host");
    if (metadata && metadata->IsString())
        m_remoteHost = metadata->GetString();

    metadata = m_data->QueryMetadata("native_debugger.remote_port");
    if (metadata && metadata->IsUnsignedInteger())
        m_remotePort = metadata->GetUnsignedInteger();
    else
        m_remotePort = 31337;

    metadata = m_data->QueryMetadata("native_debugger.adapter_type");
    if (metadata && metadata->IsUnsignedInteger())
        m_adapterType = (DebugAdapterType::AdapterType)metadata->GetUnsignedInteger();
    else
        m_adapterType = DebugAdapterType::DefaultAdapterType;

    metadata = m_data->QueryMetadata("native_debugger.request_terminal_emulator");
    if (metadata && metadata->IsUnsignedInteger())
        m_requestTerminalEmulator = metadata->GetBoolean();
    else
        m_requestTerminalEmulator = false;
}


void DebuggerState::run()
{
    BinaryNinja::LogWarn("run() requested");
    // if (DebugAdapterType::useExec(m_adapterType))


}


void DebuggerState::restart()
{
    BinaryNinja::LogWarn("restart() requested");
}


void DebuggerState::quit()
{
    BinaryNinja::LogWarn("quit() requested");
}


void DebuggerState::attach()
{
    BinaryNinja::LogWarn("attach() requested");
}


void DebuggerState::detach()
{
    BinaryNinja::LogWarn("detach() requested");
}


void DebuggerState::pause()
{
    BinaryNinja::LogWarn("pause() requested");
}


void DebuggerState::resume()
{
    BinaryNinja::LogWarn("resume() requested");
}


void DebuggerState::stepIntoAsm()
{
    BinaryNinja::LogWarn("stepIntoAsm() requested");
}


void DebuggerState::stepIntoIL()
{
    BinaryNinja::LogWarn("stepIntoIL() requested");
}


void DebuggerState::stepOverAsm()
{
    BinaryNinja::LogWarn("stepOverAsm() requested");
}


void DebuggerState::stepOverIL()
{
    BinaryNinja::LogWarn("stepOverIL() requested");
}


void DebuggerState::stepReturn()
{
    BinaryNinja::LogWarn("stepReturn() requested");
}


bool DebuggerState::canExec()
{
    // TODO: query the underlying DebugAdapter for the info
    return true;
}


bool DebuggerState::canConnect()
{
    // TODO: query the underlying DebugAdapter for the info
    return true;
}


DebuggerState* DebuggerState::getState(BinaryViewRef data)
{
    for (auto& state: g_debuggerStates)
    {
        if (state->getData() == data)
            return state;
    }

    DebuggerState* state = new DebuggerState(data);
    g_debuggerStates.push_back(state);
    return state;    
}


void DebuggerState::deleteState(BinaryViewRef data)
{
    for (auto it = g_debuggerStates.begin(); it != g_debuggerStates.end(); )
    {
        if ((*it)->getData() == data)
        {
            it = g_debuggerStates.erase(it);
        }
        else
        {
            ++it;
        }
    }
}


uint64_t DebuggerState::ip()
{
    if (!m_connected)
        throw runtime_error("Cannot read ip when disconnected");
    string archName = m_remoteArch->GetName();
    if (archName == "x86_64")
        return m_registers->getRegisterValue("rip");
    else if (archName == "x86")
        return m_registers->getRegisterValue("eip");
    else if ((archName == "aarch64") || (archName == "arm") || (archName == "armv7") || (archName == "Z80"))
        return m_registers->getRegisterValue("pc");

    throw runtime_error("unimplemented architecture " + archName);
}