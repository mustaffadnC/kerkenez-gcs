#include "ui/ParamDialog.h"

#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

#include "core/ParamController.h"

namespace kerkenez {

ParamDialog::ParamDialog(ParamController *controller, QWidget *parent)
    : QDialog(parent)
    , m_controller(controller)
{
    setWindowTitle(tr("Parameters"));
    resize(560, 620);

    m_filter = new QLineEdit(this);
    m_filter->setPlaceholderText(tr("Filter, e.g. WPNAV or BATT"));
    auto *refresh = new QPushButton(tr("Refresh"), this);

    m_table = new QTableWidget(0, 2, this);
    m_table->setHorizontalHeaderLabels({tr("Name"), tr("Value")});
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_table->verticalHeader()->setVisible(false);

    m_status = new QLabel(tr("Press Refresh to download the parameter set."), this);
    m_status->setStyleSheet(QStringLiteral("color: gray;"));

    auto *topRow = new QHBoxLayout;
    topRow->addWidget(m_filter, 1);
    topRow->addWidget(refresh);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(topRow);
    layout->addWidget(m_table, 1);
    layout->addWidget(m_status);

    connect(refresh, &QPushButton::clicked, m_controller, &ParamController::refresh);
    connect(m_filter, &QLineEdit::textChanged, this, &ParamDialog::applyFilter);
    connect(m_controller, &ParamController::progress, this, [this](int received, int total) {
        m_status->setText(tr("Downloading %1/%2…").arg(received).arg(total));
    });
    connect(m_controller, &ParamController::refreshFinished, this, [this](int count) {
        m_status->setText(tr("%1 parameters loaded.").arg(count));
        rebuild();
    });
    connect(m_controller, &ParamController::failed, this, [this](const QString &reason) {
        m_status->setText(tr("Parameter download incomplete: %1").arg(reason));
        rebuild();
    });

    connect(m_table, &QTableWidget::itemChanged, this, [this](QTableWidgetItem *item) {
        if (m_updatingTable || item->column() != 1)
            return;
        bool ok = false;
        const float value = item->text().toFloat(&ok);
        const QString name = m_table->item(item->row(), 0)->text();
        if (!ok) {
            m_status->setText(tr("“%1” is not a number.").arg(item->text()));
            rebuild();
            return;
        }
        m_controller->setParameter(name, value);
        m_status->setText(tr("Wrote %1 = %2").arg(name).arg(double(value)));
    });

    if (!m_controller->parameters().isEmpty())
        rebuild();
}

void ParamDialog::rebuild()
{
    const QMap<QString, float> values = m_controller->parameters();
    m_updatingTable = true;
    m_table->setRowCount(values.size());
    int row = 0;
    for (auto it = values.constBegin(); it != values.constEnd(); ++it, ++row) {
        auto *name = new QTableWidgetItem(it.key());
        name->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        auto *value = new QTableWidgetItem(QString::number(double(it.value()), 'g', 8));
        m_table->setItem(row, 0, name);
        m_table->setItem(row, 1, value);
    }
    m_updatingTable = false;
    applyFilter();
}

void ParamDialog::applyFilter()
{
    const QString needle = m_filter->text().trimmed();
    for (int row = 0; row < m_table->rowCount(); ++row) {
        const QTableWidgetItem *name = m_table->item(row, 0);
        const bool visible = needle.isEmpty()
            || (name && name->text().contains(needle, Qt::CaseInsensitive));
        m_table->setRowHidden(row, !visible);
    }
}

} // namespace kerkenez
