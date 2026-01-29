/*
  Rocket Controller Logic with CXSOM

  We train 3 connected maps:
  - Error (state)
  - Velocity (state)
  - Thrust (action to predict)

  The system learns the association (Error, Velocity) <-> Thrust.
*/

#include <cxsom-builder.hpp>
#include <fstream>
#include <iterator>
#include <sstream>
#include <tuple>

#define CACHE 2
#define SAVE_TRACE 1000
#define TRAIN_TRACE 10
#define OPENED true
#define OPEN_AS_NEEDED false
#define FORGET 0
#define FOREVER -1 // Infinite walltime
#define DEADLINE 1000
#define DEFAULT_DATA_SIZE 2601

// cxsom declarations
using namespace cxsom::rules;
context *cxsom::rules::ctx = nullptr;

enum class Mode : char { Calibration, Input, Train, Check, Predict, Walltime };

// ####################
// #                  #
// # General settings #
// #                  #
// ####################

#define Rext .05
#define Rctx .003

struct Params {
  kwd::parameters main, match_ctx, match_state, match_thrust, learn,
      learn_state_e, learn_state_c, learn_thrust_e, learn_thrust_c, external,
      contextual, global;

  Params() {
    main | kwd::use("walltime", FOREVER), kwd::use("epsilon", 0);

    match_ctx | main, kwd::use("sigma", .075);

    match_state | main, kwd::use("sigma", .075);
    match_thrust | main, kwd::use("sigma", .075);

    learn | main, kwd::use("alpha", .1);

    learn_state_e | learn, kwd::use("r", Rext);
    learn_state_c | learn, kwd::use("r", Rctx);
    learn_thrust_e | learn, kwd::use("r", Rext);
    learn_thrust_c | learn, kwd::use("r", Rctx);

    external | main;
    contextual | main;
    global | main, kwd::use("random-bmu", 1), kwd::use("beta", .5),
        kwd::use("delta", .02), kwd::use("deadline", DEADLINE);
  }
};

auto make_map_settings(const Params &p, unsigned int map_size) {
  auto map_settings = cxsom::builder::map::make_settings();
  map_settings.map_size = map_size;
  map_settings.cache_size = CACHE;
  map_settings.weights_file_size = TRAIN_TRACE;
  map_settings.kept_opened = OPENED;
  map_settings = {p.external, p.contextual, p.global};
  map_settings.argmax = fx::argmax;
  map_settings.toward_argmax = fx::toward_argmax;

  return map_settings;
}

// Helper pour déclarer les variables principales
auto rocket_inputs(const std::string &timeline, unsigned int trace,
                   bool to_be_defined) {
  auto ERR = cxsom::builder::variable(timeline, cxsom::builder::name("error"),
                                      "Scalar", CACHE, trace, OPENED);
  auto VEL =
      cxsom::builder::variable(timeline, cxsom::builder::name("velocity"),
                               "Scalar", CACHE, trace, OPENED);
  auto THR = cxsom::builder::variable(timeline, cxsom::builder::name("thrust"),
                                      "Scalar", CACHE, trace, OPENED);
  if (to_be_defined) {
    ERR->definition();
    VEL->definition();
    THR->definition();
  }
  return std::make_tuple(ERR, VEL, THR);
}

// #####################
// #                   #
// # Calibration stage #
// #                   #
// #####################

void make_calibration_rules(unsigned grid_side) {
  Params p;
  {
    timeline t("calibration");

    kwd::type("thrust-ref", "Scalar", 2, 1, OPENED);
    kwd::type("state-ref", "Scalar", 2, 1, OPENED);

    unsigned int volume = grid_side * grid_side;

    kwd::type("thrust-samples",
              std::string("Map1D<Scalar>=") + std::to_string(volume), 2, 1,
              OPENED);
    kwd::type("state-samples",
              std::string("Map1D<Scalar>=") + std::to_string(grid_side), 2, 1,
              OPENED);
    kwd::type("thrust-match",
              std::string("Map1D<Scalar>=") + std::to_string(volume), 2, 1,
              OPENED);
    kwd::type("state-match",
              std::string("Map1D<Scalar>=") + std::to_string(grid_side), 2, 1,
              OPENED);
    kwd::type("ctx-match",
              std::string("Map1D<Scalar>=") + std::to_string(grid_side), 2, 1,
              OPENED);

    kwd::at("thrust-match", 0) << fx::match_gaussian(
        kwd::at("thrust-ref", 0), kwd::at("thrust-samples", 0)) |
        p.match_thrust;
    kwd::at("state-match", 0) << fx::match_gaussian(
        kwd::at("state-ref", 0), kwd::at("state-samples", 0)) |
        p.match_state;
    kwd::at("ctx-match", 0) << fx::match_gaussian(kwd::at("state-ref", 0),
                                                  kwd::at("state-samples", 0)) |
        p.match_ctx;
  }
}

// ##################
// #                #
// # Walltime stage #
// #                #
// ##################

void make_walltime_rules(unsigned int walltime) {
  {
    timeline t{"train-in"};
    "index" << fx::random() | kwd::use("walltime", walltime);
  }
}

// ###############
// #             #
// # Input stage #
// #             #
// ###############

void make_input_rules(unsigned int data_size) {

  std::string type = std::string("Map1D<Scalar>=") + std::to_string(data_size);

  cxsom::builder::variable("img", cxsom::builder::name("error_data"), type, 1,
                           1, OPENED)
      ->definition();
  cxsom::builder::variable("img", cxsom::builder::name("velocity_data"), type,
                           1, 1, OPENED)
      ->definition();
  cxsom::builder::variable("img", cxsom::builder::name("thrust_data"), type, 1,
                           1, OPENED)
      ->definition();
}

// ###############
// #             #
// # Train stage #
// #             #
// ###############

void make_train_rules(unsigned int save_period, unsigned int data_size,
                      unsigned int map_size) {

  Params p;
  auto map_settings = make_map_settings(p, map_size);

  auto archi = cxsom::builder::architecture();
  archi->timelines = {"train-wgt", "train-rlx", "train-out"};

  auto [ERR, VEL, THR] = rocket_inputs("train-in", TRAIN_TRACE, true);

  std::vector<cxsom::builder::Map::Layer *> layers;
  auto out_layer = std::back_inserter(layers);

  auto ERRmap = cxsom::builder::map::make_1D("Error");
  auto VELmap = cxsom::builder::map::make_1D("Velocity");
  auto THRmap = cxsom::builder::map::make_1D("Thrust");

  // --- Connexions ---
  *(out_layer++) = ERRmap->contextual(VELmap, fx::match_gaussian, p.match_ctx,
                                      fx::learn_triangle, p.learn_state_c);
  *(out_layer++) = VELmap->contextual(ERRmap, fx::match_gaussian, p.match_ctx,
                                      fx::learn_triangle, p.learn_state_c);
  *(out_layer++) = ERRmap->contextual(THRmap, fx::match_gaussian, p.match_ctx,
                                      fx::learn_triangle, p.learn_state_c);
  *(out_layer++) = THRmap->contextual(ERRmap, fx::match_gaussian, p.match_ctx,
                                      fx::learn_triangle, p.learn_thrust_c);
  *(out_layer++) = VELmap->contextual(THRmap, fx::match_gaussian, p.match_ctx,
                                      fx::learn_triangle, p.learn_state_c);
  *(out_layer++) = THRmap->contextual(VELmap, fx::match_gaussian, p.match_ctx,
                                      fx::learn_triangle, p.learn_thrust_c);

  *(out_layer++) = ERRmap->external(ERR, fx::match_gaussian, p.match_state,
                                    fx::learn_triangle, p.learn_state_e);
  *(out_layer++) = VELmap->external(VEL, fx::match_gaussian, p.match_state,
                                    fx::learn_triangle, p.learn_state_e);
  *(out_layer++) = THRmap->external(THR, fx::match_gaussian, p.match_thrust,
                                    fx::learn_triangle, p.learn_thrust_e);

  archi << ERRmap << VELmap << THRmap;
  *archi = map_settings;

  for (auto map : archi->maps)
    map->internals_random_at(0);

  archi->realize();
  {
    std::ofstream dot_file("train.dot");
    dot_file << archi->write_dot;
  }

  // --- ALIMENTATION ---
  std::string data_map_type =
      std::string("Map1D<Scalar>=") + std::to_string(data_size);

  auto FILE_ERR = cxsom::builder::variable(
      "img", cxsom::builder::name("error_data"), data_map_type, 1, 1, OPENED);
  auto FILE_VEL =
      cxsom::builder::variable("img", cxsom::builder::name("velocity_data"),
                               data_map_type, 1, 1, OPENED);
  auto FILE_THR = cxsom::builder::variable(
      "img", cxsom::builder::name("thrust_data"), data_map_type, 1, 1, OPENED);

  auto INDEX =
      cxsom::builder::variable("train-in", cxsom::builder::name("index"),
                               "Pos1D", CACHE, TRAIN_TRACE, OPENED);
  INDEX->definition();
  INDEX->var() << fx::random() | kwd::use("walltime", 0);

  ERR->var() << fx::value_at(kwd::at(FILE_ERR->var(), 0), INDEX->var()) |
      kwd::use("walltime", FOREVER);
  VEL->var() << fx::value_at(kwd::at(FILE_VEL->var(), 0), INDEX->var()) |
      kwd::use("walltime", FOREVER);
  THR->var() << fx::value_at(kwd::at(FILE_THR->var(), 0), INDEX->var()) |
      kwd::use("walltime", FOREVER);

  // --- SAUVEGARDE ---
  for (auto layer_ptr : layers) {
    auto W = layer_ptr->_W();
    auto Wsaved = cxsom::builder::variable("saved", W->varname, W->type, CACHE,
                                           SAVE_TRACE, OPEN_AS_NEEDED);
    Wsaved->definition();
    Wsaved->var() << fx::copy(kwd::times(W->var(), save_period)) |
        kwd::use("walltime", FOREVER);
  }
}

// ###############
// #             #
// # Check stage #
// #             #
// ###############

void make_check_rules(unsigned int saved_weight_at, unsigned int data_size,
                      unsigned int map_size) {

  unsigned int trace = data_size;
  Params p;
  auto map_settings = make_map_settings(p, map_size);
  map_settings.exposure_file_size = trace;

  auto archi = cxsom::builder::architecture();
  archi->timelines = {"check-wgt", "check-rlx", "check-out"};

  // Distinction essentielle : Types Scalaire vs Position
  std::string scalar_map_type =
      std::string("Map1D<Scalar>=") + std::to_string(map_size);
  std::string pos_map_type =
      std::string("Map1D<Pos1D>=") + std::to_string(map_size);

  auto ERRmap = cxsom::builder::map::make_1D("Error");
  auto VELmap = cxsom::builder::map::make_1D("Velocity");
  auto THRmap = cxsom::builder::map::make_1D("Thrust");

  // Poids Contextuels (Wc) = Type POS1D
  auto ERR_c0 = cxsom::builder::variable(
      "saved", cxsom::builder::name("Error") / cxsom::builder::name("Wc-0"),
      pos_map_type, CACHE, trace, OPENED);
  auto ERR_c1 = cxsom::builder::variable(
      "saved", cxsom::builder::name("Error") / cxsom::builder::name("Wc-1"),
      pos_map_type, CACHE, trace, OPENED);
  auto VEL_c0 = cxsom::builder::variable(
      "saved", cxsom::builder::name("Velocity") / cxsom::builder::name("Wc-0"),
      pos_map_type, CACHE, trace, OPENED);
  auto VEL_c1 = cxsom::builder::variable(
      "saved", cxsom::builder::name("Velocity") / cxsom::builder::name("Wc-1"),
      pos_map_type, CACHE, trace, OPENED);
  auto THR_c0 = cxsom::builder::variable(
      "saved", cxsom::builder::name("Thrust") / cxsom::builder::name("Wc-0"),
      pos_map_type, CACHE, trace, OPENED);
  auto THR_c1 = cxsom::builder::variable(
      "saved", cxsom::builder::name("Thrust") / cxsom::builder::name("Wc-1"),
      pos_map_type, CACHE, trace, OPENED);

  ERRmap->contextual(VELmap, fx::match_gaussian, p.match_ctx, ERR_c0,
                     saved_weight_at);
  ERRmap->contextual(THRmap, fx::match_gaussian, p.match_ctx, ERR_c1,
                     saved_weight_at);
  VELmap->contextual(ERRmap, fx::match_gaussian, p.match_ctx, VEL_c0,
                     saved_weight_at);
  VELmap->contextual(THRmap, fx::match_gaussian, p.match_ctx, VEL_c1,
                     saved_weight_at);
  THRmap->contextual(ERRmap, fx::match_gaussian, p.match_ctx, THR_c0,
                     saved_weight_at);
  THRmap->contextual(VELmap, fx::match_gaussian, p.match_ctx, THR_c1,
                     saved_weight_at);

  auto ERR = cxsom::builder::variable("img", cxsom::builder::name("error"),
                                      "Scalar", CACHE, trace, OPENED);
  auto VEL = cxsom::builder::variable("img", cxsom::builder::name("velocity"),
                                      "Scalar", CACHE, trace, OPENED);
  auto THR = cxsom::builder::variable("img", cxsom::builder::name("thrust"),
                                      "Scalar", CACHE, trace, OPENED);
  ERR->definition();
  VEL->definition();
  THR->definition();

  std::string data_map_type =
      std::string("Map1D<Scalar>=") + std::to_string(data_size);
  auto FILE_ERR = cxsom::builder::variable(
      "img", cxsom::builder::name("error_data"), data_map_type, 1, 1, OPENED);
  auto FILE_VEL =
      cxsom::builder::variable("img", cxsom::builder::name("velocity_data"),
                               data_map_type, 1, 1, OPENED);
  auto FILE_THR = cxsom::builder::variable(
      "img", cxsom::builder::name("thrust_data"), data_map_type, 1, 1, OPENED);

  auto INDEX =
      cxsom::builder::variable("check-out", cxsom::builder::name("index"),
                               "Pos1D", CACHE, trace, OPENED);
  INDEX->definition();
  INDEX->var() << fx::random() | kwd::use("walltime", data_size);

  ERR->var() << fx::value_at(kwd::at(FILE_ERR->var(), 0), INDEX->var()) |
      kwd::use("walltime", FOREVER);
  VEL->var() << fx::value_at(kwd::at(FILE_VEL->var(), 0), INDEX->var()) |
      kwd::use("walltime", FOREVER);
  THR->var() << fx::value_at(kwd::at(FILE_THR->var(), 0), INDEX->var()) |
      kwd::use("walltime", FOREVER);

  // Poids Externes (We) = Type SCALAR
  auto ERR_e0 = cxsom::builder::variable(
      "saved", cxsom::builder::name("Error") / cxsom::builder::name("We-0"),
      scalar_map_type, CACHE, trace, OPENED);
  auto VEL_e0 = cxsom::builder::variable(
      "saved", cxsom::builder::name("Velocity") / cxsom::builder::name("We-0"),
      scalar_map_type, CACHE, trace, OPENED);
  auto THR_e0 = cxsom::builder::variable(
      "saved", cxsom::builder::name("Thrust") / cxsom::builder::name("We-0"),
      scalar_map_type, CACHE, trace, OPENED);

  ERRmap->external(ERR, fx::match_gaussian, p.match_state, ERR_e0,
                   saved_weight_at) |
      cxsom::builder::expose::weight;
  VELmap->external(VEL, fx::match_gaussian, p.match_state, VEL_e0,
                   saved_weight_at) |
      cxsom::builder::expose::weight;
  THRmap->external(THR, fx::match_gaussian, p.match_thrust, THR_e0,
                   saved_weight_at) |
      cxsom::builder::expose::weight;

  ERR_e0->definition();
  ERR_c0->definition();
  ERR_c1->definition();
  VEL_e0->definition();
  VEL_c0->definition();
  VEL_c1->definition();
  THR_e0->definition();
  THR_c0->definition();
  THR_c1->definition();

  archi << ERRmap << VELmap << THRmap;
  *archi = map_settings;

  archi->realize();
  {
    std::ofstream dot_file("check.dot");
    dot_file << archi->write_dot;
  }
}

// #################
// #               #
// # Predict stage #
// #               #
// #################

void make_predict_rules(unsigned int saved_weight_at, unsigned int data_size,
                        unsigned int map_size) {

  Params p;
  auto map_settings = make_map_settings(p, map_size);
  auto archi = cxsom::builder::architecture();
  archi->timelines = {"predict-wgt", "predict-rlx", "predict-out"};

  // Distinction essentielle
  std::string scalar_map_type =
      std::string("Map1D<Scalar>=") + std::to_string(map_size);
  std::string pos_map_type =
      std::string("Map1D<Pos1D>=") + std::to_string(map_size);
  unsigned int trace = data_size;

  auto ERRmap = cxsom::builder::map::make_1D("Error");
  auto VELmap = cxsom::builder::map::make_1D("Velocity");
  auto THRmap = cxsom::builder::map::make_1D("Thrust");

  // Poids Contextuels (Wc) = Type POS1D
  auto ERR_c0 = cxsom::builder::variable(
      "saved", cxsom::builder::name("Error") / cxsom::builder::name("Wc-0"),
      pos_map_type, CACHE, trace, OPENED);
  auto ERR_c1 = cxsom::builder::variable(
      "saved", cxsom::builder::name("Error") / cxsom::builder::name("Wc-1"),
      pos_map_type, CACHE, trace, OPENED);
  auto VEL_c0 = cxsom::builder::variable(
      "saved", cxsom::builder::name("Velocity") / cxsom::builder::name("Wc-0"),
      pos_map_type, CACHE, trace, OPENED);
  auto VEL_c1 = cxsom::builder::variable(
      "saved", cxsom::builder::name("Velocity") / cxsom::builder::name("Wc-1"),
      pos_map_type, CACHE, trace, OPENED);
  auto THR_c0 = cxsom::builder::variable(
      "saved", cxsom::builder::name("Thrust") / cxsom::builder::name("Wc-0"),
      pos_map_type, CACHE, trace, OPENED);
  auto THR_c1 = cxsom::builder::variable(
      "saved", cxsom::builder::name("Thrust") / cxsom::builder::name("Wc-1"),
      pos_map_type, CACHE, trace, OPENED);

  ERRmap->contextual(VELmap, fx::match_gaussian, p.match_state, ERR_c0,
                     saved_weight_at);
  ERRmap->contextual(THRmap, fx::match_gaussian, p.match_state, ERR_c1,
                     saved_weight_at);
  VELmap->contextual(ERRmap, fx::match_gaussian, p.match_state, VEL_c0,
                     saved_weight_at);
  VELmap->contextual(THRmap, fx::match_gaussian, p.match_state, VEL_c1,
                     saved_weight_at);
  THRmap->contextual(ERRmap, fx::match_gaussian, p.match_state, THR_c0,
                     saved_weight_at);
  THRmap->contextual(VELmap, fx::match_gaussian, p.match_state, THR_c1,
                     saved_weight_at);

  auto ERR = cxsom::builder::variable("img", cxsom::builder::name("error"),
                                      "Scalar", CACHE, trace, OPENED);
  auto VEL = cxsom::builder::variable("img", cxsom::builder::name("velocity"),
                                      "Scalar", CACHE, trace, OPENED);
  // On renomme "rgb" en "predicted-thrust"
  auto THR_OUT = cxsom::builder::variable(
      "predict-out", cxsom::builder::name("predicted-thrust"), "Scalar", CACHE,
      trace, OPENED);

  ERR->definition();
  VEL->definition();
  THR_OUT->definition();

  std::string data_map_type =
      std::string("Map1D<Scalar>=") + std::to_string(data_size);
  auto FILE_ERR = cxsom::builder::variable(
      "img", cxsom::builder::name("error_data"), data_map_type, 1, 1, OPENED);
  auto FILE_VEL =
      cxsom::builder::variable("img", cxsom::builder::name("velocity_data"),
                               data_map_type, 1, 1, OPENED);

  auto INDEX =
      cxsom::builder::variable("predict-out", cxsom::builder::name("index"),
                               "Pos1D", CACHE, trace, OPENED);
  INDEX->definition();
  INDEX->var() << fx::random() | kwd::use("walltime", data_size);

  ERR->var() << fx::value_at(kwd::at(FILE_ERR->var(), 0), INDEX->var()) |
      kwd::use("walltime", FOREVER);
  VEL->var() << fx::value_at(kwd::at(FILE_VEL->var(), 0), INDEX->var()) |
      kwd::use("walltime", FOREVER);

  // Poids Externes (We) = Type SCALAR
  auto ERR_e0 = cxsom::builder::variable(
      "saved", cxsom::builder::name("Error") / cxsom::builder::name("We-0"),
      scalar_map_type, CACHE, trace, OPENED);
  auto VEL_e0 = cxsom::builder::variable(
      "saved", cxsom::builder::name("Velocity") / cxsom::builder::name("We-0"),
      scalar_map_type, CACHE, trace, OPENED);
  auto THR_e0 = cxsom::builder::variable(
      "saved", cxsom::builder::name("Thrust") / cxsom::builder::name("We-0"),
      scalar_map_type, CACHE, trace, OPENED);

  ERRmap->external(ERR, fx::match_gaussian, p.match_ctx, ERR_e0,
                   saved_weight_at);
  VELmap->external(VEL, fx::match_gaussian, p.match_ctx, VEL_e0,
                   saved_weight_at);

  ERR_e0->definition();
  VEL_e0->definition();
  THR_e0->definition();
  ERR_c0->definition();
  ERR_c1->definition();
  VEL_c0->definition();
  VEL_c1->definition();
  THR_c0->definition();
  THR_c1->definition();

  archi << ERRmap << VELmap << THRmap;
  *archi = map_settings;

  archi->realize();
  {
    std::ofstream dot_file("predict.dot");
    dot_file << archi->write_dot;
  }

  THR_OUT->var() << fx::value_at(kwd::at(THR_e0->var(), saved_weight_at),
                                 THRmap->output_BMU()->var()) |
      kwd::use("walltime", FOREVER);
}

// ########
// #      #
// # Main #
// #      #
// ########

int main(int argc, char *argv[]) {
  context c(argc, argv);
  Mode mode;
  unsigned int walltime = 0;
  unsigned int grid_side = 0;
  unsigned int data_size = 0;
  unsigned int save_period = 0;
  unsigned int saved_weight_at = 0;
  unsigned int map_size = 0;

  std::ostringstream prefix;
  for (const auto &arg : c.argv)
    prefix << arg << ' ';
  prefix << "-- ";

  if (c.user_argv.size() == 0) {
    std::cout << "Usage:" << std::endl
              << "  " << prefix.str() << "calibration <grid-side>" << std::endl
              << "  " << prefix.str() << "walltime <max-time>" << std::endl
              << "  " << prefix.str() << "input <dummy>" << std::endl
              << "  " << prefix.str()
              << "train <save-period> <data-size> <map-size>" << std::endl
              << "  " << prefix.str()
              << "check <saved-weight-at> <data-size> <map-size>" << std::endl
              << "  " << prefix.str()
              << "predict <saved-weight-at> <data-size> <map-size>"
              << std::endl;
    c.notify_user_argv_error();
    return 0;
  }

  if (c.user_argv[0] == "calibration") {
    if (c.user_argv.size() != 2) {
      c.notify_user_argv_error();
      return 0;
    }
    grid_side = stoul(c.user_argv[1]);
    mode = Mode::Calibration;
  } else if (c.user_argv[0] == "walltime") {
    if (c.user_argv.size() != 2) {
      c.notify_user_argv_error();
      return 0;
    }
    walltime = stoul(c.user_argv[1]);
    mode = Mode::Walltime;
  } else if (c.user_argv[0] == "input") {
    if (c.user_argv.size() >= 2)
      data_size = stoul(c.user_argv[1]);
    mode = Mode::Input;
  } else if (c.user_argv[0] == "train") {
    if (c.user_argv.size() != 4) {
      c.notify_user_argv_error();
      return 0;
    }
    save_period = stoul(c.user_argv[1]);
    data_size = stoul(c.user_argv[2]);
    map_size = stoul(c.user_argv[3]);
    mode = Mode::Train;
  } else if (c.user_argv[0] == "check") {
    if (c.user_argv.size() != 4) {
      c.notify_user_argv_error();
      return 0;
    }
    saved_weight_at = stoul(c.user_argv[1]);
    data_size = stoul(c.user_argv[2]);
    map_size = stoul(c.user_argv[3]);
    mode = Mode::Check;
  } else if (c.user_argv[0] == "predict") {
    if (c.user_argv.size() != 4) {
      c.notify_user_argv_error();
      return 0;
    }
    saved_weight_at = stoul(c.user_argv[1]);
    data_size = stoul(c.user_argv[2]);
    map_size = stoul(c.user_argv[3]);
    mode = Mode::Predict;
  } else {
    std::cout << "Bad user arguments." << std::endl;
    c.notify_user_argv_error();
    return 0;
  }

  switch (mode) {
  case Mode::Calibration:
    make_calibration_rules(grid_side);
    break;
  case Mode::Walltime:
    make_walltime_rules(walltime);
    break;
  case Mode::Input:
    make_input_rules(data_size);
    break;
  case Mode::Train:
    make_train_rules(save_period, data_size, map_size);
    break;
  case Mode::Predict:
    make_predict_rules(saved_weight_at, data_size, map_size);
    break;
  case Mode::Check:
    make_check_rules(saved_weight_at, data_size, map_size);
    break;
  default:
    break;
  }

  return 0;
}