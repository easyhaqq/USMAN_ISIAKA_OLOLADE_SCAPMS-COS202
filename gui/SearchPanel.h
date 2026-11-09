#ifndef SCAPMS_GUI_SEARCHPANEL_H
#define SCAPMS_GUI_SEARCHPANEL_H

#include "gui/Panel.h"

#include <vector>

class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QTableWidget;

namespace scapms::model { class PatientRecord; }

namespace scapms::gui {

// Search & Sort tab: choose the search field/algorithm and the sort
// algorithm/key from combo boxes; explanations update as choices change.
class SearchPanel : public Panel {
    Q_OBJECT
public:
    SearchPanel(controller::ClinicManager& manager, QWidget* parent);

public slots:
    void refresh() override;
    void focusSearch();   // Ctrl+F

private slots:
    void runSearch();
    void onKeywordEdited();
    void onSearchFieldChanged();
    void runSort();
    void updateSortAdvice();

private:
    void performSearch(bool interactive);
    void showPatients(const std::vector<const model::PatientRecord*>& patients);
    void showAllPatients();

    QComboBox* fieldBox_ = nullptr;
    QComboBox* searchAlgorithmBox_ = nullptr;
    QLineEdit* keywordEdit_ = nullptr;
    QCheckBox* liveSearch_ = nullptr;
    QLabel* searchInfo_ = nullptr;

    QComboBox* sortAlgorithmBox_ = nullptr;
    QComboBox* sortKeyBox_ = nullptr;
    QLabel* rationaleLabel_ = nullptr;
    QLabel* recommendationLabel_ = nullptr;
    QLabel* sortResult_ = nullptr;
    QLabel* orderLabel_ = nullptr;

    QTableWidget* results_ = nullptr;
    bool showingSearch_ = false;
};

} // namespace scapms::gui

#endif // SCAPMS_GUI_SEARCHPANEL_H
