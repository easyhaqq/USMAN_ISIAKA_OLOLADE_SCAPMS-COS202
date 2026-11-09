# SCAPMS – Smart Clinic Appointment & Patient Management System

COS202 Programming II – Lab Assessment Project. C++17 + Qt (Qt 6 recommended, Qt 5.15 also works).

## Folder structure

```
SCAPMS/
├── CMakeLists.txt          build script (core library, tests, GUI)
├── main.cpp                program entry point
├── model/                  data classes (no GUI code)
├── controller/             business logic, algorithms, data structures (no GUI code)
├── utils/                  ID generator, file handling, validation, exceptions, dates
├── gui/                    Qt widgets: main window, panels, dialogs, delegate, style
├── data/clinic_data.txt    sample data, loaded automatically on start-up
└── tests/CoreTests.cpp     console tests for the algorithms and business rules
```

## Building in CLion (Windows)

1. Install Qt with the online installer (Qt 6.x, component **MinGW 64-bit**). Also tick the
   MinGW toolchain under *Developer and Designer Tools*.
2. In CLion: *Settings → Build, Execution, Deployment → Toolchains*, add a MinGW toolchain
   pointing at `C:\Qt\Tools\mingw1310_64` (use your installed version).
3. *Settings → Build, Execution, Deployment → CMake*, add to **CMake options**:
   `-DCMAKE_PREFIX_PATH=C:/Qt/6.x.x/mingw_64`
4. Open the `SCAPMS` folder, let CMake load, select the **SCAPMS** run configuration and run.
5. If Windows reports missing Qt DLLs, add `C:\Qt\6.x.x\mingw_64\bin` to the run configuration's
   `PATH` environment variable (or run `windeployqt SCAPMS.exe` once in the build folder).

The build copies `data/` next to the executable, so the sample clinic loads automatically.

## Building from a terminal (Linux / macOS)

```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/scapms_tests      # 82 checks for the algorithms and rules
./build/SCAPMS            # the application
```

## Keyboard shortcuts

| Keys | Action | Keys | Action |
|---|---|---|---|
| Ctrl+N | Add patient | Ctrl+S | Save |
| Ctrl+F | Find patient | Ctrl+Shift+S | Save as |
| Ctrl+Z | Undo last operation | Ctrl+O | Load |
| Ctrl+E | Export patients (CSV) | Ctrl+Shift+N | New file |
| Ctrl+1 … Ctrl+7 | Switch tab | Delete | Delete selected patient |
| F1 | Shortcut list | Ctrl+Q | Exit |
