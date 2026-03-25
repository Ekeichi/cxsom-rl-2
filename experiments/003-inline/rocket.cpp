// clang-format off
#include <algorithm>
#include <array>
#include <cstddef>
#include <cxsom/cxsomSymbols.hpp>
#include <cxsom/cxsomVariable.hpp>
#include <cxsom/cxsomPing.hpp>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>

#include <gdyn.hpp>
#include <rllib2.hpp>

#include "discrete-rocket-problem.hpp"
#include "my_rocket_config.hpp"

// Like example 4-2, but writes observations into cxsom timelines.
// Init: read last (error, speed) from root-dir/in/error and root-dir/in/speed
// (i.e. e_{t-1}, s_{t-1}), then continue the simulation from there.

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
#define EPISODE_DURATION 20 * TARGET_PERIOD

int main(int argc, char *argv[]) {

  const char *controller_file = "predictions/rocket-discrete-controller.dat";

  // -------------------------------------------------------------------------
  // Load C* : the optimal thrust table from the learned controller (ex. 4-1).
  // -------------------------------------------------------------------------
  std::array<double, types::S::size()> optimal_thrusts;

  {
    std::ifstream data{controller_file};
    if (!data) {
      std::cout << "Cannot open " << controller_file << ". Aborting." << std::endl;
      return 1;
    }
    double ignore;
    auto out = optimal_thrusts.begin();
    for (auto s_it = types::S::begin(); s_it != types::S::end(); ++s_it)
      data >> ignore >> ignore >> *(out++) >> ignore >> ignore;
  }

  // -------------------------------------------------------------------------
  // Build the rocket system (like example 4-2).
  // -------------------------------------------------------------------------
  std::random_device rd;
  std::mt19937 gen(rd());

  double dt = types::dt * DT_FACTOR;
  std::size_t target_period = std::size_t(TARGET_PERIOD / dt);

  auto params         = make_params();
  auto tgt            = target(gen, MARGIN, target_period, params);
  auto rocket         = types::base_continuous_system(params);
  auto relative_rocket = types::continuous_system(rocket, [&tgt]() { return tgt.height; });

  // Controller: converts (height, speed) → (error, speed) via relative_rocket,
  // discretises into state index, then looks up optimal thrust.
  auto controller =
    [&optimal_thrusts, &relative_rocket, dt]
    (const types::base_continuous_system::observation_type &obs)
    -> types::base_continuous_system::command_type {
      types::S current{relative_rocket.convert(obs)};
      return {.value = optimal_thrusts[static_cast<std::size_t>(current)],
              .duration = dt};
    };

  // -------------------------------------------------------------------------
  // Open cxsom timelines.
  // -------------------------------------------------------------------------
  cxsom::symbol::Variable error_var {"in", "error"};
  cxsom::symbol::Variable speed_var {"in", "speed"};
  cxsom::symbol::Variable thrust_var{"in", "thrust"};

  cxsom::data::File error_file (std::filesystem::path("root-dir"), error_var);
  error_file.realize (cxsom::type::make("Scalar"), 2, 100, true);
  cxsom::data::File speed_file (std::filesystem::path("root-dir"), speed_var);
  speed_file.realize (cxsom::type::make("Scalar"), 2, 100, true);
  cxsom::data::File thrust_file(std::filesystem::path("root-dir"), thrust_var);
  thrust_file.realize(cxsom::type::make("Scalar"), 2, 100, true);

  auto r_error  = cxsom::data::make(cxsom::type::make("Scalar"));
  auto r_speed  = cxsom::data::make(cxsom::type::make("Scalar"));
  auto r_thrust = cxsom::data::make(cxsom::type::make("Scalar"));

  // -------------------------------------------------------------------------
  // Init: read e_{t-1}, s_{t-1} from the LAST stored timestep.
  // -------------------------------------------------------------------------
  double init_error = 0.;
  double init_speed = 0.;
  std::size_t t_start = 0;

  {
    auto [error_first, error_last] = error_file.get_time_range();
    auto [speed_first, speed_last] = speed_file.get_time_range();

    if (error_last != cxsom::data::File::no_time()) {
      error_file.read(error_last, r_error);
      init_error = static_cast<cxsom::data::Scalar &>(*r_error).value;
      t_start = error_last;
    }
    if (speed_last != cxsom::data::File::no_time()) {
      speed_file.read(speed_last, r_speed);
      init_speed = static_cast<cxsom::data::Scalar &>(*r_speed).value;
    }
  }

  // Initialise the rocket at (e_{t-1}, s_{t-1}).
  relative_rocket = gdyn::problem::rocket::relative::phase{.error = init_error,
                                                           .speed = init_speed};

  // -------------------------------------------------------------------------
  // Simulation loop: continue timelines from t_start + 1.
  // -------------------------------------------------------------------------
  std::size_t orbit_size = std::size_t(EPISODE_DURATION / dt);
  std::size_t t = t_start + 1;

  for (auto [observation, action, report] :
       gdyn::views::controller(rocket, controller) |
       gdyn::views::orbit(rocket) |
       std::views::take(orbit_size)) {

    static_cast<cxsom::data::Scalar &>(*r_error ).value = relative_rocket.convert(observation).error;
    static_cast<cxsom::data::Scalar &>(*r_speed ).value = observation.speed;
    static_cast<cxsom::data::Scalar &>(*r_thrust).value = action ? action->value : 0.0;

    error_file .write(t, r_error);
    speed_file .write(t, r_speed);
    thrust_file.write(t, r_thrust);

    ++tgt;
    ++t;
  }

  std::string p_hostname;
  unsigned int p_port = 0;

  p_hostname = argv[1];
  p_port = std::stoul(argv[2]);
  
  try {
    cxsom::protocol::ping(p_hostname, p_port);
    std::cout << "Ping to " << p_hostname << ":" << p_port << " successful." << std::endl;
  }
  catch(cxsom::protocol::remote_error& e) {
    std::cout << "Server is not ok with the ping: " << e.what() << std::endl;
  }
  catch(std::exception& e) {
    std::cout << "Something went wrong: " << e.what() << std::endl;
  }

  std::cout << "Done. Wrote " << orbit_size << " steps starting at t=" << t_start + 1 << "." << std::endl;

  return 0;
}
// clang-format on
