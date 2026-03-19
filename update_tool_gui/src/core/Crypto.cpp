#include "Crypto.h"

#include <QFile>
#include <QCryptographicHash>

#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <openssl/rsa.h>
#include <openssl/md5.h>
#include <openssl/sha.h>

// Подавляем предупреждения о deprecated функциях OpenSSL 3.0
#define OPENSSL_SUPPRESS_DEPRECATED

Crypto::Crypto()
    : m_rsa(nullptr)
{
}

Crypto::~Crypto()
{
    if (m_rsa) {
        RSA_free(static_cast<RSA*>(m_rsa));
    }
}

bool Crypto::generateRSAKey(int bits)
{
    if (m_rsa) {
        RSA_free(static_cast<RSA*>(m_rsa));
        m_rsa = nullptr;
    }
    
    // Используем новый EVP API для OpenSSL 3.0+
    EVP_PKEY *pkey = nullptr;
    EVP_PKEY_CTX *ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, nullptr);
    
    if (!ctx) {
        m_lastError = "Failed to create EVP context";
        return false;
    }
    
    if (EVP_PKEY_keygen_init(ctx) <= 0) {
        m_lastError = ERR_error_string(ERR_get_error(), nullptr);
        EVP_PKEY_CTX_free(ctx);
        return false;
    }
    
    if (EVP_PKEY_CTX_set_rsa_keygen_bits(ctx, bits) <= 0) {
        m_lastError = ERR_error_string(ERR_get_error(), nullptr);
        EVP_PKEY_CTX_free(ctx);
        return false;
    }
    
    if (EVP_PKEY_keygen(ctx, &pkey) <= 0) {
        m_lastError = ERR_error_string(ERR_get_error(), nullptr);
        EVP_PKEY_CTX_free(ctx);
        return false;
    }
    
    EVP_PKEY_CTX_free(ctx);
    
    // Конвертируем в RSA для совместимости
    RSA *rsa = EVP_PKEY_get1_RSA(pkey);
    EVP_PKEY_free(pkey);
    
    if (!rsa) {
        m_lastError = "Failed to convert to RSA structure";
        return false;
    }
    
    m_rsa = rsa;
    return true;
}

bool Crypto::loadPrivateKey(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        m_lastError = "Cannot open file: " + filePath;
        return false;
    }
    
    QByteArray keyData = file.readAll();
    file.close();
    
    BIO *bio = BIO_new_mem_buf(keyData.constData(), keyData.size());
    RSA *rsa = PEM_read_bio_RSAPrivateKey(bio, nullptr, nullptr, nullptr);
    BIO_free(bio);
    
    if (!rsa) {
        m_lastError = "Failed to parse private key";
        return false;
    }
    
    if (m_rsa) {
        RSA_free(static_cast<RSA*>(m_rsa));
    }
    m_rsa = rsa;
    
    return true;
}

bool Crypto::loadPublicKey(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        m_lastError = "Cannot open file: " + filePath;
        return false;
    }
    
    QByteArray keyData = file.readAll();
    file.close();
    
    BIO *bio = BIO_new_mem_buf(keyData.constData(), keyData.size());
    RSA *rsa = PEM_read_bio_RSA_PUBKEY(bio, nullptr, nullptr, nullptr);
    BIO_free(bio);
    
    if (!rsa) {
        m_lastError = "Failed to parse public key";
        return false;
    }
    
    if (m_rsa) {
        RSA_free(static_cast<RSA*>(m_rsa));
    }
    m_rsa = rsa;
    
    return true;
}

QByteArray Crypto::getPublicKeyPEM() const
{
    if (!m_rsa) return QByteArray();
    
    BIO *bio = BIO_new(BIO_s_mem());
    PEM_write_bio_RSA_PUBKEY(bio, static_cast<RSA*>(m_rsa));
    
    BUF_MEM *buf = nullptr;
    BIO_get_mem_ptr(bio, &buf);
    QByteArray result(buf->data, buf->length);
    
    BIO_free(bio);
    return result;
}

QByteArray Crypto::getPrivateKeyPEM() const
{
    if (!m_rsa) return QByteArray();
    
    BIO *bio = BIO_new(BIO_s_mem());
    PEM_write_bio_RSAPrivateKey(bio, static_cast<RSA*>(m_rsa), nullptr, nullptr, 0, nullptr, nullptr);
    
    BUF_MEM *buf = nullptr;
    BIO_get_mem_ptr(bio, &buf);
    QByteArray result(buf->data, buf->length);
    
    BIO_free(bio);
    return result;
}

QByteArray Crypto::sign(const QByteArray &data)
{
    if (!m_rsa) return QByteArray();
    
    // Вычисляем MD5 хеш
    unsigned char md5Digest[MD5_DIGEST_LENGTH];
    MD5(reinterpret_cast<const unsigned char*>(data.constData()), data.size(), md5Digest);
    
    // Подписываем
    unsigned int sigLen = RSA_size(static_cast<RSA*>(m_rsa));
    QByteArray signature(sigLen, 0);
    
    if (RSA_sign(NID_md5, md5Digest, MD5_DIGEST_LENGTH,
                 reinterpret_cast<unsigned char*>(signature.data()), &sigLen,
                 static_cast<RSA*>(m_rsa)) != 1) {
        m_lastError = ERR_error_string(ERR_get_error(), nullptr);
        return QByteArray();
    }
    
    signature.resize(sigLen);
    return signature;
}

bool Crypto::verify(const QByteArray &data, const QByteArray &signature)
{
    if (!m_rsa) return false;
    
    unsigned char md5Digest[MD5_DIGEST_LENGTH];
    MD5(reinterpret_cast<const unsigned char*>(data.constData()), data.size(), md5Digest);
    
    int result = RSA_verify(NID_md5, md5Digest, MD5_DIGEST_LENGTH,
                           reinterpret_cast<const unsigned char*>(signature.constData()),
                           signature.size(), static_cast<RSA*>(m_rsa));
    
    return result == 1;
}

QByteArray Crypto::md5(const QByteArray &data)
{
    return QCryptographicHash::hash(data, QCryptographicHash::Md5);
}

QString Crypto::md5Hex(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return QString();
    }
    
    QCryptographicHash hash(QCryptographicHash::Md5);
    
    const qint64 chunkSize = 8192;
    while (!file.atEnd()) {
        QByteArray chunk = file.read(chunkSize);
        hash.addData(chunk);
    }
    
    file.close();
    return hash.result().toHex();
}

QByteArray Crypto::sha256(const QByteArray &data)
{
    return QCryptographicHash::hash(data, QCryptographicHash::Sha256);
}
