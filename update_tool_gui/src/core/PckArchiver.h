#ifndef PCKARCHIVER_H
#define PCKARCHIVER_H

#include <QString>
#include <QByteArray>
#include <functional>

class PckArchiver
{
public:
    using ProgressCallback = std::function<void(int, int, const QString&)>;
    
    PckArchiver();
    
    // Установка ключей вручную (для совместимости)
    void setKeys(int key1, int key2, unsigned int guardByte0, unsigned int guardByte1);
    
    // Установка ID алгоритма (автоматически вычисляет ключи)
    // 0 = Jade Dynasty / Perfect World
    // 111 = Hot Dance Party
    // 121 = Ether Saga Odyssey (ESO)
    // 131 = Forsaken World (FW)
    // 161 = Saint Seiya / Swordsman Online
    void setAlgorithmId(int id);
    
    bool pack(const QString &directory, const QString &outputFile, ProgressCallback callback = nullptr);
    bool unpack(const QString &pckFile, const QString &outputDir, ProgressCallback callback = nullptr);
    
    QString lastError() const { return m_lastError; }
    
private:
    struct FileEntry {
        QString path;
        quint64 position;
        quint32 originalSize;
        quint32 compressedSize;
    };
    
    int m_key1;
    int m_key2;
    quint32 m_guardByte0;
    quint32 m_guardByte1;
    quint32 m_maskDword;
    quint32 m_checkMask;
    int m_algorithmId;
    QString m_lastError;
};

#endif // PCKARCHIVER_H
