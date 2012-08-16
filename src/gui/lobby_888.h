#pragma once

#pragma warning(push, 3)
#include <boost/noncopyable.hpp>
#include <QWidget>
#define NOMINMAX
#include <Windows.h>
#pragma warning(pop)

#include "lobby_base.h"

class input_manager;

class lobby_888 : public lobby_base, private boost::noncopyable
{
public:
    lobby_888(WId window, input_manager& input_manager);
    void close_popups();
    bool is_window() const;
    void register_sng();
    int get_registered_sngs() const;

private:
    static BOOL CALLBACK callback(HWND window, LPARAM lParam);

    WId window_;
    input_manager& input_manager_;
    int registered_;
};
