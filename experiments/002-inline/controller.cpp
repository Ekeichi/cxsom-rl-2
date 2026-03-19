#include <algorithm>
#include <array>
#include <cstddef>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>

#include <gdyn.hpp>
#include <rllib2.hpp>

// Read these files.
#include "discrete-rocket-problem.hpp"
#include "my_rocket_config.hpp"

dir = "root-dir/in/"

#define DT_FACTOR .1
    struct target {
private:
  std::mt19937 &gen;
  std::uniform_real_distribution<double> d;
  std::size_t period;
  std::size_t count;

public:
  double height;

  target(std::mt19937 &gen, double margin, std::size_t period,
         const gdyn::problem::rocket::parameters &params)
      : gen(gen), d(margin, params.ceiling_height - margin), period(period),
        count(1) {
    ++(*this);
  }
  target &operator++() {
    if (--count == 0) {
      height = d(gen);
      count = period;
    }
    return *this;
  }
};

#define TARGET_PERIOD 20 // seconds
#define MARGIN 200
#define EPISODE_DURATION 10 * TARGET_PERIOD
int main(int argc, char *argv[]) { auto cxsom_controller = [] }