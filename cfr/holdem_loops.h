#pragma once

template<class T>
void parallel_for_each_flop(const T& f)
{
#pragma omp for schedule(dynamic)
    for (int b0 = 0; b0 < 52; ++b0)
    {
        for (int b1 = b0 + 1; b1 < 52; ++b1)
        {
            for (int b2 = b1 + 1; b2 < 52; ++b2)
            {
                for (int c0 = 0; c0 < 52; ++c0)
                {
                    if (c0 == b0 || c0 == b1 || c0 == b2)
                        continue;

                    for (int c1 = c0 + 1; c1 < 52; ++c1)
                    {
                        if (c1 == b0 || c1 == b1 || c1 == b2)
                            continue;

                        f(c0, c1, b0, b1, b2);
                    }
                }
            }
        }
    }
}

template<class T>
void parallel_for_each_turn(const T& f)
{
#pragma omp for schedule(dynamic)
    for (int b0 = 0; b0 < 52; ++b0)
    {
        for (int b1 = b0 + 1; b1 < 52; ++b1)
        {
            for (int b2 = b1 + 1; b2 < 52; ++b2)
            {
                for (int b3 = b2 + 1; b3 < 52; ++b3)
                {
                    for (int c0 = 0; c0 < 52; ++c0)
                    {
                        if (c0 == b0 || c0 == b1 || c0 == b2 || c0 == b3)
                            continue;

                        for (int c1 = c0 + 1; c1 < 52; ++c1)
                        {
                            if (c1 == b0 || c1 == b1 || c1 == b2 || c1 == b3)
                                continue;

                            f(c0, c1, b0, b1, b2, b3);
                        }
                    }
                }
            }
        }
    }
}

template<class T>
void parallel_for_each_river(const T& f)
{
#pragma omp for schedule(dynamic)
    for (int b0 = 0; b0 < 52; ++b0)
    {
        for (int b1 = b0 + 1; b1 < 52; ++b1)
        {
            for (int b2 = b1 + 1; b2 < 52; ++b2)
            {
                for (int b3 = b2 + 1; b3 < 52; ++b3)
                {
                    for (int b4 = b3 + 1; b4 < 52; ++b4)
                    {
                        for (int c0 = 0; c0 < 52; ++c0)
                        {
                            if (c0 == b0 || c0 == b1 || c0 == b2 || c0 == b3 || c0 == b4)
                                continue;

                            for (int c1 = c0 + 1; c1 < 52; ++c1)
                            {
                                if (c1 == b0 || c1 == b1 || c1 == b2 || c1 == b3 || c1 == b4)
                                    continue;

                                f(c0, c1, b0, b1, b2, b3, b4);
                            }
                        }
                    }
                }
            }
        }
    }
}
