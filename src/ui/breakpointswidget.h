#pragma once

#include <QtCore/QAbstractItemModel>
#include <QtCore/QItemSelectionModel>
#include <QtCore/QModelIndex>
#include <QtWidgets/QTableView>
#include <QtWidgets/QStyledItemDelegate>
#include "inttypes.h"
#include "binaryninjaapi.h"
#include "dockhandler.h"
#include "viewframe.h"
#include "fontsettings.h"
#include "theme.h"
#include "../debuggerstate.h"

class BreakpointItem
{
private:
    // TODO: this field actually means whether the breakpoint is active in the target. E.g., when the target is not
    // running, it will be false. However, we do need to support disable/enable breakpoints while the target is running
    bool m_enabled;
    ModuleNameAndOffset m_location;
    uint64_t m_address;

public:
    BreakpointItem(bool enabled, const ModuleNameAndOffset location, uint64_t remoteAddress);
    bool enabled() const { return m_enabled; }
    ModuleNameAndOffset location() const { return m_location; }
    uint64_t address() const { return m_address; }
    bool operator==(const BreakpointItem& other) const;
    bool operator!=(const BreakpointItem& other) const;
    bool operator<(const BreakpointItem& other) const;
};

Q_DECLARE_METATYPE(BreakpointItem);


class DebugBreakpointsListModel: public QAbstractTableModel
{
    Q_OBJECT

protected:
    QWidget* m_owner;
    BinaryViewRef m_data;
    ViewFrame* m_view;
    std::vector<BreakpointItem> m_items;

public:
    enum ColumnHeaders
    {
        EnabledColumn,
        LocationColumn,
        AddressColumn,
    };

    DebugBreakpointsListModel(QWidget* parent, BinaryViewRef data, ViewFrame* view);
    virtual ~DebugBreakpointsListModel();

    virtual QModelIndex index(int row, int col, const QModelIndex& parent = QModelIndex()) const override;

    virtual int rowCount(const QModelIndex& parent = QModelIndex()) const override
        { (void) parent; return (int)m_items.size(); }
    virtual int columnCount(const QModelIndex& parent = QModelIndex()) const override { (void) parent; return 3; }
    BreakpointItem getRow(int row) const;
    virtual QVariant data(const QModelIndex& i, int role) const override;
    virtual QVariant headerData(int column, Qt::Orientation orientation, int role) const override;
    void updateRows(std::vector<BreakpointItem> newRows);
};


class DebugBreakpointsItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT

    QFont m_font;
    int m_baseline, m_charWidth, m_charHeight, m_charOffset;

public:
    DebugBreakpointsItemDelegate(QWidget* parent);
    void updateFonts();
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& idx) const;
};


class DebugBreakpointsWidget : public SidebarWidget
{
    Q_OBJECT

    ViewFrame* m_view;
    BinaryViewRef m_data;

    UIActionHandler* M_actionHandler;
    QTableView* m_table;
    DebugBreakpointsListModel* m_model;
    DebugBreakpointsItemDelegate* m_delegate;

    QPoint m_last_selected_point{};
    QHeaderView* m_horizontal_header;
    QHeaderView* m_vertical_header;
    QAction* m_remove_action;
    QAction* m_jump_action;

    // void shouldBeVisible()
    virtual void notifyFontChanged() override;


public:
    DebugBreakpointsWidget(const QString& name, ViewFrame* view, BinaryViewRef data);

    void notifyBreakpointsChanged(std::vector<BreakpointItem> breakpoints);

private slots:
    void customContextMenu(const QPoint& point);
    void Jump();
    void Remove();
};


class DebugBreakpointsWidgetType : public SidebarWidgetType {
public:
    DebugBreakpointsWidgetType(const QImage& icon, const QString& name) : SidebarWidgetType(icon, name) { }

    bool isInReferenceArea() const override { return false; }

    SidebarWidget* createWidget(ViewFrame* frame, BinaryViewRef data) override {
        return new DebugBreakpointsWidget("Native Debugger Breakpoints", frame, data);
    }
};