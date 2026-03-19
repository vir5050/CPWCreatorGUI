#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <QString>
#include <QList>
#include <functional>

class FileSystem
{
public:
    struct FileInfo {
        QString path;
        QString md5;
        qint64 size;
        bool isModified;
    };
    
    using ProgressCallback = std::function<void(int, int, const QString&)>;
    
    QList<FileInfo> scanDirectory(const QString &path, ProgressCallback callback = nullptr);
    
    static bool compressFile(const QString &inputPath, const QString &outputPath);
    static bool decompressFile(const QString &inputPath, const QString &outputPath);
    
    static QString formatSize(qint64 bytes);
    static bool ensureDirectory(const QString &path);
    static bool copyFile(const QString &source, const QString &destination);
    
    QString lastError() const { return m_lastError; }
    
    static QString& getLastError() { static QString err; return err; }
    
private:
    QString m_lastError;
};

#endif // FILESYSTEM_H
