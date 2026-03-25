#include <iostream>
#include <fstream>
#include <random>
#include <algorithm>
#include <cstddef>
#include <chrono>
#include <string>
#include <thread>
#include <ranges>

#include <asio.hpp>

#include <cxsom/cxsomData.hpp>
#include <cxsom/cxsomSymbols.hpp>
#include <cxsom/cxsomVariable.hpp>

#include <gdyn.hpp>
#include <rllib2.hpp>

#include "predictions/discrete-rocket-problem.hpp"
#include "predictions/my_rocket_config.hpp"

#define DT_FACTOR .1
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

// HFB All this stuf should be define in a single header, included by each of your experiments.

#define TARGET_PERIOD 20 // seconds
#define MARGIN 200
#define EPISODE_DURATION 20*TARGET_PERIOD
#define MAX_THRUST 15
#define PREDICTION_WAIT_MS 1000

constexpr const char* IN_TIMELINE = "in";
constexpr const char* PRED_TIMELINE = "predict";
constexpr const char* ERROR_VAR = "error";
constexpr const char* SPEED_VAR = "speed";
constexpr const char* THRUST_VAR = "thrust";

struct MinMax {
  double min_v = 0.0;
  double max_v = 1.0;
};

double clamp01(double x) {
  return std::clamp(x, 0.0, 1.0);
}

double normalize(double x, const MinMax& mm) {
  if(mm.max_v <= mm.min_v) return 0.0;
  return clamp01((x - mm.min_v) / (mm.max_v - mm.min_v));
}

double denormalize_thrust(double t01) {
  return clamp01(t01) * MAX_THRUST;
}

// HFB ????
bool read_minmax_from_dataset(const std::string& path, MinMax& err, MinMax& spd, MinMax& thr) {
  std::ifstream in(path);
  if (!in) return false;

  bool first = true;
  double e, s, t, q0, q1;
  while (in >> e >> s >> t >> q0 >> q1) {
    if (first) {
      err = {e, e};
      spd = {s, s};
      thr = {t, t};
      first = false;
    } else {
      err.min_v = std::min(err.min_v, e); err.max_v = std::max(err.max_v, e);
      spd.min_v = std::min(spd.min_v, s); spd.max_v = std::max(spd.max_v, s);
      thr.min_v = std::min(thr.min_v, t); thr.max_v = std::max(thr.max_v, t);
    }
  }
  return !first;
}

// HFB: This function exists now in cxsom, try cxsom::protocol::ping
bool ping_processor(const std::string& hostname, const std::string& port) {
  try {
    asio::ip::tcp::iostream socket;
    socket.exceptions(std::ios::failbit | std::ios::badbit | std::ios::eofbit);
    socket.connect(hostname, port);
    socket << "ping\n" << std::flush;
    std::string line;
    std::getline(socket, line, '\n');
    return line == "ok";
  } catch (...) {
    return false;
  }
}

// HFB all these functions should be defined as private static (or not...) methods of a "Controller" class.

bool write_scalar_with_retry(cxsom::data::File& file,
                             std::size_t at,
                             cxsom::data::ref data,
                             const std::string& hostname,
                             const std::string& port,
                             unsigned int max_tries = 2000) {
  for(unsigned int k = 0; k < max_tries; ++k) {
    // HFB I think you do not need to check ws... it should always be Busy (the status before writing), since you write in a free slot.
    auto ws = file.write(at, data);
    if(ws == cxsom::data::FileAvailability::Ready) return true;
    if((k % 50) == 0) (void)ping_processor(hostname, port);
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  return false;
}

// HFB : you pass out by reference to set it in case of success... this has a C flavor.
// return a std::optional<cxsom::data::Scalar> instead, it will give something that may be a value... or not.
bool try_read_scalar_at(cxsom::data::File& file,
                        std::size_t at,
                        cxsom::data::ref data,
                        double& out) {
  auto rs = file.read(at, data);
  if(rs != cxsom::data::FileAvailability::Ready) return false;
  out = static_cast<cxsom::data::Scalar&>(*data).value;
  return true;
}

// HFB use a std::optional as well
bool try_read_latest_scalar(cxsom::data::File& file,
                            cxsom::data::ref data,
                            double& out) {
  auto n = file.get_next_time();
  if(n == 0) return false;
  return try_read_scalar_at(file, n - 1, data, out);
}

// HFB The Controller class implements operator(), so it can be used as a controller

class Controller {
  // The private tool functions defined above

public:
  types::continuous_system::command_type operator() (const types::exposed_system::observation_type& obs) {
    // The code of the lambda
  }
};

int main(int argc, char* argv[]) {

  if(argc < 5) {
    std::cout << "Usage: " << argv[0] << " <root-dir> <hostname> <port> <dataset>\n";
    std::cout << "Example: " << argv[0] << " root-dir localhost 10000 data/rocket-discrete-controller.dat\n";
    return 0;
  }

  std::string root_dir = argv[1];
  std::string hostname = argv[2];

  // unsigned int port = std::stoul(argv[3]); // HFB compatible with cxsom::protocol::ping
  std::string port = argv[3];
  std::string dataset = argv[4];

  
  // HFB why ? The link to a dataset is irrelevant for this controller.
  MinMax err_mm, spd_mm, thr_mm;
  if (!read_minmax_from_dataset(dataset, err_mm, spd_mm, thr_mm)) {
    std::cerr << "Cannot read dataset: " << dataset << std::endl;
    return 1;
  }

  cxsom::symbol::Variable E{IN_TIMELINE, ERROR_VAR};
  cxsom::symbol::Variable S{IN_TIMELINE, SPEED_VAR};
  cxsom::symbol::Variable T{PRED_TIMELINE, THRUST_VAR};

  cxsom::data::File e_file{root_dir, E};
  cxsom::data::File s_file{root_dir, S};
  cxsom::data::File t_file{root_dir, T};

  // HFB: In case of error, I don't think I throw any exception.
  // You can check the files afterwards: if(!e_file){...}
  try {
    e_file.realize(nullptr, std::nullopt, std::nullopt, false);
    s_file.realize(nullptr, std::nullopt, std::nullopt, false);
    t_file.realize(nullptr, std::nullopt, std::nullopt, false);
  } catch (std::exception& ex) {
    std::cerr << "CXSOM file open error: " << ex.what() << std::endl;
    return 1;
  }

  auto e_data = cxsom::data::make(e_file.get_type());
  auto s_data = cxsom::data::make(s_file.get_type());
  auto t_data = cxsom::data::make(t_file.get_type());

  // HFB why ? They have to be the same.
  std::size_t step = std::max(e_file.get_next_time(), s_file.get_next_time());
  
  std::random_device rd;
  std::mt19937 gen(rd());

  double dt = types::dt * DT_FACTOR;
  std::size_t target_period = std::size_t(TARGET_PERIOD / dt);
  
  auto params = make_params();
  auto tgt = target(gen, MARGIN, target_period, params);
  auto rocket = types::base_continuous_system(params);

  auto relative_rocket = types::continuous_system(rocket, [&tgt](){return tgt.height;});
  auto exposed_rocket = types::exposed_system(relative_rocket);

  // HFB such a lambda is a bit messy. See previous remarks
  // Controller controller {....};
  auto controller =
    [&e_file, &s_file, &t_file,
     &e_data, &s_data, &t_data,
     &step, &err_mm, &spd_mm,
     &hostname, &port, dt]
    (const types::exposed_system::observation_type& obs) -> types::continuous_system::command_type {
      types::S discrete_state{obs};
      auto phase = static_cast<types::S::base_type>(discrete_state);
      const double e01 = normalize(phase.error, err_mm);
      const double s01 = normalize(phase.speed, spd_mm);

      *e_data = e01;
      *s_data = s01;

      bool wrote_e = write_scalar_with_retry(e_file, step, e_data, hostname, port);
      bool wrote_s = write_scalar_with_retry(s_file, step, s_data, hostname, port);
      if(!wrote_e || !wrote_s) {
        std::cerr << "Warning: input write timeout at step " << step << std::endl;
      }

      (void)ping_processor(hostname, port);

      bool got = false;
      double thrust01 = 0.0;
      for(int k = 0; k <= PREDICTION_WAIT_MS; ++k) {
        got = try_read_scalar_at(t_file, step, t_data, thrust01);
        if(got) break;
        if((k % 5) == 0) (void)ping_processor(hostname, port);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
      }
      if(!got) {
        got = try_read_latest_scalar(t_file, t_data, thrust01);
      }

      ++step;

      const double thrust = got ? denormalize_thrust(thrust01) : 0.0;
      return {.value = thrust, .duration = dt};
    };

  {
    std::ofstream clean_file{"predictions/rocket-orbit-cxsom.dat"};
    std::ofstream debug_file{"predictions/rocket-debug-cxsom.dat"};
    if(!clean_file || !debug_file) {
      std::cerr << "Cannot open output files in predictions/" << std::endl;
      return 1;
    }

    debug_file << "# t height target error speed thrust\n";


    double t = 0.0;
    std::size_t orbit_size = std::size_t(EPISODE_DURATION / dt);

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
