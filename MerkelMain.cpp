#include "MerkelMain.h"
#include "CSVReader.h"
#include "Candlestick.h"
#include "CandlestickGraph.h"
#include "OrderBookEntry.h"
#include "VolumeGraph.h"
#include <iostream>
#include <vector>

MerkelMain::MerkelMain() {}

void MerkelMain::init() {
  int input;
  currentTime = orderBook.getEarliestTime();

  if (orderBook.isEmpty()) {
    std::cout << "No order data was loaded. Check that the CSV file exists and "
                 "has real market rows, not a Git LFS pointer."
              << std::endl;
    return;
  }

  wallet.insertCurrency("BTC", 10);

  bool running = true;
  while (running) {
    printMenu();
    input = getUserOption();
    running = processUserOption(input);
    lastTime = orderBook.getLastTime(currentTime);
  }
}

// ASCII art made using
// https://patorjk.com/software/taag/#p=display&f=Big%20Money-nw&t=Merkelrex
void MerkelMain::printMenu() {

  std::cout
      << "==============================================================="
         "====================================================================="
      << std::endl;

  std::cout << "$$\\      $$\\                     $$\\                 $$\\   "
               "                            \n";
  std::cout << "$$$\\    $$$ |                    $$ |                $$ |     "
               "                         \n";
  std::cout << "$$$$\\  $$$$ | $$$$$$\\   $$$$$$\\  $$ |  $$\\  $$$$$$\\  $$ | "
               "$$$$$$\\   $$$$$$\\  $$\\   $$\\ \n";
  std::cout << "$$\\$$\\$$ $$ |$$  __$$\\ $$  __$$\\ $$ | $$  |$$  __$$\\ $$ "
               "|$$  __$$\\ $$  __$$\\ \\$$\\ $$  |\n";
  std::cout << "$$ \\$$$  $$ |$$$$$$$$ |$$ |  \\__|$$$$$$  / $$$$$$$$ |$$ |$$ "
               "|  \\__|$$$$$$$$ | \\$$$$  / \n";
  std::cout << "$$ |\\$  /$$ |$$   ____|$$ |      $$  _$$<  $$   ____|$$ |$$ | "
               "     $$   ____| $$  $$<  \n";
  std::cout << "$$ | \\_/ $$ |\\$$$$$$$\\ $$ |      $$ | \\$$\\ \\$$$$$$$\\ $$ "
               "|$$ |      \\$$$$$$$\\ $$  /\\$$\\ \n";
  std::cout << "\\__|     \\__| \\_______|\\__|      \\__|  \\__| "
               "\\_______|\\__|\\__|       \\_______|\\__/  \\__|\n";

  // 1 print help
  std::cout << "1: Print help " << std::endl;
  // 2 print exchange stats
  std::cout << "2: Print exchange stats" << std::endl;
  // 3 make an offer
  std::cout << "3: Make an offer " << std::endl;
  // 4 make a bid
  std::cout << "4: Make a bid " << std::endl;
  // 5 print wallet
  std::cout << "5: Print wallet " << std::endl;
  // 6 draw candlestick
  std::cout << "6: Draw candlestick graph" << std::endl;
  // 7 draw volume graph
  std::cout << "7: Draw volume/notional graph" << std::endl;
  // 8 continue
  std::cout << "8: Continue" << std::endl;
  // 9 quit
  std::cout << "9: Quit" << std::endl;

  std::cout
      << "==============================================================="
         "====================================================================="
      << std::endl;

  std::cout << "Current time is: " << currentTime << std::endl;
  std::cout << "Last time is: " << lastTime << std::endl;
}

void MerkelMain::printHelp() {
  std::cout << "Help - your aim is to make money. Analyse the market and make "
               "bids and offers. "
            << std::endl;
}

void MerkelMain::printMarketStats() {

  for (std::string const &p : orderBook.getKnownProducts()) {
    std::cout << "Product: " << p << std::endl;
    std::vector<OrderBookEntry> entries_ask =
        orderBook.getOrders(OrderBookType::ask, p, currentTime);
    std::vector<OrderBookEntry> entries_ask_last =
        orderBook.getOrders(OrderBookType::ask, p, lastTime);
    std::vector<OrderBookEntry> entries_bid =
        orderBook.getOrders(OrderBookType::bid, p, currentTime);
    std::vector<OrderBookEntry> entries_bid_last =
        orderBook.getOrders(OrderBookType::bid, p, lastTime);

    std::cout << "Asks seen: " << entries_ask.size() << std::endl;
    std::cout << "Max ask: " << OrderBook::getHighPrice(entries_ask)
              << std::endl;
    std::cout << "Min ask: " << OrderBook::getLowPrice(entries_ask)
              << std::endl;
    std::cout << "Opening ask: "
              << OrderBook::getOpeningAndClosingPrice(entries_ask_last)
              << std::endl;
    std::cout << "Closing ask: "
              << OrderBook::getOpeningAndClosingPrice(entries_ask) << std::endl;

    std::cout << "Bids seen: " << entries_bid.size() << std::endl;
    std::cout << "Max bid: " << OrderBook::getHighPrice(entries_bid)
              << std::endl;
    std::cout << "Min bid: " << OrderBook::getLowPrice(entries_bid)
              << std::endl;
    std::cout << "Opening bid: "
              << OrderBook::getOpeningAndClosingPrice(entries_bid_last)
              << std::endl;
    std::cout << "Closing bid: "
              << OrderBook::getOpeningAndClosingPrice(entries_bid) << std::endl;
  }
}

void MerkelMain::enterAsk() {
  std::cout << "Make an ask - enter the amount: product,price, amount, eg  "
               "ETH/BTC,200,0.5"
            << std::endl;
  std::string input;
  std::getline(std::cin, input);

  std::vector<std::string> tokens = CSVReader::tokenise(input, ',');
  if (tokens.size() != 3) {
    std::cout << "MerkelMain::enterAsk Bad input! " << input << std::endl;
  } else {
    try {
      OrderBookEntry obe = CSVReader::stringsToOBE(
          tokens[1], tokens[2], currentTime, tokens[0], OrderBookType::ask);
      obe.username = "simuser";
      if (wallet.canFulfillOrder(obe)) {
        std::cout << "Wallet looks good. " << std::endl;
        orderBook.insertOrder(obe);
      } else {
        std::cout << "Wallet has insufficient funds . " << std::endl;
      }
    } catch (const std::exception &e) {
      std::cout << " MerkelMain::enterAsk Bad input " << std::endl;
    }
  }
}

void MerkelMain::enterBid() {
  std::cout << "Make an bid - enter the amount: product,price, amount, eg  "
               "ETH/BTC,200,0.5"
            << std::endl;
  std::string input;
  std::getline(std::cin, input);

  std::vector<std::string> tokens = CSVReader::tokenise(input, ',');
  if (tokens.size() != 3) {
    std::cout << "MerkelMain::enterBid Bad input! " << input << std::endl;
  } else {
    try {
      OrderBookEntry obe = CSVReader::stringsToOBE(
          tokens[1], tokens[2], currentTime, tokens[0], OrderBookType::bid);
      obe.username = "simuser";

      if (wallet.canFulfillOrder(obe)) {
        std::cout << "Wallet looks good. " << std::endl;
        orderBook.insertOrder(obe);
      } else {
        std::cout << "Wallet has insufficient funds . " << std::endl;
      }
    } catch (const std::exception &e) {
      std::cout << " MerkelMain::enterBid Bad input " << std::endl;
    }
  }
}

void MerkelMain::printWallet() { std::cout << wallet.toString() << std::endl; }

// #ADDITION #7
void MerkelMain::drawCandlesticks() {
  // get product and type input
  std::cout << "Enter the product / type pair"
               "draw - eg: ETH/BTC,ask"
            << std::endl;
  std::string input;
  std::getline(std::cin, input);

  // tokenise input by the comma, and check if the size is 2, if not throw error
  std::vector<std::string> tokens = CSVReader::tokenise(input, ',');
  if (tokens.size() != 2) {
    std::cout << "MerkelMain::drawCandlesticks Bad input! " << input
              << std::endl;
  } else {
    // try to calculate and draw candlestick
    try {
      // calculate candlestick from the current timestamp
      std::vector<Candlestick> candlesticks =
          orderBook.computeCandlesticks(tokens[0], tokens[1], currentTime);
      // prevent segfault if there is no data before the time or bad input
      if (candlesticks.empty()) {
        std::cout
            << "WARNING!!! No candlesticks to draw. Either there is no data \n"
               "before the current time or you misspelled the product!!!"
            << std::endl;
        return;
      } else {
        // check if the size of candlesticks is more than 8 (max amount of
        // candlesticks on the graph) i.e. when advance time
        if (candlesticks.size() > 8) {
          // delete the first entry of candlesticks until the size is 8
          while (candlesticks.size() > 8) {
            candlesticks.erase(candlesticks.begin());
          }
        }
        // draw candlestick using the CandlestickGraph class
        CandlestickGraph graph(candlesticks);
        graph.prep();
        graph.print();
      }
    } catch (const std::exception &e) {
      std::cout << " MerkelMain::drawCandlesticks Bad input " << std::endl;
    }
  }
}

// #ADDITION #8
// compute and then draw the volume/notional graph, based on the current
// timestamp, using the VolumeGraph class
void MerkelMain::drawVolumeGraph() {
  std::vector<Volume> volumes = orderBook.computeVolumes(currentTime);
  VolumeGraph graph(volumes);
  graph.prep();
  graph.print();
}

void MerkelMain::gotoNextTimeframe() {
  std::cout << "Going to next time frame. " << std::endl;
  for (std::string p : orderBook.getKnownProducts()) {
    std::cout << "matching " << p << std::endl;
    std::vector<OrderBookEntry> sales =
        orderBook.matchAsksToBids(p, currentTime);
    std::cout << "Sales: " << sales.size() << std::endl;
    for (OrderBookEntry &sale : sales) {
      std::cout << "Sale price: " << sale.price << " amount " << sale.amount
                << std::endl;
      if (sale.username == "simuser") {
        // update the wallet
        wallet.processSale(sale);
      }
    }
  }
  currentTime = orderBook.getNextTime(currentTime);
}

int MerkelMain::getUserOption() {
  int userOption = 0;
  std::string line;
  std::cout << "Type in 1-9" << std::endl;
  if (!std::getline(std::cin, line)) {
    std::cout << "No more input. Quitting." << std::endl;
    return 9;
  }
  try {
    userOption = std::stoi(line);
  } catch (const std::exception &e) {
    //
  }
  std::cout << "You chose: " << userOption << std::endl;
  std::cout
      << "==============================================================="
         "====================================================================="
      << std::endl;
  return userOption;
}

bool MerkelMain::processUserOption(int userOption) {
  if (userOption == 0) // bad input
  {
    std::cout << "Invalid choice. Choose 1-9" << std::endl;
  }
  if (userOption == 1) {
    printHelp();
  }
  if (userOption == 2) {
    printMarketStats();
  }
  if (userOption == 3) {
    enterAsk();
  }
  if (userOption == 4) {
    enterBid();
  }
  if (userOption == 5) {
    printWallet();
  }
  if (userOption == 6) {
    drawCandlesticks();
  }
  if (userOption == 7) {
    drawVolumeGraph();
  }
  if (userOption == 8) {
    gotoNextTimeframe();
  }
  if (userOption == 9) {
    std::cout << "Quitting Merkelrex." << std::endl;
    return false;
  }
  return true;
}
