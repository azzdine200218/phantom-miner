#include "../../include/utils/crypto.h"
#include <cstring>
#include <openssl/evp.h>
#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include <functional>
#include <stdexcept>
#include <map>



namespace XMR {

static const std::string base64_chars =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

std::string base64_encode(const std::string &input) {
  std::string out;
  int val = 0, valb = -6;
  for (unsigned char c : input) {
    val = (val << 8) + c;
    valb += 8;
    while (valb >= 0) {
      out.push_back(base64_chars[(val >> valb) & 0x3F]);
      valb -= 6;
    }
  }
  if (valb > -6)
    out.push_back(base64_chars[((val << 8) >> (valb + 8)) & 0x3F]);
  while (out.size() % 4)
    out.push_back('=');
  return out;
}

std::string base64_decode(const std::string &input) {
  std::string out;
  std::vector<int> T(256, -1);
  for (int i = 0; i < 64; i++)
    T[base64_chars[i]] = i;
  int val = 0, valb = -8;
  for (unsigned char c : input) {
    if (T[c] == -1)
      break;
    val = (val << 6) + T[c];
    valb += 6;
    if (valb >= 0) {
      out.push_back(char((val >> valb) & 0xFF));
      valb -= 8;
    }
  }
  return out;
}

std::vector<uint8_t> aes_encrypt(const std::vector<uint8_t> &data,
                                 const std::vector<uint8_t> &key) {
  std::vector<uint8_t> out(data.size() + 16);
  EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
  EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, key.data(), nullptr);
  int len;
  EVP_EncryptUpdate(ctx, out.data(), &len, data.data(), data.size());
  int final_len;
  EVP_EncryptFinal_ex(ctx, out.data() + len, &final_len);
  EVP_CIPHER_CTX_free(ctx);
  out.resize(len + final_len);
  return out;
}

std::vector<uint8_t> aes_decrypt(const std::vector<uint8_t> &data,
                                 const std::vector<uint8_t> &key) {
  std::vector<uint8_t> out(data.size());
  EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
  EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, key.data(), nullptr);
  int len;
  EVP_DecryptUpdate(ctx, out.data(), &len, data.data(), data.size());
  int final_len;
  EVP_DecryptFinal_ex(ctx, out.data() + len, &final_len);
  EVP_CIPHER_CTX_free(ctx);
  out.resize(len + final_len);
  return out;
}

} // namespace XMR
