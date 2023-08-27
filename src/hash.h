#ifndef HASH_H_
#define HASH_H_

#include <iostream>
#include <random>
#include <cstdint>


const int HASH_MAX_BOARD = 19;


class Hasher {
 public:
  uint64_t point_keys[2][HASH_MAX_BOARD][HASH_MAX_BOARD];

  Hasher() {
    /* Have noticed that constructor seems to be called multiple times
    unexpectedly.  Not sure why, but seems to be OK. */
    /* std::cout << "Setting hash\n"; */
    std::default_random_engine engine;
    std::uniform_int_distribution<uint64_t> dist;

    for (auto p=0; p<2; p++) {
      for (auto i=0; i<HASH_MAX_BOARD; i++) {
        for (auto j=0; j<HASH_MAX_BOARD; j++) {
          point_keys[p][i][j] = dist(engine);
        }
      }
    }
  }
};


#endif // HASH_H_