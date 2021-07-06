#pragma once

#include <QtWidgets/QDialog>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QFormLayout>
#include "inttypes.h"
#include "binaryninjaapi.h"
#include "viewframe.h"
#include "fontsettings.h"
#include "../debuggerstate.h"

class AdapterSettingsDialog: public QDialog
{
    Q_OBJECT

private:
    BinaryViewRef m_data;
    DebuggerState* m_state;

    QComboBox* m_adapterEntry;
    QLineEdit* m_pathEntry;
    QLineEdit* m_argumentsEntry;
    QLineEdit* m_addressEntry;
    QLineEdit* m_portEntry;

public:
    AdapterSettingsDialog(QWidget* parent, BinaryViewRef data);

private Q_SLOTS:
    void apply();
    void selectAdapter();
};