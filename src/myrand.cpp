#include <vector>

#include "myrand.h"

// static std::default_random_engine get_engine() {
//   std::default_random_engine rng;
//   return rng;
// }

std::default_random_engine rng(std::random_device{}());
// std::default_random_engine rng = get_engine();


std::vector<double> DirichletDistribution::sample() {
  std::vector<double> samples;
  double sum = 0.0;
  for (int i=0; i<n; ++i) {
    double sample = gamma_dist(rng);
    sum += sample;
    samples.push_back(sample);
  }

  for (auto& x : samples)
    x /= sum;

  return samples;
}
