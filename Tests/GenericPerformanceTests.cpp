#include "../EngineConcept/MatchingEngineConcept.h"
#include "../EngineTestTypes.h"
#include <gtest/gtest.h>
#include <chrono>
#include <random>
#include <iomanip>

// ============================================================================
// The tests use the MatchingEngineConcept concept. To test any implementation, you need to add it to the EngineTestTypes file.
// ============================================================================

struct BenchmarkMetrics {
    double avg_latency_ns;
    double p50_latency_ns;
    double p95_latency_ns;
    double p99_latency_ns;
    double p999_latency_ns;
    double max_latency_ns;
    double throughput_ops_per_sec;
    size_t total_trades;
    size_t total_orders;
    const char* engine_name;

    void print(const std::string& test_name) const {
        std::cout << "\n╔════════════════════════════════════════════════════════════╗\n";
        std::cout << "║ " << std::left << std::setw(56) << test_name << " ║\n";
        std::cout << "║ Engine: " << std::left << std::setw(49) << engine_name << " ║\n";
        std::cout << "╠════════════════════════════════════════════════════════════╣\n";
        std::cout << std::fixed << std::setprecision(2);
        std::cout << "║ Total Orders:    " << std::setw(38) << total_orders << " ║\n";
        std::cout << "║ Total Trades:    " << std::setw(38) << total_trades << " ║\n";
        std::cout << "║ Match Rate:      " << std::setw(35) << (100.0 * total_trades / total_orders) << "% ║\n";
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
class GenericPerformanceBenchmark : public ::testing::Test {
protected:
    BenchmarkMetrics runBenchmark(size_t num_orders) {
        Engine engine;
        std::vector<double> latencies_ns;
        latencies_ns.reserve(num_orders);

        std::mt19937 rng(42);
        std::uniform_real_distribution<double> price_dist(99.0, 101.0);
        std::uniform_int_distribution<uint64_t> qty_dist(1, 100);
        std::uniform_int_distribution<int> side_dist(0, 1);
        std::uniform_int_distribution<int> type_dist(0, 9);

        auto start_total = std::chrono::high_resolution_clock::now();

        for (size_t i = 0; i < num_orders; ++i) {
            Side side = side_dist(rng) == 0 ? Side::BUY : Side::SELL;
            OrderType type = type_dist(rng) < 9 ? OrderType::LIMIT : OrderType::MARKET;
            double price = type == OrderType::LIMIT ? price_dist(rng) : 0.0;
            uint64_t qty = qty_dist(rng);

            auto order = std::make_shared<Order>(i, "TEST", side, type, price, qty, 0);

            auto start = std::chrono::high_resolution_clock::now();
            engine.submitOrder(order);
            auto end = std::chrono::high_resolution_clock::now();

            double latency_ns = std::chrono::duration<double, std::nano>(end - start).count();
            latencies_ns.push_back(latency_ns);
        }

        auto end_total = std::chrono::high_resolution_clock::now();
        double total_time_sec = std::chrono::duration<double>(end_total - start_total).count();

        std::sort(latencies_ns.begin(), latencies_ns.end());

        BenchmarkMetrics metrics;
        metrics.total_orders = num_orders;
        metrics.total_trades = engine.getTrades().size();
        metrics.engine_name = MatchingEngineTraits<Engine>::name;

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
};

TYPED_TEST_SUITE(GenericPerformanceBenchmark, EngineTestTypes);

// ============================================================================
// BASIC PERFORMANCE TESTS
// ============================================================================

TYPED_TEST(GenericPerformanceBenchmark, SmallLoad) {
    auto metrics = this->runBenchmark(10000);
    metrics.print("Small Load (10K orders)");
    EXPECT_GT(metrics.throughput_ops_per_sec, 1000);
}

TYPED_TEST(GenericPerformanceBenchmark, MediumLoad) {
    auto metrics = this->runBenchmark(100000);
    metrics.print("Medium Load (100K orders)");
    EXPECT_GT(metrics.throughput_ops_per_sec, 1000);
}

TYPED_TEST(GenericPerformanceBenchmark, LargeLoad) {
    auto metrics = this->runBenchmark(500000);
    metrics.print("Large Load (500K orders)");
    EXPECT_GT(metrics.throughput_ops_per_sec, 500);
}

// ============================================================================
// SLA TESTS
// ============================================================================

TYPED_TEST(GenericPerformanceBenchmark, SLA_Conservative_100K_OrdersPerSec) {
    const size_t NUM_ORDERS = 100000;
    const double MIN_THROUGHPUT = 100000.0;
    const double MAX_P99_LATENCY_NS = 50000.0;

    auto metrics = this->runBenchmark(NUM_ORDERS);
    metrics.print("SLA: 100K ops/sec, P99 < 50μs");

    EXPECT_GE(metrics.throughput_ops_per_sec, MIN_THROUGHPUT)
                        << "Throughput ниже требуемого";

    EXPECT_LE(metrics.p99_latency_ns, MAX_P99_LATENCY_NS)
                        << "P99 latency превышает лимит";
}

TYPED_TEST(GenericPerformanceBenchmark, SLA_P999_Latency) {
    const size_t NUM_ORDERS = 1000000;
    const double MAX_P999_LATENCY_NS = 100000.0;

    auto metrics = this->runBenchmark(NUM_ORDERS);
    metrics.print("SLA: P99.9 < 100μs");

    EXPECT_LE(metrics.p999_latency_ns, MAX_P999_LATENCY_NS)
                        << "P99.9 latency превышает лимит";
}

// ============================================================================
// COMPARATIVE TESTS
// ============================================================================

TYPED_TEST(GenericPerformanceBenchmark, DegradationAnalysis) {
    std::cout << "\n╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║           PERFORMANCE DEGRADATION ANALYSIS                 ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n";

    auto small = this->runBenchmark(10000);
    small.print("Small (10K)");

    auto medium = this->runBenchmark(100000);
    medium.print("Medium (100K)");

    auto large = this->runBenchmark(500000);
    large.print("Large (500K)");

    double throughput_degradation =
            (small.throughput_ops_per_sec - large.throughput_ops_per_sec) /
            small.throughput_ops_per_sec * 100.0;

    std::cout << "\n📊 Throughput degradation (10K→500K): "
              << std::fixed << std::setprecision(2)
              << throughput_degradation << "%\n";

    EXPECT_LT(throughput_degradation, 50.0)
                        << "Слишком большая деградация при росте нагрузки";
}

TYPED_TEST(GenericPerformanceBenchmark, OrderBookDepthImpact) {
    using Engine = TypeParam;
    Engine engine;

    std::cout << "\n╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║           ORDER BOOK DEPTH IMPACT                          ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n";

    std::mt19937 rng(42);
    std::uniform_real_distribution<double> price_dist(95.0, 105.0);

    for (size_t i = 0; i < 10000; ++i) {
        auto buy = std::make_shared<Order>(i * 2, "TEST", Side::BUY,
                                           OrderType::LIMIT, price_dist(rng), 10, 0);
        auto sell = std::make_shared<Order>(i * 2 + 1, "TEST", Side::SELL,
                                            OrderType::LIMIT, price_dist(rng) + 10.0, 10, 0);
        engine.submitOrder(buy);
        engine.submitOrder(sell);
    }

    std::cout << "Order book depth - Buy: " << engine.getBuyOrderCount("TEST")
              << ", Sell: " << engine.getSellOrderCount("TEST") << "\n";

    std::vector<double> latencies;
    for (size_t i = 0; i < 1000; ++i) {
        auto order = std::make_shared<Order>(100000 + i, "TEST", Side::BUY,
                                             OrderType::LIMIT, 100.0, 10, 0);

        auto start = std::chrono::high_resolution_clock::now();
        engine.submitOrder(order);
        auto end = std::chrono::high_resolution_clock::now();

        latencies.push_back(std::chrono::duration<double, std::nano>(end - start).count());
    }

    std::sort(latencies.begin(), latencies.end());

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "\nLatency with deep order book:\n";
    std::cout << "  P50:  " << latencies[latencies.size() / 2] << " ns\n";
    std::cout << "  P95:  " << latencies[latencies.size() * 95 / 100] << " ns\n";
    std::cout << "  P99:  " << latencies[latencies.size() * 99 / 100] << " ns\n";

    EXPECT_GT(engine.getBuyOrderCount("TEST"), 9000);
}

TYPED_TEST(GenericPerformanceBenchmark, BaselinePerformance) {
    const size_t NUM_ORDERS = 100000;

    auto metrics = this->runBenchmark(NUM_ORDERS);
    metrics.print("BASELINE - Current Performance");

    std::cout << "\n📝 Baseline metrics recorded. Use for regression tracking.\n";
    std::cout << "   Compare future optimizations against these numbers.\n\n";

    std::cout << "Baseline Summary:\n";
    std::cout << "  - Throughput: " << std::fixed << std::setprecision(0)
              << metrics.throughput_ops_per_sec << " ops/sec\n";
    std::cout << "  - P99 Latency: " << std::fixed << std::setprecision(2)
              << metrics.p99_latency_ns << " ns\n\n";

    SUCCEED();
}