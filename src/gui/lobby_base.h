#pragma once

class lobby_base
{
public:
    virtual ~lobby_base() {}
    virtual void close_popups() = 0;
    virtual bool is_window() const = 0;
    virtual void register_sng() = 0;
    virtual int get_registered_sngs() const = 0;
};
