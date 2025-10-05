#include "./EngineConcept/MatchingEngineConcept.h"
#include "EnginImpl/V2/MatchingEngineV2.h"
#include <chrono>
#include <random>
#include <iomanip>
#include <iostream>
#include <vector>
#include <algorithm>

struct BenchmarkMetrics {
    double avg_latency_ns;
    double p50_latency_ns;
    double p95_latency_ns;
    double p99_latency_ns;
    double p999_latency_ns;
    double max_latency_ns;
    double throughput_ops_per_sec;
    size_t total_orders;
    const char* engine_name;

    void print(const std::string& test_name) const {
        std::cout << "\n╔════════════════════════════════════════════════════════════╗\n";
        std::cout << "║ " << std::left << std::setw(56) << test_name << " ║\n";
        std::cout << "║ Engine: " << std::left << std::setw(49) << engine_name << " ║\n";
        std::cout << "╠════════════════════════════════════════════════════════════╣\n";
        std::cout << std::fixed << std::setprecision(2);
        std::cout << "║ Total Orders:    " << std::setw(38) << total_orders << " ║\n";
        std::cout << "╟────────────────────────────────────────────────────────────╢\n";
        std::cout << "║ LATENCY (nanoseconds)                                      ║\n";
        std::cout << "║   Average:       " << std::setw(38) << avg_latency_ns << " ns ║\n";
        std::cout << "║   P50 (median):  " << std::setw(38) << p50_latency_ns << " ns ║\n";
        std::cout << "║   P95:           " << std::setw(38) << p95_latency_ns << " ns ║\n";
        std::cout << "║   P99:           " << std::setw(38) << p99_latency_ns << " ns ║\n";
        std::cout << "║   P99.9:         " << std::setw(38) << p999_latency_ns << " ns ║\n";
        std::cout << "║   Max:           " << std::setw(38) << max_latency_ns << " ns ║\n";
        std::cout << "╟────────────────────────────────────────────────────────────╢\n";
        std::cout << "║ THROUGHPUT                                                 ║\n";
        std::cout << "║   " << std::setw(54) << throughput_ops_per_sec << " ops/sec ║\n";
        std::cout << "╚════════════════════════════════════════════════════════════╝\n";
    }
};

template<MatchingEngineConcept Engine>
BenchmarkMetrics runBenchmark(size_t num_orders) {
    Engine engine;
    std::vector<double> latencies_ns;
    latencies_ns.reserve(num_orders);

    std::mt19937 rng(42);
    std::uniform_int_distribution<int> price_dist(9998, 10003);
    std::uniform_int_distribution<uint64_t> qty_dist(1, 100);
    std::uniform_int_distribution<int> side_dist(0, 1);
    std::uniform_int_distribution<int> type_dist(0, 9);

    auto start_total = std::chrono::high_resolution_clock::now();

    for (size_t i = 0; i < num_orders; ++i) {
        Side side = side_dist(rng) == 0 ? Side::BUY : Side::SELL;
        OrderType type = type_dist(rng) < 9 ? OrderType::LIMIT : OrderType::MARKET;
        int price = type == OrderType::LIMIT ? price_dist(rng) : 0;
        uint64_t qty = qty_dist(rng);

        auto order = std::make_unique<Order>(i, "TEST", side, type, price, qty, 0);

        auto start = std::chrono::high_resolution_clock::now();
        engine.submitOrder(std::move(order));
        auto end = std::chrono::high_resolution_clock::now();

        double latency_ns = std::chrono::duration<double, std::nano>(end - start).count();
        latencies_ns.push_back(latency_ns);
    }

    auto end_total = std::chrono::high_resolution_clock::now();
    double total_time_sec = std::chrono::duration<double>(end_total - start_total).count();

    std::sort(latencies_ns.begin(), latencies_ns.end());

    BenchmarkMetrics metrics;
    metrics.total_orders = num_orders;
    metrics.engine_name = engine.name();

    double sum = 0;
    for (double lat : latencies_ns) sum += lat;
    metrics.avg_latency_ns = sum / latencies_ns.size();

    metrics.p50_latency_ns = latencies_ns[latencies_ns.size() / 2];
    metrics.p95_latency_ns = latencies_ns[latencies_ns.size() * 95 / 100];
    metrics.p99_latency_ns = latencies_ns[latencies_ns.size() * 99 / 100];
    metrics.p999_latency_ns = latencies_ns[latencies_ns.size() * 999 / 1000];
    metrics.max_latency_ns = latencies_ns.back();
    metrics.throughput_ops_per_sec = num_orders / total_time_sec;

    return metrics;
}

int main() {
    const size_t NUM_ORDERS = 5'000'000;

    std::cout << "Starting baseline performance test...\n";
    std::cout << "Testing with " << NUM_ORDERS << " orders\n\n";

    auto metrics1 = runBenchmark<MatchingEngineV2>(NUM_ORDERS);
    metrics1.print("BASELINE - MatchingEngineV2");

    std::cout << "\n📝 Baseline complete.\n";

    return 0;
}