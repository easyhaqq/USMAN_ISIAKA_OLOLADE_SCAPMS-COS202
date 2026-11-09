#include "gui/GuiHelpers.h"

#include "utils/Exceptions.h"

#include <QHeaderView>
#include <QMessageBox>
#include <QTableWidget>

namespace scapms::gui {

bool runGuarded(QWidget* parent, const std::function<void()>& action) {
    try {
        action();
        return true;
    } catch (const utils::ClinicException& error) {          // expected, user-correctable errors
        QMessageBox::warning(parent, qs(error.title()), QStringLiteral("Error: ") + qs(error.what()));
    } catch (const std::exception& error) {                  // library errors, e.g. bad_alloc
        QMessageBox::critical(parent, QStringLiteral("Unexpected Error"),
                              QStringLiteral("The operation could not be completed:\n") + qs(error.what()));
    } catch (...) {
        QMessageBox::critical(parent, QStringLiteral("Unexpected Error"),
                              QStringLiteral("An unknown error occurred. The operation was cancelled."));
    }
    return false;
}

QTableWidget* createTable(const QStringList& headers, QWidget* parent) {
    auto* table = new QTableWidget(0, headers.size(), parent);
    table->setHorizontalHeaderLabels(headers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setAlternatingRowColors(true);
    table->setWordWrap(false);
    table->verticalHeader()->setVisible(false);
    table->verticalHeader()->setDefaultSectionSize(30);
    table->horizontalHeader()->setStretchLastSection(true);
    table->horizontalHeader()->setHighlightSections(false);
    table->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    return table;
}

void setRow(QTableWidget* table, int row, const QStringList& values, const QString& key) {
    for (int column = 0; column < values.size() && column < table->columnCount(); ++column) {
        auto* item = new QTableWidgetItem(values[column]);
        if (column == 0) item->setData(Qt::UserRole, key);
        table->setItem(row, column, item);
    }
}

QString selectedKey(const QTableWidget* table) {
    const int row = table->currentRow();
    if (row < 0 || !table->item(row, 0) || !table->item(row, 0)->isSelected()) return {};
    return table->item(row, 0)->data(Qt::UserRole).toString();
}

} // namespace scapms::gui
