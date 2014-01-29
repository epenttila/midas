#include "nlhe_strategy.h"
#ifdef _MSC_VER
#pragma warning(push, 1)
#endif
#include <boost/regex.hpp>
#include <boost/filesystem.hpp>
#include <boost/log/trivial.hpp>
#ifdef MSC_VER_
#pragma warning(pop)
#endif
#include "gamelib/nlhe_state.h"
#include "abslib/holdem_abstraction.h"
#include "strategy.h"

nlhe_strategy::nlhe_strategy(const std::string& filepath, bool read_only)
{
    BOOST_LOG_TRIVIAL(info) << "Opening strategy file: " << filepath;

    const std::string filename = boost::filesystem::path(filepath).filename().string();

    boost::regex r("([^_]+)_([^_]+)(_.*)?\\.str");
    boost::smatch m;

    if (!boost::regex_match(filename, m, r))
        throw std::runtime_error("Unable to parse filename");

    root_state_ = nlhe_state::create(m[1].str());
    stack_size_ = root_state_->get_stack_size();

    BOOST_LOG_TRIVIAL(info) << "Stack size: " << stack_size_;

    const auto state_count = nlhe_state::get_state_vector(*root_state_).size();

    const auto abs_name = m[2].str();

    auto dir = boost::filesystem::path(filepath).parent_path();
    dir /= std::string(abs_name + ".abs");

    abstraction_.reset(new holdem_abstraction);
    abstraction_->read(dir.string());
    strategy_.reset(new strategy(filepath, state_count, read_only));
}

const holdem_abstraction_base& nlhe_strategy::get_abstraction() const
{
    return *abstraction_;
}

const nlhe_state& nlhe_strategy::get_root_state() const
{
    return *root_state_;
}

const strategy& nlhe_strategy::get_strategy() const
{
    return *strategy_;
}

strategy& nlhe_strategy::get_strategy()
{
    return *strategy_;
}

int nlhe_strategy::get_stack_size() const
{
    return stack_size_;
}
