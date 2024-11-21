#include "CryptoManager.h"
#include <iostream>
#include <sstream>

CryptoManager::CryptoManager() { generateKeyPair(); }

CryptoManager::~CryptoManager() {
  if (privateKey)
    RSA_free(privateKey);
  if (publicKey)
    RSA_free(publicKey);
}

void CryptoManager::generateKeyPair() {
  BIGNUM *e = BN_new();
  BN_set_word(e, RSA_F4);

  privateKey = RSA_new();
  if (!RSA_generate_key_ex(privateKey, 2048, e, nullptr)) {
    BN_free(e);
    throw std::runtime_error("Key generation failed: " + getOpenSSLError());
  }

  // Extract the public key from the private key
  publicKey = RSAPublicKey_dup(privateKey);
  BN_free(e);

  if (!publicKey) {
    throw std::runtime_error("Failed to extract public key: " +
                             getOpenSSLError());
  }
}

std::string CryptoManager::getPublicKey() const {
  BIO *bio = BIO_new(BIO_s_mem());
  PEM_write_bio_RSA_PUBKEY(bio, publicKey);

  char *keyBuffer;
  size_t keyLength = BIO_get_mem_data(bio, &keyBuffer);
  std::string pubKey(keyBuffer, keyLength);

  BIO_free(bio);
  return pubKey;
}

std::vector<uint8_t>
CryptoManager::encrypt(const std::string &plaintext, const std::string &recipientPublicKeyPEM) const {
  BIO *bio = BIO_new_mem_buf(recipientPublicKeyPEM.data(), static_cast<int>(recipientPublicKeyPEM.size()));
  if (!bio) {
    throw std::runtime_error("Failed to create BIO for recipient public key");
  }

  RSA *recipientPublicKey = PEM_read_bio_RSA_PUBKEY(bio, nullptr, nullptr, nullptr);
  BIO_free(bio);
  if (!recipientPublicKey) {
    throw std::runtime_error("Failed to load recipient public key: " + getOpenSSLError());
  }

  std::vector<uint8_t> encrypted(RSA_size(recipientPublicKey));

  int len = RSA_public_encrypt(
      plaintext.size(),
      reinterpret_cast<const uint8_t *>(plaintext.c_str()),
      encrypted.data(),
      recipientPublicKey,
      RSA_PKCS1_OAEP_PADDING);

  RSA_free(recipientPublicKey);

  if (len < 0) {
    throw std::runtime_error("Encryption failed: " + getOpenSSLError());
  }

  encrypted.resize(len);
  return encrypted;
}

std::string
CryptoManager::decrypt(const std::vector<uint8_t> &ciphertext) const {
  std::vector<uint8_t> decrypted(RSA_size(privateKey));
  int len =
      RSA_private_decrypt(ciphertext.size(), ciphertext.data(),
                          decrypted.data(), privateKey, RSA_PKCS1_OAEP_PADDING);

  if (len < 0) {
    throw std::runtime_error("Decryption failed: " + getOpenSSLError());
  }

  return std::string(decrypted.begin(), decrypted.begin() + len);
}

std::string CryptoManager::getOpenSSLError() {
  char errBuffer[120];
  ERR_error_string_n(ERR_get_error(), errBuffer, sizeof(errBuffer));
  return std::string(errBuffer);
}
