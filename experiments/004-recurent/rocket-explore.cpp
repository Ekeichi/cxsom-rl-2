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

#include "data/discrete-rocket-problem.hpp"
#include "data/my_rocket_config.hpp"

// Multi-episode exploration version of rocket.cpp.
// Each episode starts from a random (error, speed) state drawn uniformly
// from the full discretised state space, ensuring dense coverage of all
// (error, speed, thrust) transitions for recsom training.

#define DT_FACTOR      .1
#define NB_EPISODES    200
#define EPISODE_STEPS  500   // steps per episode (~50 s at dt=0.1)
#define MARGIN         100   // metres excluded near floor/ceiling for targets

int main(int argc, char *argv[]) {

  const char *controller_file = "data/rocket-discrete-controller.dat";

  // -------------------------------------------------------------------------
  // Load C* : the optimal thrust table from the learned controller.
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
  // RNG setup.
  // -------------------------------------------------------------------------
  std::random_device rd;
  std::mt19937 gen(rd());

  // Uniform distributions over the full error/speed ranges.
  std::uniform_real_distribution<double> d_error(types::min_errors, types::max_errors);
  std::uniform_real_distribution<double> d_speed(types::min_speeds, types::max_speeds);

  auto params = make_params();
  std::uniform_real_distribution<double> d_target(MARGIN, params.ceiling_height - MARGIN);

  double dt = types::dt * DT_FACTOR;

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
  // Determine starting timestep from existing data.
  // -------------------------------------------------------------------------
  std::size_t t = 1;
  {
    auto [first, last] = error_file.get_time_range();
    if (last != cxsom::data::File::no_time())
      t = last + 1;
  }

  std::size_t total_steps = 0;

  // -------------------------------------------------------------------------
  // Multi-episode loop.
  // -------------------------------------------------------------------------
  for (std::size_t ep = 0; ep < NB_EPISODES; ++ep) {

    // Draw a random initial (error, speed) from the full state space.
    double init_error = d_error(gen);
    double init_speed = d_speed(gen);
    double tgt_height = d_target(gen);

    auto rocket          = types::base_continuous_system(params);
    auto relative_rocket = types::continuous_system(rocket, [&tgt_height]() { return tgt_height; });

    // Place the rocket at the sampled (error, speed).
    relative_rocket = gdyn::problem::rocket::relative::phase{.error = init_error,
                                                             .speed = init_speed};

    auto controller =
      [&optimal_thrusts, &relative_rocket, dt]
      (const types::base_continuous_system::observation_type &obs)
      -> types::base_continuous_system::command_type {
        types::S current{relative_rocket.convert(obs)};
        return {.value = optimal_thrusts[static_cast<std::size_t>(current)],
                .duration = dt};
      };

    std::size_t step = 0;
    for (auto [observation, action, report] :
         gdyn::views::controller(rocket, controller) |
         gdyn::views::orbit(rocket) |
         std::views::take(EPISODE_STEPS)) {

      static_cast<cxsom::data::Scalar &>(*r_error ).value = relative_rocket.convert(observation).error;
      static_cast<cxsom::data::Scalar &>(*r_speed ).value = observation.speed;
      static_cast<cxsom::data::Scalar &>(*r_thrust).value = action ? action->value : 0.0;

      error_file .write(t, r_error);
      speed_file .write(t, r_speed);
      thrust_file.write(t, r_thrust);

      ++t;
      ++step;
    }

    total_steps += step;
  }

  // -------------------------------------------------------------------------
  // Ping cxsom server.
  // -------------------------------------------------------------------------
  std::string p_hostname;
  unsigned int p_port = 0;

  p_hostname = argv[1];
  p_port     = std::stoul(argv[2]);

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

  std::cout << "Done. " << NB_EPISODES << " episodes, "
            << total_steps << " total steps written." << std::endl;

  return 0;
}
// clang-format on
