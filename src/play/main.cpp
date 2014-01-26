#ifdef _MSC_VER
#pragma warning(push, 1)
#endif
#include <boost/asio.hpp>
#include <cstdio>
#include <boost/regex.hpp>
#include <iostream>
#ifdef _MSC_VER
#pragma warning(pop)
#endif
#include "util/card.h"
#include "cfrlib/nlhe_strategy.h"
#include "util/game.h"

static const int MAX_STACK = 500;
static const int BIG_BLIND = 20;

struct match_state
{
    const nlhe_state_base* state;
    std::array<int, 2> pot;
    int index;
    int pos;
};

void read_state(const std::string& str, match_state* s)
{
    auto& i = s->index;

    for (; i < static_cast<int>(str.size()); ++i)
    {
        if (s->state == nullptr)
            break;

        const auto player = s->state->get_player();

        if (str[i] == 'f')
        {
            //assert(false);
            s->state = nullptr;
            //s->index = -1;
            break;
        }
        else if (str[i] == 'c')
        {
            s->state = s->state->call();
            s->pot[player] = s->pot[1 - player];
        }
        else if (str[i] == 'r')
        {
            const auto p_pot = std::atoi(&str[i + 1]);
            double factor;

            if (p_pot == MAX_STACK)
                factor = 999.0;
            else
            {
                const auto o_pot = s->pot[1 - player];
                factor = static_cast<double>(p_pot - o_pot) / (2 * o_pot);
            }

            s->state = s->state->raise(factor);
            s->pot[player] = p_pot;
        }
    }
}

void read_cards(const std::string& str, std::array<int, 2>& hole, std::array<int, 2>& o_hole,
    std::array<int, 5>& board)
{
    const auto len = static_cast<int>(str.size());
    int i = 0;

    if (str[i] == '|')
        ++i;

    hole[0] = string_to_card(std::string(&str[i], 2));
    hole[1] = string_to_card(std::string(&str[i+2], 2));
    i += 4;

    if (str[i] == '|')
        ++i;

    if (str[i] != '/' && i + 4 <= len)
    {
        o_hole[0] = string_to_card(std::string(&str[i], 2));
        o_hole[1] = string_to_card(std::string(&str[i+2], 2));
        i += 4;
    }

    if (str[i] == '/')
        ++i;

    if (i + 6 <= len)
    {
        board[0] = string_to_card(std::string(&str[i], 2));
        board[1] = string_to_card(std::string(&str[i+2], 2));
        board[2] = string_to_card(std::string(&str[i+4], 2));
        i += 6;
    }

    if (str[i] == '/')
        ++i;

    if (i + 2 <= len)
    {
        board[3] = string_to_card(std::string(&str[i], 2));
        i += 2;
    }

    if (str[i] == '/')
        ++i;

    if (i + 2 <= len)
        board[4] = string_to_card(std::string(&str[i], 2));
}

int main(int argc, char* argv[])
{
    try
    {
        using boost::asio::ip::tcp;

        if (argc != 4)
        {
            return 1;
        }

        nlhe_strategy strategy(argv[1]);

        boost::asio::io_service io_service;
        tcp::resolver resolver(io_service);
        tcp::resolver::query query(argv[2], argv[3]);
        tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
        tcp::resolver::iterator end;

        tcp::socket socket(io_service);
        boost::system::error_code error = boost::asio::error::host_not_found;

        while (error && endpoint_iterator != end)
        {
            socket.close();
            socket.connect(*endpoint_iterator++, error);
        }

        if (error)
            throw boost::system::system_error(error);

        boost::asio::write(socket, boost::asio::buffer("VERSION:2.0.0\r\n", 15));

        boost::asio::streambuf b;

        match_state ms;
        ms.pos = -1;

        for (;;)
        {
            std::string buf;
            boost::system::error_code error;

            boost::asio::read_until(socket, b, "\r\n", error);

            if (error == boost::asio::error::eof)
                break; // Connection closed cleanly by peer.
            else if (error)
                throw boost::system::system_error(error); // Some other error.

            std::istream is(&b);

            while (std::getline(is, buf, '\n'))
            {
                buf.pop_back();

                if (buf[0] == '#' || buf[0] == ';')
                    continue;

                static const boost::regex re("^MATCHSTATE:(\\d+):(\\d+):([^:]*):([^\r\n]+)$");
                boost::smatch match;

                if (!boost::regex_match(buf, match, re))
                    continue;

                const auto my_pos = 1 - std::stoi(match[1]);

                if (my_pos != ms.pos)
                {
                    ms.state = &strategy.get_root_state();
                    const std::array<int, 2> blinds = {{BIG_BLIND / 2, BIG_BLIND}};
                    ms.pot = blinds;
                    ms.index = 0;
                    ms.pos = my_pos;
                }

                read_state(match[3], &ms);

                const auto state = ms.state;

                if (!state || state->get_player() != my_pos)
                    continue;

                std::array<int, 2> hole = {{-1, -1}};
                std::array<int, 2> o_hole = {{-1, -1}};
                std::array<int, 5> board = {{-1, -1, -1, -1, -1}};

                read_cards(match[4], hole, o_hole, board);

                if (o_hole[0] != -1)
                    continue;

                buf += ':';

                nlhe_state_base::holdem_action action;

                if (state->is_terminal())
                {
                    action = nlhe_state_base::RAISE_A;
                }
                else
                {
                    holdem_abstraction_base::bucket_type buckets;
                    strategy.get_abstraction().get_buckets(hole[0], hole[1], board[0], board[1], board[2],
                        board[3], board[4], &buckets);
                    const int index = strategy.get_strategy().get_action(state->get_id(), buckets[state->get_round()]);
                    action = static_cast<nlhe_state_base::holdem_action>(state->get_action(index));
                }

                if (action != nlhe_state_base::FOLD
                    && (state->get_action_index() != -1 && state->get_action(state->get_action_index()) == nlhe_state_base::RAISE_A))
                {
                    action = nlhe_state_base::RAISE_A;
                }

                std::string action_str;

                switch (action)
                {
                case nlhe_state_base::FOLD:
                    action_str = 'f';
                    break;
                case nlhe_state_base::CALL:
                    action_str = 'c';
                    ms.pot[my_pos] = ms.pot[1 - my_pos];
                    break;
                default:
                    {
                        int val = 0;

                        if (action == nlhe_state_base::RAISE_A)
                            val = MAX_STACK;
                        else
                        {
                            const auto factor = state->get_raise_factor(action);
                            const auto o_pot = ms.pot[1 - my_pos];
                            const auto p_pot = ms.pot[my_pos];

                            val = static_cast<int>(o_pot + factor * (2 * o_pot));

                            if (state->get_parent() == nullptr)
                                val = std::max(val, 2 * BIG_BLIND);
                            else
                                val = std::max(val, BIG_BLIND);

                            val = std::max(val, o_pot + o_pot - p_pot);
                        }

                        val = std::min(val, MAX_STACK);
                        ms.pot[my_pos] = val;

                        action_str = 'r' + std::to_string(val);
                    }
                    break;
                }

                ms.state = ms.state->get_child(ms.state->get_action_index(action));
                ms.index += static_cast<int>(action_str.size());

                buf += action_str + "\r\n";

                boost::asio::write(socket, boost::asio::buffer(buf));
            }
        }

        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << "\n";
        return 1;
    }
}
