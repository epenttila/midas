project(sitegen)

set(sitegen_SOURCES
    main.cpp
    main_window.cpp
    main_window.h
    image_widget.cpp
    image_widget.h
)

set(sitegen_RESOURCES
    sitegen.qrc
)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

qt5_add_resources(sitegen_RESOURCES_RCC ${sitegen_RESOURCES})
add_executable(sitegen WIN32 ${sitegen_SOURCES} ${sitegen_RESOURCES_RCC})
target_link_libraries(sitegen ${Boost_LIBRARIES} Qt5::Widgets)
