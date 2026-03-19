#include "PatchGenerator.h"
#include "Crypto.h"
#include "FileSystem.h"

#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTextStream>
#include <QDateTime>

PatchGenerator::PatchGenerator()
    : m_version("1.0.0")
    , m_serverUrl("http://yourserver.com")
{
}

void PatchGenerator::setVersion(const QString &version)
{
    m_version = version;
}

void PatchGenerator::setServerUrl(const QString &url)
{
    m_serverUrl = url;
}

bool PatchGenerator::loadPrivateKey(const QString &filePath)
{
    if (!QFile::exists(filePath)) {
        m_lastError = "Private key file not found: " + filePath;
        return false;
    }
    
    m_privateKeyPath = filePath;
    return true;
}

bool PatchGenerator::generate(const QString &gameDir, const QString &outputDir,
                               const QList<FileSystem::FileInfo> &files,
                               ProgressCallback callback)
{
    m_files = files;
    
    QDir outDir(outputDir);
    if (!outDir.mkpath(".")) {
        m_lastError = "Cannot create output directory: " + outputDir;
        return false;
    }
    
    // Создаем структуру директорий
    outDir.mkpath("lists");
    outDir.mkpath("compressed");
    outDir.mkpath("newlauncher/files/resources");
    outDir.mkpath("newlauncher/files_compressed");
    
    // Сжимаем файлы
    if (!compressFiles(gameDir, outputDir, callback)) {
        return false;
    }
    
    // Создаем список файлов
    if (!createFileList(outputDir)) {
        return false;
    }
    
    // Создаем JSON индекс
    if (!createJsonIndex(outputDir)) {
        return false;
    }
    
    return true;
}

bool PatchGenerator::createFileList(const QString &outputPath)
{
    QString listPath = outputPath + "/lists/files.md5";
    
    QFile file(listPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        m_lastError = "Cannot create file list: " + listPath;
        return false;
    }
    
    QTextStream stream(&file);
    stream.setEncoding(QStringConverter::Utf8);
    
    // Заголовок
    stream << QString("# %1 %2\n").arg(m_version).arg(m_files.size() + 1);
    
    // Группируем по директориям
    QMap<QString, QList<const FileSystem::FileInfo*>> dirMap;
    for (const auto &file : m_files) {
        QString dir = file.path.section('/', 0, -2);
        if (dir.isEmpty()) dir = "/";
        dirMap[dir].append(&file);
    }
    
    // Записываем файлы
    for (auto it = dirMap.constBegin(); it != dirMap.constEnd(); ++it) {
        bool first = true;
        for (const auto *file : it.value()) {
            QChar mark = file->isModified ? '+' : ' ';
            QString path = first 
                ? it.key() + "/" + file->path.section('/', -1)
                : file->path.section('/', -1);
            
            stream << QString("%1%2 %3\n").arg(mark).arg(file->md5).arg(path);
            first = false;
        }
    }
    
    // Добавляем подпись
    if (!m_privateKeyPath.isEmpty()) {
        Crypto crypto;
        if (crypto.loadPrivateKey(m_privateKeyPath)) {
            file.seek(0);
            QByteArray content = file.readAll();
            QByteArray signature = crypto.sign(content);
            
            if (!signature.isEmpty()) {
                stream << "-----BEGIN ELEMENT SIGNATURE-----\n";
                stream << signature.toBase64() << "\n";
            }
        }
    }
    
    file.close();
    return true;
}

bool PatchGenerator::createJsonIndex(const QString &outputPath)
{
    QJsonObject root;
    
    root["Version"] = m_version;
    root["FileURL"] = m_serverUrl + "/launcher/eso_launcher.exe";
    
    // Папки
    QJsonArray folders;
    folders.append(QJsonObject{{"Name", "update"}});
    folders.append(QJsonObject{{"Name", "resources"}});
    folders.append(QJsonObject{{"Name", "tools"}});
    root["Folders"] = folders;
    
    // Файлы
    QJsonArray files;
    for (const auto &file : m_files) {
        QJsonObject fileObj;
        fileObj["Name"] = file.path;
        fileObj["MD5HASH"] = file.md5;
        files.append(fileObj);
    }
    root["Files"] = files;
    
    // Сохраняем
    QString jsonPath = outputPath + "/config.json";
    QFile jsonFile(jsonPath);
    if (!jsonFile.open(QIODevice::WriteOnly)) {
        m_lastError = "Cannot create JSON index: " + jsonPath;
        return false;
    }
    
    QJsonDocument doc(root);
    jsonFile.write(doc.toJson(QJsonDocument::Indented));
    jsonFile.close();
    
    return true;
}

bool PatchGenerator::compressFiles(const QString &gameDir, const QString &outputDir,
                                    ProgressCallback callback)
{
    QDir gameDirectory(gameDir);
    QString compressedDir = outputDir + "/compressed";
    
    int total = m_files.size();
    int current = 0;
    
    for (const auto &file : m_files) {
        QString inputPath = gameDirectory.filePath(file.path);
        QString outputPath = QString("%1/%2.zip").arg(compressedDir, file.md5);
        
        bool success = FileSystem::compressFile(inputPath, outputPath);
        
        if (callback) {
            callback(file.path, success);
        }
        
        current++;
    }
    
    return true;
}
