#ifndef PATCHGENERATOR_H
#define PATCHGENERATOR_H

#include <QString>
#include <QList>
#include <functional>
#include "FileSystem.h"

class PatchGenerator
{
public:
    using ProgressCallback = std::function<void(const QString&, bool)>;
    
    PatchGenerator();
    
    void setVersion(const QString &version);
    void setServerUrl(const QString &url);
    bool loadPrivateKey(const QString &filePath);
    
    bool generate(const QString &gameDir, const QString &outputDir,
                  const QList<FileSystem::FileInfo> &files,
                  ProgressCallback callback = nullptr);
    
    QString lastError() const { return m_lastError; }
    
private:
    bool createFileList(const QString &outputPath);
    bool createJsonIndex(const QString &outputPath);
    bool compressFiles(const QString &gameDir, const QString &outputDir, ProgressCallback callback);
    
    QString m_version;
    QString m_serverUrl;
    QString m_privateKeyPath;
    QString m_lastError;
    
    QList<FileSystem::FileInfo> m_files;
};

#endif // PATCHGENERATOR_H
