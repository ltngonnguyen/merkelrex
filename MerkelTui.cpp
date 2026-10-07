#include "MerkelTui.h"

#include "OrderBook.h"
#include "OrderBookEntry.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

namespace {

struct MarketSnapshot {
  std::string product;
  std::string timestamp;
  double bidPrice = 0;
  double bidQty = 0;
  double askPrice = 0;
  double askQty = 0;
  double spread = 0;
  double mid = 0;
  bool complete = false;
};

long long timestampToMillis(const std::string &timestamp) {
  int year = 0;
  int month = 0;
  int day = 0;
  int hour = 0;
  int minute = 0;
  int second = 0;
  int millis = 0;

  if (std::sscanf(timestamp.c_str(), "%d-%d-%d %d:%d:%d.%d", &year, &month,
                  &day, &hour, &minute, &second, &millis) != 7) {
    return -1;
  }

  long long days = static_cast<long long>(year) * 372 + month * 31 + day;
  return (((days * 24 + hour) * 60 + minute) * 60 + second) * 1000 + millis;
}

std::string formatDouble(double value, int precision) {
  std::ostringstream output;
  output << std::fixed << std::setprecision(precision) << value;
  return output.str();
}

std::string repeatBlock(const std::string &block, int count) {
  std::string output;
  for (int i = 0; i < count; ++i) {
    output += block;
  }
  return output;
}

std::string bar(double value, double maxValue, int width) {
  if (maxValue <= 0) {
    return repeatBlock("░", width);
  }
  int filled = static_cast<int>((value / maxValue) * width);
  filled = std::max(0, std::min(width, filled));
  return repeatBlock("█", filled) + repeatBlock("░", width - filled);
}

std::string pulse(int frame, int width) {
  static const std::vector<std::string> glyphs = {"░", "▒", "▓", "█", "▓", "▒"};
  std::string output;
  for (int i = 0; i < width; ++i) {
    output += glyphs[(i + frame) % glyphs.size()];
  }
  return output;
}

std::string sparkline(const std::vector<double> &values) {
  static const std::vector<std::string> marks = {"▁", "▂", "▃", "▄", "▅", "▆", "▇", "█"};
  if (values.empty()) {
    return "awaiting market data";
  }

  auto [minIt, maxIt] = std::minmax_element(values.begin(), values.end());
  double minValue = *minIt;
  double maxValue = *maxIt;
  std::string output;
  for (double value : values) {
    int index = 0;
    if (maxValue > minValue) {
      index = static_cast<int>(((value - minValue) / (maxValue - minValue)) *
                               (marks.size() - 1));
    }
    output += marks[std::max(0, std::min(static_cast<int>(marks.size()) - 1,
                                         index))];
  }
  return output;
}

class MarketPlayback {
public:
  explicit MarketPlayback(OrderBook &book) : orderBook(book) {
    products = orderBook.getKnownProducts();
    if (!products.empty()) {
      product = products[0];
    }
    currentTime = orderBook.getEarliestTime();
    autoDelayMs = estimateDelayMs();
    delayMs = autoDelayMs;
    updateSnapshot();
  }

  void step() {
    previousBid = snapshot.bidPrice;
    previousAsk = snapshot.askPrice;
    previousSpread = snapshot.spread;
    currentTime = orderBook.getNextTime(currentTime);
    updateSnapshot();
  }

  int estimateDelayMs() {
    std::vector<int> deltas;
    std::string timestamp = currentTime;
    for (int i = 0; i < 100; ++i) {
      std::string next = orderBook.getNextTime(timestamp);
      if (next.empty() || next <= timestamp) {
        break;
      }

      long long currentMillis = timestampToMillis(timestamp);
      long long nextMillis = timestampToMillis(next);
      if (currentMillis >= 0 && nextMillis > currentMillis) {
        deltas.push_back(static_cast<int>(nextMillis - currentMillis));
      }
      timestamp = next;
    }

    if (deltas.empty()) {
      return 100;
    }

    std::sort(deltas.begin(), deltas.end());
    int median = deltas[deltas.size() / 2];
    return std::max(35, std::min(750, median));
  }

  void faster() { delayMs = std::max(20, delayMs - 15); }
  void slower() { delayMs = std::min(1500, delayMs + 25); }
  void resetDelay() { delayMs = autoDelayMs; }

  OrderBook &orderBook;
  std::vector<std::string> products;
  std::string product;
  std::string currentTime;
  MarketSnapshot snapshot;
  std::vector<double> midHistory;
  double previousBid = 0;
  double previousAsk = 0;
  double previousSpread = 0;
  int autoDelayMs = 100;
  int delayMs = 100;

private:
  void updateSnapshot() {
    MarketSnapshot next;
    next.product = product;
    next.timestamp = currentTime;

    std::vector<OrderBookEntry> asks =
        orderBook.getOrders(OrderBookType::ask, product, currentTime);
    std::vector<OrderBookEntry> bids =
        orderBook.getOrders(OrderBookType::bid, product, currentTime);
    if (!asks.empty() && !bids.empty()) {
      OrderBookEntry bestAsk = asks[0];
      OrderBookEntry bestBid = bids[0];
      for (const OrderBookEntry &ask : asks) {
        if (ask.price < bestAsk.price) {
          bestAsk = ask;
        }
      }
      for (const OrderBookEntry &bid : bids) {
        if (bid.price > bestBid.price) {
          bestBid = bid;
        }
      }

      next.bidPrice = bestBid.price;
      next.bidQty = bestBid.amount;
      next.askPrice = bestAsk.price;
      next.askQty = bestAsk.amount;
      next.spread = bestAsk.price - bestBid.price;
      next.mid = (bestAsk.price + bestBid.price) / 2;
      next.complete = true;
    }

    snapshot = next;
    if (snapshot.complete) {
      midHistory.push_back(snapshot.mid);
      if (midHistory.size() > 56) {
        midHistory.erase(midHistory.begin());
      }
    }
  }
};

ftxui::Element metricPanel(const std::string &title, const std::string &value,
                           const std::string &subtitle, ftxui::Color panelColor,
                           bool flare) {
  using namespace ftxui;
  Decorator tone = color(panelColor);
  Decorator fill = flare ? bgcolor(Color::RGB(55, 48, 8)) : bgcolor(Color::RGB(8, 10, 18));
  return vbox({
             text(title) | bold | center | tone,
             separator(),
             text(value) | bold | center | tone,
             text(subtitle) | center | color(Color::GrayLight),
         }) |
         borderHeavy | fill | size(WIDTH, EQUAL, 24) | size(HEIGHT, EQUAL, 7);
}

ftxui::Element renderExchangeFloor(const MarketPlayback &playback, bool playing,
                                   int frame) {
  using namespace ftxui;
  const MarketSnapshot &s = playback.snapshot;
  double maxQty = std::max(s.bidQty, s.askQty);
  bool bidFlare = s.complete && s.bidPrice != playback.previousBid;
  bool askFlare = s.complete && s.askPrice != playback.previousAsk;
  bool spreadFlare = s.complete && s.spread != playback.previousSpread;
  std::string status = playing ? "REPLAY STREAM ACTIVE" : "REPLAY STREAM PAUSED";
  std::string pressure = s.bidQty >= s.askQty ? "BID SIDE" : "ASK SIDE";

  auto title = hbox({
                   text("  MERKELREX EXCHANGE FLOOR") | bold |
                       color(Color::RGB(0, 255, 214)),
                   filler(),
                   text(status + "  ") | bold |
                       color(playing ? Color::RGB(255, 65, 180)
                                     : Color::RGB(255, 215, 0)),
               }) |
               bgcolor(Color::RGB(7, 8, 22));

  auto subtitle = hbox({
                      text("  " + s.product) | bold | color(Color::RGB(255, 215, 0)),
                      filler(),
                      text(s.timestamp) | color(Color::RGB(160, 255, 255)),
                      filler(),
                      text("CADENCE " + std::to_string(playback.delayMs) + "ms  ") |
                          bold | color(Color::RGB(255, 85, 255)),
                  }) |
                  bgcolor(Color::RGB(16, 18, 36));

  auto panels = hbox({
                    metricPanel("BID LANE", formatDouble(s.bidPrice, 8),
                                "depth " + formatDouble(s.bidQty, 2),
                                Color::RGB(0, 255, 160), bidFlare),
                    text("  <<<  ") | bold | center | color(Color::RGB(255, 215, 0)),
                    metricPanel("SPREAD LANE", formatDouble(s.spread, 8),
                                "mid " + formatDouble(s.mid, 8),
                                Color::RGB(255, 215, 0), spreadFlare),
                    text("  >>>  ") | bold | center | color(Color::RGB(255, 215, 0)),
                    metricPanel("ASK LANE", formatDouble(s.askPrice, 8),
                                "depth " + formatDouble(s.askQty, 2),
                                Color::RGB(255, 72, 160), askFlare),
                }) |
                center;

  auto depth = vbox({
      text("DEPTH PULSE") | bold | color(Color::RGB(160, 255, 255)),
      hbox({text("BID  ") | bold | color(Color::RGB(0, 255, 160)),
            text(bar(s.bidQty, maxQty, 34)) | color(Color::RGB(0, 255, 160)),
            text(" " + formatDouble(s.bidQty, 2)) | bold}),
      hbox({text("ASK  ") | bold | color(Color::RGB(255, 72, 160)),
            text(bar(s.askQty, maxQty, 34)) | color(Color::RGB(255, 72, 160)),
            text(" " + formatDouble(s.askQty, 2)) | bold}),
  });

  auto runway = vbox({
      text("PRICE RUNWAY") | bold | color(Color::RGB(255, 215, 0)),
      text("◆ " + sparkline(playback.midHistory) + " ◆") | bold |
          color(Color::RGB(0, 255, 214)),
  });

  auto intensity = vbox({
      text("FLOW INTENSITY") | bold | color(Color::RGB(255, 85, 255)),
      hbox({text(pulse(frame, 28)) | bold | color(Color::RGB(255, 85, 255)),
            text("   PRESSURE: " + pressure) | bold |
                color(Color::RGB(255, 215, 0))}),
  });

  auto controls = text(" SPACE Play/Pause    N Next Tick    F/S Speed    R Auto    Q Exit ") |
                  bold | center | bgcolor(Color::RGB(20, 14, 35)) |
                  color(Color::RGB(230, 230, 255));

  return vbox({
             title,
             subtitle,
             separatorHeavy(),
             filler(),
             panels,
             filler(),
             depth,
             filler(),
             runway,
             filler(),
             intensity,
             filler(),
             separatorHeavy(),
             controls,
         }) |
         borderDouble | bgcolor(Color::RGB(4, 5, 13)) |
         color(Color::RGB(220, 230, 255));
}

} // namespace

int runMerkelTui() {
  OrderBook orderBook{"ADAUSD_230929-bookTicker-2023-09-29.zip", false};
  if (orderBook.isEmpty()) {
    return 1;
  }

  MarketPlayback playback(orderBook);
  std::mutex mutex;
  std::atomic<bool> running{true};
  std::atomic<bool> playing{true};
  int frame = 0;

  auto screen = ftxui::ScreenInteractive::Fullscreen();

  std::thread ticker([&] {
    while (running) {
      {
        std::lock_guard<std::mutex> lock(mutex);
        frame++;
        if (playing) {
          playback.step();
        }
      }
      screen.PostEvent(ftxui::Event::Custom);
      int delayMs = 50;
      {
        std::lock_guard<std::mutex> lock(mutex);
        delayMs = playback.delayMs;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
    }
  });

  auto renderer = ftxui::Renderer([&] {
    std::lock_guard<std::mutex> lock(mutex);
    return renderExchangeFloor(playback, playing, frame);
  });

  auto component = ftxui::CatchEvent(renderer, [&](ftxui::Event event) {
    std::lock_guard<std::mutex> lock(mutex);
    if (event == ftxui::Event::Character('q') ||
        event == ftxui::Event::Character('Q')) {
      running = false;
      screen.ExitLoopClosure()();
      return true;
    }
    if (event == ftxui::Event::Character(' ')) {
      playing = !playing;
      return true;
    }
    if (event == ftxui::Event::Character('n') ||
        event == ftxui::Event::Character('N')) {
      playback.step();
      return true;
    }
    if (event == ftxui::Event::Character('f') ||
        event == ftxui::Event::Character('F')) {
      playback.faster();
      return true;
    }
    if (event == ftxui::Event::Character('s') ||
        event == ftxui::Event::Character('S')) {
      playback.slower();
      return true;
    }
    if (event == ftxui::Event::Character('r') ||
        event == ftxui::Event::Character('R')) {
      playback.resetDelay();
      return true;
    }
    return false;
  });

  screen.Loop(component);
  running = false;
  if (ticker.joinable()) {
    ticker.join();
  }
  return 0;
}
