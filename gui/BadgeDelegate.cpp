#include "gui/BadgeDelegate.h"

#include <QApplication>
#include <QPainter>
#include <QPainterPath>

namespace scapms::gui {

BadgeDelegate::BadgeDelegate(QObject* parent) : QStyledItemDelegate(parent) {}

const QHash<QString, QColor>& BadgeDelegate::palette() {
    static const QHash<QString, QColor> colors = {
        {QStringLiteral("Emergency"), QColor(0xC6, 0x28, 0x28)},
        {QStringLiteral("High Priority"), QColor(0xC6, 0x28, 0x28)},
        {QStringLiteral("General"), QColor(0x1E, 0x63, 0xB5)},
        {QStringLiteral("Normal"), QColor(0x1E, 0x63, 0xB5)},
        {QStringLiteral("Follow-Up"), QColor(0x6A, 0x3D, 0xB8)},
        {QStringLiteral("Scheduled"), QColor(0x6A, 0x3D, 0xB8)},
        {QStringLiteral("Pending"), QColor(0xC7, 0x7C, 0x02)},
        {QStringLiteral("Completed"), QColor(0x2E, 0x7D, 0x32)},
        {QStringLiteral("Cancelled"), QColor(0x75, 0x75, 0x75)},
    };
    return colors;
}

QColor BadgeDelegate::colorFor(const QString& value) {
    return palette().value(value, QColor());
}

void BadgeDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option,
                          const QModelIndex& index) const {
    const QString text = index.data(Qt::DisplayRole).toString();
    const QColor color = colorFor(text);
    if (!color.isValid()) {                       // not a badge value: default rendering
        QStyledItemDelegate::paint(painter, option, index);
        return;
    }

    // Draw the normal background (selection / alternating colour) without text.
    QStyleOptionViewItem background(option);
    initStyleOption(&background, index);
    background.text.clear();
    const QWidget* widget = option.widget;
    QStyle* style = widget ? widget->style() : QApplication::style();
    style->drawControl(QStyle::CE_ItemViewItem, &background, painter, widget);

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    QFont font = option.font;
    font.setBold(true);
    if (font.pointSizeF() > 0) font.setPointSizeF(font.pointSizeF() * 0.9);
    painter->setFont(font);

    const QFontMetrics metrics(font);
    const int width = metrics.horizontalAdvance(text) + 18;
    const int height = metrics.height() + 6;
    const QRect pill(option.rect.left() + 6, option.rect.center().y() - height / 2 + 1, width, height);

    QPainterPath path;
    path.addRoundedRect(pill, height / 2.0, height / 2.0);
    painter->fillPath(path, color);
    painter->setPen(Qt::white);
    painter->drawText(pill, Qt::AlignCenter, text);
    painter->restore();
}

QSize BadgeDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const {
    QSize size = QStyledItemDelegate::sizeHint(option, index);
    if (colorFor(index.data(Qt::DisplayRole).toString()).isValid()) size.rwidth() += 28;
    return size;
}

} // namespace scapms::gui
