#include <iostream>
#include <fstream>
#include <random>
#include <algorithm>
#include <cstddef>
#include <chrono>
#include <string>
#include <thread>
#include <ranges>
#include <optional>

#include <asio.hpp>

#include <cxsom/cxsomData.hpp>
#include <cxsom/cxsomSymbols.hpp>
#include <cxsom/cxsomVariable.hpp>

#include <gdyn.hpp>
#include <rllib2.hpp>

#include "controller-config.hpp"
#include "predictions/discrete-rocket-problem.hpp"
#include "predictions/my_rocket_config.hpp"

struct target {
private:
  std::mt19937& gen;
  std::uniform_real_distribution<double> d;
  std::size_t period;
  std::size_t count;

public:
  double height;

  target(std::mt19937& gen, double margin, std::size_t period, const gdyn::problem::rocket::parameters& params)
    : gen(gen),
      d(margin, params.ceiling_height - margin),
      period(period),
      count(1) {++(*this);}

  target& operator++() {
    if(--count == 0) {
      height = d(gen);
      count = period;
    }
    return *this;
  }
};

double clamp01(double x) {
  return std::clamp(x, 0.0, 1.0);
}

double normalize(double x, double min_v, double max_v) {
  if(max_v <= min_v) return 0.0;
  return clamp01((x - min_v) / (max_v - min_v));
}

double denormalize_thrust(double t01) {
  return clamp01(t01) * controller_config::max_thrust;
}

bool ping_processor(const std::string& hostname, const std::string& port) {
  try {
    asio::ip::tcp::iostream socket;
    socket.exceptions(std::ios::failbit | std::ios::badbit | std::ios::eofbit);
    socket.connect(hostname, port);
    socket << "ping\n" << std::flush;
    std::string line;
    std::getline(socket, line, '\n');
    return line == "ok";
  }
  catch(...) {
    return false;
  }
}

class Controller {
private:
  cxsom::data::File& e_file;
  cxsom::data::File& s_file;
  cxsom::data::ref e_data;
  cxsom::data::ref s_data;
  std::size_t& step;
  const std::string& root_dir;
  const std::string& hostname;
  const std::string& port;
  double dt;
  mutable bool prediction_failed = false;

  static std::optional<double> try_read_scalar_at(const std::string& root_dir, std::size_t at) {
    cxsom::symbol::Variable thrust{controller_config::predict_timeline, controller_config::thrust_var};
    cxsom::data::File file{root_dir, thrust};
    file.realize(nullptr, std::nullopt, std::nullopt, false);
    if(!file) return std::nullopt;

    auto data = cxsom::data::make(file.get_type());
    auto status = file.read(at, data);
    if(status != cxsom::data::FileAvailability::Ready) return std::nullopt;

    return static_cast<cxsom::data::Scalar&>(*data).value;
  }

public:
  Controller(cxsom::data::File& e_file,
             cxsom::data::File& s_file,
             cxsom::data::ref e_data,
             cxsom::data::ref s_data,
             std::size_t& step,
             const std::string& root_dir,
             const std::string& hostname,
             const std::string& port,
             double dt)
    : e_file(e_file),
      s_file(s_file),
      e_data(e_data),
      s_data(s_data),
      step(step),
      root_dir(root_dir),
      hostname(hostname),
      port(port),
      dt(dt) {}

  types::continuous_system::command_type operator()(const types::exposed_system::observation_type& obs) const {
    if(prediction_failed) return {.value = 0.0, .duration = dt};

    types::S discrete_state{obs};
    auto phase = static_cast<types::S::base_type>(discrete_state);

    *e_data = normalize(phase.error, types::min_errors, types::max_errors);
    *s_data = normalize(phase.speed, types::min_speeds, types::max_speeds);

    e_file.write(step, e_data);
    s_file.write(step, s_data);
    (void)ping_processor(hostname, port);

    std::optional<double> thrust01;
    for(int k = 0; k <= controller_config::prediction_wait_ms; ++k) {
      thrust01 = try_read_scalar_at(root_dir, step);
      if(thrust01) break;
      if((k % 5) == 0) (void)ping_processor(hostname, port);
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    if(!thrust01) {
      prediction_failed = true;
      std::cerr << "Prediction timeout at step " << step << std::endl;
      return {.value = 0.0, .duration = dt};
    }

    ++step;

    return {.value = thrust01 ? denormalize_thrust(*thrust01) : 0.0,
            .duration = dt};
  }

  bool failed() const {
    return prediction_failed;
  }
};

int main(int argc, char* argv[]) {
  if(argc < 4) {
    std::cout << "Usage: " << argv[0] << " <root-dir> <hostname> <port>\n";
    std::cout << "Example: " << argv[0] << " root-dir localhost 10000\n";
    return 0;
  }

  std::string root_dir = argv[1];
  std::string hostname = argv[2];
  std::string port     = argv[3];

  cxsom::symbol::Variable error_var {controller_config::in_timeline, controller_config::error_var};
  cxsom::symbol::Variable speed_var {controller_config::in_timeline, controller_config::speed_var};
  cxsom::symbol::Variable thrust_var{controller_config::predict_timeline, controller_config::thrust_var};

  cxsom::data::File e_file{root_dir, error_var};
  cxsom::data::File s_file{root_dir, speed_var};
  cxsom::data::File t_file{root_dir, thrust_var};

  e_file.realize(nullptr, std::nullopt, std::nullopt, false);
  s_file.realize(nullptr, std::nullopt, std::nullopt, false);
  t_file.realize(nullptr, std::nullopt, std::nullopt, false);

  if(!e_file || !s_file || !t_file) {
    std::cerr << "CXSOM file open error." << std::endl;
    return 1;
  }

  auto e_data = cxsom::data::make(e_file.get_type());
  auto s_data = cxsom::data::make(s_file.get_type());

  auto error_step = e_file.get_next_time();
  auto speed_step = s_file.get_next_time();
  if(error_step != speed_step) {
    std::cerr << "CXSOM timeline mismatch: in/error next time is "
              << error_step
              << " while in/speed next time is "
              << speed_step
              << std::endl;
    return 1;
  }

  std::size_t step = error_step;

  std::random_device rd;
  std::mt19937 gen(rd());

  double dt = types::dt * controller_config::dt_factor;
  std::size_t target_period = std::size_t(controller_config::target_period / dt);

  auto params = make_params();
  auto tgt = target(gen, controller_config::margin, target_period, params);
  auto rocket = types::base_continuous_system(params);
  auto relative_rocket = types::continuous_system(rocket, [&tgt](){return tgt.height;});
  auto exposed_rocket = types::exposed_system(relative_rocket);

  Controller controller {e_file, s_file, e_data, s_data, step, root_dir, hostname, port, dt};

  {
    std::ofstream clean_file{"predictions/rocket-orbit-cxsom.dat"};
    std::ofstream debug_file{"predictions/rocket-debug-cxsom.dat"};
    if(!clean_file || !debug_file) {
      std::cerr << "Cannot open output files in predictions/" << std::endl;
      return 1;
    }

    debug_file << "# t height target error speed thrust\n";

    double t = 0.0;
    std::size_t orbit_size = std::size_t(controller_config::episode_duration / dt);

    for(auto [observation, action, report]
        : gdyn::views::controller(exposed_rocket, controller)
        | gdyn::views::orbit(exposed_rocket)
        | std::views::take(orbit_size)) {
      (void)report;

      const double rocket_height = tgt.height + observation.error;
      clean_file << t << ' ' << rocket_height << ' ' << tgt.height << '\n';

      if(action) {
        debug_file << t << ' '
                   << rocket_height << ' '
                   << tgt.height << ' '
                   << observation.error << ' '
                   << observation.speed << ' '
                   << action->value << '\n';
      }

      if(controller.failed()) break;

      ++tgt;
      if(action) t += dt;
    }

    std::cout << "File predictions/rocket-orbit-cxsom.dat generated." << std::endl;
    std::cout << "File predictions/rocket-debug-cxsom.dat generated." << std::endl;
  }

  {
    std::ofstream plotfile{"predictions/rocket-orbit-cxsom.plot"};
    plotfile << "plot 'predictions/rocket-orbit-cxsom.dat' using 1:2 with lines lc rgb \"black\" title \"rocket height\", \\\n"
             << "     'predictions/rocket-orbit-cxsom.dat' using 1:3 with lines lc rgb \"red\" title \"target height\"\n";
    std::cout << "Run : gnuplot -p predictions/rocket-orbit-cxsom.plot" << std::endl;

    std::ofstream plotdbg{"predictions/rocket-debug-cxsom.plot"};
    plotdbg << "set multiplot layout 2,1 title 'CXSOM controller debug'\n"
            << "plot 'predictions/rocket-debug-cxsom.dat' using 1:4 with lines title 'error', \\\n"
            << "     '' using 1:5 with lines title 'speed'\n"
            << "plot 'predictions/rocket-debug-cxsom.dat' using 1:6 with lines title 'thrust'\n"
            << "unset multiplot\n";
    std::cout << "Run : gnuplot -p predictions/rocket-debug-cxsom.plot" << std::endl;
  }

  return 0;
}
