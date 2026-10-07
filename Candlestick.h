// ADDITION #10 - CANDLESTICK CLASS HEADER FILE
// This file contains the header file for the Candlestick class, which is used
// to represent a candlestick in the candlestick chart. A candlestick object
// includes its timestamp, open, high, low, close, and type (bid/ask).
#pragma once

#include "OrderBookEntry.h"
#include <vector>

class Candlestick {
private:
  std::string timestamp;
  long double open;
  long double high;
  long double low;
  long double close;
  OrderBookType type;

public:
  // constructors
  Candlestick(std::string timestamp, long double open, long double high,
              long double low, long double close);

  // getters
  std::string getTimestamp() const;
  long double getOpen() const;
  long double getHigh() const;
  long double getLow() const;
  long double getClose() const;
  OrderBookType getType() const;

  // setters
  void setTimestamp(std::string timestamp);
  void setOpen(long double open);
  void setHigh(long double high);
  void setLow(long double low);
  void setClose(long double close);
  void setType(OrderBookType type);

  // static methods
  static std::vector<long double>
  getIntervalRanges(std::vector<Candlestick> entries, long double min,
                    long double max, int entry_count);
  static long double getMin(std::vector<Candlestick> entries);
  static long double getMax(std::vector<Candlestick> entries);
};
