#include "nlhe_strategy.h"
#include <boost/filesystem.hpp>
#include <regex>
#include "nl_holdem_state.h"
#include "abslib/holdem_abstraction.h"
#include "strategy.h"

nlhe_strategy::nlhe_strategy(const std::string& filepath)
{
    const std::string filename = boost::filesystem::path(filepath).filename().string();
    std::regex r("([^-]+)-([^-]+)-[0-9]+\\.str");
    std::regex r_nlhe("nlhe\\.([a-z]+)\\.([0-9]+)");
    std::smatch m;
    std::smatch m_nlhe;

    if (!std::regex_match(filename, m, r))
        throw std::runtime_error("invalid filename");

    if (!std::regex_match(m[1].first, m[1].second, m_nlhe, r_nlhe))
        throw std::runtime_error("invalid filename");

    const std::string actions = m_nlhe[1];
    const auto stack_size = std::atoi(m_nlhe[2].str().c_str());

    if (actions == "fchpa")
        root_state_.reset(new nl_holdem_state<F_MASK | C_MASK | H_MASK | P_MASK | A_MASK>(stack_size));
    else if (actions == "fchqpa")
        root_state_.reset(new nl_holdem_state<F_MASK | C_MASK | H_MASK | Q_MASK | P_MASK | A_MASK>(stack_size));

    std::size_t state_count = 0;
    std::vector<const nlhe_state_base*> stack(1, root_state_.get());

    while (!stack.empty())
    {
        const nlhe_state_base* s = stack.back();
        stack.pop_back();

        if (!s->is_terminal())
            ++state_count;

        for (int i = s->get_action_count() - 1; i >= 0; --i)
        {
            if (const nlhe_state_base* child = s->get_child(i))
                stack.push_back(child);
        }
    }

    auto dir = boost::filesystem::path(filepath).parent_path();
    dir /= std::string(m[2].str() + ".abs");
    abstraction_.reset(new holdem_abstraction(dir.string()));

    strategy_.reset(new strategy(filepath, state_count, root_state_->get_action_count()));
}

const holdem_abstraction& nlhe_strategy::get_abstraction() const
{
    return *abstraction_;
}

const nlhe_state_base& nlhe_strategy::get_root_state() const
{
    return *root_state_;
}

const strategy& nlhe_strategy::get_strategy() const
{
    return *strategy_;
}
