#include "CSVReader.h"
#include <cstdio>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace {

bool endsWith(const std::string &value, const std::string &suffix) {
  if (suffix.size() > value.size()) {
    return false;
  }
  return value.compare(value.size() - suffix.size(), suffix.size(), suffix) == 0;
}

std::string shellQuote(const std::string &value) {
  std::string quoted = "'";
  for (char c : value) {
    if (c == '\'') {
      quoted += "'\\''";
    } else {
      quoted += c;
    }
  }
  quoted += "'";
  return quoted;
}

bool isBinanceBookTickerHeader(const std::string &line) {
  return line == "update_id,best_bid_price,best_bid_qty,best_ask_price,best_ask_qty,transaction_time,event_time";
}

std::string productFromFilename(const std::string &filename) {
  std::size_t slash = filename.find_last_of("/");
  std::string basename = slash == std::string::npos ? filename : filename.substr(slash + 1);
  std::size_t marker = basename.find("-bookTicker-");
  if (marker != std::string::npos) {
    return basename.substr(0, marker);
  }
  std::size_t dot = basename.find_last_of('.');
  return dot == std::string::npos ? basename : basename.substr(0, dot);
}

std::string formatTimestampMs(const std::string &timestampMs) {
  long long millis = std::stoll(timestampMs);
  std::time_t seconds = static_cast<std::time_t>(millis / 1000);
  int remainder = static_cast<int>(millis % 1000);

  std::tm *utc = std::gmtime(&seconds);
  if (utc == nullptr) {
    return timestampMs;
  }

  std::ostringstream output;
  output << std::put_time(utc, "%Y-%m-%d %H:%M:%S") << "."
         << std::setw(3) << std::setfill('0') << remainder;
  return output.str();
}

void addBinanceBookTickerRow(std::vector<OrderBookEntry> &entries,
                             const std::vector<std::string> &tokens,
                             const std::string &product) {
  if (tokens.size() != 7) {
    throw std::exception{};
  }

  std::string timestamp = formatTimestampMs(tokens[6]);

  entries.push_back(CSVReader::stringsToOBE(
      tokens[1], tokens[2], timestamp, product, OrderBookType::bid));
  entries.push_back(CSVReader::stringsToOBE(
      tokens[3], tokens[4], timestamp, product, OrderBookType::ask));
}

} // namespace

CSVReader::CSVReader() {}

std::vector<OrderBookEntry> CSVReader::readCSV(std::string csvFilename,
                                               bool verbose) {
  std::vector<OrderBookEntry> entries;
  std::string line;
  int skippedRows = 0;

  bool sawHeader = false;
  bool binanceBookTicker = false;
  std::string product = productFromFilename(csvFilename);

  auto processLine = [&](const std::string &line) {
    if (line.empty()) {
      return;
    }
    if (!sawHeader) {
      sawHeader = true;
      if (isBinanceBookTickerHeader(line)) {
        binanceBookTicker = true;
        return;
      }
    }

    if (binanceBookTicker) {
      try {
        addBinanceBookTickerRow(entries, tokenise(line, ','), product);
      } catch (const std::exception &e) {
        skippedRows++;
      }
      return;
    }

    try {
      OrderBookEntry obe = stringsToOBE(tokenise(line, ','));
      entries.push_back(obe);
    } catch (const std::exception &e) {
      skippedRows++;
    }
  };

  if (endsWith(csvFilename, ".zip")) {
    std::string command = "unzip -p " + shellQuote(csvFilename);
    FILE *pipe = popen(command.c_str(), "r");
    if (pipe == nullptr) {
      std::cout << "CSVReader::readCSV could not open zip " << csvFilename
                << std::endl;
      return entries;
    }

    char buffer[8192];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
      line = buffer;
      if (!line.empty() && line.back() == '\n') {
        line.pop_back();
      }
      if (!line.empty() && line.back() == '\r') {
        line.pop_back();
      }
      processLine(line);
    }
    pclose(pipe);
  } else {
    std::ifstream csvFile{csvFilename};
    if (csvFile.is_open()) {
      while (std::getline(csvFile, line)) {
        processLine(line);
      }
    } else {
      std::cout << "CSVReader::readCSV could not open " << csvFilename
                << std::endl;
    }
  }

  if (verbose) {
    std::cout << "CSVReader::readCSV read " << entries.size() << " entries"
              << std::endl;
  }
  if (verbose && skippedRows > 0) {
    std::cout << "CSVReader::readCSV skipped " << skippedRows
              << " malformed rows" << std::endl;
  }
  return entries;
}

std::vector<std::string> CSVReader::tokenise(std::string csvLine,
                                             char separator) {
  std::vector<std::string> tokens;
  signed int start, end;
  std::string token;
  start = csvLine.find_first_not_of(separator, 0);
  do {
    end = csvLine.find_first_of(separator, start);
    if (start == csvLine.length() || start == end)
      break;
    if (end >= 0)
      token = csvLine.substr(start, end - start);
    else
      token = csvLine.substr(start, csvLine.length() - start);
    tokens.push_back(token);
    start = end + 1;
  } while (end > 0);

  return tokens;
}

OrderBookEntry CSVReader::stringsToOBE(std::vector<std::string> tokens) {
  double price, amount;

  if (tokens.size() != 5) // bad
  {
    std::cout << "Bad line " << std::endl;
    throw std::exception{};
  }
  // we have 5 tokens
  try {
    price = std::stod(tokens[3]);
    amount = std::stod(tokens[4]);
  } catch (const std::exception &e) {
    std::cout << "CSVReader::stringsToOBE Bad float! " << tokens[3]
              << std::endl;
    std::cout << "CSVReader::stringsToOBE Bad float! " << tokens[4]
              << std::endl;
    throw;
  }

  OrderBookType type = OrderBookEntry::stringToOrderBookType(tokens[2]);
  if (type == OrderBookType::unknown || price <= 0 || amount <= 0) {
    throw std::exception{};
  }

  OrderBookEntry obe{price, amount, tokens[0], tokens[1], type};

  return obe;
}

OrderBookEntry CSVReader::stringsToOBE(std::string priceString,
                                       std::string amountString,
                                       std::string timestamp,
                                       std::string product,
                                       OrderBookType orderType) {
  double price, amount;
  try {
    price = std::stod(priceString);
    amount = std::stod(amountString);
  } catch (const std::exception &e) {
    std::cout << "CSVReader::stringsToOBE Bad float! " << priceString
              << std::endl;
    std::cout << "CSVReader::stringsToOBE Bad float! " << amountString
              << std::endl;
    throw;
  }
  if (orderType == OrderBookType::unknown || price <= 0 || amount <= 0) {
    throw std::exception{};
  }
  OrderBookEntry obe{price, amount, timestamp, product, orderType};

  return obe;
}
