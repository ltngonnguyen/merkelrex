#pragma once
#include "CSVReader.h"
#include "Candlestick.h"
#include "OrderBookEntry.h"
#include "Volume.h"
#include <string>
#include <vector>

class OrderBook {
private:
  std::vector<OrderBookEntry> orders;

public:
  /** construct, reading a csv data file */
  OrderBook(std::string filename);
  bool isEmpty() const;
  std::size_t size() const;
  /** return vector of all know products in the dataset*/
  std::vector<std::string> getKnownProducts();
  /** return vector of Orders according to the sent filters*/
  std::vector<OrderBookEntry> getOrders(OrderBookType type, std::string product,
                                        std::string timestamp);

  /** returns the earliest time in the orderbook*/
  std::string getEarliestTime();
  /** returns the next time after the
   * sent time in the orderbook
   * If there is no next timestamp, wraps around to the start
   * */
  std::string getNextTime(std::string timestamp);

  /** returns the last time before the
   * sent time in the orderbook
   * If there is no last timestamp, return empty string
   * */

  std::string getLastTime(std::string timestamp);

  void insertOrder(OrderBookEntry &order);

  std::vector<OrderBookEntry> matchAsksToBids(std::string product,
                                              std::string timestamp);

  static long double getHighPrice(std::vector<OrderBookEntry> &orders);
  static long double getLowPrice(std::vector<OrderBookEntry> &orders);
  static long double
  getOpeningAndClosingPrice(std::vector<OrderBookEntry> &orders);
  std::vector<Candlestick> computeCandlesticks(std::string product,
                                               std::string type,
                                               std::string currentTime);
  std::vector<std::string> getIndividualProducts();
  std::vector<std::string> getSecondaryProducts();
  std::vector<Volume> computeVolumes(std::string currentTime);
  double averagePrice(std::string product, std::string timestamp);
};
