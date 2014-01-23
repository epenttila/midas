#include "game_state_base.h"
#include <cassert>

std::vector<const game_state_base*> game_state_base::get_state_vector(const game_state_base& root)
{
    std::vector<const game_state_base*> states;
    std::vector<const game_state_base*> stack(1, &root);

    while (!stack.empty())
    {
        const auto s = stack.back();
        stack.pop_back();

        if (!s->is_terminal())
        {
            assert(s->get_id() == static_cast<int>(states.size()));
            states.push_back(s);
        }

        for (int i = s->get_child_count() - 1; i >= 0; --i)
        {
            if (const game_state_base* child = s->get_child(i))
                stack.push_back(child);
        }
    }

    return states;
}
