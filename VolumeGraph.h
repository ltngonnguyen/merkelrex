// ADDITION #23 - VOLUMEGRAPH CLASS HEADER FILE
#pragma once

#include "Canvas.h"
#include "Coord.h"
#include "Helpers.h"
#include "Volume.h"

// This class is another child of the Canvas class, which shares all the public
// getters and setters and print methods. It can also access protected members
// such as width or height of the canvas. As a sibling of the CandlestickGraph
// class, it has many similarities with the elder sibling, yet is much more
// concise and considerably simpler.
class VolumeGraph : public Canvas {
private:
  std::vector<Volume> entries;

  const int X_ENTRY_LENGTH;
  const int Y_ENTRY_LENGTH;
  const int X_ENTRY_COUNT;
  const int Y_ENTRY_COUNT;
  const int X_INTERVAL;
  const int Y_INTERVAL;
  const int PADDING_LEFT;
  const int PADDING_BOTTOM;

  std::vector<Coord> xAxisPoints;

  Coord maxPointY;
  Coord minPointY;

public:
  // constructor - with all defaults defined
  VolumeGraph(std::vector<Volume> entries, int width = 132, int height = 33,
              int xEntryLength = 10, int xEntryCount = 3, int yEntryLength = 12,
              int yEntryCount = 10);

  // getters
  std::vector<Volume> getEntries() const;
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
  void setVolumeGraph();

  // other methods
  void prep();
};
