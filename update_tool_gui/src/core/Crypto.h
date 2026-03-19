#ifndef CRYPTO_H
#define CRYPTO_H

#include <QString>
#include <QByteArray>

class Crypto
{
public:
    Crypto();
    ~Crypto();
    
    bool generateRSAKey(int bits = 2048);
    bool loadPrivateKey(const QString &filePath);
    bool loadPublicKey(const QString &filePath);
    
    QByteArray getPublicKeyPEM() const;
    QByteArray getPrivateKeyPEM() const;
    
    QByteArray sign(const QByteArray &data);
    bool verify(const QByteArray &data, const QByteArray &signature);
    
    static QByteArray md5(const QByteArray &data);
    static QString md5Hex(const QString &filePath);
    static QByteArray sha256(const QByteArray &data);
    
    QString lastError() const { return m_lastError; }
    
private:
    void *m_rsa;  // RSA*
    QString m_lastError;
};

#endif // CRYPTO_H
