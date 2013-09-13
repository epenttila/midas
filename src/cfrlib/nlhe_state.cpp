#include "nlhe_state.h"
#include <cassert>

namespace detail
{
    int soft_translate(const double b1, const double b, const double b2)
    {
        assert(b1 <= b && b <= b2);
        static std::random_device rd;
        static std::mt19937 engine(rd());
        static std::uniform_real_distribution<double> dist;
        const double s1 = (b1 / b - b1 / b2) / (1 - b1 / b2);
        const double s2 = (b / b2 - b1 / b2) / (1 - b1 / b2);
        return dist(engine) < (s1 / (s1 + s2)) ? 0 : 1;
    }
}

std::ostream& operator<<(std::ostream& os, const nlhe_state_base& state)
{
    state.print(os);
    return os;
}
