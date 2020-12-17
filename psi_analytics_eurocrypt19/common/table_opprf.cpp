#include "table_opprf.h"
#include <openssl/sha.h>
#include <random>
#include<cstring>
/*
std::size_t num_of_luts_ = 5;
std::size_t num_of_tables_in_lut_ = 32;
std::vector<std::vector<std::uint64_t>> luts_;
std::size_t seed_ = 0;
std::mt19937_64 generator_;
std::size_t elem_byte_length_ = 8;

bool allocateLUTs() {
    generator_.seed(seed_);
    luts_.resize(num_of_luts_);
    for (auto& entry : luts_) {
      entry.resize(num_of_tables_in_lut_);
    }
  return true;
}

bool generateLUTs() {
    for (auto j = 0ull; j < num_of_luts_; ++j) {
      for (auto k = 0ull; k < num_of_tables_in_lut_; k++) {
        luts_.at(j).at(k) = generator_();
      }
    }
  return true;
}

std::uint64_t hashToPosition(uint64_t element) {
  std::uint64_t address = element;
  for (auto lut_i = 0ull; lut_i < num_of_luts_; ++lut_i) {
    std::size_t lut_id = ((address >> (lut_i * elem_byte_length_ / num_of_luts_)) & 0x000000FFu);
    lut_id %= num_of_tables_in_lut_;
    address ^= luts_.at(lut_i).at(lut_id);
  }
  return address;
}
*/
std::uint64_t hashToPosition(std::uint64_t element, osuCrypto::block nonce) {
  SHA_CTX ctx;
  unsigned char hash[SHA_DIGEST_LENGTH];

  unsigned char* message=(unsigned char*)malloc(sizeof(uint64_t)+sizeof(osuCrypto::block));
  memcpy(message, &element,sizeof(uint64_t));
  memcpy(message+sizeof(uint64_t), &nonce, sizeof(osuCrypto::block));

  SHA1_Init(&ctx);
  SHA1_Update(&ctx, message, sizeof(uint64_t)+sizeof(osuCrypto::block));
  SHA1_Final(hash, &ctx);

  uint64_t result = 0;
  std::copy(hash, hash + sizeof(result), reinterpret_cast<unsigned char*>(&result));

  return result;
}