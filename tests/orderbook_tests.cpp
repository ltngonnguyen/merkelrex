#include "CSVReader.h"
#include "MatchingEngine.h"
#include "OrderBook.h"

#include <cmath>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void require(bool condition, const std::string &message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

bool almostEqual(long double left, long double right) {
  return fabsl(left - right) < 0.000001L;
}

void writeFixture(const std::string &path) {
  std::ofstream file(path);
  file << "2020/01/01 00:00:00,ETH/USDT,ask,100,2\n";
  file << "2020/01/01 00:00:00,ETH/USDT,bid,99,1\n";
  file << "2020/01/01 00:00:05,ETH/USDT,ask,110,1\n";
  file << "2020/01/01 00:00:05,ETH/USDT,ask,120,3\n";
  file << "2020/01/01 00:00:05,ETH/USDT,bid,108,1\n";
  file << "bad,row\n";
  file << "2020/01/01 00:00:10,BTC/USDT,ask,10000,0.5\n";
  file << "2020/01/01 00:00:10,BTC/USDT,bid,9900,0.5\n";
}

void testCsvReaderRejectsBadRows() {
  const std::string path = "test_orders.csv";
  writeFixture(path);

  std::vector<OrderBookEntry> entries = CSVReader::readCSV(path);
  require(entries.size() == 7, "CSV reader should skip malformed rows");
  require(entries[0].product == "ETH/USDT", "CSV reader should keep product");
  require(entries[0].orderType == OrderBookType::ask,
          "CSV reader should parse order side");
}

void testOrderBookTimeNavigation() {
  OrderBook book{"test_orders.csv"};

  require(!book.isEmpty(), "OrderBook should load fixture rows");
  require(book.size() == 7, "OrderBook should expose loaded row count");
  require(book.getEarliestTime() == "2020/01/01 00:00:00",
          "Earliest timestamp should come from first row");
  require(book.getNextTime("2020/01/01 00:00:00") ==
              "2020/01/01 00:00:05",
          "Next timestamp should advance to next market snapshot");
  require(book.getLastTime("2020/01/01 00:00:05") ==
              "2020/01/01 00:00:00",
          "Last timestamp should move backwards safely");
}

void testPriceAggregates() {
  OrderBook book{"test_orders.csv"};
  std::vector<OrderBookEntry> asks =
      book.getOrders(OrderBookType::ask, "ETH/USDT", "2020/01/01 00:00:05");

  require(almostEqual(OrderBook::getHighPrice(asks), 120),
          "High price should be the max ask");
  require(almostEqual(OrderBook::getLowPrice(asks), 110),
          "Low price should be the min ask");
  require(almostEqual(OrderBook::getOpeningAndClosingPrice(asks), 117.5),
          "Weighted average price should use amount as weight");
}

void testCandlesticksAndVolumes() {
  OrderBook book{"test_orders.csv"};

  std::vector<Candlestick> candles =
      book.computeCandlesticks("ETH/USDT", "ask", "2020/01/01 00:00:10");
  require(candles.size() == 1,
          "Candlestick calculation should produce one complete candle");
  require(almostEqual(candles[0].getOpen(), 100),
          "Candle open should use previous timestamp weighted price");
  require(almostEqual(candles[0].getClose(), 117.5),
          "Candle close should use current timestamp weighted price");

  std::vector<Volume> volumes = book.computeVolumes("2020/01/01 00:00:05");
  require(!volumes.empty(), "Volume calculation should return product volumes");
}

void testFixedPointParsing() {
  FixedPoint value = FixedPoint::fromString("123.45000000");
  require(value.toString() == "123.45",
          "FixedPoint should trim trailing decimal zeroes");
  require(FixedPoint::fromString("0.00000001").raw() == 1,
          "FixedPoint should preserve satoshi-like precision");

  bool threw = false;
  try {
    FixedPoint::fromString("12.bad");
  } catch (const std::exception &) {
    threw = true;
  }
  require(threw, "FixedPoint should reject malformed decimals");
}

void testMatchingEngineDoesNotCrossWhenPricesMiss() {
  MatchingEngine engine;
  engine.submitLimitOrder("ETH/USDT", Side::Sell,
                          FixedPoint::fromString("101"),
                          FixedPoint::fromString("2"));
  SubmitResult result = engine.submitLimitOrder(
      "ETH/USDT", Side::Buy, FixedPoint::fromString("100"),
      FixedPoint::fromString("1"));

  require(result.trades.empty(), "Bid below ask should not trade");
  BookSnapshot book = engine.snapshot("ETH/USDT", 5);
  require(book.bids.size() == 1, "Unmatched bid should rest on the book");
  require(book.asks.size() == 1, "Unmatched ask should remain on the book");
}

void testMatchingEnginePartialFill() {
  MatchingEngine engine;
  SubmitResult sell = engine.submitLimitOrder(
      "ETH/USDT", Side::Sell, FixedPoint::fromString("100"),
      FixedPoint::fromString("5"));
  SubmitResult buy = engine.submitLimitOrder(
      "ETH/USDT", Side::Buy, FixedPoint::fromString("105"),
      FixedPoint::fromString("2"));

  require(sell.trades.empty(), "First resting order should not trade alone");
  require(buy.trades.size() == 1, "Crossing order should produce one trade");
  require(buy.trades[0].price == FixedPoint::fromString("100"),
          "Trade should execute at resting maker price");
  require(buy.trades[0].quantity == FixedPoint::fromString("2"),
          "Trade should use smaller available quantity");

  BookSnapshot book = engine.snapshot("ETH/USDT", 5);
  require(book.asks.size() == 1, "Partially filled ask should remain");
  require(book.asks[0].quantity == FixedPoint::fromString("3"),
          "Remaining ask quantity should be updated");
  require(book.bids.empty(), "Fully filled incoming bid should not rest");
}

void testMatchingEnginePriceTimePriority() {
  MatchingEngine engine;
  SubmitResult first = engine.submitLimitOrder(
      "ETH/USDT", Side::Sell, FixedPoint::fromString("100"),
      FixedPoint::fromString("1"));
  SubmitResult second = engine.submitLimitOrder(
      "ETH/USDT", Side::Sell, FixedPoint::fromString("100"),
      FixedPoint::fromString("1"));
  SubmitResult third = engine.submitLimitOrder(
      "ETH/USDT", Side::Sell, FixedPoint::fromString("99"),
      FixedPoint::fromString("1"));

  SubmitResult buy = engine.submitLimitOrder(
      "ETH/USDT", Side::Buy, FixedPoint::fromString("101"),
      FixedPoint::fromString("3"));

  require(buy.trades.size() == 3, "Buy should consume three resting asks");
  require(buy.trades[0].sellOrderId == third.orderId,
          "Lower ask price should trade first");
  require(buy.trades[1].sellOrderId == first.orderId,
          "At same price, earlier ask should trade first");
  require(buy.trades[2].sellOrderId == second.orderId,
          "At same price, later ask should trade second");
}

void testMatchingEngineBookStats() {
  MatchingEngine engine;
  engine.submitLimitOrder("ETH/USDT", Side::Buy, FixedPoint::fromString("99"),
                          FixedPoint::fromString("4"));
  engine.submitLimitOrder("ETH/USDT", Side::Buy, FixedPoint::fromString("98"),
                          FixedPoint::fromString("1"));
  engine.submitLimitOrder("ETH/USDT", Side::Sell,
                          FixedPoint::fromString("101"),
                          FixedPoint::fromString("5"));

  BookStats stats = engine.stats("ETH/USDT", 5);
  require(stats.hasBid, "Stats should report a best bid");
  require(stats.hasAsk, "Stats should report a best ask");
  require(stats.bestBid == FixedPoint::fromString("99"),
          "Best bid should be highest resting bid");
  require(stats.bestAsk == FixedPoint::fromString("101"),
          "Best ask should be lowest resting ask");
  require(stats.spread == FixedPoint::fromString("2"),
          "Spread should be ask minus bid");
  require(stats.midPrice == FixedPoint::fromString("100"),
          "Mid-price should average best bid and best ask");
  require(stats.bidQuantity == FixedPoint::fromString("5"),
          "Stats should aggregate bid depth quantity");
  require(stats.askQuantity == FixedPoint::fromString("5"),
          "Stats should aggregate ask depth quantity");
  require(almostEqual(stats.imbalance, 0.5L),
          "Imbalance should be bid depth over total depth");
}

void testMatchingEngineCancelsRestingBid() {
  MatchingEngine engine;
  SubmitResult bid = engine.submitLimitOrder(
      "ETH/USDT", Side::Buy, FixedPoint::fromString("99"),
      FixedPoint::fromString("2"));

  require(engine.cancelOrder(bid.orderId), "Open bid should be cancellable");
  BookSnapshot book = engine.snapshot("ETH/USDT", 5);
  require(book.bids.empty(), "Cancelled bid should leave the book");
}

void testMatchingEngineCancelsRestingAsk() {
  MatchingEngine engine;
  SubmitResult ask = engine.submitLimitOrder(
      "ETH/USDT", Side::Sell, FixedPoint::fromString("101"),
      FixedPoint::fromString("2"));

  require(engine.cancelOrder(ask.orderId), "Open ask should be cancellable");
  BookSnapshot book = engine.snapshot("ETH/USDT", 5);
  require(book.asks.empty(), "Cancelled ask should leave the book");
}

void testMatchingEngineRejectsMissingAndFilledCancellation() {
  MatchingEngine engine;
  SubmitResult ask = engine.submitLimitOrder(
      "ETH/USDT", Side::Sell, FixedPoint::fromString("100"),
      FixedPoint::fromString("1"));
  engine.submitLimitOrder("ETH/USDT", Side::Buy, FixedPoint::fromString("100"),
                          FixedPoint::fromString("1"));

  require(!engine.cancelOrder(ask.orderId),
          "Fully filled order should no longer be cancellable");
  require(!engine.cancelOrder(999), "Missing order should not be cancellable");
}

} // namespace

int main() {
  try {
    testCsvReaderRejectsBadRows();
    testOrderBookTimeNavigation();
    testPriceAggregates();
    testCandlesticksAndVolumes();
    testFixedPointParsing();
    testMatchingEngineDoesNotCrossWhenPricesMiss();
    testMatchingEnginePartialFill();
    testMatchingEnginePriceTimePriority();
    testMatchingEngineBookStats();
    testMatchingEngineCancelsRestingBid();
    testMatchingEngineCancelsRestingAsk();
    testMatchingEngineRejectsMissingAndFilledCancellation();
  } catch (const std::exception &e) {
    std::cerr << "Test failed: " << e.what() << std::endl;
    return 1;
  }

  std::cout << "All orderbook tests passed" << std::endl;
  return 0;
}
