#ifndef SCAPMS_GUI_APPSTYLE_H
#define SCAPMS_GUI_APPSTYLE_H

class QApplication;

namespace scapms::gui {

// Applies the application-wide font, palette and Qt style sheet.
void applyAppStyle(QApplication& app);

} // namespace scapms::gui

#endif // SCAPMS_GUI_APPSTYLE_H
