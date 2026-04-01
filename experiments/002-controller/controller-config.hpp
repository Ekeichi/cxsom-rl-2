#pragma once

namespace controller_config {
  inline constexpr double dt_factor = .1;
  inline constexpr int target_period = 50;
  inline constexpr int margin = 200;
  inline constexpr int episode_duration = 10 * target_period;
  inline constexpr int max_thrust = 15;
  inline constexpr int prediction_wait_ms = 1500;

  inline constexpr const char* in_timeline = "in";
  inline constexpr const char* predict_timeline = "predict";
  inline constexpr const char* error_var = "error";
  inline constexpr const char* speed_var = "speed";
  inline constexpr const char* thrust_var = "thrust";
}
