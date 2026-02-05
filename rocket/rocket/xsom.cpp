#include <cxsom-builder.hpp>
#include <fstream>
#include <sstream>
#include <tuple>
#include <iterator>

#define CACHE              2

#define TRACE          10000
#define SAVE_TRACE      1000
#define OPENED          true
#define OPEN_AS_NEEDED false
#define FORGET             0
#define DEADLINE         100
#define WALLTIME          -1  // infinite

#define MAP_SIZE        1500

// cxsom declarations
using namespace cxsom::rules;
context* cxsom::rules::ctx = nullptr;

// ####################
// #                  #
// #  Building archi  #
// #                  #
// ####################

int main(int argc, char* argv[]) {
  context c(argc, argv);

  kwd::parameters p_main, p_match, p_learn, p_learn_e, p_learn_c, p_external, p_contextual, p_global;
  p_main       | kwd::use("walltime", WALLTIME), kwd::use("epsilon", 0);
  p_match      | p_main, kwd::use("sigma", .2);
  p_learn      | p_main, kwd::use("alpha", .05);
  p_learn_e    | p_learn, kwd::use("r", .25 );
  p_learn_c    | p_learn, kwd::use("r", .075);
  p_external   | p_main;
  p_contextual | p_main;
  p_global     | p_main, kwd::use("random-bmu", 1), kwd::use("sigma", .01), kwd::use("beta", .5), kwd::use("delta", .01), kwd::use("deadline", 100);

  auto archi = cxsom::builder::architecture();

  auto error_input  = cxsom::builder::variable("in", cxsom::builder::name("xi_error"),  "Scalar", CACHE, TRACE, OPENED);
  auto speed_input  = cxsom::builder::variable("in", cxsom::builder::name("xi_speed"),  "Scalar", CACHE, TRACE, OPENED);
  auto thrust_input = cxsom::builder::variable("in", cxsom::builder::name("xi_thrust"), "Scalar", CACHE, TRACE, OPENED);

  error_input-> definition();
  speed_input-> definition();
  thrust_input->definition();

  auto errorMap  = cxsom::builder::map::make_1D("error");
  auto speedMap  = cxsom::builder::map::make_1D("speed");
  auto thrustMap = cxsom::builder::map::make_1D("thrust");

  auto map_settings = cxsom::builder::map::make_settings();
  map_settings.map_size          = MAP_SIZE;
  map_settings.cache_size        = CACHE;
  map_settings.weights_file_size = TRACE;
  map_settings.kept_opened       = OPENED;
  map_settings                   = {p_external, p_contextual, p_global};

  std::vector<cxsom::builder::Map::Layer*> layers;
  auto out_layer = std::back_inserter(layers); 

  // Liens entre les inputs et les cartes
  *(out_layer++) = errorMap->external    (error_input, fx::match_gaussian, p_match, fx::learn_triangle, p_learn_e);
  *(out_layer++) = speedMap->external    (speed_input, fx::match_gaussian, p_match, fx::learn_triangle, p_learn_e);
  *(out_layer++) = thrustMap->external   (thrust_input,fx::match_gaussian, p_match, fx::learn_triangle, p_learn_e);

  // Liens entre les cartes
  *(out_layer++) = errorMap ->contextual (speedMap, fx::match_gaussian, p_match, fx::learn_triangle, p_learn_c);
  *(out_layer++) = speedMap ->contextual (thrustMap,fx::match_gaussian, p_match, fx::learn_triangle, p_learn_c);
  *(out_layer++) = thrustMap->contextual (errorMap, fx::match_gaussian, p_match, fx::learn_triangle, p_learn_c);
  *(out_layer++) = errorMap ->contextual (thrustMap,fx::match_gaussian, p_match, fx::learn_triangle, p_learn_c);
  *(out_layer++) = speedMap ->contextual (errorMap, fx::match_gaussian, p_match, fx::learn_triangle, p_learn_c);
  *(out_layer++) = thrustMap->contextual (speedMap, fx::match_gaussian, p_match, fx::learn_triangle, p_learn_c);


  archi << errorMap << speedMap << thrustMap;
  *archi = map_settings;

  archi->realize();
  
  for(auto map : archi->maps) map->internals_random_at(0);

  int save_period = TRACE / SAVE_TRACE;

  // Saving weights
  for(auto layer_ptr : layers) {
    auto W = layer_ptr->_W();
    auto Wsaved = cxsom::builder::variable("save", W->varname, W->type, CACHE, SAVE_TRACE, OPEN_AS_NEEDED);
    Wsaved->definition();
    Wsaved->var() << fx::copy(kwd::times(W->var(), save_period)) | kwd::use("walltime", WALLTIME);
  }

  std::ofstream dot_file("architecture/3soms-archi.dot");
  dot_file << archi->write_dot;

  return 0;
}
