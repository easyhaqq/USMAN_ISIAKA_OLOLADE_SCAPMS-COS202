#ifndef SCAPMS_GUI_GUIHELPERS_H
#define SCAPMS_GUI_GUIHELPERS_H

#include <QString>
#include <QStringList>

#include <functional>
#include <string>

class QTableWidget;
class QWidget;

namespace scapms::gui {

inline QString qs(const std::string& text) { return QString::fromStdString(text); }
inline std::string ss(const QString& text) { return text.trimmed().toStdString(); }

// Runs an action inside try/catch and shows a meaningful dialog on failure,
// so a user error never terminates the program. Returns true on success.
bool runGuarded(QWidget* parent, const std::function<void()>& action);

// Read-only, row-selecting table with the given column headers.
QTableWidget* createTable(const QStringList& headers, QWidget* parent);

// Fills one table row. `key` (e.g. a patient ID) is stored in column 0 so the
// selected record can be found again regardless of sorting or filtering.
void setRow(QTableWidget* table, int row, const QStringList& values, const QString& key);
QString selectedKey(const QTableWidget* table);

} // namespace scapms::gui

#endif // SCAPMS_GUI_GUIHELPERS_H
