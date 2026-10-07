// ADDITION #12 - CANDLESTICK CLASS IMPLEMENTATION FILE
#include "Candlestick.h"

// constructor
Candlestick::Candlestick(std::string timestamp, long double high,
                         long double low, long double open, long double close)
    : timestamp(timestamp), high(high), low(low), open(open), close(close) {}

// getters
std::string Candlestick::getTimestamp() const { return timestamp; }
long double Candlestick::getHigh() const { return high; }
long double Candlestick::getLow() const { return low; }
long double Candlestick::getOpen() const { return open; }
long double Candlestick::getClose() const { return close; }

// setters
void Candlestick::setTimestamp(std::string timestamp) {
  this->timestamp = timestamp;
}
void Candlestick::setHigh(long double high) { this->high = high; }
void Candlestick::setLow(long double low) { this->low = low; }
void Candlestick::setOpen(long double open) { this->open = open; }
void Candlestick::setClose(long double close) { this->close = close; }
void Candlestick::setType(OrderBookType type) { this->type = type; }

// static methods
/** Calculate the ranges for the candlestick (from lowest to highest in the
 * input entries vector) to populate the Y axis of the Candlestick Graph
 * @Params entries: the input candlestick vector, min: lowest price in the input
 * entries, max: the reverse, entry_count: Y axis number of entries
 * @Return a vector of long doubles, each is an entry on the Y axis*/
std::vector<long double>
Candlestick::getIntervalRanges(std::vector<Candlestick> entries,
                               long double min, long double max,
                               int entry_count) {

  std::vector<long double> ranges;
  long double range = max - min;
  long double interval = range / entry_count;

  for (int i = entry_count - 1; i >= 0; i--) {
    ranges.push_back(min + (interval * i));
  }

  return ranges;
}

/** Get the lowest price in the input entries vector
 * @Params entries: the input candlestick vector
 * @Return the lowest price in the input entries vector*/
long double Candlestick::getMin(std::vector<Candlestick> entries) {
  long double min = entries[0].low;
  for (auto const &entry : entries) {
    if (entry.low < min) {
      min = entry.low;
    }
  }
  return min;
}

/** Get the highest price in the input entries vector
 * @Params entries: the input candlestick vector
 * @Return the highest price in the input entries vector*/
long double Candlestick::getMax(std::vector<Candlestick> entries) {
  long double max = entries[0].high;
  for (auto const &entry : entries) {
    if (entry.high > max) {
      max = entry.high;
    }
  }
  return max;
}
