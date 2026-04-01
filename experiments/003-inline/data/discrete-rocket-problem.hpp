#pragma once

#include <gdyn.hpp>
#include <rllib2.hpp>

namespace rocket {
  
  namespace enumerable {

    // This sets all the types needed for a specific rocket
    // implementation.
    
    template<
      unsigned int NB_ERRORS, double MIN_ERROR, double MAX_ERROR,
      unsigned int NB_SPEEDS, double MIN_SPEED, double MAX_SPEED,
      double COMMAND_VALUE, double COMMAND_DURATION>
    struct types {
      constexpr static double       thrust     {COMMAND_VALUE};
      constexpr static double       dt         {COMMAND_DURATION};
      constexpr static unsigned int nb_errors  {NB_ERRORS};
      constexpr static double       min_errors {MIN_ERROR};
      constexpr static double       max_errors {MAX_ERROR};
      constexpr static unsigned int nb_speeds  {NB_SPEEDS};
      constexpr static double       min_speeds {MIN_SPEED};
      constexpr static double       max_speeds {MAX_SPEED};
      
      using rocket_phase   = gdyn::problem::rocket::relative::phase;
      using rocket_command = gdyn::problem::rocket::thrust;
      
      // We use digitized types internally in order to discretize the
      // {error, speed} type corresponding to the rocket phase.
      using error = rl2::enumerable::utils::digitize::scalar<NB_ERRORS, MIN_ERROR, MAX_ERROR>::set;
      using speed = rl2::enumerable::utils::digitize::scalar<NB_SPEEDS, MIN_SPEED, MAX_SPEED>::set;
      using error_speed = rl2::enumerable::pair<error, speed>;
      
      struct S_convertor {
	static constexpr std::size_t size() {return error_speed::size();}
	
	static rocket_phase to(std::size_t index) {
	  error_speed es {index};
	  auto [e, s] = static_cast<error_speed::base_type>(es);
	  return {e, s};
	}
	
	static std::size_t from(rocket_phase value) {
	  error_speed es {std::make_pair(value.error, value.speed)};
	  return static_cast<std::size_t>(es);
	}
      };
      
      using S = rl2::enumerable::set<rocket_phase, S_convertor::size(), S_convertor>;

      // We consider two thrust values only, 0 and COMMAND_VALUE.
      struct A_convertor {
	static constexpr std::size_t size() {return 2;}
	
	static rocket_command to (std::size_t index) {
	  if(index == 0) return {.value = 0., .duration = COMMAND_DURATION};
	  return {.value = COMMAND_VALUE, .duration = COMMAND_DURATION};   
	}
	static std::size_t from(const rocket_command d)  {
	  if(d.value > 0) return 1;
	  return 0; 
	}
      };
      
      using A = rl2::enumerable::set<rocket_command, A_convertor::size(), A_convertor>;
      
      using SA = rl2::enumerable::pair<S, A>;

      using base_continuous_system = gdyn::problem::rocket::system;
      
      using continuous_system = gdyn::problem::rocket::relative::system;
      
      // The continuous_system has error only as observation. This is
      // not Markovian. We need the rocket speed to handle a markovian
      // observation. This is why we get the exposed system.
      using exposed_system = gdyn::system::exposed<continuous_system>;
      
      // Now we can descretize it.
      using discrete_system = rl2::enumerable::system<S, S, A, exposed_system>;
    };
    
  }
  
}
