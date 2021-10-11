#include "debuggercontroller.h"
#include <thread>
#include "../../ui/mainwindow.h"

DebuggerController::DebuggerController(BinaryViewRef data): m_data(data)
{
    m_state = new DebuggerState(data, this);
//    This is just for compatability with old code
    DebuggerState::RegisterState(m_state);
//    m_state = new DebuggerState(data, this);

    m_hasUI = BinaryNinja::IsUIEnabled();
    if (m_hasUI)
    {
        // DebugerUI is an abstract container of three things, the DebugView, the SideBar widget, and the status bar.
        // None of the three necessarily exists when the DebuggerUI is constructed. So they must register themselves to
        // the DebuggerUI when they are constructed.
        m_ui = new DebuggerUI(this);
    }

    // TODO: we should add an option whether to add a breakpoint at program entry
    AddEntryBreakpoint();

    RegisterEventCallback([this](const DebuggerEvent& event){
        EventHandler(event);
    });

    // start the event queue worker
    std::thread worker([&](){
        Worker();
    });
    worker.detach();
}


void DebuggerController::AddEntryBreakpoint()
{
    uint64_t entryPoint = m_data->GetEntryPoint();
    uint64_t localEntryOffset = entryPoint - m_data->GetStart();
    ModuleNameAndOffset address(m_data->GetFile()->GetOriginalFilename(), localEntryOffset);

    AddBreakpoint(address);
}


void DebuggerController::AddBreakpoint(uint64_t address)
{
    m_state->AddBreakpoint(address);
    if (hasUI())
    {
        emit absoluteBreakpointAdded(address);
    }
}


void DebuggerController::AddBreakpoint(const ModuleNameAndOffset& address)
{
    m_state->AddBreakpoint(address);
    if (hasUI())
    {
        emit relativeBreakpointAdded(address);
    }
}


void DebuggerController::DeleteBreakpoint(uint64_t address)
{
    m_state->DeleteBreakpoint(address);
    if (hasUI())
    {
        emit absoluteBreakpointDeleted(address);
    }
}


void DebuggerController::DeleteBreakpoint(const ModuleNameAndOffset& address)
{
    m_state->DeleteBreakpoint(address);
    if (hasUI())
    {
        emit relativeBreakpointDeleted(address);
    }
}


void DebuggerController::Run()
{
    std::thread worker([this](){
        m_state->Run();
        NotifyStopped(DebugStopReason::InitalBreakpoint, nullptr);
    });
    worker.detach();
}


//    If one wishes to have a synchronous version of this, wait on a semaphore
void DebuggerController::Go()
{
    std::thread worker([this](){
//        This should return the stop reason
        m_state->Go();
        NotifyStopped(DebugStopReason::Breakpoint, nullptr);
    });
    worker.detach();
}


void DebuggerController::StepInto(BNFunctionGraphType il)
{
    std::thread worker([this, il](){
        m_state->StepInto(il);
        NotifyStopped(DebugStopReason::Breakpoint, nullptr);
    });
    worker.detach();
}


void DebuggerController::StepOver(BNFunctionGraphType il)
{
    std::thread worker([this, il](){
        m_state->StepOver(il);
        NotifyStopped(DebugStopReason::Breakpoint, nullptr);
    });
    worker.detach();
}


DebuggerController* DebuggerController::GetController(BinaryViewRef data)
{
    for (auto& controller: g_debuggerControllers)
    {
        if (controller->GetData()->GetFile()->GetOriginalFilename() == data->GetFile()->GetOriginalFilename())
            return controller;
        if (controller->GetData()->GetFile()->GetOriginalFilename() == data->GetParentView()->GetFile()->GetOriginalFilename())
            return controller;
    }

    LogWarn("Creating new debugger controller");
    DebuggerController* controller = new DebuggerController(data);
    g_debuggerControllers.push_back(controller);
    return controller;
}


void DebuggerController::DeleteController(BinaryViewRef data)
{
    for (auto it = g_debuggerControllers.begin(); it != g_debuggerControllers.end(); )
    {
        if ((*it)->GetData()->GetFile()->GetOriginalFilename() == data->GetFile()->GetOriginalFilename())
        {
            it = g_debuggerControllers.erase(it);
        }
        else
        {
            ++it;
        }
    }
}


// This is the central hub of event dispatch. All events first arrive here and then get dispatched based on the content
void DebuggerController::EventHandler(const DebuggerEvent& event)
{
    switch (event.type)
    {
    case TargetStoppedEventType:
    {
        m_state->SetExecutionStatus(DebugAdapterPausedStatus);
        emit stopped(event.data.targetStoppedData.reason, nullptr);

        // Initial breakpoint is reached after successfully launching or attaching to the target
        if (event.data.targetStoppedData.reason == DebugStopReason::InitalBreakpoint)
        {
            // There are some extra processing needed when the initial breakpoint hits
            // HELP NEEDED: I do not think we should do it in this way, but I cannot think of a better one
            m_state->SetConnectionStatus(DebugAdapterConnectedStatus);

            m_state->UpdateRemoteArch();

            m_state->UpdateCaches();
            // We need to apply the breakpoints that the user has set up before launching the target. Note this requires
            // the modules to be updated properly beforehand.
            m_state->ApplyBreakpoints();

            // Rebase the binary and create DebugView
            uint64_t remoteBase = m_state->GetRemoteBase();
            LogWarn("the remote base is 0x%" PRIx64, remoteBase);

            FileMetadata* fileMetadata = m_data->GetFile();
            if (remoteBase != m_data->GetStart())
            {
                // remote base is different from the local base, first need a rebase
//                DatabaseProgress progress(nullptr, "Rebase", "Rebasing...");
//                if (!fileMetadata->Rebase(m_data, remoteBase, [&](size_t cur, size_t total) { progress.update((int)cur, (int)total); }))
                if (!fileMetadata->Rebase(m_data, remoteBase))
                {
                    LogWarn("rebase failed");
                }
            }

            Ref<BinaryView> rebasedView = fileMetadata->GetViewOfType(m_data->GetTypeName());
            SetData(rebasedView);
            LogWarn("the base of the rebased view is 0x%lx", rebasedView->GetStart());
//            DatabaseProgress progress(nullptr, "Debug View", "Creating a BinaryView for debugging...");
//            if (!fileMetadata->CreateSnapshotedView(rebasedView, "Debugged Process", "Debugged Process Memory",
            if (!fileMetadata->CreateSnapshotedView(rebasedView, "Debugged Process", ""))
            {
                LogWarn("create snapshoted view failed");
            }
            else
            {
                LogWarn("create snapshoted view ok");
            }
            Ref<BinaryView> liveView = fileMetadata->GetViewOfType("Debugged Process");
            SetLiveView(liveView);

            DebuggerEvent event;
            event.type = InitialViewRebasedEventType;
            PostDebuggerEvent(event);



        }
        else
        {
            m_state->UpdateCaches();
        }

        emit cacheUpdated(event.data.targetStoppedData.reason, nullptr);
        emit IPChanged(m_state->IP());


        break;
    }
    default:
        break;
    }
}


void DebuggerController::RegisterEventCallback(std::function<void(const DebuggerEvent&)> callback)
{
    m_eventCallbacks.push_back(callback);
}


void DebuggerController::PostDebuggerEvent(const DebuggerEvent& event)
{
    std::unique_lock<std::mutex> lock(m_queueMutex);
    m_events.push(event);
}


void DebuggerController::Worker()
{
    while (true)
    {
        std::unique_lock<std::mutex> lock(m_queueMutex);
        if (m_events.size() != 0)
        {
            const DebuggerEvent event = m_events.front();
            m_events.pop();

            lock.unlock();
            for (auto cb: m_eventCallbacks)
            {
                cb(event);
            }
        }
    }
}


void DebuggerController::NotifyStopped(DebugStopReason reason, void *data)
{
    DebuggerEvent event;
    event.type = TargetStoppedEventType;
    event.data.targetStoppedData.reason = reason;
    event.data.targetStoppedData.data = data;
    PostDebuggerEvent(event);
}


void DebuggerController::NotifyError(const std::string& error, void *data)
{
    DebuggerEvent event;
    event.type = ErrorEventType;
    event.data.errorData.error = error;
    event.data.errorData.data = data;
    PostDebuggerEvent(event);
}


void DebuggerController::NotifyEvent(const std::string& eventString, void *data)
{
    DebuggerEvent event;
    event.type = GeneralEventType;
    event.data.generalData.event = eventString;
    event.data.generalData.data = data;
    PostDebuggerEvent(event);
}