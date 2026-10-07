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

void writeBookTickerFixture(const std::string &path) {
  std::ofstream file(path);
  file << "update_id,best_bid_price,best_bid_qty,best_ask_price,best_ask_qty,transaction_time,event_time\n";
  file << "1,0.24910000,123.00000000,0.24920000,8.00000000,1695945600934,1695945600949\n";
  file << "2,0.24900000,10.00000000,0.24930000,11.00000000,1695945601934,1695945601949\n";
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

void testCsvReaderParsesBinanceBookTickerRows() {
  const std::string path = "ADAUSD_TEST-bookTicker-fixture.csv";
  writeBookTickerFixture(path);

  std::vector<OrderBookEntry> entries = CSVReader::readCSV(path);
  require(entries.size() == 4,
          "Each bookTicker row should produce one bid and one ask entry");
  require(entries[0].product == "ADAUSD_TEST",
          "BookTicker product should come from the filename");
  require(entries[0].orderType == OrderBookType::bid,
          "First converted bookTicker entry should be bid");
  require(entries[1].orderType == OrderBookType::ask,
          "Second converted bookTicker entry should be ask");
  require(entries[0].timestamp == "2023-09-29 00:00:00.949",
          "BookTicker event_time should become a readable UTC timestamp");
  require(almostEqual(entries[0].price, 0.2491),
          "BookTicker bid price should be parsed");
  require(almostEqual(entries[1].amount, 8),
          "BookTicker ask quantity should be parsed");
}

void testBookTickerNotionalFallbackForSingleSymbolProducts() {
  const std::string path = "ADAUSD_TEST-bookTicker-fixture.csv";
  writeBookTickerFixture(path);
  OrderBook book{path};

  std::vector<Volume> volumes = book.computeVolumes("2023-09-29 00:00:00.949");
  require(volumes.size() == 1,
          "BookTicker single-symbol data should produce one notional bar");
  require(volumes[0].getProduct() == "ADAUSD_TEST",
          "BookTicker notional should keep the symbol name");
  require(almostEqual(volumes[0].getVolume(), 32.6329),
          "BookTicker notional should sum bid and ask price times quantity");
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

void testMatchingEngineSnapshotTruncatesDepth() {
  MatchingEngine engine;
  for (int i = 0; i < 5; ++i) {
    engine.submitLimitOrder("ETH/USDT", Side::Buy,
                            FixedPoint::fromString(std::to_string(99 - i)),
                            FixedPoint::fromString("1"));
    engine.submitLimitOrder("ETH/USDT", Side::Sell,
                            FixedPoint::fromString(std::to_string(101 + i)),
                            FixedPoint::fromString("1"));
  }

  BookSnapshot book = engine.snapshot("ETH/USDT", 2);
  require(book.bids.size() == 2, "Snapshot should truncate bid depth");
  require(book.asks.size() == 2, "Snapshot should truncate ask depth");
  require(book.bids[0].price == FixedPoint::fromString("99"),
          "Truncated bids should keep the best prices first");
  require(book.bids[1].price == FixedPoint::fromString("98"),
          "Second bid level should be the next best price");
  require(book.asks[0].price == FixedPoint::fromString("101"),
          "Truncated asks should keep the best prices first");
  require(book.asks[1].price == FixedPoint::fromString("102"),
          "Second ask level should be the next best price");

  BookStats stats = engine.stats("ETH/USDT", 2);
  require(stats.bidQuantity == FixedPoint::fromString("2"),
          "Stats should aggregate only the truncated bid depth");
  require(stats.askQuantity == FixedPoint::fromString("2"),
          "Stats should aggregate only the truncated ask depth");
  require(stats.spread == FixedPoint::fromString("2"),
          "Spread should come from the best levels only");
}

void testMatchingEngineSymbolsAreIsolated() {
  MatchingEngine engine;
  engine.submitLimitOrder("ETH/USDT", Side::Sell,
                          FixedPoint::fromString("100"),
                          FixedPoint::fromString("1"));
  engine.submitLimitOrder("BTC/USDT", Side::Sell,
                          FixedPoint::fromString("50000"),
                          FixedPoint::fromString("2"));

  SubmitResult buy = engine.submitLimitOrder(
      "ETH/USDT", Side::Buy, FixedPoint::fromString("100"),
      FixedPoint::fromString("1"));

  require(buy.trades.size() == 1, "Crossing ETH order should trade");
  require(buy.trades[0].symbol == "ETH/USDT",
          "Trade should record its own symbol");
  BookSnapshot btc = engine.snapshot("BTC/USDT", 5);
  require(btc.asks.size() == 1,
          "BTC book should be untouched by ETH matching");
  require(btc.asks[0].quantity == FixedPoint::fromString("2"),
          "BTC resting quantity should be unchanged");
  BookSnapshot eth = engine.snapshot("ETH/USDT", 5);
  require(eth.bids.empty() && eth.asks.empty(),
          "Fully matched ETH book should be empty");
}

void testMatchingEngineEmptyBookStats() {
  MatchingEngine engine;

  BookSnapshot book = engine.snapshot("ETH/USDT", 5);
  require(book.bids.empty() && book.asks.empty(),
          "Unknown symbol snapshot should be empty");

  BookStats stats = engine.stats("ETH/USDT", 5);
  require(!stats.hasBid, "Empty book should report no best bid");
  require(!stats.hasAsk, "Empty book should report no best ask");
  require(almostEqual(stats.imbalance, 0),
          "Empty book imbalance should default to zero");
  require(engine.tradeHistory().empty(),
          "Fresh engine should have no trade history");
}

} // namespace

int main() {
  try {
    testCsvReaderRejectsBadRows();
    testCsvReaderParsesBinanceBookTickerRows();
    testBookTickerNotionalFallbackForSingleSymbolProducts();
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
    testMatchingEngineSnapshotTruncatesDepth();
    testMatchingEngineSymbolsAreIsolated();
    testMatchingEngineEmptyBookStats();
  } catch (const std::exception &e) {
    std::cerr << "Test failed: " << e.what() << std::endl;
    return 1;
  }

  std::cout << "All orderbook tests passed" << std::endl;
  return 0;
}
