#ifndef SCAPMS_GUI_BADGEDELEGATE_H
#define SCAPMS_GUI_BADGEDELEGATE_H

#include <QColor>
#include <QHash>
#include <QStyledItemDelegate>

namespace scapms::gui {

// Custom table rendering: draws known values (patient category, priority,
// appointment status) as coloured "pills" so they can be told apart at a glance.
//   Emergency / High Priority -> red     General / Normal -> blue
//   Follow-Up / Scheduled     -> purple  Pending -> amber, Completed -> green, Cancelled -> grey
class BadgeDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    explicit BadgeDelegate(QObject* parent = nullptr);

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;

    static QColor colorFor(const QString& value);   // invalid colour if not a badge value

private:
    static const QHash<QString, QColor>& palette();
};

} // namespace scapms::gui

#endif // SCAPMS_GUI_BADGEDELEGATE_H
