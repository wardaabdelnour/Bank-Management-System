cmake_minimum_required(VERSION 3.16)
project(BankGUI LANGUAGES CXX)

# Optional alternative to BankGUI.pro (Qt Creator can open either one).
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

add_executable(BankGUI
    main.cpp
    mainwindow.cpp
    mainwindow.h
    bank_project.cpp
    bank_backend.h
)

target_compile_definitions(BankGUI PRIVATE BANK_GUI)
target_link_libraries(BankGUI PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

if(WIN32)
    set_target_properties(BankGUI PROPERTIES WIN32_EXECUTABLE ON)
endif()
