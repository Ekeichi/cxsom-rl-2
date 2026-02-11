#include "xsom.hpp"
#include <string>

// // ############################
// // #                          #
// // #  Building archi predict  #
// // #                          #
// // ############################

int main(int argc, char *argv[]) {
  context c(argc, argv);

  if (c.user_argv.size() != 1) {
    std::cout << "error, il faut passer un arg" << std::endl;
    c.notify_user_argv_error();
    return 0;
  }

  auto saved_weight_at = std::stoul(c.user_argv[0]);

  // à quoi ça sert ?
  std::string wtype_ext =
      std::string("Map1D<Scalar>=") + std::to_string(MAP_SIZE);
  std::string wtype_ctx =
      std::string("Map1D<Pos1D>=") + std::to_string(MAP_SIZE);
  Params params;

  auto archi = cxsom::builder::architecture();

  // definition des cartes
  auto errorMap = cxsom::builder::map::make_1D("error");
  auto speedMap = cxsom::builder::map::make_1D("speed");
  auto thrustMap = cxsom::builder::map::make_1D("thrust");

  auto map_settings = cxsom::builder::map::make_settings();
  map_settings.map_size = MAP_SIZE;
  map_settings.cache_size = CACHE;
  map_settings.weights_file_size = TRACE;
  map_settings.kept_opened = OPENED;
  map_settings = {params.p_external, params.p_contextual, params.p_global};

  auto errorWc0 = cxsom::builder::variable(
      "save", cxsom::builder::name("error") / cxsom::builder::name("Wc-0"),
      wtype_ctx, CACHE, TRACE, OPENED);
  auto errorWc1 = cxsom::builder::variable(
      "save", cxsom::builder::name("error") / cxsom::builder::name("Wc-1"),
      wtype_ctx, CACHE, TRACE, OPENED);
  auto speedWc0 = cxsom::builder::variable(
      "save", cxsom::builder::name("speed") / cxsom::builder::name("Wc-0"),
      wtype_ctx, CACHE, TRACE, OPENED);
  auto speedWc1 = cxsom::builder::variable(
      "save", cxsom::builder::name("speed") / cxsom::builder::name("Wc-1"),
      wtype_ctx, CACHE, TRACE, OPENED);
  auto thrustWc0 = cxsom::builder::variable(
      "save", cxsom::builder::name("thrust") / cxsom::builder::name("Wc-0"),
      wtype_ctx, CACHE, TRACE, OPENED);
  auto thrustWc1 = cxsom::builder::variable(
      "save", cxsom::builder::name("thrust") / cxsom::builder::name("Wc-1"),
      wtype_ctx, CACHE, TRACE, OPENED);
  errorMap->contextual(speedMap, fx::match_gaussian, params.p_match, errorWc0,
                       saved_weight_at);
  errorMap->contextual(thrustMap, fx::match_gaussian, params.p_match, errorWc1,
                       saved_weight_at);
  speedMap->contextual(errorMap, fx::match_gaussian, params.p_match, speedWc1,
                       saved_weight_at);
  speedMap->contextual(thrustMap, fx::match_gaussian, params.p_match, speedWc0,
                       saved_weight_at);
  thrustMap->contextual(errorMap, fx::match_gaussian, params.p_match, thrustWc0,
                        saved_weight_at);
  thrustMap->contextual(speedMap, fx::match_gaussian, params.p_match, thrustWc1,
                        saved_weight_at);

  // declaration des inputs et de la sortie
  auto error = cxsom::builder::variable("in", cxsom::builder::name("error"),
                                        "Scalar", CACHE, TRACE, OPENED);
  auto speed = cxsom::builder::variable("in", cxsom::builder::name("speed"),
                                        "Scalar", CACHE, TRACE, OPENED);
  auto thrust =
      cxsom::builder::variable("predict", cxsom::builder::name("thrust"),
                               "Scalar", CACHE, TRACE, OPENED);

  error->definition();
  speed->definition();
  thrust->definition();

  auto errorWe0 = cxsom::builder::variable(
      "save", cxsom::builder::name("error") / cxsom::builder::name("We-0"),
      wtype_ext, CACHE, TRACE, OPENED);
  auto speedWe0 = cxsom::builder::variable(
      "save", cxsom::builder::name("speed") / cxsom::builder::name("We-0"),
      wtype_ext, CACHE, TRACE, OPENED);
  auto thrustWe0 = cxsom::builder::variable(
      "save", cxsom::builder::name("thrust") / cxsom::builder::name("We-0"),
      wtype_ext, CACHE, TRACE, OPENED);

  errorMap->external(error, fx::match_gaussian, params.p_match, errorWe0,
                     saved_weight_at);
  speedMap->external(speed, fx::match_gaussian, params.p_match, speedWe0,
                     saved_weight_at);

  errorWe0->definition();
  errorWc0->definition();
  errorWc1->definition();
  speedWe0->definition();
  speedWc0->definition();
  speedWc1->definition();
  thrustWe0->definition();
  thrustWc0->definition();
  thrustWc1->definition();

  archi << errorMap << speedMap << thrustMap;
  *archi = map_settings;

  archi->realize();
  {
    std::ofstream dot_file("architecture/predict.dot");
    dot_file << archi->write_dot;
  }

  thrust->var() << fx::value_at(kwd::at(thrustWe0->var(), saved_weight_at),
                                thrustMap->output_BMU()->var()) |
      kwd::use("walltime", WALLTIME);
}