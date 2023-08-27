#include "myrand.h"

// static std::default_random_engine get_engine() {
//   std::default_random_engine rng;
//   return rng;
// }

std::default_random_engine rng(std::random_device{}());
// std::default_random_engine rng = get_engine();
