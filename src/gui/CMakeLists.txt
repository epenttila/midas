project(gui)

set(gui_SOURCES
    gui.cpp
    main.cpp
)

set(gui_HEADERS
    gui.h
)

qt4_wrap_cpp(gui_HEADERS_MOC ${gui_HEADERS})
add_executable(gui WIN32 ${gui_SOURCES} ${gui_HEADERS_MOC})
target_link_libraries(gui ${QT_LIBRARIES})
