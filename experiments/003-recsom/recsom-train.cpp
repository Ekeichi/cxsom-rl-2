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
      "in", cxsom::builder::name("error"), "Scalar", CACHE, TRACE, OPENED);
  auto speed_input = cxsom::builder::variable(
      "in", cxsom::builder::name("speed"), "Scalar", CACHE, TRACE, OPENED);
  auto thrust_input = cxsom::builder::variable(
      "in", cxsom::builder::name("thrust"), "Scalar", CACHE, TRACE, OPENED);

  error_input->definition();
  speed_input->definition();
  thrust_input->definition();

  auto errorMap = cxsom::builder::map::make_1D("error");
  auto speedMap = cxsom::builder::map::make_1D("speed");
  auto thrustMap = cxsom::builder::map::make_1D("thrust");

  auto map_settings = build_map_settings(params);

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

  // autre couche pour permettre de tenir compte du BMU de la map à t-1
  *(out_layer++) =
      errorMap->external(errorMap, fx::match_triangle, params.p_match,
                         cxsom::builder::timestep::previous(),
                         fx::learn_triangle, params.p_learn_e);
  *(out_layer++) =
      speedMap->external(speedMap, fx::match_triangle, params.p_match,
                         cxsom::builder::timestep::previous(),
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

  int save_period = SAVE_PERIOD;

  // Saving weights
  for (auto layer_ptr : layers) {
    auto W = layer_ptr->_W();
    auto Wsaved = cxsom::builder::variable("save", W->varname, W->type, CACHE,
                                           SAVE_TRACE, OPEN_AS_NEEDED);
    Wsaved->definition();
    Wsaved->var() << fx::copy(kwd::times(W->var(), save_period)) |
        kwd::use("walltime", WALLTIME);
  }

  std::ofstream dot_file("architecture-rec/train.dot");
  dot_file << archi->write_dot;

  return 0;
}
