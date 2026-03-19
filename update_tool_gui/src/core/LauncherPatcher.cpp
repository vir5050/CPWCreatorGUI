#include "LauncherPatcher.h"
#include <QFile>
#include <QRegularExpression>

// Сигнатуры для поиска
const QByteArray LauncherPatcher::PEM_BEGIN_MARKER = "-----BEGIN PUBLIC KEY-----";
const QByteArray LauncherPatcher::PEM_END_MARKER = "-----END PUBLIC KEY-----";

LauncherPatcher::LauncherPatcher()
    : m_keysFound(0)
    , m_isPE(false)
    , m_isELF(false)
    , m_isMachO(false)
{
}

bool LauncherPatcher::loadExecutable(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        m_lastError = "Cannot open file: " + filePath;
        return false;
    }
    
    m_data = file.readAll();
    file.close();
    
    m_filePath = filePath;
    m_keysFound = 0;
    
    return detectFileType();
}

bool LauncherPatcher::saveExecutable(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        m_lastError = "Cannot create file: " + filePath;
        return false;
    }
    
    file.write(m_data);
    file.close();
    
    return true;
}

bool LauncherPatcher::detectFileType()
{
    if (m_data.size() < 4) {
        m_lastError = "File too small";
        return false;
    }
    
    // Проверяем магические числа
    const unsigned char *ptr = reinterpret_cast<const unsigned char*>(m_data.constData());
    
    // PE (Windows): MZ header
    if (ptr[0] == 'M' && ptr[1] == 'Z') {
        m_isPE = true;
        return true;
    }
    
    // ELF (Linux): 0x7F ELF
    if (ptr[0] == 0x7F && ptr[1] == 'E' && ptr[2] == 'L' && ptr[3] == 'F') {
        m_isELF = true;
        return true;
    }
    
    // Mach-O (macOS): 0xFE ED FA CE or 0xCE FA ED FE
    if ((ptr[0] == 0xFE && ptr[1] == 0xED && ptr[2] == 0xFA && ptr[3] == 0xCE) ||
        (ptr[0] == 0xCE && ptr[1] == 0xFA && ptr[2] == 0xED && ptr[3] == 0xFE)) {
        m_isMachO = true;
        return true;
    }
    
    // Неизвестный формат, но продолжаем работу
    m_lastError = "Unknown file format, proceeding anyway";
    return true;
}

int LauncherPatcher::findPublicKey(const QByteArray &oldKey)
{
    if (m_data.isEmpty()) {
        m_lastError = "No data loaded";
        return -1;
    }
    
    // Ищем ключ в формате PEM
    QByteArray searchKey = oldKey;
    if (!searchKey.contains(PEM_BEGIN_MARKER)) {
        // Если ключ без PEM заголовков, добавляем их
        searchKey = PEM_BEGIN_MARKER + "\n" + searchKey + "\n" + PEM_END_MARKER;
    }
    
    int offset = m_data.indexOf(searchKey);
    if (offset >= 0) {
        m_keysFound = 1;
        return offset;
    }
    
    // Ищем без PEM заголовков (raw key)
    QByteArray rawKey = pemToRaw(oldKey);
    if (!rawKey.isEmpty()) {
        offset = m_data.indexOf(rawKey);
        if (offset >= 0) {
            m_keysFound = 1;
            return offset;
        }
    }
    
    m_lastError = "Public key not found in executable";
    return -1;
}

bool LauncherPatcher::replacePublicKey(int offset, const QByteArray &newKey)
{
    if (m_data.isEmpty()) {
        m_lastError = "No data loaded";
        return false;
    }
    
    if (offset < 0 || offset >= m_data.size()) {
        m_lastError = "Invalid offset";
        return false;
    }
    
    // Форматируем новый ключ в PEM
    QByteArray formattedKey = newKey;
    if (!formattedKey.contains(PEM_BEGIN_MARKER)) {
        formattedKey = PEM_BEGIN_MARKER + "\n" + formattedKey + "\n" + PEM_END_MARKER;
    }
    
    // Проверяем, что новый ключ не больше старого (иначе повредим файл)
    // Ищем конец старого ключа
    int endOffset = m_data.indexOf(PEM_END_MARKER, offset);
    if (endOffset < 0) {
        m_lastError = "Cannot find end of existing key";
        return false;
    }
    
    int oldKeyLength = endOffset + PEM_END_MARKER.length() - offset;
    
    if (formattedKey.length() > oldKeyLength) {
        m_lastError = QString("New key is larger than old key (%1 > %2 bytes)")
            .arg(formattedKey.length()).arg(oldKeyLength);
        return false;
    }
    
    // Дополняем нулями если новый ключ меньше
    while (formattedKey.length() < oldKeyLength) {
        formattedKey.append('\0');
    }
    
    // Заменяем ключ
    m_data.replace(offset, oldKeyLength, formattedKey);
    
    return true;
}

bool LauncherPatcher::findAndReplaceKey(const QByteArray &oldKey, const QByteArray &newKey)
{
    int offset = findPublicKey(oldKey);
    if (offset < 0) {
        return false;
    }
    
    return replacePublicKey(offset, newKey);
}

QList<int> LauncherPatcher::findRSAPublicKeyMarkers(int keySize)
{
    QList<int> offsets;
    
    if (m_data.isEmpty()) {
        return offsets;
    }
    
    // Ищем PEM заголовки
    int pos = 0;
    while ((pos = m_data.indexOf(PEM_BEGIN_MARKER, pos)) != -1) {
        offsets.append(pos);
        pos++;
    }
    
    // Ищем характерные паттерны RSA ключей
    // RSA public key обычно начинается с последовательности байтов
    // для 2048 бит: 0x30 0x82 0x01 0x0A (SEQUENCE длиной ~266 байт)
    QByteArray rsa2048marker = QByteArray::fromHex("3082010A");
    pos = 0;
    while ((pos = m_data.indexOf(rsa2048marker, pos)) != -1) {
        // Проверяем, что это не дубликат
        bool duplicate = false;
        for (int offset : offsets) {
            if (qAbs(offset - pos) < 50) {
                duplicate = true;
                break;
            }
        }
        if (!duplicate) {
            offsets.append(pos);
        }
        pos++;
    }
    
    // Для 1024 бит: 0x30 0x81 0x9F
    QByteArray rsa1024marker = QByteArray::fromHex("30819F");
    pos = 0;
    while ((pos = m_data.indexOf(rsa1024marker, pos)) != -1) {
        bool duplicate = false;
        for (int offset : offsets) {
            if (qAbs(offset - pos) < 50) {
                duplicate = true;
                break;
            }
        }
        if (!duplicate) {
            offsets.append(pos);
        }
        pos++;
    }
    
    m_keysFound = offsets.size();
    return offsets;
}

QByteArray LauncherPatcher::extractPublicKey(int offset, int keySize)
{
    if (m_data.isEmpty() || offset < 0 || offset >= m_data.size()) {
        return QByteArray();
    }
    
    // Ищем PEM заголовок
    if (m_data.mid(offset, PEM_BEGIN_MARKER.size()) == PEM_BEGIN_MARKER) {
        int endOffset = m_data.indexOf(PEM_END_MARKER, offset);
        if (endOffset > offset) {
            return m_data.mid(offset, endOffset + PEM_END_MARKER.size() - offset);
        }
    }
    
    // Пытаемся извлечь raw ключ и конвертировать в PEM
    int keyLength = keySize / 8 * 2; // Приблизительный размер
    if (offset + keyLength <= m_data.size()) {
        return rawToPem(m_data.mid(offset, keyLength));
    }
    
    return QByteArray();
}

QByteArray LauncherPatcher::pemToRaw(const QByteArray &pemKey)
{
    // Удаляем PEM заголовки и конвертируем base64 в raw
    QString key = QString::fromUtf8(pemKey);
    key.remove(PEM_BEGIN_MARKER);
    key.remove(PEM_END_MARKER);
    key.remove('\n');
    key.remove('\r');
    key.remove(' ');
    
    return QByteArray::fromBase64(key.toUtf8());
}

QByteArray LauncherPatcher::rawToPem(const QByteArray &rawKey)
{
    QByteArray base64 = rawKey.toBase64();
    QByteArray result = PEM_BEGIN_MARKER + "\n";
    
    // Форматируем по 64 символа
    for (int i = 0; i < base64.size(); i += 64) {
        result += base64.mid(i, 64) + "\n";
    }
    
    result += PEM_END_MARKER + "\n";
    return result;
}
