#include "openssl_aes.c"

EVP_CIPHER_CTX* en;
EVP_CIPHER_CTX* de;

int aes_init_w(void* key, int keylen) {
  en = EVP_CIPHER_CTX_new();
  de = EVP_CIPHER_CTX_new();
  unsigned int salt[] = {1685449954, 133742069};
  aes_init(key, keylen, (unsigned char *)&salt, en, de);
}

void aes_deinit() {
  EVP_CIPHER_CTX_free(en);
  EVP_CIPHER_CTX_free(de);
}

void* enc_buf(char* plain, int* buflen) {
  return aes_encrypt(en, (unsigned char *)plain, buflen);
}

char* dec_buf(void* cipher, int* buflen) {
  return (char *)aes_decrypt(de, cipher, buflen);
}
