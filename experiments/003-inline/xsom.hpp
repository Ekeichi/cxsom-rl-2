#include <cxsom-builder.hpp>
#include <fstream>
#include <iterator>
#include <sstream>
#include <tuple>

#define CACHE 2

#define TRACE 100000
#define SAVE_TRACE 10000
#define SAVE_PERIOD 500
#define OPENED true
#define OPEN_AS_NEEDED false
#define FORGET 0
#define DEADLINE 100
#define WALLTIME -1 // infinite

#define MAP_SIZE 2000

// cxsom declarations
using namespace cxsom::rules;
context *cxsom::rules::ctx = nullptr;

struct Params {
  kwd::parameters p_main, p_match, p_learn, p_learn_e, p_learn_c, p_external,
      p_contextual, p_global;
  Params() {
    p_main | kwd::use("walltime", WALLTIME), kwd::use("epsilon", 0);
    p_match | p_main, kwd::use("sigma", .2);
    p_learn | p_main, kwd::use("alpha", .1);
    p_learn_e | p_learn, kwd::use("r", .25); // 0.25
    p_learn_c | p_learn, kwd::use("r", .075);
    p_external | p_main;
    p_contextual | p_main;
    p_global | p_main, kwd::use("random-bmu", 1), kwd::use("sigma", .01),
        kwd::use("beta", .5), kwd::use("delta", .01), kwd::use("deadline", 100);
  }
};

auto build_map_settings(const Params &params) {
  auto map_settings = cxsom::builder::map::make_settings();
  map_settings.map_size = MAP_SIZE;
  map_settings.cache_size = CACHE;
  map_settings.weights_file_size = TRACE;
  map_settings.bmu_file_size = TRACE;
  map_settings.kept_opened = OPENED;
  map_settings = {params.p_external, params.p_contextual, params.p_global};
  return map_settings;
}
