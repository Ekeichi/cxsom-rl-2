#include "xsom.hpp"

// ####################
// #                  #
// #  Building archi  #
// #                  #
// ####################

int main(int argc, char *argv[]) {
  context c(argc, argv);

  auto archi = cxsom::builder::architecture();
  Params params;

  auto error_input = cxsom::builder::variable(
      "in", cxsom::builder::name("xi_error"), "Scalar", CACHE, TRACE, OPENED);
  auto speed_input = cxsom::builder::variable(
      "in", cxsom::builder::name("xi_speed"), "Scalar", CACHE, TRACE, OPENED);
  auto thrust_input = cxsom::builder::variable(
      "in", cxsom::builder::name("xi_thrust"), "Scalar", CACHE, TRACE, OPENED);

  error_input->definition();
  speed_input->definition();
  thrust_input->definition();

  auto errorMap = cxsom::builder::map::make_1D("error");
  auto speedMap = cxsom::builder::map::make_1D("speed");
  auto thrustMap = cxsom::builder::map::make_1D("thrust");

  auto map_settings = cxsom::builder::map::make_settings();
  map_settings.map_size = MAP_SIZE;
  map_settings.cache_size = CACHE;
  map_settings.weights_file_size = TRACE;
  map_settings.kept_opened = OPENED;
  map_settings = {params.p_external, params.p_contextual, params.p_global};

  std::vector<cxsom::builder::Map::Layer *> layers;
  auto out_layer = std::back_inserter(layers);

  // Liens entre les inputs et les cartes
  *(out_layer++) =
      errorMap->external(error_input, fx::match_gaussian, params.p_match,
                         fx::learn_triangle, params.p_learn_e);
  *(out_layer++) =
      speedMap->external(speed_input, fx::match_gaussian, params.p_match,
                         fx::learn_triangle, params.p_learn_e);
  *(out_layer++) =
      thrustMap->external(thrust_input, fx::match_gaussian, params.p_match,
                          fx::learn_triangle, params.p_learn_e);

  // Liens entre les cartes
  *(out_layer++) =
      errorMap->contextual(speedMap, fx::match_gaussian, params.p_match,
                           fx::learn_triangle, params.p_learn_c);
  *(out_layer++) =
      speedMap->contextual(thrustMap, fx::match_gaussian, params.p_match,
                           fx::learn_triangle, params.p_learn_c);
  *(out_layer++) =
      thrustMap->contextual(errorMap, fx::match_gaussian, params.p_match,
                            fx::learn_triangle, params.p_learn_c);
  *(out_layer++) =
      errorMap->contextual(thrustMap, fx::match_gaussian, params.p_match,
                           fx::learn_triangle, params.p_learn_c);
  *(out_layer++) =
      speedMap->contextual(errorMap, fx::match_gaussian, params.p_match,
                           fx::learn_triangle, params.p_learn_c);
  *(out_layer++) =
      thrustMap->contextual(speedMap, fx::match_gaussian, params.p_match,
                            fx::learn_triangle, params.p_learn_c);

  archi << errorMap << speedMap << thrustMap;
  *archi = map_settings;

  archi->realize();

  for (auto map : archi->maps)
    map->internals_random_at(0);

  int save_period = TRACE / SAVE_TRACE;

  // Saving weights
  for (auto layer_ptr : layers) {
    auto W = layer_ptr->_W();
    auto Wsaved = cxsom::builder::variable("save", W->varname, W->type, CACHE,
                                           SAVE_TRACE, OPEN_AS_NEEDED);
    Wsaved->definition();
    Wsaved->var() << fx::copy(kwd::times(W->var(), save_period)) |
        kwd::use("walltime", WALLTIME);
  }

  std::ofstream dot_file("architecture/train.dot");
  dot_file << archi->write_dot;

  return 0;
}
