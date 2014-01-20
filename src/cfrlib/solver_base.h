#pragma once

static const double EPSILON = 1e-7;

class solver_base
{
    friend std::ostream& operator<<(std::ostream& os, const solver_base& solver);

public:
    virtual ~solver_base() {}
    virtual void solve(const std::uint64_t iterations, std::int64_t seed, int threads = -1) = 0;
    virtual void save_state(const std::string& filename) const = 0;
    virtual void load_state(const std::string& filename) = 0;
    virtual void save_strategy(const std::string& filename) const = 0;
    virtual void init_storage() = 0;
    virtual std::vector<int> get_bucket_counts() const = 0;
    virtual std::vector<int> get_state_counts() const = 0;
    virtual std::size_t get_required_memory() const = 0;
    virtual void connect_progressed(const std::function<void (std::uint64_t)>& f) = 0;

protected:
    virtual void print(std::ostream& os) const = 0;
};

std::ostream& operator<<(std::ostream& os, const solver_base& solver)
{
    solver.print(os);
    return os;
}
