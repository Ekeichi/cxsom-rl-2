#include <cxsom-builder.hpp>
#include <fstream>
#include <sstream>
#include <tuple>
#include <iterator>

#define CACHE              2

#define TRACE          10000
#define OPENED          true
#define OPEN_AS_NEEDED false
#define FORGET             0
#define DEADLINE         100
#define WALLTIME          -1  // infinite

#define MAP_SIZE        1500

using namespace cxsom::rules;
context* cxsom::rules::ctx = nullptr;

// ####################
// #                  #
// #  Building arch   #
// #                  #
// ####################

int main(int argc, char* argv[]) {
  context c(argc, argv);

  auto archi = cxsom::builder::architecture();
  
  kwd::parameters p_main, p_match, p_learn, p_learn_e, p_learn_c, p_external, p_contextual, p_global;
  p_main       | kwd::use("walltime", WALLTIME), kwd::use("epsilon", 0);
  p_match      | p_main, kwd::use("sigma", .2);
  p_learn      | p_main, kwd::use("alpha", .05);
  p_learn_e    | p_learn, kwd::use("r", .25 );
  p_learn_c    | p_learn, kwd::use("r", .075);
  p_external   | p_main;
  p_contextual | p_main;
  p_global     | p_main, kwd::use("random-bmu", 1), kwd::use("sigma", .01), kwd::use("beta", .5), kwd::use("delta", .01), kwd::use("deadline", 100);
  
  auto map_settings = cxsom::builder::map::make_settings();
  map_settings.map_size          = MAP_SIZE;
  map_settings.cache_size        = CACHE;
  map_settings.weights_file_size = TRACE;
  map_settings.kept_opened       = OPENED;
  map_settings                   = {p_external, p_contextual, p_global};

  auto error  = cxsom::builder::variable("in", cxsom::builder::name("error"),  "Scalar", CACHE, TRACE, OPENED);
  auto speed  = cxsom::builder::variable("in", cxsom::builder::name("speed"),  "Scalar", CACHE, TRACE, OPENED);
  auto thrust = cxsom::builder::variable("in", cxsom::builder::name("thrust"), "Scalar", CACHE, TRACE, OPENED);

  error->definition();
  speed->definition();
  thrust->definition();

  auto errorMap  = cxsom::builder::map::make_1D("error");
  auto speedMap  = cxsom::builder::map::make_1D("speed");
  auto thrustMap = cxsom::builder::map::make_1D("thrust");
  
  // Liens entre les inputs et les cartes
  errorMap->external   (error,    fx::match_gaussian, p_match, fx::learn_triangle, p_learn_e);
  speedMap->external   (speed,    fx::match_gaussian, p_match, fx::learn_triangle, p_learn_e);
  thrustMap->external  (thrust,   fx::match_gaussian, p_match, fx::learn_triangle, p_learn_e);

  // Liens entre les cartes
  errorMap->contextual (speedMap, fx::match_gaussian, p_match, fx::learn_triangle, p_learn_c);
  speedMap->contextual (thrustMap, fx::match_gaussian, p_match, fx::learn_triangle, p_learn_c);
  thrustMap->contextual(errorMap, fx::match_gaussian, p_match, fx::learn_triangle, p_learn_c);
  errorMap->contextual (thrustMap, fx::match_gaussian, p_match, fx::learn_triangle, p_learn_c);
  speedMap->contextual (errorMap, fx::match_gaussian, p_match, fx::learn_triangle, p_learn_c);
  thrustMap->contextual(speedMap, fx::match_gaussian, p_match, fx::learn_triangle, p_learn_c);

  archi << errorMap << speedMap << thrustMap;
  *archi = map_settings;
  archi->realize();
  errorMap->internals_random_at(0);
  speedMap->internals_random_at(0);
  thrustMap->internals_random_at(0);

  std::ofstream dot_file("3soms-archi.dot");
  dot_file << archi->write_dot;

  return 0;
}
