#include "MatchingEngine.h"

#include <chrono>
#include <cstdint>
#include <iostream>
#include <string>

namespace {

constexpr int kSubmitCount = 100000;
constexpr int kSweepFills = 50000;

void require(bool condition, const std::string &message) {
  if (!condition) {
    std::cerr << "Benchmark sanity check failed: " << message << std::endl;
    std::exit(1);
  }
}

FixedPoint priceAt(int whole) {
  return FixedPoint::fromRaw(static_cast<std::int64_t>(whole) * FixedPoint::SCALE);
}

double millisecondsSince(std::chrono::steady_clock::time_point start) {
  const auto elapsed = std::chrono::steady_clock::now() - start;
  return std::chrono::duration<double, std::milli>(elapsed).count();
}

} // namespace

// Times the hot paths of MatchingEngine:
//   1. building a book with kSubmitCount non-crossing limit submits
//      (250 price levels per side, 200 orders per level),
//   2. one snapshot call,
//   3. one aggressive sweep order that fills against 50k resting orders.
// Printed timings are informational; the sanity checks are the pass/fail part.
int main() {
  using Clock = std::chrono::steady_clock;
  const FixedPoint one = FixedPoint::fromRaw(FixedPoint::SCALE);

  MatchingEngine engine;

  const auto submitStart = Clock::now();
  for (int i = 0; i < kSubmitCount; ++i) {
    const int tick = i % 500;
    if (i % 2 == 0) {
      engine.submitLimitOrder("BENCH/USDT", Side::Buy, priceAt(1000 - tick),
                              one);
    } else {
      engine.submitLimitOrder("BENCH/USDT", Side::Sell, priceAt(1000 + tick),
                              one);
    }
  }
  const double submitMs = millisecondsSince(submitStart);

  BookSnapshot top = engine.snapshot("BENCH/USDT", 5);
  require(top.bids.size() == 5 && top.asks.size() == 5,
          "built book should expose five levels per side");
  require(top.bids[0].price == priceAt(1000), "best bid should be 1000");
  require(top.asks[0].price == priceAt(1001), "best ask should be 1001");
  require(top.bids[0].quantity == FixedPoint::fromString("200"),
          "each built level should aggregate 200 orders of quantity 1");

  const auto snapshotStart = Clock::now();
  const BookSnapshot deep = engine.snapshot("BENCH/USDT", 10);
  const double snapshotMs = millisecondsSince(snapshotStart);
  require(deep.bids.size() == 10 && deep.asks.size() == 10,
          "snapshot should honor the requested depth");

  const auto sweepStart = Clock::now();
  const SubmitResult sweep = engine.submitLimitOrder(
      "BENCH/USDT", Side::Buy, priceAt(2000),
      FixedPoint::fromString("50000"));
  const double sweepMs = millisecondsSince(sweepStart);

  require(static_cast<int>(sweep.trades.size()) == kSweepFills,
          "sweep order should fill against every resting ask");
  require(!sweep.remainingQuantity.isPositive(),
          "sweep order should be fully filled");
  require(engine.snapshot("BENCH/USDT", 10).asks.empty(),
          "sweep should empty the ask side");

  const double submitsPerSecond = kSubmitCount / (submitMs / 1000.0);
  const double fillsPerSecond = kSweepFills / (sweepMs / 1000.0);

  std::cout << "[benchmark] " << kSubmitCount << " limit submits (book build) in "
            << submitMs << " ms (" << submitsPerSecond / 1000.0
            << "K submits/sec)\n";
  std::cout << "[benchmark] book: 250 bid levels + 250 ask levels, best bid "
               "1000, best ask 1001\n";
  std::cout << "[benchmark] snapshot(depth=10) in " << snapshotMs << " ms\n";
  std::cout << "[benchmark] aggressive buy sweeping " << kSweepFills
            << " resting orders in " << sweepMs << " ms ("
            << fillsPerSecond / 1000.0 << "K fills/sec)\n";
  std::cout << "[benchmark] ok" << std::endl;
  return 0;
}
