/*
Copyright (C) 2011 Srivats P.

This file is part of "Ostinato"

This is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>
*/

#include "qheaderview.h"
#include "qtablewidget.h"
#include "qpushbutton.h"
#include "qspinbox.h"
#include "qpushbutton.h"
#include "qscrollarea.h"
#include "qscrollbar.h"
#include "qlineedit.h"
#include "QRegExpValidator"
#include "qregexp.h"

#include "portconfigdialog.h"
#include "settings.h"


PortConfigDialog::PortConfigDialog(
    OstProto::Port &portConfig,
    const OstProto::PortState &portState,
    QWidget *parent)
        : QDialog(parent), portConfig_(portConfig)
{
    QString currentUser(portConfig_.user_name().c_str());

    qDebug("In %s", __FUNCTION__);

    setupUi(this);

    //connect(this->testOK, SIGNAL(clicked()), this, SLOT(testOKcall()));
    auto sender = this->testOK;
    connect(sender,SIGNAL(press),this ,SLOT(BBB(bool)));
    QScrollArea *scrollArea = new QScrollArea();
    scrollArea->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
    scrollArea->setVerticalScrollBarPolicy( Qt::ScrollBarAsNeeded );
    scrollArea->setWidgetResizable(true);

    QBoxLayout* triggersLayout = new QVBoxLayout(triggersGroup);
    triggersLayout->addWidget(scrollArea);

    QGroupBox* groupBox = new QGroupBox();
    scrollArea->setWidget(groupBox);

    QBoxLayout* layout = new QVBoxLayout();
    groupBox->setLayout(layout);

    layout->addStretch();

    addTriggerView(layout, groupBox);
    addTriggerView(layout, groupBox);
    addTriggerView(layout, groupBox);
    addTriggerView(layout, groupBox);

    adjustSize();

    description->setPlaceholderText(portConfig_.description().c_str());
    description->setText(portConfig_.user_description().c_str());
    switch(portConfig_.transmit_mode())
    {
    case OstProto::kSequentialTransmit:
        sequentialStreamsButton->setChecked(true);
        break;
    case OstProto::kInterleavedTransmit:
        interleavedStreamsButton->setChecked(true);
        break;
    default:
        Q_ASSERT(false); // Unreachable!!!
        break;
    }

    // Port Reservation
    myself_ = appSettings->value(kUserKey, kUserDefaultValue).toString();
    // XXX: what if myself_ is empty?
    if (currentUser.isEmpty()) {
        reservedBy_ = kNone;
        reservedBy->setText("Unreserved");
        reserveButton->setText("Reserve");
    }
    else if (currentUser == myself_) {
        reservedBy_ = kSelf;
        reservedBy->setText("Reserved by: me <i>("+currentUser+")</i>");
        reserveButton->setText("Reserve (uncheck to unreserve)");
        reserveButton->setChecked(true);
    }
    else {
        reservedBy_ = kOther;
        reservedBy->setText("Reserved by: <i>"+currentUser+"</i>");
        reserveButton->setText("Force reserve");
    }
    qDebug("reservedBy_ = %d", reservedBy_);

    exclusiveControlButton->setChecked(portConfig_.is_exclusive_control());
    streamStatsButton->setChecked(portConfig_.is_tracking_stream_stats());

    // Disable UI elements based on portState
    if (portState.is_transmit_on()) {
        transmitModeBox->setDisabled(true);
        streamStatsButton->setDisabled(true);
    }

    // Fill triggers tables
    fillTriggerView(portConfig_.user_trigger1(), 0);
    fillTriggerView(portConfig_.user_trigger2(), 1);
    fillTriggerView(portConfig_.user_trigger3(), 2);
    fillTriggerView(portConfig_.user_trigger4(), 3);

}

void PortConfigDialog::accept()
{
    OstProto::Port pc;

    pc.set_user_description(description->text().toStdString());

    if (sequentialStreamsButton->isChecked())
        pc.set_transmit_mode(OstProto::kSequentialTransmit);
    else if (interleavedStreamsButton->isChecked())
        pc.set_transmit_mode(OstProto::kInterleavedTransmit);
    else
        Q_ASSERT(false); // Unreachable!!!

    pc.set_user_name(portConfig_.user_name());
    switch (reservedBy_) {
        case kSelf:
            if (!reserveButton->isChecked())
                pc.set_user_name(""); // unreserve
            break;

        case kOther:
        case kNone:
            if (reserveButton->isChecked())
                pc.set_user_name(myself_.toStdString()); // (force) reserve
            break;

        default:
            qWarning("Unreachable code");
            break;
    }

    pc.set_is_exclusive_control(exclusiveControlButton->isChecked());
    pc.set_is_tracking_stream_stats(streamStatsButton->isChecked());

    // Update fields that have changed, clear the rest
    if (pc.user_description() != portConfig_.user_description())
        portConfig_.set_user_description(pc.user_description());
    else
        portConfig_.clear_user_description();

    if (pc.transmit_mode() != portConfig_.transmit_mode())
        portConfig_.set_transmit_mode(pc.transmit_mode());
    else
        portConfig_.clear_transmit_mode();

    if (pc.user_name() != portConfig_.user_name())
        portConfig_.set_user_name(pc.user_name());
    else
        portConfig_.clear_user_name();

    if (pc.is_exclusive_control() != portConfig_.is_exclusive_control())
        portConfig_.set_is_exclusive_control(pc.is_exclusive_control());
    else
        portConfig_.clear_is_exclusive_control();

    if (pc.is_tracking_stream_stats() != portConfig_.is_tracking_stream_stats())
        portConfig_.set_is_tracking_stream_stats(pc.is_tracking_stream_stats());
    else
        portConfig_.clear_is_tracking_stream_stats();

    createTriggerFromTriggerView(0, * portConfig_.mutable_user_trigger1());
    createTriggerFromTriggerView(1, * portConfig_.mutable_user_trigger2());
    createTriggerFromTriggerView(2, * portConfig_.mutable_user_trigger3());
    createTriggerFromTriggerView(3, * portConfig_.mutable_user_trigger4());


    QDialog::accept();
}

static void verticalResizeTableViewToContents(QTableView *tableView)
{
    int count = tableView->verticalHeader()->count();
    int scrollBarHeight = tableView->horizontalScrollBar()->height();
    int horizontalHeaderHeight = tableView->horizontalHeader()->height();
    int rowTotalHeight=0;
    for (int i = 0; i < count; ++i)
    {
        rowTotalHeight += tableView->verticalHeader()->sectionSize(i);
    }
    tableView->setMinimumHeight(horizontalHeaderHeight+rowTotalHeight+scrollBarHeight);
}

void PortConfigDialog::fillTriggerView(const ::OstProto::Trigger& trigger, int triggerIndex)
{
    int termsSize = trigger.terms_size();
    if (termsSize > 0)
    {
        for (int t=0; t < termsSize; ++t)
        {
                OstProto::TriggerTerm triggerTerm = trigger.terms(t);
                int offset = triggerTerm.offset();
                std::string pattern = triggerTerm.pattern();
                std::string mask = triggerTerm.mask();
                bool isNot = triggerTerm.is_not();

                addTriggerTableRow( m_triggersViews[triggerIndex].m_triggerTableWidget );
                ((QLineEdit*) m_triggersViews[triggerIndex].m_triggerTableWidget->cellWidget(t, 0))->setText( pattern.c_str() );
                ((QLineEdit*) m_triggersViews[triggerIndex].m_triggerTableWidget->cellWidget(t, 1))->setText( mask.c_str() );
                ((QSpinBox*) m_triggersViews[triggerIndex].m_triggerTableWidget->cellWidget(t, 2))->setValue(offset);
                QCheckBox* isNotCheckBox = (dynamic_cast<QCheckBox*>(
                    m_triggersViews[triggerIndex].m_triggerTableWidget->cellWidget(t, 3)->layout()->itemAt(0)->widget()));
                isNotCheckBox->setChecked(isNot);
        }
    }
}

void PortConfigDialog::createTriggerFromTriggerView(int triggerIndex, ::OstProto::Trigger& trigger) const
{
    trigger.clear_terms();

    QTableWidget* triggerTable = m_triggersViews[triggerIndex].m_triggerTableWidget;
    for (int r=0; r < triggerTable->rowCount(); ++r)
    {
        QString pattern = ((QLineEdit*) m_triggersViews[triggerIndex].m_triggerTableWidget->cellWidget(r, 0))->text();
        QString mask = ((QLineEdit*) m_triggersViews[triggerIndex].m_triggerTableWidget->cellWidget(r, 1))->text();
        int offset = ((QSpinBox*) m_triggersViews[triggerIndex].m_triggerTableWidget->cellWidget(r, 2))->value();
        QCheckBox* isNotCheckBox = (dynamic_cast<QCheckBox*>(
            m_triggersViews[triggerIndex].m_triggerTableWidget->cellWidget(r, 3)->layout()->itemAt(0)->widget()));
        bool isNot = isNotCheckBox->isChecked();

        ::OstProto::TriggerTerm* term = trigger.add_terms();
        term->set_pattern( pattern.toStdString() );
        term->set_mask( mask.toStdString() );
        term->set_offset(offset);
        term->set_is_not(isNot);
    }
}

void PortConfigDialog::testOKcall(bool a)
{
    qDebug("In %s", __FUNCTION__);
}

void PortConfigDialog::addTriggerView(QBoxLayout* layout, QGroupBox* parent)
{
    int triggerNumber = m_triggersViews.size() + 1;

    QGroupBox* headerGroup = new QGroupBox();
    layout->addWidget(headerGroup);
    QBoxLayout* headersLayout = new QHBoxLayout();
    headersLayout->setAlignment(Qt::AlignLeft);
    headerGroup->setLayout(headersLayout);

    QLabel* title = new QLabel(parent);
    title->setTextFormat(Qt::RichText);
    title->setText("<b>Trigger" + QString::number(triggerNumber) + "</b>");
    //layout->addWidget(title);
    headersLayout->addWidget(title);

    QPushButton* addTermButton = new QPushButton(parent);
    addTermButton->setText("Add");
    addTermButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    QPushButton* deleteTermButton = new QPushButton(parent);
    deleteTermButton->setText("Delete");
    deleteTermButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    headersLayout->addWidget(addTermButton);
    headersLayout->addWidget(deleteTermButton);

    connect(addTermButton, SIGNAL(clicked()), this, SLOT(addTerm()));
    connect(deleteTermButton, SIGNAL(clicked()), this, SLOT(deleteTerms()));
    connect(deleteTermButton, SIGNAL(clicked()),description, SLOT(selectAll()));

    QTableWidget* tableWidget = new QTableWidget(parent);
    tableWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    layout->addWidget(tableWidget);

    //tableWidget->verticalScrollBar()->setDisabled(true);
    tableWidget->setColumnCount(4);

    tableWidget->horizontalHeader()->setSectionResizeMode( 0, QHeaderView::Stretch );
    tableWidget->horizontalHeader()->setSectionResizeMode( 1, QHeaderView::Stretch );
    tableWidget->horizontalHeader()->setSectionResizeMode( 2, QHeaderView::Stretch );
    tableWidget->horizontalHeader()->setSectionResizeMode( 3, QHeaderView::ResizeToContents );
    tableWidget->horizontalHeader()->setHighlightSections(false);

    QStringList headers;
    headers << "Pattern" << "Mask" << "Offset" << "Is Not" << "";
    tableWidget->setHorizontalHeaderLabels(headers);

    tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    tableWidget->setSelectionMode(QAbstractItemView::MultiSelection);

    //addTriggerTableRow(tableWidget);

    TriggerView triggerView;
    triggerView.m_addTermButton = addTermButton;
    triggerView.m_deleteTermButton = deleteTermButton;
    triggerView.m_triggerTableWidget = tableWidget;

    m_triggersViews.push_back(triggerView);

    verticalResizeTableViewToContents(tableWidget);
}

void PortConfigDialog::addTriggerTableRow(QTableWidget* tableWidget)
{
    int row = tableWidget->rowCount();

    tableWidget->insertRow(row);

    //QTableWidgetItem* pattern = new QTableWidgetItem("");
    //tableWidget->setItem(row, 0, pattern);
    QRegExp hexRegEx("([0-9a-fA-F ]{2})*");

    QLineEdit* pattern = new  QLineEdit();
    QValidator *validator = new QRegExpValidator(hexRegEx, pattern);
    pattern->setValidator(validator);
    pattern->setFrame(false);
    tableWidget->setCellWidget(row, 0, pattern);

    /*
    QWidget* w0 = new QWidget();
    QHBoxLayout* l0 = new QHBoxLayout();
    l0->setAlignment( Qt::AlignCenter );
    l0->addWidget(pattern);
    w0->setLayout( l0 );
    tableWidget->setCellWidget(row, 0, w0);*/

    QLineEdit* mask = new  QLineEdit();
    validator = new QRegExpValidator(hexRegEx, mask);
    mask->setValidator(validator);
    mask->setFrame(false);
    tableWidget->setCellWidget(row, 1, mask);

    //QTableWidgetItem* mask = new QTableWidgetItem("");
    //tableWidget->setItem(row, 1, mask);

    //QWidget * w = new QWidget();
    //QHBoxLayout *l = new QHBoxLayout();
    QSpinBox* spinBox = new  QSpinBox();
    spinBox->setMaximum(999999999);
    spinBox->setFrame(false);
    //l->addWidget(spinBox);
    //w->setLayout( l );
    tableWidget->setCellWidget(row, 2, spinBox);

    QWidget* w = new QWidget();
    QHBoxLayout* l = new QHBoxLayout();
    l->setAlignment( Qt::AlignCenter );
    l->addWidget( new  QCheckBox());
    w->setLayout( l );
    tableWidget->setCellWidget(row, 3, w);

    verticalResizeTableViewToContents(tableWidget);
}

void PortConfigDialog::deleteTriggerTableRows(QTableWidget* tableWidget)
{
    QItemSelectionModel *selection = tableWidget->selectionModel();

    if (selection->hasSelection() )
    {
        QModelIndexList selectedRows = selection->selectedIndexes();
        typedef std::set<int> RowsSet;
        RowsSet sortedSelectedRows;
        for (QModelIndexList::const_iterator indexIter = selectedRows.begin(); indexIter != selectedRows.end();
             ++indexIter)
        {
                sortedSelectedRows.insert( indexIter->row() );
        }

        // Removed rows from the higher to lower in order to preserve the row numbering
        for (RowsSet::reverse_iterator selectedRowRIter = sortedSelectedRows.rbegin(); selectedRowRIter != sortedSelectedRows.rend();
             ++selectedRowRIter)
        {
                tableWidget->removeRow(*selectedRowRIter);
        }
    }

    verticalResizeTableViewToContents(tableWidget);
}

const PortConfigDialog::TriggerView* PortConfigDialog::findTriggerViewBySender(QObject* sender) const
{
    for (TriggerViewList::const_iterator triggerViewIter = m_triggersViews.begin(); triggerViewIter != m_triggersViews.end();
         ++triggerViewIter)
    {
        if (sender == triggerViewIter->m_addTermButton || sender == triggerViewIter->m_deleteTermButton ||
            sender == triggerViewIter->m_triggerTableWidget)
        {
                return & *triggerViewIter;
        }
    }

    return 0;
}

void PortConfigDialog::addTerm()
{
    QObject* _sender = sender();
    QTableWidget* triggerTableWidget = findTriggerViewBySender(_sender)->m_triggerTableWidget;

    addTriggerTableRow(triggerTableWidget);
}

void PortConfigDialog::deleteTerms()
{
    QObject* _sender = sender();
    QTableWidget* triggerTableWidget = findTriggerViewBySender(_sender)->m_triggerTableWidget;

    deleteTriggerTableRows(triggerTableWidget);
}

