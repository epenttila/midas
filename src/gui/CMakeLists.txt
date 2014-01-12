project(gui)

set(gui_SOURCES
    holdem_strategy_widget.cpp
    holdem_strategy_widget.h
    input_manager.cpp
    input_manager.h
    lobby_manager.cpp
    lobby_manager.h
    main.cpp
    main_window.cpp
    main_window.h
    table_manager.cpp
    table_manager.h
    state_widget.cpp
    state_widget.h
    table_widget.cpp
    table_widget.h
    window_utils.cpp
    window_utils.h
    qt_log_sink.cpp
    qt_log_sink.h
    fake_window.cpp
    fake_window.h
    smtp.cpp
    smtp.h
    site_settings.cpp
    site_settings.h
    captcha_manager.cpp
    captcha_manager.h
)

set(gui_RESOURCES
    gui.qrc
)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

qt5_add_resources(gui_RESOURCES_RCC ${gui_RESOURCES})
add_executable(gui WIN32 ${gui_SOURCES} ${gui_RESOURCES_RCC})
target_link_libraries(gui ${Boost_LIBRARIES} Qt5::Widgets Qt5::Network cfrlib lutlib evallib abslib)
