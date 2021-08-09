#pragma once

// #include <QtWidgets/QScrollArea>
#include <QtWidgets/QPushButton>
#include "viewframe.h"
#include "controlswidget.h"
#include "../debuggerstate.h"
#include "linearview.h"
#include "disassemblyview.h"
#include "tokenizedtextview.h"
#include <QtWidgets/QSplitter>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QLabel>
#include <QtCore/QTimer>
// #include "byte.h"


class DebugView: public QWidget, public View
{
    Q_OBJECT

	BinaryViewRef m_data;
	uint64_t m_currentOffset = 0;
    bool m_isRawDisassembly;
    uint64_t m_rawAddress, m_memoryHistoryAddress;

    bool m_isNavigatingHistory;

    DebuggerState* m_state;
    DebugControlsWidget* m_controls;

    QSplitter* m_splitter;

    QVBoxLayout* m_binaryViewLayout;
    QWidget* m_binaryViewWidget;
    QLabel* m_bianryViewLabel;

    QVBoxLayout* m_disassemblyLayout;
    QWidget* m_disassemblyWidget;
    QLabel* m_disassemblyLabel;

    QVBoxLayout* memoryLayout;
    QWidget* m_memoryWidget;
    QLabel* m_memoryLabel;

    TokenizedTextView* m_binaryText;
    DisassemblyContainer* m_binaryEditor;

    bool m_needsUpdate;
    QTimer* m_updateTimer;

    QTabWidget* m_memoryTabs;
    size_t m_numMemoryTabs = 3;

public:
	DebugView(QWidget* parent, BinaryViewRef data);
    virtual ~DebugView() {}

	virtual BinaryViewRef getData() override;
	virtual uint64_t getCurrentOffset() override;
	virtual BNAddressRange getSelectionOffsets() override;
	virtual FunctionRef getCurrentFunction() override;
	virtual BasicBlockRef getCurrentBasicBlock() override;
	virtual ArchitectureRef getCurrentArchitecture() override;
	virtual LowLevelILFunctionRef getCurrentLowLevelILFunction() override;
	virtual MediumLevelILFunctionRef getCurrentMediumLevelILFunction() override;

	virtual void setSelectionOffsets(BNAddressRange range) override;
	virtual QFont getFont() override;

	virtual bool navigateToFunction(FunctionRef func, uint64_t offset) override;
	virtual bool navigate(uint64_t addr) override;

    bool navigateLive(uint64_t addr);
    bool navigateRaw(uint64_t addr);
    void showRawAssembly(bool raw);
    void loadRawDisassembly(uint64_t startIP);
    void refreshRawDisassembly();

	void setCurrentOffset(uint64_t offset);
	// void navigateToFileOffset(uint64_t offset);

    DebugControlsWidget* getControls() const { return m_controls; }
    DisassemblyContainer* getBinaryEditor() const { return m_binaryEditor; }

protected:
	virtual void focusInEvent(QFocusEvent* event) override;

private Q_SLOTS:
	void updateTimerEvent();
};


class DebugViewType: public ViewType
{
public:
	DebugViewType();
	virtual int getPriority(BinaryViewRef data, const QString& filename) override;
	virtual QWidget* create(BinaryViewRef data, ViewFrame* frame) override;
};
