#pragma once

#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <stdexcept>
#include <string>
#include <vector>

class CryptoManager {
public:
  CryptoManager();
  ~CryptoManager();

  std::string getPublicKey() const;

  std::vector<uint8_t> encrypt(const std::string &plaintext,
                               const std::string &recipientPublicKey) const;
  std::string decrypt(const std::vector<uint8_t> &ciphertext) const;

private:
  RSA *privateKey = nullptr;
  RSA *publicKey = nullptr;

  void generateKeyPair();
  static std::string getOpenSSLError();
};
