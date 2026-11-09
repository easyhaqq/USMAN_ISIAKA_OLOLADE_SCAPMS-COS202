#include "gui/ReportPanel.h"

#include "controller/ClinicManager.h"
#include "gui/BadgeDelegate.h"
#include "gui/GuiHelpers.h"
#include "utils/FileHandler.h"

#include <QComboBox>
#include <QDateEdit>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

namespace scapms::gui {

using controller::ReportType;

ReportPanel::ReportPanel(controller::ClinicManager& manager, QWidget* parent) : Panel(manager, parent) {
    setObjectName(QStringLiteral("reportPanel"));

    reportBox_ = new QComboBox;
    for (auto type : controller::allReportTypes()) reportBox_->addItem(qs(controller::reportName(type)), static_cast<int>(type));
    reportBox_->setToolTip(tr("Choose the report to generate."));
    dateLabel_ = new QLabel(tr("Date:"));
    dateEdit_ = new QDateEdit(QDate::currentDate());
    dateEdit_->setCalendarPopup(true);
    dateEdit_->setDisplayFormat(QStringLiteral("yyyy-MM-dd"));
    dateEdit_->setToolTip(tr("Day to summarise (Daily Appointment Summary only)."));
    auto* generateButton = new QPushButton(tr("Generate Report"));
    generateButton->setObjectName(QStringLiteral("primaryButton"));
    generateButton->setToolTip(tr("Build the selected report from the current data."));
    auto* exportButton = new QPushButton(tr("Export..."));
    exportButton->setToolTip(tr("Save the report as a plain-text file."));

    auto* controls = new QHBoxLayout;
    controls->addWidget(new QLabel(tr("Report:")));
    controls->addWidget(reportBox_, 1);
    controls->addWidget(dateLabel_);
    controls->addWidget(dateEdit_);
    controls->addWidget(generateButton);
    controls->addWidget(exportButton);

    titleLabel_ = new QLabel;
    titleLabel_->setObjectName(QStringLiteral("sectionTitle"));
    table_ = createTable({}, this);
    table_->setObjectName(QStringLiteral("reportTable"));
    table_->setSelectionMode(QAbstractItemView::NoSelection);
    badges_ = new BadgeDelegate(table_);
    summaryLabel_ = new QLabel;
    summaryLabel_->setObjectName(QStringLiteral("summaryBox"));
    summaryLabel_->setWordWrap(true);
    summaryLabel_->setTextInteractionFlags(Qt::TextSelectableByMouse);

    auto* layout = new QVBoxLayout(this);
    layout->addLayout(controls);
    layout->addWidget(titleLabel_);
    layout->addWidget(table_, 1);
    layout->addWidget(summaryLabel_);

    connect(generateButton, &QPushButton::clicked, this, &ReportPanel::generate);
    connect(exportButton, &QPushButton::clicked, this, &ReportPanel::exportReport);
    connect(reportBox_, qOverload<int>(&QComboBox::currentIndexChanged), this, &ReportPanel::onReportChanged);
    connect(dateEdit_, &QDateEdit::dateChanged, this, &ReportPanel::generate);

    onReportChanged();
}

void ReportPanel::onReportChanged() {
    const bool daily = static_cast<ReportType>(reportBox_->currentData().toInt()) == ReportType::DailySummary;
    dateLabel_->setVisible(daily);   // the date picker only appears when it is relevant
    dateEdit_->setVisible(daily);
    generate();
}

void ReportPanel::refresh() {
    generate();
}

void ReportPanel::generate() {
    runGuarded(this, [this] {
        const auto type = static_cast<ReportType>(reportBox_->currentData().toInt());
        render(manager_.report(type, ss(dateEdit_->date().toString(QStringLiteral("yyyy-MM-dd")))));
    });
}

void ReportPanel::render(const controller::Report& report) {
    current_ = report;
    titleLabel_->setText(qs(report.title));

    QStringList headers;
    for (const auto& h : report.headers) headers << qs(h);
    table_->setRowCount(0);   // also deletes progress bars from a previous report
    table_->setColumnCount(headers.size());
    table_->setHorizontalHeaderLabels(headers);
    table_->setRowCount(static_cast<int>(report.rows.size()));

    // Columns holding category/status values get coloured badges.
    for (int c = 0; c < headers.size(); ++c) {
        const bool badge = headers[c] == QLatin1String("Category") || headers[c] == QLatin1String("Status");
        table_->setItemDelegateForColumn(c, badge ? badges_ : nullptr);
    }

    const bool distribution = headers.contains(QStringLiteral("Chart"));
    for (int r = 0; r < static_cast<int>(report.rows.size()); ++r) {
        QStringList values;
        for (const auto& cell : report.rows[static_cast<std::size_t>(r)]) values << qs(cell);
        if (distribution) {
            // Replace the text bar with a real progress bar widget.
            const int chartColumn = headers.indexOf(QStringLiteral("Chart"));
            const int share = qRound(values.value(headers.indexOf(QStringLiteral("Share"))).chopped(1).toDouble());
            values[chartColumn].clear();
            auto* bar = new QProgressBar;
            bar->setRange(0, 100);
            bar->setValue(share);
            bar->setTextVisible(false);   // the Share column already shows the value
            table_->setCellWidget(r, chartColumn, bar);
        }
        setRow(table_, r, values, values.value(0));
    }
    table_->resizeColumnsToContents();
    if (distribution) table_->setColumnWidth(headers.indexOf(QStringLiteral("Chart")), 320);

    QStringList summary;
    for (const auto& line : report.summary) summary << qs(line).toHtmlEscaped().replace(QStringLiteral("  "), QStringLiteral("&nbsp;&nbsp;&nbsp;&nbsp;"));
    summaryLabel_->setText(summary.join(QStringLiteral("<br>")));
}

void ReportPanel::exportReport() {
    QString suggested = qs(current_.title).section(QLatin1Char(' '), 0, 3).replace(QLatin1Char(' '), QLatin1Char('_'));
    const QString path = QFileDialog::getSaveFileName(this, tr("Export Report"), suggested + QStringLiteral(".txt"),
                                                      tr("Text files (*.txt);;All files (*)"));
    if (path.isEmpty()) return;
    runGuarded(this, [&] {
        utils::FileHandler::writeText(ss(path), current_.toText());
        emit statusMessage(tr("Exported \"%1\" to %2").arg(qs(current_.title), path));
    });
}

} // namespace scapms::gui
