cmake_minimum_required(VERSION 3.16)

project(merkelrex LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(ORDERBOOK_SOURCES
    Candlestick.cpp
    CandlestickGraph.cpp
    Canvas.cpp
    Coord.cpp
    CSVReader.cpp
    FixedPoint.cpp
    Helpers.cpp
    MatchingEngine.cpp
    MerkelMain.cpp
    Order.cpp
    OrderBook.cpp
    OrderBookEntry.cpp
    Volume.cpp
    VolumeGraph.cpp
    Wallet.cpp
)

add_library(orderbook_core ${ORDERBOOK_SOURCES})
target_include_directories(orderbook_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(merkelrex main.cpp)
target_link_libraries(merkelrex PRIVATE orderbook_core)

enable_testing()
add_executable(orderbook_tests tests/orderbook_tests.cpp)
target_link_libraries(orderbook_tests PRIVATE orderbook_core)
add_test(NAME orderbook_tests COMMAND orderbook_tests)
