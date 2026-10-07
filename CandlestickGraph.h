// ADDITION #19 - CANDLESTICKGRAPH CLASS HEADER FILE
#pragma once

#include "Canvas.h"
#include "Coord.h"
#include "Helpers.h"

// This class is a child of the Canvas class, which shares all the public
// getters and setters and print methods. It can also access protected members
// such as width or height of the canvas.
class CandlestickGraph : public Canvas {
private:
  std::vector<Candlestick> entries;

  const int X_ENTRY_LENGTH;
  const int Y_ENTRY_LENGTH;
  const int X_ENTRY_COUNT;
  const int Y_ENTRY_COUNT;
  const int X_INTERVAL;
  const int Y_INTERVAL;
  const int PADDING_LEFT;
  const int PADDING_BOTTOM;

  const int START_Y_DRAWING;

  long double entriesMin;
  long double entriesMax;
  std::vector<long double> ranges;
  std::vector<std::vector<long double>> candleCoords;
  std::vector<Coord> xAxisPoints;

  Coord maxPointY;
  Coord minPointY;

public:
  // constructor - with all defaults defined so that when we just want a
  // generic, safe graph, we can just pass a vector of candlesticks and that's
  // it, no need to pass all the other parameters
  CandlestickGraph(std::vector<Candlestick> entries, int width = 132,
                   int height = 33, int xEntryLength = 10, int xEntryCount = 8,
                   int yEntryLength = 12, int yEntryCount = 10);

  // getters
  std::vector<Candlestick> getEntries() const;
  int getXEntryLength() const;
  int getYEntryLength() const;
  int getXEntryCount() const;
  int getYEntryCount() const;
  int getXInterval() const;
  int getYInterval() const;
  int getPaddingLeft() const;
  int getPaddingBottom() const;

  // setters
  void setXAxis();
  void setYAxis();
  void completeYAxis();
  void completeXAxis();
  void setCandlestickGraph();

  // other methods
  void prep();
};
