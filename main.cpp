// SCAPMS - Smart Clinic Appointment & Patient Management System
// COS202 Programming II - Lab Assessment Project
#include "gui/AppStyle.h"
#include "gui/MainWindow.h"

#include <QApplication>
#include <QMessageBox>

#include <exception>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("SCAPMS"));
    QApplication::setOrganizationName(QStringLiteral("COS202"));
    scapms::gui::applyAppStyle(app);

    try {
        scapms::gui::MainWindow window;
        window.show();
        window.loadDefaultData();
        return app.exec();
    } catch (const std::exception& error) {   // last line of defence: report instead of crashing silently
        QMessageBox::critical(nullptr, QStringLiteral("SCAPMS - Fatal Error"),
                              QStringLiteral("The application must close:\n%1").arg(QString::fromStdString(error.what())));
    } catch (...) {
        QMessageBox::critical(nullptr, QStringLiteral("SCAPMS - Fatal Error"),
                              QStringLiteral("The application must close because of an unknown error."));
    }
    return 1;
}
