// ADDITION #20 - CANDLESTICKGRAPH CLASS IMPLEMENTATION FILE
// --- SATISFYING TASK 2 REQUIREMENTS ---

#include "CandlestickGraph.h"
#include "Helpers.h"

// constructor
CandlestickGraph::CandlestickGraph(std::vector<Candlestick> entries, int width,
                                   int height, int xEntryLength,
                                   int xEntryCount, int yEntryLength,
                                   int yEntryCount)
    : Canvas(width, height), entries(entries), X_ENTRY_LENGTH(xEntryLength),
      X_ENTRY_COUNT(entries.size()), Y_ENTRY_LENGTH(yEntryLength),
      Y_ENTRY_COUNT(yEntryCount),
      X_INTERVAL((width - (xEntryLength * xEntryCount)) / (xEntryCount + 1)),
      Y_INTERVAL(height / yEntryCount), PADDING_LEFT(width / 10),
      PADDING_BOTTOM(height / 10), maxPointY{PADDING_LEFT, 0},
      minPointY(PADDING_LEFT, 0),
      START_Y_DRAWING(height - PADDING_BOTTOM - Y_INTERVAL) {
  this->entriesMin = Candlestick::getMin(entries);
  this->entriesMax = Candlestick::getMax(entries);
  this->ranges = Candlestick::getIntervalRanges(entries, entriesMin, entriesMax,
                                                Y_ENTRY_COUNT);
}

// getters
std::vector<Candlestick> CandlestickGraph::getEntries() const {
  return entries;
}

int CandlestickGraph::getXEntryLength() const { return X_ENTRY_LENGTH; };
int CandlestickGraph::getYEntryLength() const { return Y_ENTRY_LENGTH; };
int CandlestickGraph::getXEntryCount() const { return X_ENTRY_COUNT; };
int CandlestickGraph::getYEntryCount() const { return Y_ENTRY_COUNT; }
int CandlestickGraph::getXInterval() const { return X_INTERVAL; }
int CandlestickGraph::getYInterval() const { return Y_INTERVAL; }
int CandlestickGraph::getPaddingLeft() const { return PADDING_LEFT; }
int CandlestickGraph::getPaddingBottom() const { return PADDING_BOTTOM; }

// setters
/** Functions to set canvas pixels to be a raw Cartesian system consists of x
 * and y axes*/
void CandlestickGraph::setXAxis() {
  setHorizontalLine(0, WIDTH, HEIGHT - PADDING_BOTTOM, "-");
}

void CandlestickGraph::setYAxis() {
  setVerticalLine(0, HEIGHT, PADDING_LEFT, "|");
}

/** Function to set Y axis, including Y entries, markers, and also a bit of code
 * to set the correct minPoint on the Y axis (aka the lowest marker on the Y
 * axis), and calculate the candlecoords*/
void CandlestickGraph::completeYAxis() {
  for (int y = 0; y < HEIGHT - PADDING_BOTTOM; y += Y_INTERVAL) {
    setPixel(PADDING_LEFT, y, "ᗎ"); // draw Y interval

    // draw Y entries
    std::string text = std::to_string(ranges[y / Y_INTERVAL]);
    if (text.length() < Y_ENTRY_LENGTH) {
      // prepend spaces to the text
      text = std::string(Y_ENTRY_LENGTH - text.length(), ' ') + text;
    }
    for (int x = 0; x < Y_ENTRY_LENGTH; x++) {
      setPixel(x, y, typewriter(x, text)); // write Y entries
    }

    // update min and conver candlesticks to coordinations
    if (y == (Y_ENTRY_COUNT - 1) * Y_INTERVAL) {
      minPointY.setY(y);
      // this actually doesn't really belong here, as it's not
      // technically setting anything, but because it depended
      // on minPointY, and minPointY depended on the setting of
      // the Y axis markers, I put it here for convenience
      // sake. Because even if I ended up taking it outside as
      // its own method, it would still have to be sequentially
      // coupled with this completeYAxis method and that only
      // clutter up the code. So I decided to put it here for
      // brevity's sake and reduce the likelihood of bugs caused by the
      // sequential coupling.
      candleCoords = Coord::convertCandlestickToCoord(
          entries, minPointY, maxPointY, entriesMin, entriesMax);
    }
  }
}

/** Function to set X axis, including X entries, markers, and also a bit of code
 * to push the markers on the X axis to a vector of coordinations*/
void CandlestickGraph::completeXAxis() {
  for (int x = PADDING_LEFT + X_INTERVAL + 1; x < WIDTH;
       x += (X_ENTRY_LENGTH + X_INTERVAL)) {

    setPixel(x, HEIGHT - PADDING_BOTTOM, "ᗗ"); // draw X interval

    // calculate the index of the entry to be used below
    int index =
        (x - (PADDING_LEFT + X_INTERVAL + 1)) / (X_ENTRY_LENGTH + X_INTERVAL);

    // save each X axis marker to a vector of coordinations
    if (index < entries.size() && entries[index].getTimestamp() != "") {
      xAxisPoints.push_back(Coord(x, HEIGHT - PADDING_BOTTOM));

      // use the processTimestamp helper function to split the timestamp into
      // two lines, and write them to the canvas
      std::vector<std::string> text =
          processTimestamp(entries[index].getTimestamp());

      for (int j = 0; j < X_ENTRY_LENGTH; j++) {
        setPixel(x + j - X_INTERVAL, xAxisPoints[index].getY() + 1,
                 typewriter(j, text[0]));
      }
      for (int k = 0; k < X_ENTRY_LENGTH; k++) {
        setPixel(x + k - X_INTERVAL, xAxisPoints[index].getY() + 2,
                 typewriter(k, text[1]));
      }
    }
  }
}

/** Function to set the candlestick graph, including the stalk of the candle,
 * the candlesticks, and the style of the candlesticks*/
void CandlestickGraph::setCandlestickGraph() {
  for (int i = 0; i < xAxisPoints.size(); i++) {
    int xAnchor = xAxisPoints[i].getX();
    // symbols
    std::string stalkSym;
    std::string candlestickSym;
    // maps and remainder for the candlesticks
    std::vector<long double> openMap;
    std::vector<long double> closeMap;
    std::string openRemainder;
    std::string closeRemainder;
    // get the open, close, high, and low values of the candlestick
    long double high = candleCoords[i][0];
    long double low = candleCoords[i][1];
    long double open = candleCoords[i][2];
    long double close = candleCoords[i][3];
    // check if the candlestick is green or red and if it's less than one tile
    bool isGreen = open < close;
    bool isLessThanOneTile = fabsl(open - close) < 1;
    // if the candlestick is red, change the color of the symbols
    if (!isGreen) {
      stalkSym = colorize("│", "red");
      candlestickSym = colorize("█", "red");
      // process the mapping and remainder of the candlestick, and the
      // remainders' colors
      openMap = processMappingUpper(open);
      openRemainder = colorize(processRemainder(openMap[1], "upper"), "red");
      closeMap = processMappingLower(close);
      closeRemainder = colorize(processRemainder(closeMap[1], "lower"), "red");
    } else {
      // if the candlestick is green, change the color of the symbols
      stalkSym = colorize("│", "green");
      candlestickSym = colorize("█", "green");
      // process the mapping and remainder of the candlestick, and the
      // remainders' colors
      openMap = processMappingLower(open);
      openRemainder = colorize(processRemainder(openMap[1], "lower"), "green");
      closeMap = processMappingUpper(close);
      closeRemainder =
          colorize(processRemainder(closeMap[1], "upper"), "green");
    }
    // some convenience variables to make the code more readable
    int openY = START_Y_DRAWING - openMap[0];
    int closeY = START_Y_DRAWING - closeMap[0];
    int roundY = START_Y_DRAWING - round(open);
    // start drawing the stalk
    setVerticalLine(START_Y_DRAWING - high, START_Y_DRAWING - low, xAnchor,
                    stalkSym);

    // start drawing the candlestick, we have 4 cases to consider
    // 1. the candlestick is red and not less than one tile
    if (!isGreen && !isLessThanOneTile) {
      // first we draw the candlestick
      setColumn(openY, closeY, xAnchor, candlestickSym, 3);
      // then we draw the remainders at the top and bottom of it
      setColumn(closeY, closeY, xAnchor, closeRemainder, 3);
      setColumn(openY - 1, openY - 1, xAnchor, openRemainder, 3);

      // 2. the candlestick is green and not less than one tile
    } else if (isGreen && !isLessThanOneTile) {
      // first we draw the candlestick
      setColumn(openY, closeY, xAnchor, candlestickSym, 3);
      // then we draw the remainders at the top and bottom of it
      setColumn(openY, openY, xAnchor, openRemainder, 3);
      setColumn(closeY - 1, closeY - 1, xAnchor, closeRemainder, 3);

      // 3. the candlestick is red and less than one tile
    } else if (!isGreen && isLessThanOneTile) {
      // check if it should be drawn below the mark
      if ((round(open) - open) > 0) {
        // do so if that's the case
        openRemainder =
            colorize(processRemainder(fabsl(open - close), "lower"), "red");
        setColumn(roundY, roundY, xAnchor, openRemainder, 3);
        // or above the mark
      } else {
        // do so if that's the case
        openRemainder =
            colorize(processRemainder(fabsl(open - close), "upper"), "red");
        setColumn(roundY - 1, roundY - 1, xAnchor, openRemainder, 3);
      }

      // 4. the candlestick is green and less than one tile
    } else if (isGreen && isLessThanOneTile) {
      // check if it should be drawn below the mark
      if (round(open) - open > 0) {
        // do so if that's the case
        openRemainder =
            colorize(processRemainder(fabsl(open - close), "lower"), "green");
        setColumn(roundY, roundY, xAnchor, openRemainder, 3);
      }
      // or above the mark
      else {
        // do so if that's the case
        openRemainder =
            colorize(processRemainder(fabsl(open - close), "upper"), "green");
        setColumn(roundY - 1, roundY - 1, xAnchor, openRemainder, 3);
      }
    }
    if (round(high) == floor(high) && !isGreen) {
      setPixel(xAnchor, START_Y_DRAWING - high, colorize("╷", "red"));
    } else if (round(high) == floor(high) && isGreen) {
      setPixel(xAnchor, START_Y_DRAWING - high, colorize("╷", "green"));
    }
    if (round(low) == ceil(low) && !isGreen) {
      setPixel(xAnchor, START_Y_DRAWING - low, colorize("╵", "red"));
    } else if (round(low) == ceil(low) && isGreen) {
      setPixel(xAnchor, START_Y_DRAWING - low, colorize("╵", "green"));
    }
  }
}

/** Function to bundle every neccessary actions to prepare a CandlestickGraph,
 * from nothing to being ready to be drawn to the canvas, as I don't want to
 * call these 5 methods everytime, cluttering up the code in main*/
void CandlestickGraph::prep() {
  setXAxis();
  setYAxis();
  completeYAxis();
  completeXAxis();
  setCandlestickGraph();
}
