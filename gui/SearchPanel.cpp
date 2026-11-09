#include "gui/SearchPanel.h"

#include "controller/ClinicManager.h"
#include "gui/BadgeDelegate.h"
#include "gui/GuiHelpers.h"
#include "utils/Exceptions.h"

#include <QCheckBox>
#include <QComboBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

namespace scapms::gui {

using controller::SearchAlgorithm;
using controller::SearchField;
using controller::SortAlgorithm;
using model::SortKey;

namespace {
enum Column { ColId, ColName, ColAge, ColCategory, ColPriority, ColNext, ColDetails };
}

SearchPanel::SearchPanel(controller::ClinicManager& manager, QWidget* parent) : Panel(manager, parent) {
    setObjectName(QStringLiteral("searchPanel"));

    // ---------------------------------------------------------- search box
    auto* searchBox = new QGroupBox(tr("Search Patients"));
    fieldBox_ = new QComboBox;
    fieldBox_->addItem(tr("Patient ID"), static_cast<int>(SearchField::Id));
    fieldBox_->addItem(tr("Patient Name"), static_cast<int>(SearchField::Name));
    fieldBox_->addItem(tr("Category"), static_cast<int>(SearchField::Category));
    fieldBox_->setCurrentIndex(1);
    fieldBox_->setToolTip(tr("Field to search."));

    searchAlgorithmBox_ = new QComboBox;
    for (auto algorithm : {SearchAlgorithm::Auto, SearchAlgorithm::Linear, SearchAlgorithm::Binary,
                           SearchAlgorithm::Recursive}) {
        searchAlgorithmBox_->addItem(qs(controller::searchAlgorithmName(algorithm)), static_cast<int>(algorithm));
    }
    searchAlgorithmBox_->setToolTip(tr("Auto uses binary search when the list is sorted on the searched field, "
                                       "otherwise linear (ID/category) or recursive (name) search."));

    keywordEdit_ = new QLineEdit;
    keywordEdit_->setObjectName(QStringLiteral("keywordEdit"));
    keywordEdit_->setClearButtonEnabled(true);
    keywordEdit_->setToolTip(tr("Keyword to search for (Ctrl+F)."));
    auto* searchButton = new QPushButton(tr("Search"));
    searchButton->setObjectName(QStringLiteral("primaryButton"));
    searchButton->setToolTip(tr("Run the search (Enter)."));
    auto* showAllButton = new QPushButton(tr("Show All"));
    showAllButton->setToolTip(tr("Clear the search and list every patient in the current order."));
    liveSearch_ = new QCheckBox(tr("Search as I type"));
    liveSearch_->setChecked(true);
    liveSearch_->setToolTip(tr("Re-run the search every time the keyword changes."));

    searchInfo_ = new QLabel;
    searchInfo_->setObjectName(QStringLiteral("infoLabel"));
    searchInfo_->setWordWrap(true);

    auto* searchGrid = new QGridLayout(searchBox);
    searchGrid->addWidget(new QLabel(tr("Search by:")), 0, 0);
    searchGrid->addWidget(fieldBox_, 0, 1);
    searchGrid->addWidget(new QLabel(tr("Algorithm:")), 0, 2);
    searchGrid->addWidget(searchAlgorithmBox_, 0, 3);
    searchGrid->addWidget(new QLabel(tr("Keyword:")), 1, 0);
    searchGrid->addWidget(keywordEdit_, 1, 1, 1, 3);
    auto* searchButtons = new QHBoxLayout;
    searchButtons->addWidget(liveSearch_);
    searchButtons->addStretch();
    searchButtons->addWidget(showAllButton);
    searchButtons->addWidget(searchButton);
    searchGrid->addLayout(searchButtons, 2, 0, 1, 4);
    searchGrid->addWidget(searchInfo_, 3, 0, 1, 4);
    searchGrid->setColumnStretch(1, 1);
    searchGrid->setColumnStretch(3, 1);

    // ------------------------------------------------------------ sort box
    auto* sortBox = new QGroupBox(tr("Sort Patient Records"));
    sortAlgorithmBox_ = new QComboBox;
    for (auto algorithm : controller::allSortAlgorithms()) {
        sortAlgorithmBox_->addItem(qs(controller::algorithmName(algorithm)), static_cast<int>(algorithm));
    }
    sortAlgorithmBox_->setToolTip(tr("Sorting algorithm (all implemented by hand, no std::sort)."));
    sortKeyBox_ = new QComboBox;
    for (auto key : {SortKey::Name, SortKey::Id, SortKey::Age, SortKey::AppointmentDate}) {
        sortKeyBox_->addItem(qs(model::sortKeyToString(key)), static_cast<int>(key));
    }
    sortKeyBox_->setToolTip(tr("Field to sort by (ascending)."));
    auto* sortButton = new QPushButton(tr("SORT"));
    sortButton->setObjectName(QStringLiteral("primaryButton"));
    sortButton->setToolTip(tr("Sort the patient list in place. Sorting by ID or name enables binary search."));

    rationaleLabel_ = new QLabel;
    rationaleLabel_->setObjectName(QStringLiteral("hintLabel"));
    rationaleLabel_->setWordWrap(true);
    recommendationLabel_ = new QLabel;
    recommendationLabel_->setObjectName(QStringLiteral("infoLabel"));
    recommendationLabel_->setWordWrap(true);
    sortResult_ = new QLabel;
    sortResult_->setWordWrap(true);

    auto* sortGrid = new QGridLayout(sortBox);
    sortGrid->addWidget(new QLabel(tr("Sort Algorithm:")), 0, 0);
    sortGrid->addWidget(sortAlgorithmBox_, 0, 1);
    sortGrid->addWidget(new QLabel(tr("Sort By:")), 0, 2);
    sortGrid->addWidget(sortKeyBox_, 0, 3);
    sortGrid->addWidget(sortButton, 0, 4);
    sortGrid->addWidget(rationaleLabel_, 1, 0, 1, 5);
    sortGrid->addWidget(recommendationLabel_, 2, 0, 1, 5);
    sortGrid->addWidget(sortResult_, 3, 0, 1, 5);
    sortGrid->setColumnStretch(1, 1);
    sortGrid->setColumnStretch(3, 1);

    auto* top = new QHBoxLayout;
    top->addWidget(searchBox, 1);
    top->addWidget(sortBox, 1);

    // --------------------------------------------------------- results
    results_ = createTable({tr("Patient ID"), tr("Name"), tr("Age"), tr("Category"), tr("Priority"),
                            tr("Next Appointment"), tr("Details")}, this);
    results_->setObjectName(QStringLiteral("searchResults"));
    auto* badges = new BadgeDelegate(results_);
    results_->setItemDelegateForColumn(ColCategory, badges);
    results_->setItemDelegateForColumn(ColPriority, badges);
    orderLabel_ = new QLabel;
    orderLabel_->setObjectName(QStringLiteral("hintLabel"));

    auto* layout = new QVBoxLayout(this);
    layout->addLayout(top);
    layout->addWidget(results_, 1);
    layout->addWidget(orderLabel_);

    connect(searchButton, &QPushButton::clicked, this, &SearchPanel::runSearch);
    connect(keywordEdit_, &QLineEdit::returnPressed, this, &SearchPanel::runSearch);
    connect(keywordEdit_, &QLineEdit::textChanged, this, &SearchPanel::onKeywordEdited);
    connect(fieldBox_, qOverload<int>(&QComboBox::currentIndexChanged), this, &SearchPanel::onSearchFieldChanged);
    connect(searchAlgorithmBox_, qOverload<int>(&QComboBox::currentIndexChanged), this, &SearchPanel::onKeywordEdited);
    connect(showAllButton, &QPushButton::clicked, this, [this] {
        keywordEdit_->clear();
        showAllPatients();
    });
    connect(sortButton, &QPushButton::clicked, this, &SearchPanel::runSort);
    connect(sortAlgorithmBox_, qOverload<int>(&QComboBox::currentIndexChanged), this, &SearchPanel::updateSortAdvice);
    connect(sortKeyBox_, qOverload<int>(&QComboBox::currentIndexChanged), this, &SearchPanel::updateSortAdvice);

    onSearchFieldChanged();
    refresh();
}

void SearchPanel::focusSearch() {
    keywordEdit_->setFocus();
    keywordEdit_->selectAll();
}

void SearchPanel::refresh() {
    if (showingSearch_ && !keywordEdit_->text().trimmed().isEmpty()) performSearch(false);
    else showAllPatients();
    orderLabel_->setText(tr("Current order of the patient list: %1").arg(qs(model::sortKeyToString(manager_.sortedBy()))));
    updateSortAdvice();
}

void SearchPanel::onSearchFieldChanged() {
    switch (static_cast<SearchField>(fieldBox_->currentData().toInt())) {
        case SearchField::Id:       keywordEdit_->setPlaceholderText(tr("e.g. PAT-0007 or just 7")); break;
        case SearchField::Name:     keywordEdit_->setPlaceholderText(tr("e.g. ami  (part of a name)")); break;
        case SearchField::Category: keywordEdit_->setPlaceholderText(tr("General, Emergency or Follow-Up")); break;
    }
    onKeywordEdited();
}

void SearchPanel::onKeywordEdited() {
    if (keywordEdit_->text().trimmed().isEmpty()) {
        searchInfo_->setText(tr("Enter a keyword. Choose Auto to let the system pick the algorithm."));
        searchInfo_->setStyleSheet(QString());
        if (showingSearch_) showAllPatients();
        return;
    }
    if (liveSearch_->isChecked()) performSearch(false);
}

void SearchPanel::runSearch() {
    performSearch(true);
}

void SearchPanel::performSearch(bool interactive) {
    const auto field = static_cast<SearchField>(fieldBox_->currentData().toInt());
    const auto algorithm = static_cast<SearchAlgorithm>(searchAlgorithmBox_->currentData().toInt());
    const std::string keyword = ss(keywordEdit_->text());

    try {
        const controller::SearchResult result = manager_.search(field, keyword, algorithm);
        showPatients(result.matches);
        showingSearch_ = true;
        searchInfo_->setStyleSheet(QString());
        searchInfo_->setText(tr("<b>%1</b>: %2 match(es), %3 comparison(s).<br>%4")
                                 .arg(qs(result.algorithmUsed)).arg(result.matches.size())
                                 .arg(result.comparisons).arg(qs(result.explanation)));
        if (interactive) {
            emit statusMessage(tr("%1 for \"%2\" found %3 patient(s).")
                                   .arg(qs(result.algorithmUsed), keywordEdit_->text().trimmed())
                                   .arg(result.matches.size()));
        }
    } catch (const utils::ClinicException& error) {
        // While typing, show the problem inline; on an explicit search, show a dialog as well.
        searchInfo_->setStyleSheet(QStringLiteral("color:#c62828;"));
        searchInfo_->setText(qs(error.what()));
        if (interactive) QMessageBox::warning(this, qs(error.title()), tr("Error: ") + qs(error.what()));
    }
}

void SearchPanel::showPatients(const std::vector<const model::PatientRecord*>& patients) {
    results_->setRowCount(static_cast<int>(patients.size()));
    int row = 0;
    for (const auto* p : patients) {
        const std::string next = manager_.database().nextAppointmentDate(p->id());
        setRow(results_, row++,
               {qs(p->id()), qs(p->name()), QString::number(p->age()), qs(p->categoryName()), qs(p->priorityLabel()),
                next.empty() ? tr("-") : qs(next), qs(p->details())},
               qs(p->id()));
    }
    results_->resizeColumnsToContents();
}

void SearchPanel::showAllPatients() {
    std::vector<const model::PatientRecord*> all;
    for (const auto& p : manager_.patients()) all.push_back(p.get());
    showPatients(all);
    showingSearch_ = false;
}

void SearchPanel::updateSortAdvice() {
    const auto algorithm = static_cast<SortAlgorithm>(sortAlgorithmBox_->currentData().toInt());
    const auto key = static_cast<SortKey>(sortKeyBox_->currentData().toInt());
    rationaleLabel_->setText(tr("<b>%1</b> - %2.<br>%3")
                                 .arg(qs(controller::algorithmName(algorithm)), qs(controller::algorithmComplexity(algorithm)),
                                      qs(controller::algorithmRationale(algorithm))));
    const controller::SortRecommendation advice = manager_.recommendSort(key);
    recommendationLabel_->setText(tr("Recommended for the current data: <b>%1</b>. %2")
                                      .arg(qs(controller::algorithmName(advice.algorithm)), qs(advice.reason)));
}

void SearchPanel::runSort() {
    const auto algorithm = static_cast<SortAlgorithm>(sortAlgorithmBox_->currentData().toInt());
    const auto key = static_cast<SortKey>(sortKeyBox_->currentData().toInt());
    runGuarded(this, [&] {
        const controller::SortStats stats = manager_.sortPatients(key, algorithm);
        const QString message = tr("Sorted %1 records by %2 using %3: %4 comparisons, %5 moves, %6 ms.")
                                    .arg(manager_.patients().size())
                                    .arg(qs(model::sortKeyToString(key)), qs(controller::algorithmName(algorithm)))
                                    .arg(stats.comparisons).arg(stats.moves)
                                    .arg(stats.milliseconds, 0, 'f', 3);
        sortResult_->setText(QStringLiteral("<span style='color:#2e7d32'>%1</span>").arg(message));
        keywordEdit_->clear();
        emit statusMessage(message);
        emit dataChanged();
    });
}

} // namespace scapms::gui
