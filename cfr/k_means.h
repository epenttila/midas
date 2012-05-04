#pragma once
#include <random>
#include <cassert>

namespace detail
{
    template<class T>
    const std::vector<T> init_means(const std::vector<T>& points, const int k)
    {
        std::random_device rd;
        std::mt19937 engine(rd());
        std::uniform_int_distribution<std::size_t> dist(0, points.size() - 1);
        std::vector<T> means(k);

        for (auto i = 0; i < k; ++i)
            means[i] = points[dist(engine)];

        return means;
    }
}

template<class Distance, class T>
const std::vector<int> k_means(const std::vector<T>& points, const int k, const int runs)
{
    const auto dimensions = points[0].size();
    Distance get_distance;
    std::vector<int> best_clusters(points.size(), -1);
    double min_distance_sum = std::numeric_limits<double>::max();

#pragma omp parallel for schedule(dynamic)
    for (auto n = 0; n < runs; ++n)
    {
        std::vector<T> means = detail::init_means(points, k);
        std::vector<T> new_means = means;
        std::vector<int> clusters(points.size(), -1);
        std::vector<int> cluster_sizes(k);
        bool converged = false;
        int iters = 0;

        while (!converged)
        {
            converged = true;

            for (auto i = 0; i < k; ++i)
                std::fill(new_means[i].begin(), new_means[i].end(), 0);

            std::fill(cluster_sizes.begin(), cluster_sizes.end(), 0);

            for (auto i = 0; i < points.size(); ++i)
            {
                double min_distance = std::numeric_limits<double>::max();
                int cluster = 0;

                for (auto j = 0; j < k; ++j)
                {
                    const double distance = get_distance(points[i], means[j]);
                    assert(distance >= 0);

                    if (distance < min_distance)
                    {
                        min_distance = distance;
                        cluster = j;

                        if (distance == 0)
                            break;
                    }
                }

                if (cluster != clusters[i])
                {
                    clusters[i] = cluster;
                    converged = false;
                }
                
                for (auto j = 0; j < dimensions; ++j)
                    new_means[cluster][j] += points[i][j];

                ++cluster_sizes[cluster];
            }

            for (auto i = 0; i < k; ++i)
            {
                for (auto j = 0; j < dimensions; ++j)
                    new_means[i][j] /= double(cluster_sizes[i]);
            }

            std::swap(means, new_means);

            ++iters;
        }

        double distance_sum = 0;

        for (auto i = 0; i < points.size(); ++i)
            distance_sum += get_distance(points[i], means[clusters[i]]);

#pragma omp critical
        {
            std::cout << omp_get_thread_num() << " converged after " << iters << ", dist: " << distance_sum << ", best: " << min_distance_sum << "\n";
            if (distance_sum < min_distance_sum)
            {
                min_distance_sum = distance_sum;
                best_clusters = clusters;
            }
        }
    }

    return best_clusters;
}
