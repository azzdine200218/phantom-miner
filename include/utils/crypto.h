#ifndef XMR_CRYPTO_H
#define XMR_CRYPTO_H

#include <string>
#include <vector>
#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <thread>
#include <functional>
#include <stdexcept>
#include <map>


    namespace XMR {

  std::string base64_encode(const std::string &input);
  std::string base64_decode(const std::string &input);
  std::vector<uint8_t> aes_encrypt(const std::vector<uint8_t> &data,
                                   const std::vector<uint8_t> &key);
  std::vector<uint8_t> aes_decrypt(const std::vector<uint8_t> &data,
                                   const std::vector<uint8_t> &key);

} // namespace XMR

#endif
