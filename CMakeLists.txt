cmake_minimum_required(VERSION 3.16)
project(SCAPMS VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/W4 /utf-8)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# ---------------------------------------------------------------------------
# Core library: model + controller + utils. Pure standard C++ (no Qt), so the
# business logic is fully separated from the GUI and can be tested on its own.
# ---------------------------------------------------------------------------
add_library(scapms_core STATIC
    utils/DateUtils.cpp
    utils/Validator.cpp
    utils/IDGenerator.cpp
    utils/FileHandler.cpp
    model/PatientRecord.cpp
    model/GeneralPatient.cpp
    model/EmergencyPatient.cpp
    model/FollowUpPatient.cpp
    model/SchedulingRules.cpp
    model/PatientFactory.cpp
    model/Appointment.cpp
    model/ClinicDatabase.cpp
    controller/SortEngine.cpp
    controller/SearchEngine.cpp
    controller/UndoManager.cpp
    controller/RecentPatientCache.cpp
    controller/WaitingQueue.cpp
    controller/PatientProcessor.cpp
    controller/AppointmentController.cpp
    controller/ReportGenerator.cpp
    controller/ClinicManager.cpp
)
target_include_directories(scapms_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Console tests for the hand-written algorithms and business rules.
add_executable(scapms_tests tests/CoreTests.cpp)
target_link_libraries(scapms_tests PRIVATE scapms_core)

# ---------------------------------------------------------------------------
# Qt GUI (Qt 6 preferred, Qt 5.15 also works)
# ---------------------------------------------------------------------------
option(SCAPMS_BUILD_GUI "Build the Qt desktop application" ON)
if(SCAPMS_BUILD_GUI)
    set(CMAKE_AUTOMOC ON)
    find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
    find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

    add_executable(SCAPMS
        main.cpp
        gui/Panel.h
        gui/AppStyle.cpp
        gui/GuiHelpers.cpp
        gui/BadgeDelegate.cpp
        gui/PatientDialog.cpp
        gui/DashboardPanel.cpp
        gui/PatientPanel.cpp
        gui/AppointmentPanel.cpp
        gui/QueuePanel.cpp
        gui/SearchPanel.cpp
        gui/ReportPanel.cpp
        gui/AdminPanel.cpp
        gui/MainWindow.cpp
    )
    target_link_libraries(SCAPMS PRIVATE scapms_core Qt${QT_VERSION_MAJOR}::Widgets)
    set_target_properties(SCAPMS PROPERTIES WIN32_EXECUTABLE ON MACOSX_BUNDLE ON)

    # Copy the sample data next to the executable so it loads on first run.
    add_custom_command(TARGET SCAPMS POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${CMAKE_CURRENT_SOURCE_DIR}/data $<TARGET_FILE_DIR:SCAPMS>/data)
endif()
