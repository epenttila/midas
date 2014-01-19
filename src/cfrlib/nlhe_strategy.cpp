#include "nlhe_strategy.h"
#ifdef _MSC_VER
#pragma warning(push, 1)
#endif
#include <regex>
#include <boost/filesystem.hpp>
#include <boost/log/trivial.hpp>
#ifdef MSC_VER_
#pragma warning(pop)
#endif
#include "nlhe_state.h"
#include "abslib/holdem_abstraction.h"
#include "abslib/holdem_abstraction_v2.h"
#include "strategy.h"

nlhe_strategy::nlhe_strategy(const std::string& filepath)
{
    BOOST_LOG_TRIVIAL(info) << "Opening strategy file: " << filepath;

    const std::string filename = boost::filesystem::path(filepath).filename().string();
    
    std::regex r("([^_]+)_([^_]+)(_.*)?\\.str");
    std::smatch m;

    if (!std::regex_match(filename, m, r))
        throw std::runtime_error("Unable to parse filename");

    root_state_ = nlhe_state_base::create(m[1].str());
    stack_size_ = root_state_->get_stack_size();

    BOOST_LOG_TRIVIAL(info) << "Stack size: " << stack_size_;

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

    const auto abs_name = m[2].str();

    auto dir = boost::filesystem::path(filepath).parent_path();
    dir /= std::string(abs_name + ".abs");

    if (abs_name.substr(0, 2) == "ir" || abs_name.substr(0, 2) == "pr")
        abstraction_.reset(new holdem_abstraction_v2);
    else
        abstraction_.reset(new holdem_abstraction);

    abstraction_->read(dir.string());
    strategy_.reset(new strategy(filepath, state_count, root_state_->get_action_count()));
}

const holdem_abstraction_base& nlhe_strategy::get_abstraction() const
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

int nlhe_strategy::get_stack_size() const
{
    return stack_size_;
}
