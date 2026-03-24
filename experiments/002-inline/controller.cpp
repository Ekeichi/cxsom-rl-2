#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <thread>

#include <cxsom/cxsomData.hpp>
#include <cxsom/cxsomSymbols.hpp>
#include <cxsom/cxsomVariable.hpp>

#include <gdyn.hpp>
#include <rllib2.hpp>

#include "predictions/discrete-rocket-problem.hpp"
#include "predictions/my_rocket_config.hpp"

namespace {

constexpr double DT_FACTOR = .1;
constexpr double TARGET_PERIOD = 20; // seconds
constexpr double MARGIN = 200;
constexpr double EPISODE_DURATION = 10 * TARGET_PERIOD;

constexpr double MIN_ERROR = -100.0;
constexpr double MAX_ERROR = 100.0;
constexpr double MIN_SPEED = -100.0;
constexpr double MAX_SPEED = 100.0;
constexpr double MAX_THRUST = 15.0;

constexpr const char* IN_TIMELINE = "in";
constexpr const char* OUT_TIMELINE = "predict";
constexpr const char* ERROR_VAR = "error";
constexpr const char* SPEED_VAR = "speed";
constexpr const char* THRUST_VAR = "thrust";

struct target {
private:
  std::mt19937& gen;
  std::uniform_real_distribution<double> d;
  std::size_t period;
  std::size_t count;

public:
  double height;

  target(std::mt19937& gen, double margin, std::size_t period,
         const gdyn::problem::rocket::parameters& params)
      : gen(gen),
        d(margin, params.ceiling_height - margin),
        period(period),
        count(1),
        height(0) {
    ++(*this);
  }

  target& operator++() {
    if (--count == 0) {
      height = d(gen);
      count = period;
    }
    return *this;
  }
};

double clamp01(double x) { return std::clamp(x, 0.0, 1.0); }

double normalize(double x, double min_v, double max_v) {
  if (max_v <= min_v) return 0.0;
  return clamp01((x - min_v) / (max_v - min_v));
}

// We trained thrust normalized in [0, 1], so we denormalize to [0, 15].
double denormalize_thrust(double t01) { return clamp01(t01) * MAX_THRUST; }

void write_scalar(cxsom::data::Center& center, const cxsom::symbol::Instance& who,
                  double value) {
  center[who]->set([&](cxsom::data::Availability& status, std::size_t&,
                        cxsom::data::Base& d) {
    static_cast<cxsom::data::Scalar&>(d).value = value;
    status = cxsom::data::Availability::Ready;
  });
}

bool try_read_scalar(cxsom::data::Center& center, const cxsom::symbol::Instance& who,
                     double& out) {
  auto inst = center[who];
  cxsom::data::Availability synced = cxsom::data::Availability::Busy;

  inst->sync([&](cxsom::data::Availability s) { synced = s; });
  if (synced != cxsom::data::Availability::Ready) return false;

  inst->get([&](cxsom::data::Availability status, std::size_t,
                const cxsom::data::Base& d) {
    if (status == cxsom::data::Availability::Ready)
      out = static_cast<const cxsom::data::Scalar&>(d).value;
  });
  return true;
}

} // namespace

int main(int argc, char* argv[]) {
  std::string root_dir = "root-dir";
  if (argc >= 2) root_dir = argv[1];

  std::random_device rd;
  std::mt19937 gen(rd());

  double dt = types::dt * DT_FACTOR;
  std::size_t target_period = static_cast<std::size_t>(TARGET_PERIOD / dt);

  auto params = make_params();
  auto tgt = target(gen, MARGIN, target_period, params);
  auto rocket = types::base_continuous_system(params);
  auto relative_rocket =
      types::continuous_system(rocket, [&tgt]() { return tgt.height; });
  auto exposed_rocket = types::exposed_system(relative_rocket);

  cxsom::data::Center center{root_dir};
  center.check_all();

  // Ensure variables are discovered in center cache.
  (void)center.history_length({IN_TIMELINE, ERROR_VAR});
  (void)center.history_length({IN_TIMELINE, SPEED_VAR});
  (void)center.history_length({OUT_TIMELINE, THRUST_VAR});

  std::size_t step = 0;

  auto cxsom_controller =
      [&](const types::exposed_system::observation_type& obs)
      -> types::continuous_system::command_type {
    const double e01 = normalize(obs.error, MIN_ERROR, MAX_ERROR);
    const double s01 = normalize(obs.speed, MIN_SPEED, MAX_SPEED);

    write_scalar(center, {IN_TIMELINE, ERROR_VAR, static_cast<unsigned int>(step)}, e01);
    write_scalar(center, {IN_TIMELINE, SPEED_VAR, static_cast<unsigned int>(step)}, s01);

    double predicted_t01 = 0.0;
    bool got_prediction = false;

    for (unsigned int attempt = 0; attempt < 2000; ++attempt) {
      got_prediction = try_read_scalar(
          center, {OUT_TIMELINE, THRUST_VAR, static_cast<unsigned int>(step)},
          predicted_t01);
      if (got_prediction) break;
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    ++step;

    const double thrust = got_prediction ? denormalize_thrust(predicted_t01) : 0.0;
    return {.value = thrust, .duration = dt};
  };

  std::ofstream datafile{"predictions/rocket-orbit-inline.dat"};
  if (!datafile) {
    std::cerr << "Cannot open predictions/rocket-orbit-inline.dat" << std::endl;
    return 1;
  }

  double t = 0;
  std::size_t orbit_size = static_cast<std::size_t>(EPISODE_DURATION / dt);
  for (auto [observation, action, report] :
       gdyn::views::controller(exposed_rocket, cxsom_controller) |
           gdyn::views::orbit(exposed_rocket) | std::views::take(orbit_size)) {
    ++tgt;
    if (action) {
      datafile << t << ' ' << observation.error << ' ' << observation.speed << ' '
               << action->value << '\n';
      t += dt;
    }
  }

  std::cout << "File predictions/rocket-orbit-inline.dat generated." << std::endl;
  return 0;
}
