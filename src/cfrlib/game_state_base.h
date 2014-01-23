#pragma once

#include <vector>
#include <boost/noncopyable.hpp>

class game_state_base : private boost::noncopyable
{
public:
    static std::vector<const game_state_base*> get_state_vector(const game_state_base& root);

    virtual int get_id() const = 0;
    virtual bool is_terminal() const = 0;
    virtual const game_state_base* get_child(int index) const = 0;
    virtual int get_action_count() const = 0;
};
