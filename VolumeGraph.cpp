// ADDITION #24 - VOLUMEGRAPH CLASS IMPLEMENTATION FILE
// --- SATISFYING TASK 3 REQUIREMENTS ---
#include "VolumeGraph.h"
#include "Helpers.h"
#include <cmath>

// constructor
VolumeGraph::VolumeGraph(std::vector<Volume> entries, int width, int height,
                         int xEntryLength, int xEntryCount, int yEntryLength,
                         int yEntryCount)
    : Canvas(width, height), entries(entries), X_ENTRY_LENGTH(xEntryLength),
      X_ENTRY_COUNT(entries.size()), Y_ENTRY_LENGTH(yEntryLength),
      Y_ENTRY_COUNT(yEntryCount), PADDING_LEFT(width / 10),
      PADDING_BOTTOM(height / 10), maxPointY{PADDING_LEFT, 0},
      minPointY(PADDING_LEFT, 0),
      X_INTERVAL((width - (width / 10)) / (xEntryCount + 1)),
      Y_INTERVAL(height / yEntryCount) {}

// getters
std::vector<Volume> VolumeGraph::getEntries() const { return entries; }
int VolumeGraph::getXEntryLength() const { return X_ENTRY_LENGTH; };
int VolumeGraph::getYEntryLength() const { return Y_ENTRY_LENGTH; };
int VolumeGraph::getXEntryCount() const { return X_ENTRY_COUNT; };
int VolumeGraph::getYEntryCount() const { return Y_ENTRY_COUNT; }
int VolumeGraph::getXInterval() const { return X_INTERVAL; }
int VolumeGraph::getYInterval() const { return Y_INTERVAL; }
int VolumeGraph::getPaddingLeft() const { return PADDING_LEFT; }
int VolumeGraph::getPaddingBottom() const { return PADDING_BOTTOM; }

// setters
/** Functions to set canvas pixels to be a raw Cartesian system consists of x
 * and y axes*/
void VolumeGraph::setXAxis() {
  setHorizontalLine(0, WIDTH, HEIGHT - PADDING_BOTTOM, "-");
}

void VolumeGraph::setYAxis() { setVerticalLine(0, HEIGHT, PADDING_LEFT, "|"); }

/** Function to set Y axis, including Y entries, markers, and also a bit of code
 * to set the correct minPoint on the Y axis (aka the lowest marker on the Y
 * axis)*/
void VolumeGraph::completeYAxis() {
  for (int y = 0; y < HEIGHT - PADDING_BOTTOM; y += Y_INTERVAL) {
    long double totalVolume = Volume::getTotalVolume(entries);
    int index = y / Y_INTERVAL;
    setPixel(PADDING_LEFT, y, "ᗎ"); // draw Y interval

    // draw Y entries
    std::string text =
        shortenNumber(totalVolume - (totalVolume / Y_ENTRY_COUNT) * index);
    // prepend to the text with spaces if the text is shorter than the length
    // of the Y entries
    if (text.length() < Y_ENTRY_LENGTH) {
      text = std::string(Y_ENTRY_LENGTH - text.length(), ' ') + text;
    }
    for (int x = 0; x < Y_ENTRY_LENGTH; x++) {
      setPixel(x, y, typewriter(x, text)); // write Y entries
    }

    // update min point on Y axis
    if (y == Y_ENTRY_COUNT * Y_INTERVAL) {
      minPointY.setY(y);
    }
  }
}

/** Function to set X axis, including X entries, markers*/
void VolumeGraph::completeXAxis() {
  for (int x = PADDING_LEFT + X_INTERVAL; x < WIDTH; x += X_INTERVAL) {

    // get index
    int index = (x - (PADDING_LEFT + X_INTERVAL)) / X_INTERVAL;

    if (index < entries.size()) {
      setPixel(x, HEIGHT - PADDING_BOTTOM, "ᗗ"); // draw X interval
      xAxisPoints.push_back(Coord(x, HEIGHT - PADDING_BOTTOM));

      std::string text = entries[index].getProduct();

      for (int j = 0; j < text.size(); j++) {
        setPixel(x + j - 1, xAxisPoints[index].getY() + 1,
                 typewriter(j, text)); // draw X entries
      }
    }
  }
}

/** Function to set the volume graph, including the percentage of each volume
 * entry*/
void VolumeGraph::setVolumeGraph() {
  // get total volume of all entries
  long double totalVolume = Volume::getTotalVolume(entries);

  for (int i = 0; i < xAxisPoints.size(); i++) {
    int xAnchor = xAxisPoints[i].getX();
    // some convenience variables to make the code more readable
    int yFoot = HEIGHT - PADDING_BOTTOM - 1;
    std::vector<long double> drawVector = processMappingUpper(
        mapping(entries[i].getVolume(), 0, totalVolume, 0,
                yFoot)); // get y location of each entry by using the helper
                         // mapping function
    int yLoc = drawVector[0];
    int yRemain = drawVector[1];
    int yHead = HEIGHT - PADDING_BOTTOM - yLoc - 1;
    std::string sym = processRemainder(yRemain, "upper");
    std::string fullSym = "\033[33;41m" + sym.append("\033[0m");
    // get percentage of each entry, creating the entry text
    long double percentage = entries[i].getVolume() / totalVolume * 100;
    std::string text = std::to_string(percentage) + "%";
    // draw the columns, width 5
    setColumn(yFoot, maxPointY.getY(), xAnchor, colorize("█", "red"),
              5); // background column in red
    // We have 4 cases here:
    // 1. yLoc > 0 and no remainder
    if (yLoc > 0 && yRemain == 0)
      // draw the column with the full symbol
      setColumn(yFoot, yHead, xAnchor, colorize("█", "yellow"), 5);
    // 2. yLoc > 0 and there is remainder
    else if (yLoc > 0 && yRemain > 0) {
      // draw the column with the full symbol
      setColumn(yFoot, yHead, xAnchor, colorize("█", "yellow"), 5);
      // then draw the remainder on top
      setColumn(yHead - 1, yHead - 1, xAnchor, fullSym, 5);
    }
    // 3/ yLoc == 0 and there is remainder
    else if (yLoc == 0 && yRemain > 0) {
      // no full column, just draw the remainder
      setColumn(yFoot, yHead, xAnchor, fullSym, 5);
    }
    // draw the entry text (names of the product)
    for (int j = 0; j < text.size(); j++) {
      setPixel(xAnchor + j + 3, yHead, typewriter(j, text));
    }
    // draw the volume text
    std::string volumeText = shortenNumber(entries[i].getVolume());
    for (int j = 0; j < volumeText.size(); j++) {
      setPixel(xAnchor + j - (volumeText.size() + 2), yHead,
               typewriter(j, volumeText));
    }
  }
}

/** Function to bundle every neccessary actions to prepare a CandlestickGraph,
 * from nothing to being ready to be drawn to the canvas, as I don't want to
 * call these 5 methods everytime, cluttering up the code in main*/
void VolumeGraph::prep() {
  setXAxis();
  setYAxis();
  completeYAxis();
  completeXAxis();
  setVolumeGraph();
}
