#pragma once

#include "OrderBook.h"
#include "OrderBookEntry.h"
#include "Wallet.h"
#include <vector>

class MerkelMain {
public:
  MerkelMain();
  /** Call this to start the sim */
  void init();

private:
  void printMenu();
  void printHelp();
  void printMarketStats();
  void enterAsk();
  void enterBid();
  void printWallet();
  void drawCandlesticks();
  void drawVolumeGraph();
  void gotoNextTimeframe();
  int getUserOption();
  bool processUserOption(int userOption);

  std::string currentTime;
  std::string lastTime;

  // OrderBook orderBook{"20200317.csv"};
  // OrderBook orderBook{"20200601.csv"};
  OrderBook orderBook{"ADAUSD_230929-bookTicker-2023-09-29.zip"};
  Wallet wallet;
};
