#include "PckArchiver.h"

#include <QFile>
#include <QDir>
#include <QDirIterator>
#include <QDataStream>
#include <QTextCodec>
#include <QDebug>
#include <zlib.h>

// Константы PCK формата
static const quint32 PCK_DATA_START_AT = 12;  // Размер заголовка
static const int MAX_PATH_PCK_256 = 256;       // Путь в V2020
static const int MAX_PATH_PCK_260 = 260;       // Путь в V2030
static const int PCK_ADDITIONAL_INFO_SIZE = 252;  // Размер additional info
static const quint32 PCK_VERSION_202 = 0x00020002;  // Версия 2.0.2
static const quint32 PCK_VERSION_203 = 0x00020003;  // Версия 2.0.3

PckArchiver::PckArchiver()
    : m_key1(0)
    , m_key2(0)
    , m_guardByte0(0xFDFDFEEE)  // id=0 (PW/JD default)
    , m_guardByte1(0xF00DBEEF)
    , m_maskDword(0xA8937462)
    , m_checkMask(0x59374231)
    , m_algorithmId(0)  // Default: PW/JD
{
}

void PckArchiver::setKeys(int key1, int key2, unsigned int guardByte0, unsigned int guardByte1)
{
    m_key1 = key1;
    m_key2 = key2;
    m_guardByte0 = guardByte0;
    m_guardByte1 = guardByte1;
    
    // Вычисляем маски на основе algorithm ID
    m_maskDword = 0xA8937462 + m_algorithmId * 0xAB2321F;
    m_checkMask = 0x59374231 + m_algorithmId * 0x987A223;
}

void PckArchiver::setAlgorithmId(int id)
{
    m_algorithmId = id;
    
    // Вычисляем ключи по формулам из WinPCK
    m_guardByte0 = 0xFDFDFEEE + id * 0x72341F2;
    m_guardByte1 = 0xF00DBEEF + id * 0x1237A73;
    m_maskDword = 0xA8937462 + id * 0xAB2321F;
    m_checkMask = 0x59374231 + id * 0x987A223;
}

bool PckArchiver::pack(const QString &directory, const QString &outputFile, ProgressCallback callback)
{
    QDir dir(directory);
    if (!dir.exists()) {
        m_lastError = "Directory does not exist: " + directory;
        return false;
    }
    
    // Собираем список файлов
    QStringList filePaths;
    QDirIterator it(directory, QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        filePaths.append(it.next());
    }
    
    if (filePaths.isEmpty()) {
        m_lastError = "No files to pack";
        return false;
    }
    
    QFile out(outputFile);
    if (!out.open(QIODevice::WriteOnly)) {
        m_lastError = "Cannot create output file: " + outputFile;
        return false;
    }
    
    QDataStream stream(&out);
    stream.setByteOrder(QDataStream::LittleEndian);
    
    // === ЗАГОЛОВОК PCK (12 байт) ===
    stream << quint32(0x4dca23ef);  // FSIG_1
    stream << quint32(0);           // Размер файла (заполним позже)
    stream << quint32(0x56a089b7);  // FSIG_2
    
    QList<FileEntry> entries;
    int totalFiles = filePaths.size();
    int currentFile = 0;
    
    // === УПАКОВКА ФАЙЛОВ ===
    for (const QString &filePath : filePaths) {
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly)) {
            continue;
        }
        
        FileEntry entry;
        entry.path = dir.relativeFilePath(filePath);
        entry.position = out.pos();
        
        QByteArray data = file.readAll();
        entry.originalSize = data.size();
        
        // Сжатие zlib
        uLongf compressedSize = compressBound(data.size());
        QByteArray compressed(compressedSize, 0);
        
        if (compress2(
            reinterpret_cast<Bytef*>(compressed.data()), &compressedSize,
            reinterpret_cast<const Bytef*>(data.constData()), data.size(),
            Z_BEST_COMPRESSION) == Z_OK)
        {
            compressed.resize(compressedSize);
            entry.compressedSize = compressedSize;
            
            // Используем сжатые данные только если они меньше оригинала
            if (entry.compressedSize < entry.originalSize) {
                out.write(compressed);
            } else {
                out.write(data);
                entry.compressedSize = entry.originalSize;
            }
        } else {
            out.write(data);
            entry.compressedSize = entry.originalSize;
        }
        
        entries.append(entry);
        file.close();
        
        currentFile++;
        if (callback) {
            callback(currentFile, totalFiles, entry.path);
        }
    }
    
    quint64 tablePosition = out.pos();
    
    // === ТАБЛИЦА ФАЙЛОВ ===
    // Кодек для конвертации путей в GB2312/CP936
    QTextCodec *gb2312 = QTextCodec::codecForName("GB18030");
    if (!gb2312) gb2312 = QTextCodec::codecForName("GB2312");
    if (!gb2312) gb2312 = QTextCodec::codecForName("GBK");
    
    for (const FileEntry &entry : entries) {
        // Формируем структуру PCKFILEINDEX_V2020 (276 байт)
        // szFilename[256] + dwUnknown1[4] + dwAddressOffset[4] + dwFileClearTextSize[4] + dwFileCipherTextSize[4] + dwUnknown2[4]
        QByteArray indexData;
        indexData.resize(276);
        memset(indexData.data(), 0, 276);
        
        // Путь файла (256 байт) в кодировке GB2312/CP936
        QByteArray pathData;
        if (gb2312) {
            pathData = gb2312->fromUnicode(entry.path);
        } else {
            pathData = entry.path.toLocal8Bit();
        }
        memcpy(indexData.data(), pathData.constData(), qMin(pathData.size(), 256));
        
        // Записываем поля в правильном порядке
        QDataStream indexStream(&indexData, QIODevice::WriteOnly);
        indexStream.setByteOrder(QDataStream::LittleEndian);
        indexStream.skipRawData(256);  // Пропускаем путь (256 байт для V2020)
        
        indexStream << quint32(0);                    // dwUnknown1
        indexStream << quint32(entry.position);       // dwAddressOffset
        indexStream << quint32(entry.originalSize);   // dwFileClearTextSize
        indexStream << quint32(entry.compressedSize); // dwFileCipherTextSize
        indexStream << quint32(0);                    // dwUnknown2
        
        // Сжимаем индекс
        uLongf compressedIndexSize = compressBound(indexData.size());
        QByteArray compressedIndex(compressedIndexSize, 0);
        
        int result = compress2(
            reinterpret_cast<Bytef*>(compressedIndex.data()), &compressedIndexSize,
            reinterpret_cast<const Bytef*>(indexData.constData()), indexData.size(),
            Z_BEST_COMPRESSION
        );
        
        if (result == Z_OK) {
            compressedIndex.resize(compressedIndexSize);
        } else {
            compressedIndex = indexData;
            compressedIndexSize = indexData.size();
        }
        
        // Записываем размер сжатого индекса (XOR с ключами)
        stream << quint32(compressedIndexSize ^ m_maskDword);
        stream << quint32(compressedIndexSize ^ (m_checkMask ^ m_maskDword));
        
        // Записываем сжатый индекс
        out.write(compressedIndex);
    }
    
    // === FOOTER (280 байт) ===
    // PCKTAIL_V2020 структура:
    // dwIndexTableCheckHead (4) - GuardByte0
    // dwVersion0 (4) - 0x00020002
    // dwEntryOffset (4) - позиция таблицы XOR MaskDword
    // dwFlags (4) - 0
    // szAdditionalInfo (252) - строка
    // dwIndexTableCheckTail (4) - GuardByte1
    // dwFileCount (4)
    // dwVersion (4) - 0x00020002
    
    // dwIndexTableCheckHead
    stream << quint32(m_guardByte0);
    
    // dwVersion0
    stream << quint32(PCK_VERSION_202);
    
    // dwEntryOffset - позиция таблицы XOR MaskDword
    stream << quint32(tablePosition ^ m_maskDword);
    
    // dwFlags
    stream << quint32(0);
    
    // szAdditionalInfo (252 байта)
    QString additionalInfoStr = QString("Angelica File Package, Perfect World Co. Ltd. 2002~2008. All Rights Reserved.\r\n"
                                        "Created by ESO Update Tool v1.0.0");
    QByteArray additionalInfo = additionalInfoStr.toLocal8Bit();
    additionalInfo = additionalInfo.leftJustified(PCK_ADDITIONAL_INFO_SIZE, '\0', true);
    out.write(additionalInfo);
    
    // dwIndexTableCheckTail
    stream << quint32(m_guardByte1);
    
    // dwFileCount
    stream << quint32(entries.size());
    
    // dwVersion
    stream << quint32(PCK_VERSION_202);
    
    // Обновляем размер файла в заголовке
    quint32 fileSize = out.pos();
    out.seek(4);
    stream << fileSize;
    
    out.close();
    
    // Отладочный вывод
    qDebug() << "PCK created successfully:";
    qDebug() << "  File size:" << fileSize << "bytes";
    qDebug() << "  Files count:" << entries.size();
    qDebug() << "  Table position:" << tablePosition;
    qDebug() << "  Algorithm ID:" << m_algorithmId;
    qDebug() << "  GuardByte0: 0x" << QString::number(m_guardByte0, 16);
    qDebug() << "  GuardByte1: 0x" << QString::number(m_guardByte1, 16);
    qDebug() << "  MaskDword: 0x" << QString::number(m_maskDword, 16);
    
    return true;
}

bool PckArchiver::unpack(const QString &pckFile, const QString &outputDir, ProgressCallback callback)
{
    QFile file(pckFile);
    if (!file.open(QIODevice::ReadOnly)) {
        m_lastError = "Cannot open PCK file: " + pckFile;
        return false;
    }
    
    QDataStream stream(&file);
    stream.setByteOrder(QDataStream::LittleEndian);
    
    // === ЧИТАЕМ ЗАГОЛОВОК (12 байт) ===
    quint32 sig1, fileSize, sig2;
    stream >> sig1 >> fileSize >> sig2;
    
    if (sig1 != 0x4dca23ef || sig2 != 0x56a089b7) {
        m_lastError = "Invalid PCK file signature";
        file.close();
        return false;
    }
    
    // === ЧИТАЕМ FOOTER (с конца файла) ===
    // Структура PCKTAIL_V2020 (280 байт):
    // dwIndexTableCheckHead (4)
    // dwVersion0 (4)
    // dwEntryOffset (4)
    // dwFlags (4)
    // szAdditionalInfo (252)
    // dwIndexTableCheckTail (4)
    // dwFileCount (4)
    // dwVersion (4)
    
    // Читаем количество файлов и версию (последние 8 байт)
    file.seek(file.size() - 8);
    quint32 fileCount, version;
    stream >> fileCount >> version;
    
    // Читаем позицию таблицы (смещение 8 от начала футера)
    file.seek(file.size() - 280 + 8);
    quint32 tablePosEncrypted;
    stream >> tablePosEncrypted;
    
    quint32 tablePos = tablePosEncrypted ^ m_maskDword;
    
    file.seek(tablePos);
    
    QDir outDir(outputDir);
    outDir.mkpath(".");
    
    int currentFile = 0;
    
    for (quint32 i = 0; i < fileCount; i++) {
        // Читаем размер сжатого индекса (XOR с ключами)
        quint32 sizeXor1, sizeXor2;
        stream >> sizeXor1 >> sizeXor2;
        
        quint32 indexSize = sizeXor1 ^ m_maskDword;
        
        // Читаем сжатый индекс
        QByteArray compressedIndex = file.read(indexSize);
        
        // Распаковываем индекс
        QByteArray indexData(276, 0);
        uLongf destLen = 276;
        
        int result = uncompress(
            reinterpret_cast<Bytef*>(indexData.data()), &destLen,
            reinterpret_cast<const Bytef*>(compressedIndex.constData()), indexSize
        );
        
        if (result != Z_OK) {
            // Пробуем без распаковки
            indexData = compressedIndex;
        }
        
        // Парсим структуру PCKFILEINDEX_V2020 (276 байт)
        // szFilename[256] + dwUnknown1[4] + dwAddressOffset[4] + dwFileClearTextSize[4] + dwFileCipherTextSize[4] + dwUnknown2[4]
        if (indexData.size() < 276) {
            continue;
        }
        
        // Извлекаем путь (первые 256 байт для V2020)
        QTextCodec *gb2312 = QTextCodec::codecForName("GB18030");
        if (!gb2312) gb2312 = QTextCodec::codecForName("GB2312");
        if (!gb2312) gb2312 = QTextCodec::codecForName("GBK");
        
        QByteArray pathData = indexData.mid(0, 256);
        QString path;
        if (gb2312) {
            path = gb2312->toUnicode(pathData);
        } else {
            path = QString::fromLocal8Bit(pathData);
        }
        path = path.trimmed();
        path.remove('\0');
        
        // Читаем остальные поля (смещение 256)
        QDataStream indexStream(indexData.mid(256));
        indexStream.setByteOrder(QDataStream::LittleEndian);
        
        quint32 unknown1, position, originalSize, compressedSize, unknown2;
        indexStream >> unknown1 >> position >> originalSize >> compressedSize >> unknown2;
        
        if (position == 0 || originalSize == 0) {
            continue;
        }
        
        // Создаём путь для файла
        QString outPath = outDir.filePath(path);
        QDir().mkpath(QFileInfo(outPath).absolutePath());
        
        // Извлекаем файл
        qint64 currentPos = file.pos();
        file.seek(position);
        
        QByteArray data = file.read(compressedSize);
        
        if (compressedSize < originalSize) {
            // Распаковка zlib
            QByteArray decompressed(originalSize, 0);
            uLongf destLen = originalSize;
            
            int result = uncompress(
                reinterpret_cast<Bytef*>(decompressed.data()), &destLen,
                reinterpret_cast<const Bytef*>(data.constData()), compressedSize
            );
            
            if (result == Z_OK) {
                QFile outFile(outPath);
                if (outFile.open(QIODevice::WriteOnly)) {
                    outFile.write(decompressed);
                    outFile.close();
                }
            } else {
                // Ошибка распаковки, записываем как есть
                QFile outFile(outPath);
                if (outFile.open(QIODevice::WriteOnly)) {
                    outFile.write(data);
                    outFile.close();
                }
            }
        } else {
            // Несжатые данные
            QFile outFile(outPath);
            if (outFile.open(QIODevice::WriteOnly)) {
                outFile.write(data);
                outFile.close();
            }
        }
        
        file.seek(currentPos);
        
        currentFile++;
        if (callback) {
            callback(currentFile, fileCount, path);
        }
    }
    
    file.close();
    return true;
}
