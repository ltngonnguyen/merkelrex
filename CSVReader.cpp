#include "CSVReader.h"
#include <fstream>
#include <iostream>

CSVReader::CSVReader() {}

std::vector<OrderBookEntry> CSVReader::readCSV(std::string csvFilename) {
  std::vector<OrderBookEntry> entries;

  std::ifstream csvFile{csvFilename};
  std::string line;
  int skippedRows = 0;
  if (csvFile.is_open()) {
    while (std::getline(csvFile, line)) {
      try {
        OrderBookEntry obe = stringsToOBE(tokenise(line, ','));
        entries.push_back(obe);
      } catch (const std::exception &e) {
        skippedRows++;
      }
    } // end of while
  } else {
    std::cout << "CSVReader::readCSV could not open " << csvFilename
              << std::endl;
  }

  std::cout << "CSVReader::readCSV read " << entries.size() << " entries"
            << std::endl;
  if (skippedRows > 0) {
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
