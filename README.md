# Merkelrex: C++ Orderbook Simulator

Merkelrex is a C++ orderbook simulator and market-data visualizer built around a simple crypto exchange dataset. It started as coursework, but I wanted to keep improving it because the core problem is useful: load messy orderbook snapshots, model bids and asks, match orders, compute market statistics, and make the result inspectable from a terminal.

This is not trying to be a production exchange. Real exchanges need much stricter guarantees around precision, persistence, latency, concurrency, and risk controls. The goal here is more modest and more honest: show the backend pieces of a small exchange simulator in C++, with enough safety and tests that the project is not just a visual demo.

## What It Does

- Loads orderbook rows from CSV market-data snapshots.
- Parses bids and asks into C++ domain objects.
- Runs a separate price-time-priority matching engine for limit orders.
- Uses fixed-point integer values in the new matching core instead of floating point money.
- Reports live top-of-book stats: best bid, best ask, spread, mid-price, depth, and imbalance.
- Tracks known products such as `ETH/BTC` and `BTC/USDT`.
- Computes basic market statistics: high, low, weighted average open/close.
- Computes candlestick data from historical snapshots.
- Computes product volume in USDT terms.
- Simulates bid/ask matching and updates a user wallet after fills.
- Draws terminal candlestick and volume graphs using a small custom canvas class.
- Provides a CMake build and a small C++ test harness.

## Why This Project Exists

I wanted a project that sits closer to backend work than to a pure UI demo. The terminal graphs are the visible part, but the more interesting part is the orderbook logic behind them: ingestion, validation, filtering, matching, aggregation, and keeping the program safe when input is incomplete or malformed.

This also gave me a good reason to work in C++ without hiding everything behind a framework. Most of the project is plain classes and standard library containers, which makes the tradeoffs easier to see.

## Current Shape

```text
main.cpp                  Application entry point and CLI subcommands
MatchingEngine.*          Price-time-priority matching and live-book stats
FixedPoint.*              8-decimal fixed-point value type for matching logic
Order.* / Trade.*         Matching-engine domain objects
MerkelMain.*              Original interactive menu workflow
OrderBook.*               Market-data storage, filtering, legacy matching, OHLCV logic
OrderBookEntry.*          Single historical bid/ask/trade row
CSVReader.*               CSV parsing and row validation
Wallet.*                  User balance tracking
Candlestick.*             OHLC data model
Volume.*                  Volume data model
Canvas.*                  Terminal drawing surface
CandlestickGraph.*        Terminal candlestick renderer
VolumeGraph.*             Terminal volume renderer
sample_orders.csv         Small public demo file for the matching engine
tests/orderbook_tests.cpp Small no-dependency C++ test runner
```

## Build

The project uses CMake and C++17.

```bash
cmake -S . -B build
cmake --build build
```

Run the interactive app:

```bash
./build/merkelrex
```

Run the matching-engine demo:

```bash
./build/merkelrex match-demo
```

You can also pass a different 4-column order file:

```bash
./build/merkelrex match-demo path/to/orders.csv
```

Run the tests:

```bash
ctest --test-dir build --output-on-failure
```

## Data

The full historical CSV snapshots are intentionally not meant to be committed to a public GitHub repo. They are large local data files, so `.gitignore` excludes:

- `20200317.csv`
- `20200601.csv`

The app currently looks for `20200601.csv` in the project root. If that file is missing, the program now exits cleanly instead of crashing on an empty orderbook.

For the matching-engine path, the repo includes a tiny `sample_orders.csv` that is safe to commit and useful for demos. Its format is:

```text
symbol,side,price,quantity
ETH/USDT,sell,100.00,5
ETH/USDT,buy,100.50,3
```

The expected CSV format is:

```text
timestamp,product,side,price,amount
2020/06/01 11:57:30.328127,ETH/BTC,bid,0.02482205,23.9999428
```

Malformed rows, unknown sides, and non-positive price/amount values are skipped during loading. The loader reports how many valid rows were read and how many malformed rows were skipped.

## Tests

The tests are intentionally simple and framework-free for now. They cover the parts I care about most for a backend-style project:

- CSV parsing skips malformed rows.
- The orderbook exposes safe empty/non-empty state.
- Timestamp navigation works forward and backward.
- High, low, and weighted average calculations are correct.
- Candlestick and volume calculations return usable results from a small fixture.
- Fixed-point parsing preserves 8-decimal precision and rejects malformed values.
- Matching rejects non-crossing orders.
- Matching handles partial fills.
- Matching follows price-time priority.
- Live-book stats calculate best bid/ask, spread, mid-price, depth, and imbalance.

This is not a complete test suite yet, but it is a useful guardrail. Before adding more features, I would expand this around the matching engine and wallet settlement rules.

## Safety Improvements Made After The Original Coursework Version

- Added a proper `9: Quit` menu option.
- Stopped the CLI from looping forever when standard input reaches EOF.
- Added empty-data checks before reading the first order.
- Added CSV validation for malformed rows, unknown order sides, and non-positive values.
- Added safer handling for malformed product symbols.
- Removed the checked-in binary from the showcase copy.
- Added CMake build targets for the app and tests.
- Added a fixed-point matching-engine core separate from the older coursework classes.
- Added a public sample order file and a `match-demo` CLI command.
- Added live-book analytics for spread, mid-price, depth, and imbalance.

## What Worked

- The custom `Canvas` class made terminal graph rendering much easier to reason about than printing directly inside nested loops.
- Weighted average prices made the candlestick open/close values more meaningful than picking an arbitrary row.
- Separating `MatchingEngine` from the terminal UI made the exchange logic much easier to test.
- Adding book stats made the demo more backend-relevant than adding another visual chart.
- Keeping the app in C++ made the data structures and ownership model explicit.
- The terminal UI makes the project easy to demo without a browser or database.

## What Is Still Not Production-Grade

- The historical market-data path still uses `double`/`long double`; the newer matching engine uses fixed-point integers.
- The matching engine now supports price-time-priority matching, but it is still single-process and in-memory.
- There is no REST API yet.
- The CSV parser is deliberately small and does not handle every valid CSV edge case.
- There is no persistence layer, authentication, concurrency model, or audit log.

I am keeping these limitations visible because they are part of the project. The point is not to pretend this is a real exchange. The point is to show that I understand where the gap is between a learning project and backend software that handles money.

## Next Improvements

The next version I would build for a backend-focused portfolio would add:

- More matching tests, especially partial fills.
- JSON output mode for market stats and candles.
- Order cancellation.
- JSON output for the matching demo and live-book stats.
- A small REST API around the core engine.
- Sample screenshots or terminal recordings of the graph output.

## Main Takeaway

This project is useful because it touches several things that matter in exchange backend work: validating external data, modeling orders, computing market aggregates, handling bad input safely, and testing the parts where quiet mistakes can become expensive. The terminal visualization makes it easier to see, but the backend logic is the part I would keep building on.
