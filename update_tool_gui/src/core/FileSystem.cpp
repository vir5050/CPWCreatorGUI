#include "FileSystem.h"
#include "Crypto.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <zlib.h>

QList<FileSystem::FileInfo> FileSystem::scanDirectory(const QString &path, ProgressCallback callback)
{
    QList<FileInfo> files;
    
    QDir dir(path);
    if (!dir.exists()) {
        m_lastError = "Directory does not exist: " + path;
        return files;
    }
    
    // Подсчет общего количества файлов
    int totalFiles = 0;
    QDirIterator countIt(path, QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (countIt.hasNext()) {
        countIt.next();
        totalFiles++;
    }
    
    // Сканирование
    int currentFile = 0;
    QDirIterator it(path, QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    
    while (it.hasNext()) {
        QString filePath = it.next();
        
        FileInfo info;
        info.path = dir.relativeFilePath(filePath);
        info.size = QFileInfo(filePath).size();
        info.md5 = Crypto::md5Hex(filePath);
        info.isModified = true;
        
        files.append(info);
        
        currentFile++;
        if (callback) {
            callback(currentFile, totalFiles, info.path);
        }
    }
    
    return files;
}

bool FileSystem::compressFile(const QString &inputPath, const QString &outputPath)
{
    QFile inputFile(inputPath);
    if (!inputFile.open(QIODevice::ReadOnly)) {
        getLastError() = "Cannot open input file: " + inputPath;
        return false;
    }
    
    QByteArray inputData = inputFile.readAll();
    inputFile.close();
    
    uLongf compressedSize = compressBound(inputData.size());
    QByteArray compressedData(compressedSize, 0);
    
    int result = compress2(
        reinterpret_cast<Bytef*>(compressedData.data()), &compressedSize,
        reinterpret_cast<const Bytef*>(inputData.constData()), inputData.size(),
        Z_BEST_COMPRESSION
    );
    
    if (result != Z_OK) {
        getLastError() = QString("Compression failed with code %1").arg(result);
        return false;
    }
    
    compressedData.resize(compressedSize);
    
    QFile outputFile(outputPath);
    if (!outputFile.open(QIODevice::WriteOnly)) {
        getLastError() = "Cannot create output file: " + outputPath;
        return false;
    }
    
    // Формат: [4 байта размер оригинала][сжатые данные]
    quint32 originalSize = inputData.size();
    outputFile.write(reinterpret_cast<const char*>(&originalSize), 4);
    outputFile.write(compressedData);
    outputFile.close();
    
    return true;
}

bool FileSystem::decompressFile(const QString &inputPath, const QString &outputPath)
{
    QFile inputFile(inputPath);
    if (!inputFile.open(QIODevice::ReadOnly)) {
        getLastError() = "Cannot open input file: " + inputPath;
        return false;
    }
    
    // Читаем размер оригинала
    quint32 originalSize;
    inputFile.read(reinterpret_cast<char*>(&originalSize), 4);
    
    QByteArray compressedData = inputFile.readAll();
    inputFile.close();
    
    QByteArray decompressedData(originalSize, 0);
    uLongf destLen = originalSize;
    
    int result = uncompress(
        reinterpret_cast<Bytef*>(decompressedData.data()), &destLen,
        reinterpret_cast<const Bytef*>(compressedData.constData()), compressedData.size()
    );
    
    if (result != Z_OK) {
        getLastError() = QString("Decompression failed with code %1").arg(result);
        return false;
    }
    
    QFile outputFile(outputPath);
    if (!outputFile.open(QIODevice::WriteOnly)) {
        getLastError() = "Cannot create output file: " + outputPath;
        return false;
    }
    
    outputFile.write(decompressedData);
    outputFile.close();
    
    return true;
}

QString FileSystem::formatSize(qint64 bytes)
{
    const char *units[] = {"B", "KB", "MB", "GB", "TB"};
    int unitIndex = 0;
    double size = bytes;
    
    while (size >= 1024.0 && unitIndex < 4) {
        size /= 1024.0;
        unitIndex++;
    }
    
    if (unitIndex == 0) {
        return QString("%1 %2").arg(bytes).arg(units[unitIndex]);
    }
    
    return QString("%1 %2").arg(size, 0, 'f', 2).arg(units[unitIndex]);
}

bool FileSystem::ensureDirectory(const QString &path)
{
    QDir dir(path);
    if (dir.exists()) {
        return true;
    }
    
    return dir.mkpath(".");
}

bool FileSystem::copyFile(const QString &source, const QString &destination)
{
    // Создаем директорию назначения
    QFileInfo destInfo(destination);
    if (!ensureDirectory(destInfo.absolutePath())) {
        getLastError() = "Cannot create directory: " + destInfo.absolutePath();
        return false;
    }
    
    // Копируем файл
    if (!QFile::copy(source, destination)) {
        getLastError() = QString("Cannot copy %1 to %2").arg(source, destination);
        return false;
    }
    
    return true;
}
