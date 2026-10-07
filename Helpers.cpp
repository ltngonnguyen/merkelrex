// ADDITION #16 - HELPER FUNCTIONS IMPLEMENTATION FILE
// This is a file that contains the implementation of assorted helper functions
// that doesn't fit in any classes of the project. My first instinct was to make
// this a class as well. However, I decided against it as those helper functions
// are used quite widely throughout the project and creating a Helper object
// everytime I want something quick and simple isn't an efficient use of memory.
// I also don't want to create a class full of static objects only, it just
// feels wrong to create an empty class with only static objects, and seem
// overly complicated while I can achieve the same effect with just function
// prototypes. Hence, this file.
#include "Helpers.h"
#include <cmath>

/** Function to tokenise timestamp by the space between the date and time, with
 * an added check for empty input, @Params a string of raw timestamp, @Returns a
 * 2-element vector, with date and time, ready to be written onto the X axis*/
std::vector<std::string> processTimestamp(std::string raw_timestamp) {

  std::vector<std::string> processed_timestamps = {"1970/01/01", "00:00:00.00"};
  // tokenise if not raw_timestamp not null, else return default
  if (raw_timestamp != "") {
    processed_timestamps = CSVReader::tokenise(raw_timestamp, ' ');
  }
  return processed_timestamps;
}

/** Function to return a string of a single character at a given index, @Params
 * an index and a string, @Returns a string of a single character at the given
 * index*/
std::string typewriter(int index, std::string text) {
  std::string str(1, text[index]);
  return str;
}

/** Function to map a value from one range to another, @Params a input value,
 * the input range, and the output range, @Returns a value mapped to the output
 * range*/
long double mapping(long double val, long double i_min, long double i_max,
                    long double o_min, long double o_max) {
  // This algorithm took me a while to figure out, mostly due to the offset of
  // -i_min and +o_min
  return (val - i_min) * (o_max - o_min) / (i_max - i_min) + o_min;
}

/** Function to round a long double to the nearest floor long double, @Params a
 * long double,
 * @Returns a vector of long doubles: the floored long double and the
 * remainder*/
std::vector<long double> processMappingUpper(long double value) {
  long double rounded = floor(value);
  long double difference = value - rounded;
  return {rounded, difference};
}

/** Function to round a long double to the nearest ceiling long double, @Params
 * a long double,
 * @Returns a vector of long doubles: the ceiled long double and the remainder*/
std::vector<long double> processMappingLower(long double value) {
  long double rounded = ceil(value);
  long double difference = rounded - value;
  return {rounded, difference};
}

/** Function to process the remainder of a value into appropriate candlestick
 * shapes on the top and bottom of the candlestick column, @Params a long double
 * and a string of the position, @Returns a string of the appropriate unicode
 * character*/
std::string processRemainder(long double remainder, std::string position) {
  // The partials vector contains the partial values of the candlestick shapes
  std::vector<long double> partials{1.0 / 8, 1.0 / 4, 3.0 / 8, 1.0 / 2,
                                    5.0 / 8, 3.0 / 4, 7.0 / 8};

  // The column_upper and column_lower vectors contain the unicode characters
  // for the candlestick shapes according to partials' index
  std::vector<std::string> column_upper{"▁", "▂", "▃", "▄", "▅", "▆", "▇"};

  std::vector<std::string> column_lower{"▔", "🮂", "🮃", "▀", "🮄", "🮅", "🮆"};

  // Iterate through the partials vector, and return the appropriate unicode
  // character if the remainder is smaller than the partial value
  if (position == "lower") {
    for (int i = 0; i < partials.size(); i++) {
      if (remainder <= partials[i]) {
        return column_lower[i];
      }
    }
  } else {
    for (int i = 0; i < partials.size(); i++) {
      if (remainder <= partials[i]) {
        return column_upper[i];
      }
    }
  }
  // If the remainder is larger than 7/8, return the full block unicode
  // character
  return "█";
}

/** Function to shorten a number to a more readable format, @Params a number,
 * @Returns a string of the shortened number*/
std::string shortenNumber(long long number) {
  // This algorithm is based on the metric unit prefixes, with the addition of
  // decimal places for numbers that are not multiples of 1000
  const std::vector<std::string> suffixes{"", "K", "M", "B"};
  const std::vector<long long> values{1LL, 1000LL, 1000000LL, 1000000000LL};

  // Iterate through the values vector, starting from the largest value
  for (int i = 3; i >= 0; i--) {
    // If the number is larger than the current value, divide the number by the
    // value and return the quotient with the appropriate suffix
    // If the number is not a multiple of the value, add a decimal place and
    // append the first 3 digits of the remainder
    if (number >= values[i]) {
      long long quotient = number / values[i];
      long long remainder = number % values[i];
      std::string quotient_str = std::to_string(quotient);
      std::string remainder_str = std::to_string(remainder);
      if (remainder != 0) {
        quotient_str += "." + remainder_str.substr(0, 3);
      }
      // Append the suffix and return the string
      quotient_str += suffixes[i];
      return quotient_str;
    }
  }
  // If the number is smaller than 1000, return the number as a string
  return std::to_string(number);
}

/** Function to return a colored string, @Params text: a string to be colored,
 * color: a string of the color to be used,
 * @Returns a string with the color code appened to the front and the reset code
 * to the back*/
std::string colorize(std::string text, std::string color) {
  std::string color_code = "";
  if (color == "red") {
    color_code = "\033[31m";
  } else if (color == "green") {
    color_code = "\033[32m";
  } else if (color == "yellow") {
    color_code = "\033[33m";
  } else if (color == "blue") {
    color_code = "\033[34m";
  } else if (color == "magenta") {
    color_code = "\033[35m";
  } else if (color == "cyan") {
    color_code = "\033[36m";
  } else if (color == "white") {
    color_code = "\033[37m";
  }
  return color_code + text + "\033[0m";
}
