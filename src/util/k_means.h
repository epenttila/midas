#pragma once
#include <random>
#include <cassert>

namespace detail
{
    template<class T>
    void vector_add(T& a, const T& b)
    {
        assert(a.size() == b.size());

        for (std::size_t i = 0; i < a.size(); ++i)
            a[i] += b[i];
    }

    template<class T>
    void vector_sub(T& a, const T& b)
    {
        assert(a.size() == b.size());

        for (std::size_t i = 0; i < a.size(); ++i)
            a[i] -= b[i];
    }
};

enum init_type { RANDOM, PP, PARALLEL, OPTIMAL };

template<class Point, class ClusterIndex, class DistanceFunction>
class k_means
{
public:
    typedef Point point_t;
    typedef std::vector<point_t> point_vector_t;
    typedef double distance_t;
    typedef ClusterIndex cluster_idx_t;
    typedef DistanceFunction distance_fun_t;
    typedef std::size_t cluster_size_t;
    typedef std::size_t point_idx_t;
    typedef typename point_t::size_type dim_idx_t;

    void operator()(const point_vector_t& points, const int cluster_count, const int max_iterations, init_type init, std::vector<cluster_idx_t>* point_clusters_out)
    {
        assert(cluster_count < points.size());

        auto& point_clusters = *point_clusters_out;
        point_vector_t cluster_centers;
        const std::size_t max_init_rounds = 5;

        switch (init)
        {
        case PP:
            cluster_centers = init_k_means_pp(points, cluster_count);
            break;
        case PARALLEL:
            cluster_centers = init_k_means_parallel(points, cluster_count, max_init_rounds);
            break;
        case OPTIMAL:
            cluster_centers = (cluster_count < 5 || (cluster_count < 20 && points.size() < 10000))
                ? init_k_means_pp(points, cluster_count)
                : init_k_means_parallel(points, cluster_count, max_init_rounds);
            break;
        default:
            cluster_centers = init_random(points, cluster_count);
            break;
        }

        point_vector_t old_cluster_centers(cluster_count);
        point_vector_t cluster_point_sums(cluster_count);
        std::vector<cluster_size_t> cluster_sizes(cluster_count);
        std::vector<distance_t> cluster_move_distances(cluster_count);

        std::vector<distance_t> upper_bounds(points.size());
        std::vector<distance_t> lower_bounds(points.size());
        point_clusters.resize(points.size());

        initialize(cluster_centers, points, &cluster_sizes, &cluster_point_sums, &upper_bounds, &lower_bounds, &point_clusters);

        std::vector<distance_t> intercluster_distances(cluster_count);

        int iters = 0;
        bool converged = false;

        while (iters < 2 || !converged)
        {
            if (iters == max_iterations)
                break;

            converged = true;

            update_intercluster_distances(cluster_centers, &intercluster_distances);

            const auto threads = omp_get_max_threads();

            struct thread_data_t
            {
                point_vector_t cluster_point_sums;
                std::vector<cluster_size_t> cluster_sizes;
            };

            std::vector<thread_data_t> thread_data(threads);

            for (std::size_t tid = 0; tid < thread_data.size(); ++tid)
            {
                thread_data[tid].cluster_point_sums.resize(cluster_count);
                thread_data[tid].cluster_sizes.resize(cluster_count);

                for (cluster_idx_t cluster_idx = 0; cluster_idx < cluster_sizes.size(); ++cluster_idx)
                {
                    thread_data[tid].cluster_sizes[cluster_idx] = cluster_size_t();
                    //thread_data[tid].cluster_point_sums[cluster_idx].resize(points[0].size());
                }
            }

#pragma omp parallel for
            for (std::int64_t point_idx = 0; point_idx < static_cast<std::int64_t>(points.size()); ++point_idx)
            {
                const auto max_distance = std::max(intercluster_distances[point_clusters[point_idx]] / 2.0, lower_bounds[point_idx]);

                if (upper_bounds[point_idx] > max_distance)
                {
                    upper_bounds[point_idx] = distance_fun_t()(points[point_idx], cluster_centers[point_clusters[point_idx]]);

                    if (upper_bounds[point_idx] > max_distance)
                    {
                        const auto old_cluster = point_clusters[point_idx];

                        point_all_ctrs(points[point_idx], cluster_centers, &point_clusters[point_idx], &upper_bounds[point_idx], &lower_bounds[point_idx]);

                        if (point_clusters[point_idx] != old_cluster)
                        {
                            const auto tid = omp_get_thread_num();

                            --thread_data[tid].cluster_sizes[old_cluster];
                            ++thread_data[tid].cluster_sizes[point_clusters[point_idx]];
                            detail::vector_sub(thread_data[tid].cluster_point_sums[old_cluster], points[point_idx]);
                            detail::vector_add(thread_data[tid].cluster_point_sums[point_clusters[point_idx]], points[point_idx]);

                            converged = false;
                        }
                    }
                }
            }

            for (std::size_t tid = 0; tid < thread_data.size(); ++tid)
            {
                detail::vector_add(cluster_sizes, thread_data[tid].cluster_sizes);

                for (cluster_idx_t i = 0; i < cluster_point_sums.size(); ++i)
                    detail::vector_add(cluster_point_sums[i], thread_data[tid].cluster_point_sums[i]);
            }

            move_centers(cluster_point_sums, cluster_sizes, &old_cluster_centers, &cluster_centers, &cluster_move_distances);
            update_bounds(cluster_move_distances, point_clusters, &upper_bounds, &lower_bounds);

            ++iters;
        }
    }

private:
    static void update_cost(const point_t& point, const point_vector_t& cluster_centers,
        const cluster_idx_t new_clusters_idx, distance_t* old_distance, cluster_idx_t* point_cluster)
    {
        auto& distance_min = *old_distance;

        for (cluster_idx_t cluster = new_clusters_idx; cluster < cluster_centers.size(); ++cluster)
        {
            const distance_t d = distance_fun_t()(point, cluster_centers[cluster]);

            if (d < distance_min)
            {
                distance_min = d;
                *point_cluster = cluster;
            }
        }
    }

    static const point_vector_t init_random(const point_vector_t& points, const int cluster_count)
    {
        if (points.size() <= cluster_count)
            return points;

        std::random_device rd;
        std::mt19937 engine(rd());
        std::uniform_int_distribution<point_idx_t> dist(0, points.size() - 1);
        point_vector_t cluster_centers(cluster_count);

        for (cluster_idx_t i = 0; i < cluster_centers.size(); ++i)
            cluster_centers[i] = points[dist(engine)];

        return cluster_centers;
    }

    static const point_vector_t init_k_means_parallel(const point_vector_t& points, const int cluster_count, const std::size_t max_rounds)
    {
        if (points.size() <= cluster_count)
            return points;

        std::random_device rd;
        std::mt19937 engine(rd());
        std::uniform_int_distribution<point_idx_t> dist(0, points.size() - 1);
        point_vector_t cluster_centers;

        cluster_centers.push_back(points[dist(engine)]);

        // initialize the costs for each point vs the single initial cluster center
        std::vector<distance_t> costs(points.size(), std::numeric_limits<distance_t>::max());
        std::vector<cluster_idx_t> point_clusters(points.size());

#pragma omp parallel for
        for (std::int64_t point = 0; point < static_cast<std::int64_t>(points.size()); ++point)
            update_cost(points[point], cluster_centers, 0, &costs[point], &point_clusters[point]);

        auto total_cost = std::accumulate(costs.begin(), costs.end(), distance_t());

        const double oversampling_factor = 2.0 * cluster_count;
        const std::size_t iterations_todo = static_cast<std::size_t>(std::ceil(std::log(total_cost)));

        for (std::size_t i = 0; i < iterations_todo && (i < max_rounds || cluster_centers.size() < cluster_count) && total_cost > 0; ++i)
        {
            struct thread_data_t
            {
                point_vector_t cluster_centers;
                std::mt19937 engine;
            };

            std::vector<thread_data_t> thread_data(omp_get_max_threads());

            for (std::size_t tid = 0; tid < thread_data.size(); ++tid)
                thread_data[tid].engine = std::mt19937(rd());

#pragma omp parallel for
            for (std::int64_t point = 0; point < static_cast<std::int64_t>(points.size()); ++point)
            {
                const auto tid = omp_get_thread_num();
                const auto prob = oversampling_factor * costs[point] / total_cost;
                const auto x = std::uniform_real_distribution<distance_t>(0, 1)(thread_data[tid].engine);

                if (x < prob)
                    thread_data[tid].cluster_centers.push_back(points[point]);
            }

            const cluster_idx_t old_cluster_count = static_cast<cluster_idx_t>(cluster_centers.size());

            for (std::size_t tid = 0; tid < thread_data.size(); ++tid)
            {
                cluster_centers.insert(cluster_centers.end(), thread_data[tid].cluster_centers.begin(),
                    thread_data[tid].cluster_centers.end());
            }

            // update costs for all points as per the new cluster center candidates
#pragma omp parallel for
            for (std::int64_t point = 0; point < static_cast<std::int64_t>(points.size()); ++point)
                update_cost(points[point], cluster_centers, old_cluster_count, &costs[point], &point_clusters[point]);

            // update total cost
            total_cost = std::accumulate(costs.begin(), costs.end(), distance_t());
        }

        if (cluster_centers.size() <= cluster_count)
            return cluster_centers; // locally optimal seed already found

        std::vector<std::vector<std::size_t>> thread_cluster_weights(omp_get_max_threads());

        for (std::size_t tid = 0; tid < thread_cluster_weights.size(); ++tid)
            thread_cluster_weights[tid].resize(cluster_centers.size());

#pragma omp parallel for
        for (std::int64_t point = 0; point < static_cast<std::int64_t>(points.size()); ++point)
            ++thread_cluster_weights[omp_get_thread_num()][point_clusters[point]];

        std::vector<std::size_t> cluster_weights(cluster_centers.size());

        for (std::size_t tid = 0; tid < thread_cluster_weights.size(); ++tid)
            detail::vector_add(cluster_weights, thread_cluster_weights[tid]);

        return init_k_means_pp(cluster_centers, cluster_count, cluster_weights);
    }

    static const point_vector_t init_k_means_pp(const point_vector_t& points, const cluster_idx_t cluster_count,
        const std::vector<std::size_t> weights = std::vector<std::size_t>())
    {
        if (points.size() <= cluster_count)
            return points;

        std::random_device rd;
        std::mt19937 engine(rd());
        std::uniform_int_distribution<point_idx_t> dist(0, points.size() - 1);
        point_vector_t cluster_centers(cluster_count);

        std::vector<distance_t> distances(points.size(), std::numeric_limits<distance_t>::max());

        cluster_centers[0] = points[dist(engine)];

        for (cluster_idx_t cluster = 1; cluster < cluster_count; ++cluster)
        {
            distance_t sum = 0;

            for (point_idx_t point = 0; point < points.size(); ++point)
            {
                // note that this is the same metric which we are minimizing
                // normally the metric is squared euclidean distance, i.e., ||x-y||^2, or, D^2 in the k-means++ paper)
                // this is the reason we use d instead of d*d, as the point is to choose a random cluster proportional
                // to the contribution it provides
                const distance_t d = distance_fun_t()(points[point], cluster_centers[cluster - 1]) * (weights.empty() ? 1.0 : weights[point]);

                if (d < distances[point])
                    distances[point] = d;

                sum += distances[point];
            }

            sum = std::uniform_real_distribution<distance_t>(0, sum)(engine);

            for (point_idx_t point = 0; point < points.size(); ++point)
            {
                const distance_t p = distances[point];

                if (sum < p)
                {
                    cluster_centers[cluster] = points[point];
                    break;
                }

                sum -= p;
            }
        }

        return cluster_centers;
    }

    static void point_all_ctrs(const point_t& point, const point_vector_t& cluster_centers, cluster_idx_t* cluster_idx_out, distance_t* upper_bound, distance_t* lower_bound)
    {
        auto& cluster_idx = *cluster_idx_out;
        cluster_idx_t second_cluster_idx = static_cast<cluster_idx_t>(-1);

        distance_t distance_min = std::numeric_limits<distance_t>::max();
        distance_t second_distance_min = distance_min;

        for (cluster_idx_t i = 0; i < cluster_centers.size(); ++i)
        {
            const auto distance = distance_fun_t()(point, cluster_centers[i]);

            assert(distance >= 0);

            if (distance < distance_min)
            {
                second_cluster_idx = cluster_idx;
                second_distance_min = distance_min;

                cluster_idx = i;
                distance_min = distance;
            }
            else if (distance < second_distance_min)
            {
                second_cluster_idx = i;
                second_distance_min = distance;
            }
        }

        *upper_bound = distance_min;
        *lower_bound = second_distance_min;
    }

    static void initialize(const point_vector_t& cluster_centers, const point_vector_t& points,
        std::vector<cluster_size_t>* cluster_sizes_out, point_vector_t* cluster_point_sums_out, std::vector<distance_t>* upper_bounds_out,
        std::vector<distance_t>* lower_bounds_out, std::vector<cluster_idx_t>* point_clusters_out)
    {
        auto& cluster_sizes = *cluster_sizes_out;
        auto& cluster_point_sums = *cluster_point_sums_out;
        auto& upper_bounds = *upper_bounds_out;
        auto& lower_bounds = *lower_bounds_out;
        auto& point_clusters = *point_clusters_out;

        std::fill(cluster_sizes.begin(), cluster_sizes.end(), cluster_size_t());

        struct thread_data_t
        {
            point_vector_t cluster_point_sums;
            std::vector<cluster_size_t> cluster_sizes;
        };

        std::vector<thread_data_t> thread_data(omp_get_max_threads());

        for (std::size_t tid = 0; tid < thread_data.size(); ++tid)
        {
            thread_data[tid].cluster_point_sums.resize(cluster_point_sums.size());
            thread_data[tid].cluster_sizes.resize(cluster_sizes.size());
            std::fill(thread_data[tid].cluster_sizes.begin(), thread_data[tid].cluster_sizes.end(), cluster_size_t());
        }

#pragma omp parallel for
        for (std::int64_t i = 0; i < static_cast<std::int64_t>(points.size()); ++i)
        {
            point_all_ctrs(points[i], cluster_centers, &point_clusters[i], &upper_bounds[i], &lower_bounds[i]);
            const auto tid = omp_get_thread_num();
            ++thread_data[tid].cluster_sizes[point_clusters[i]];
            detail::vector_add(thread_data[tid].cluster_point_sums[point_clusters[i]], points[i]);
        }

        for (std::size_t tid = 0; tid < thread_data.size(); ++tid)
        {
            detail::vector_add(cluster_sizes, thread_data[tid].cluster_sizes);

            for (cluster_idx_t i = 0; i < cluster_point_sums.size(); ++i)
                detail::vector_add(cluster_point_sums[i], thread_data[tid].cluster_point_sums[i]);
        }
    }

    static void update_intercluster_distances(const point_vector_t& cluster_centers, std::vector<distance_t>* intercluster_distances)
    {
        std::fill(intercluster_distances->begin(), intercluster_distances->end(), std::numeric_limits<distance_t>::max());

#pragma omp parallel for
        for (std::int64_t i = 0; i < static_cast<std::int64_t>(cluster_centers.size()); ++i)
        {
            for (cluster_idx_t j = 0; j < cluster_centers.size(); ++j)
            {
                if (i == j)
                    continue;

                const auto d = distance_fun_t()(cluster_centers[i], cluster_centers[j]);

                if (d < (*intercluster_distances)[i])
                    (*intercluster_distances)[i] = d;
            }
        }
    }

    static void move_centers(const point_vector_t& cluster_point_sums, const std::vector<cluster_size_t>& cluster_sizes, point_vector_t* old_cluster_centers_out,
        point_vector_t* cluster_centers_out, std::vector<distance_t>* cluster_move_distances_out)
    {
        auto& old_cluster_centers = *old_cluster_centers_out;
        auto& cluster_centers = *cluster_centers_out;
        auto& cluster_move_distances = *cluster_move_distances_out;

        for (cluster_idx_t cluster_idx = 0; cluster_idx < cluster_centers.size(); ++cluster_idx)
        {
            old_cluster_centers[cluster_idx] = cluster_centers[cluster_idx];

            for (dim_idx_t dim_idx = 0; dim_idx < cluster_centers[cluster_idx].size(); ++dim_idx)
                cluster_centers[cluster_idx][dim_idx] = cluster_point_sums[cluster_idx][dim_idx] / cluster_sizes[cluster_idx];

            cluster_move_distances[cluster_idx] = distance_fun_t()(old_cluster_centers[cluster_idx], cluster_centers[cluster_idx]);
        }
    }

    static void update_bounds(const std::vector<distance_t>& cluster_move_distances, const std::vector<cluster_idx_t> point_clusters,
        std::vector<distance_t>* upper_bounds_out, std::vector<distance_t>* lower_bounds_out)
    {
        auto& upper_bounds = *upper_bounds_out;
        auto& lower_bounds = *lower_bounds_out;

        distance_t first_max_move_distance = distance_t();
        distance_t second_max_move_distance = distance_t();
        cluster_idx_t first_max_move_distance_idx = static_cast<cluster_idx_t>(-1);
        cluster_idx_t second_max_move_distance_idx = static_cast<cluster_idx_t>(-1);

        for (cluster_idx_t i = 0; i < cluster_move_distances.size(); ++i)
        {
            const auto distance = cluster_move_distances[i];

            if (distance > first_max_move_distance)
            {
                second_max_move_distance_idx = first_max_move_distance_idx;
                second_max_move_distance = first_max_move_distance;

                first_max_move_distance_idx = i;
                first_max_move_distance = distance;
            }
            else if (distance > second_max_move_distance)
            {
                second_max_move_distance_idx = i;
                second_max_move_distance = distance;
            }
        }

#pragma omp parallel for
        for (std::int64_t i = 0; i < static_cast<std::int64_t>(upper_bounds.size()); ++i)
        {
            upper_bounds[i] += cluster_move_distances[point_clusters[i]];

            if (point_clusters[i] == first_max_move_distance_idx)
                lower_bounds[i] -= second_max_move_distance;
            else
                lower_bounds[i] -= first_max_move_distance;
        }
    }
};
