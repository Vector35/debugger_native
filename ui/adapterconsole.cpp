#include "adapterconsole.h"
#include "binaryninjaapi.h"
#include "debuggerapi.h"
#include <QScrollBar>

using namespace BinaryNinja;
using namespace BinaryNinjaDebuggerAPI;

AdapterConsole::AdapterConsole(QWidget* parent, ViewFrame* frame, BinaryViewRef data):
	QWidget(parent), m_view(frame)
{
	m_debugger = DebuggerController::GetController(data);
	m_prompt = QString::fromStdString(m_debugger->GetAdapterType());

	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(0);
	setFont(getMonospaceFont(this));

	// Initialize widgets and layout
	m_consoleLog = new QTextBrowser(this);
	m_consoleLog->setReadOnly(true);
	m_consoleLog->setTextInteractionFlags(m_consoleLog->textInteractionFlags() | Qt::LinksAccessibleByMouse);

	QHBoxLayout* inputLayout = new QHBoxLayout();
	inputLayout->setContentsMargins(4, 4, 4, 4);

	QLabel* promptLabel = new QLabel(m_prompt + ' ');
	inputLayout->addWidget(promptLabel);

	m_consoleInput = new QLineEdit(this);
	m_consoleInput->setPlaceholderText("");

	inputLayout->addWidget(m_consoleInput, 1);

	connect(m_consoleInput, &QLineEdit::returnPressed, this, &AdapterConsole::sendMessage);

	layout->addWidget(m_consoleLog);
	layout->addLayout(inputLayout);
	setLayout(layout);

	// Set up colors
	QPalette widgetPalette = this->palette();
	QColor foreground = widgetPalette.color(QWidget::foregroundRole());
	QColor background = widgetPalette.color(QWidget::backgroundRole());
}


AdapterConsole::~AdapterConsole()
{
}


void AdapterConsole::sendMessage()
{
	QString message = m_consoleInput->text();
	addMessage(QString('(') + m_prompt + ") " + message + '\n');
	sendText(message);
	m_consoleInput->clear();
}


void AdapterConsole::addMessage(const QString &msg)
{
	QTextCursor cursor(m_consoleLog->textCursor());
	cursor.movePosition(QTextCursor::End);

	QScrollBar* bar = m_consoleLog->verticalScrollBar();
	bool atBottom = bar->value() == bar->maximum();

	cursor.insertText(msg);

	if (atBottom)
		bar->setValue(bar->maximum());
}


void AdapterConsole::sendText(const QString &msg)
{
	std::string result = m_debugger->InvokeBackendCommand(msg.toStdString());
	addMessage(QString::fromStdString(result));
}


GlobalAdapterConsoleContainer::GlobalAdapterConsoleContainer(const QString& title) : GlobalAreaWidget(title),
	m_currentFrame(nullptr), m_consoleStack(new QStackedWidget)
{
	auto *layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->addWidget(m_consoleStack);

	auto* noViewLabel = new QLabel("No active view.");
	noViewLabel->setStyleSheet("QLabel { background: palette(base); }");
	noViewLabel->setAlignment(Qt::AlignCenter);

	m_consoleStack->addWidget(noViewLabel);
}


AdapterConsole* GlobalAdapterConsoleContainer::currentConsole() const
{
	if (m_consoleStack->currentIndex() == 0)
		return nullptr;

	return qobject_cast<AdapterConsole*>(m_consoleStack->currentWidget());
}


void GlobalAdapterConsoleContainer::freeDebuggerConsoleForView(QObject* obj)
{
	// A old-style cast must be used here since qobject_cast will fail because
	// the object is on the brink of deletion.
	auto* vf = (ViewFrame*)obj;

	auto data = vf->getCurrentBinaryView();
	auto controller = DebuggerController::GetController(data);

	// Confirm there is a record of this view.
	if (!m_consoleMap.count(controller)) {
		LogWarn("Attempted to free AdapterConsole for untracked view %p", obj);
		return;
	}

	auto* console = m_consoleMap[controller];
	m_consoleStack->removeWidget(console);
	m_consoleMap.erase(controller);

	// Must be called so the ChatBox is guaranteed to be destoryed. If two
	// instances for the same view/database exist, things will break.
	console->deleteLater();
}


void GlobalAdapterConsoleContainer::sendText(const QString &msg) const
{
	auto* cc = currentConsole();
	if (!cc)
		return;

	cc->sendText(msg);
}


void GlobalAdapterConsoleContainer::notifyViewChanged(ViewFrame* frame)
{
	// The "no active view" message widget is always located at index 0. If the
	// frame passed is nullptr, show it.
	if (!frame) {
		m_consoleStack->setCurrentIndex(0);
		m_currentFrame = nullptr;

		return;
	}

	// The notifyViewChanged event can fire multiple times for the same frame
	// even if there is no apparent change. Compare the new frame to the
	// current one before continuing to avoid unnecessary work.
	if (frame == m_currentFrame)
		return;
	m_currentFrame = frame;

	auto data = frame->getCurrentBinaryView();
	Ref<DebuggerController> controller = DebuggerController::GetController(data);

	// Get the appropriate AdapterConsole for this ViewFrame, or create a new one if it
	// doesn't yet exist. The default value for non-existent keys of pointer
	// types in Qt containers is nullptr, which allows this logic below to work.
	auto iter = m_consoleMap.find(controller);
	AdapterConsole* currentConsole;
	if (iter == m_consoleMap.end())
	{
		currentConsole = new AdapterConsole(this, frame, data);

		// DockWidgets related to a ViewFrame are automatically cleaned up as
		// part of the ViewFrame destructor. To ensure there is never a AdapterConsole
		// for a non-existent ViewFrame, the cleanup must be configured manually.
//		connect(frame, &QObject::destroyed, this, &GlobalAdapterConsoleContainer::freeDebuggerConsoleForView);

		m_consoleMap[controller] = currentConsole;
		m_consoleStack->addWidget(currentConsole);
	}
	else
	{
		currentConsole = iter->second;
	}

	m_consoleStack->setCurrentWidget(currentConsole);
}