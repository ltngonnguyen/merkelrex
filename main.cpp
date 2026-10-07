#include "MerkelMain.h"
#include "CSVReader.h"
#include "MatchingEngine.h"
#include "MerkelTui.h"
#include "Wallet.h"

#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

struct DemoEvent {
  std::string type;
  int lineNumber = 0;
  bool success = false;
  std::uint64_t orderId = 0;
  std::string symbol;
  std::string side;
  std::string price;
  std::string quantity;
  std::string remainingQuantity;
  std::string message;
  std::vector<Trade> trades;
};

void addSymbol(std::vector<std::string> &symbols, const std::string &symbol) {
  for (const std::string &existing : symbols) {
    if (existing == symbol) {
      return;
    }
  }
  symbols.push_back(symbol);
}

std::string jsonEscape(const std::string &value) {
  std::string escaped;
  for (char c : value) {
    if (c == '"') {
      escaped += "\\\"";
    } else if (c == '\\') {
      escaped += "\\\\";
    } else if (c == '\n') {
      escaped += "\\n";
    } else {
      escaped += c;
    }
  }
  return escaped;
}

std::string jsonString(const std::string &value) {
  return "\"" + jsonEscape(value) + "\"";
}

void printJsonBookSide(const std::vector<BookLevel> &side) {
  std::cout << "[";
  for (std::size_t i = 0; i < side.size(); i++) {
    if (i > 0) {
      std::cout << ",";
    }
    std::cout << "{\"price\":" << jsonString(side[i].price.toString())
              << ",\"quantity\":" << jsonString(side[i].quantity.toString())
              << "}";
  }
  std::cout << "]";
}

void printJsonStats(const BookStats &stats) {
  std::cout << "{"
            << "\"bestBid\":"
            << (stats.hasBid ? jsonString(stats.bestBid.toString()) : "null")
            << ",\"bestAsk\":"
            << (stats.hasAsk ? jsonString(stats.bestAsk.toString()) : "null")
            << ",\"spread\":"
            << (stats.hasBid && stats.hasAsk ? jsonString(stats.spread.toString())
                                              : "null")
            << ",\"midPrice\":"
            << (stats.hasBid && stats.hasAsk ? jsonString(stats.midPrice.toString())
                                              : "null")
            << ",\"bidDepth\":" << jsonString(stats.bidQuantity.toString())
            << ",\"askDepth\":" << jsonString(stats.askQuantity.toString())
            << ",\"imbalance\":" << static_cast<double>(stats.imbalance)
            << "}";
}

void printJsonTrade(const Trade &trade) {
  std::cout << "{\"id\":" << trade.id
            << ",\"symbol\":" << jsonString(trade.symbol)
            << ",\"buyOrderId\":" << trade.buyOrderId
            << ",\"sellOrderId\":" << trade.sellOrderId
            << ",\"price\":" << jsonString(trade.price.toString())
            << ",\"quantity\":" << jsonString(trade.quantity.toString()) << "}";
}

void printJsonOutput(const std::string &path, const MatchingEngine &engine,
                     const std::vector<DemoEvent> &events,
                     const std::vector<std::string> &symbols, int accepted,
                     int cancelled, int skipped) {
  std::cout << "{\"inputFile\":" << jsonString(path)
            << ",\"summary\":{\"accepted\":" << accepted
            << ",\"cancelled\":" << cancelled << ",\"skipped\":" << skipped
            << ",\"trades\":" << engine.tradeHistory().size() << "}";

  std::cout << ",\"events\":[";
  for (std::size_t i = 0; i < events.size(); i++) {
    const DemoEvent &event = events[i];
    if (i > 0) {
      std::cout << ",";
    }
    std::cout << "{\"type\":" << jsonString(event.type)
              << ",\"line\":" << event.lineNumber
              << ",\"success\":" << (event.success ? "true" : "false");
    if (event.orderId != 0) {
      std::cout << ",\"orderId\":" << event.orderId;
    }
    if (!event.symbol.empty()) {
      std::cout << ",\"symbol\":" << jsonString(event.symbol);
    }
    if (!event.side.empty()) {
      std::cout << ",\"side\":" << jsonString(event.side);
    }
    if (!event.price.empty()) {
      std::cout << ",\"price\":" << jsonString(event.price);
    }
    if (!event.quantity.empty()) {
      std::cout << ",\"quantity\":" << jsonString(event.quantity);
    }
    if (!event.remainingQuantity.empty()) {
      std::cout << ",\"remainingQuantity\":"
                << jsonString(event.remainingQuantity);
    }
    if (!event.message.empty()) {
      std::cout << ",\"message\":" << jsonString(event.message);
    }
    std::cout << ",\"trades\":[";
    for (std::size_t j = 0; j < event.trades.size(); j++) {
      if (j > 0) {
        std::cout << ",";
      }
      printJsonTrade(event.trades[j]);
    }
    std::cout << "]}";
  }
  std::cout << "]";

  std::cout << ",\"books\":{"
            << "";
  for (std::size_t i = 0; i < symbols.size(); i++) {
    BookSnapshot book = engine.snapshot(symbols[i], 5);
    BookStats stats = engine.stats(symbols[i], 5);
    if (i > 0) {
      std::cout << ",";
    }
    std::cout << jsonString(symbols[i]) << ":{\"bids\":";
    printJsonBookSide(book.bids);
    std::cout << ",\"asks\":";
    printJsonBookSide(book.asks);
    std::cout << ",\"stats\":";
    printJsonStats(stats);
    std::cout << "}";
  }
  std::cout << "}}" << std::endl;
}

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

int runMatchDemo(const std::string &path, bool jsonMode) {
  std::ifstream file(path);
  if (!file.is_open()) {
    std::cerr << "Could not open sample order file: " << path << std::endl;
    return 1;
  }

  MatchingEngine engine;
  std::vector<DemoEvent> events;
  std::vector<std::string> symbols;
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
      DemoEvent event;
      event.type = "cancel";
      event.lineNumber = lineNumber;
      try {
        std::uint64_t orderId = std::stoull(tokens[1]);
        event.orderId = orderId;
        if (engine.cancelOrder(orderId)) {
          cancelled++;
          event.success = true;
          event.message = "cancelled";
          if (!jsonMode) {
            std::cout << "Cancelled order " << orderId << std::endl;
          }
        } else {
          skipped++;
          event.message = "order is not resting on the book";
          if (!jsonMode) {
            std::cout << "Could not cancel order " << orderId
                      << ": it is not resting on the book" << std::endl;
          }
        }
      } catch (const std::exception &e) {
        skipped++;
        event.message = e.what();
        if (!jsonMode) {
          std::cout << "Skipping line " << lineNumber << ": " << e.what()
                    << std::endl;
        }
      }
      events.push_back(event);
      continue;
    }

    if (tokens.size() != 4) {
      skipped++;
      events.push_back(DemoEvent{"skip", lineNumber, false, 0, "", "", "", "",
                                 "", "expected order row or cancel row", {}});
      if (!jsonMode) {
        std::cout << "Skipping line " << lineNumber
                  << ": expected order row or cancel row"
                  << std::endl;
      }
      continue;
    }

    DemoEvent event;
    event.type = "order";
    event.lineNumber = lineNumber;
    event.symbol = tokens[0];
    event.side = tokens[1];
    event.price = tokens[2];
    event.quantity = tokens[3];
    try {
      SubmitResult result = engine.submitLimitOrder(
          tokens[0], sideFromString(tokens[1]), FixedPoint::fromString(tokens[2]),
          FixedPoint::fromString(tokens[3]));
      accepted++;
      addSymbol(symbols, tokens[0]);
      event.success = true;
      event.orderId = result.orderId;
      event.remainingQuantity = result.remainingQuantity.toString();
      event.trades = result.trades;

      if (!jsonMode) {
        std::cout << "Accepted order " << result.orderId << ": " << tokens[1]
                  << " " << tokens[3] << " " << tokens[0] << " @ " << tokens[2]
                  << std::endl;
        for (const Trade &trade : result.trades) {
          std::cout << "  trade " << trade.id << ": "
                    << trade.quantity.toString() << " " << trade.symbol << " @ "
                    << trade.price.toString() << " (buy " << trade.buyOrderId
                    << ", sell " << trade.sellOrderId << ")" << std::endl;
        }
        if (result.trades.empty()) {
          std::cout << "  no trade, resting quantity "
                    << result.remainingQuantity.toString() << std::endl;
        } else if (result.remainingQuantity.raw() > 0) {
          std::cout << "  remaining quantity "
                    << result.remainingQuantity.toString() << " rests on book"
                    << std::endl;
        }
      }
    } catch (const std::exception &e) {
      skipped++;
      event.message = e.what();
      if (!jsonMode) {
        std::cout << "Skipping line " << lineNumber << ": " << e.what()
                  << std::endl;
      }
    }
    events.push_back(event);
  }

  if (jsonMode) {
    printJsonOutput(path, engine, events, symbols, accepted, cancelled, skipped);
    return 0;
  }

  std::cout << std::endl;
  std::cout << "Accepted " << accepted << " orders, cancelled " << cancelled
            << ", skipped " << skipped << " rows" << std::endl;
  std::cout << "Total trades: " << engine.tradeHistory().size() << std::endl;

  for (const std::string &symbol : symbols) {
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
  if (argc > 1 && std::string(argv[1]) == "tui") {
    return runMerkelTui();
  }

  if (argc > 1 &&
      (std::string(argv[1]) == "replay" || std::string(argv[1]) == "match-demo")) {
    std::string path = "sample_orders.csv";
    bool jsonMode = false;
    for (int i = 2; i < argc; i++) {
      std::string arg = argv[i];
      if (arg == "--json") {
        jsonMode = true;
      } else {
        path = arg;
      }
    }
    return runMatchDemo(path, jsonMode);
  }

  MerkelMain app{};
  app.init();
}
