#include "MerkelMain.h"
#include "CSVReader.h"
#include "MatchingEngine.h"
#include "Wallet.h"

#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void printBookSide(const std::string &title, const std::vector<BookLevel> &side) {
  std::cout << title << std::endl;
  if (side.empty()) {
    std::cout << "  (empty)" << std::endl;
    return;
  }
  for (const BookLevel &level : side) {
    std::cout << "  " << level.quantity.toString() << " @ "
              << level.price.toString() << std::endl;
  }
}

void printStats(const BookStats &stats) {
  std::cout << "Stats" << std::endl;
  std::cout << "  best bid: "
            << (stats.hasBid ? stats.bestBid.toString() : "n/a") << std::endl;
  std::cout << "  best ask: "
            << (stats.hasAsk ? stats.bestAsk.toString() : "n/a") << std::endl;
  std::cout << "  spread: "
            << (stats.hasBid && stats.hasAsk ? stats.spread.toString() : "n/a")
            << std::endl;
  std::cout << "  mid price: "
            << (stats.hasBid && stats.hasAsk ? stats.midPrice.toString() : "n/a")
            << std::endl;
  std::cout << "  bid depth: " << stats.bidQuantity.toString() << std::endl;
  std::cout << "  ask depth: " << stats.askQuantity.toString() << std::endl;
  std::cout << "  imbalance: " << static_cast<double>(stats.imbalance)
            << std::endl;
}

int runMatchDemo(const std::string &path) {
  std::ifstream file(path);
  if (!file.is_open()) {
    std::cout << "Could not open sample order file: " << path << std::endl;
    return 1;
  }

  MatchingEngine engine;
  std::string line;
  int lineNumber = 0;
  int accepted = 0;
  int cancelled = 0;
  int skipped = 0;

  while (std::getline(file, line)) {
    lineNumber++;
    if (line.empty() || line[0] == '#') {
      continue;
    }

    std::vector<std::string> tokens = CSVReader::tokenise(line, ',');
    if (tokens.size() == 2 && tokens[0] == "cancel") {
      try {
        std::uint64_t orderId = std::stoull(tokens[1]);
        if (engine.cancelOrder(orderId)) {
          cancelled++;
          std::cout << "Cancelled order " << orderId << std::endl;
        } else {
          skipped++;
          std::cout << "Could not cancel order " << orderId
                    << ": it is not resting on the book" << std::endl;
        }
      } catch (const std::exception &e) {
        skipped++;
        std::cout << "Skipping line " << lineNumber << ": " << e.what()
                  << std::endl;
      }
      continue;
    }

    if (tokens.size() != 4) {
      skipped++;
      std::cout << "Skipping line " << lineNumber
                << ": expected order row or cancel row"
                << std::endl;
      continue;
    }

    try {
      SubmitResult result = engine.submitLimitOrder(
          tokens[0], sideFromString(tokens[1]), FixedPoint::fromString(tokens[2]),
          FixedPoint::fromString(tokens[3]));
      accepted++;

      std::cout << "Accepted order " << result.orderId << ": " << tokens[1]
                << " " << tokens[3] << " " << tokens[0] << " @ " << tokens[2]
                << std::endl;
      for (const Trade &trade : result.trades) {
        std::cout << "  trade " << trade.id << ": " << trade.quantity.toString()
                  << " " << trade.symbol << " @ " << trade.price.toString()
                  << " (buy " << trade.buyOrderId << ", sell "
                  << trade.sellOrderId << ")" << std::endl;
      }
      if (result.trades.empty()) {
        std::cout << "  no trade, resting quantity "
                  << result.remainingQuantity.toString() << std::endl;
      } else if (result.remainingQuantity.raw() > 0) {
        std::cout << "  remaining quantity "
                  << result.remainingQuantity.toString() << " rests on book"
                  << std::endl;
      }
    } catch (const std::exception &e) {
      skipped++;
      std::cout << "Skipping line " << lineNumber << ": " << e.what()
                << std::endl;
    }
  }

  std::cout << std::endl;
  std::cout << "Accepted " << accepted << " orders, cancelled " << cancelled
            << ", skipped " << skipped << " rows" << std::endl;
  std::cout << "Total trades: " << engine.tradeHistory().size() << std::endl;

  for (const std::string &symbol : {std::string("ETH/USDT"),
                                   std::string("BTC/USDT")}) {
    BookSnapshot book = engine.snapshot(symbol, 5);
    BookStats stats = engine.stats(symbol, 5);
    std::cout << std::endl << symbol << " book" << std::endl;
    printBookSide("Bids", book.bids);
    printBookSide("Asks", book.asks);
    printStats(stats);
  }

  return 0;
}

} // namespace

int main(int argc, char *argv[]) {
  if (argc > 1 && std::string(argv[1]) == "match-demo") {
    std::string path = argc > 2 ? argv[2] : "sample_orders.csv";
    return runMatchDemo(path);
  }

  MerkelMain app{};
  app.init();
}
