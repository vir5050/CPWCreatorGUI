#ifndef LAUNCHERPATCHER_H
#define LAUNCHERPATCHER_H

#include <QString>
#include <QByteArray>

class LauncherPatcher
{
public:
    LauncherPatcher();
    
    // Загрузка исполняемого файла
    bool loadExecutable(const QString &filePath);
    bool saveExecutable(const QString &filePath);
    
    // Поиск и замена публичного ключа
    int findPublicKey(const QByteArray &oldKey);
    bool replacePublicKey(int offset, const QByteArray &newKey);
    
    // Автоматический поиск и замена по сигнатуре
    bool findAndReplaceKey(const QByteArray &oldKey, const QByteArray &newKey);
    
    // Проверка формата файла
    bool isPEFile() const { return m_isPE; }
    bool isELFFile() const { return m_isELF; }
    bool isMachOFile() const { return m_isMachO; }
    
    // Получение информации
    QString lastError() const { return m_lastError; }
    QByteArray data() const { return m_data; }
    
    // Статистика
    qint64 fileSize() const { return m_data.size(); }
    int keysFound() const { return m_keysFound; }
    
    // Поиск по маске (для поиска RSA ключей)
    QList<int> findRSAPublicKeyMarkers(int keySize = 2048);
    
    // Экспорт публичного ключа из файла
    QByteArray extractPublicKey(int offset, int keySize = 2048);
    
private:
    bool detectFileType();
    QByteArray pemToRaw(const QByteArray &pemKey);
    QByteArray rawToPem(const QByteArray &rawKey);
    
    QByteArray m_data;
    QString m_filePath;
    QString m_lastError;
    int m_keysFound;
    
    bool m_isPE;
    bool m_isELF;
    bool m_isMachO;
    
    // Сигнатуры для поиска RSA ключей
    static const QByteArray RSA_PUBKEY_HEADER;
    static const QByteArray PEM_BEGIN_MARKER;
    static const QByteArray PEM_END_MARKER;
};

#endif // LAUNCHERPATCHER_H
