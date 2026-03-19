#include "Logger.h"
#include <QDir>
#include <QCoreApplication>
#include <QFileInfo>
#include <QStringConverter>

Logger& Logger::instance()
{
    static Logger logger;
    return logger;
}

Logger::Logger()
    : m_maxFileSize(10 * 1024 * 1024)  // 10 MB по умолчанию
    , m_maxFiles(10)                    // 10 файлов логов
    , m_initialized(false)
{
}

Logger::~Logger()
{
    if (m_logFile.isOpen()) {
        m_stream.flush();
        m_logFile.close();
    }
}

bool Logger::init(const QString &logDir)
{
    QMutexLocker locker(&m_mutex);
    
    // Определяем директорию логов
    if (logDir.isEmpty()) {
        // По умолчанию: <app_dir>/logs
        m_logDir = QCoreApplication::applicationDirPath() + "/logs";
    } else {
        // Преобразуем в абсолютный путь относительно директории приложения
        QDir appDir(QCoreApplication::applicationDirPath());
        m_logDir = appDir.absoluteFilePath(logDir);
    }
    
    // Создаем директорию если не существует
    if (!QDir(m_logDir).exists()) {
        if (!QDir().mkpath(m_logDir)) {
            return false;
        }
    }
    
    // Генерируем имя файла
    m_logFilePath = generateLogFileName();
    
    // Открываем файл
    m_logFile.setFileName(m_logFilePath);
    if (!m_logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        return false;
    }
    
    m_stream.setDevice(&m_logFile);
    m_stream.setEncoding(QStringConverter::Utf8);
    m_initialized = true;
    
    // Записываем заголовок
    m_stream << QString("=").repeated(80) << "\n";
    m_stream << QString("ESO Update Tool GUI - Запуск: %1\n")
        .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"));
    m_stream << QString("=").repeated(80) << "\n";
    m_stream.flush();
    
    // Очищаем старые логи
    cleanOldLogs();
    
    return true;
}

void Logger::log(const QString &message, const QString &level)
{
    if (!m_initialized) {
        return;
    }
    
    QMutexLocker locker(&m_mutex);
    
    // Проверяем размер файла
    if (m_logFile.size() > m_maxFileSize) {
        rotateLogs();
    }
    
    // Форматируем сообщение
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz");
    QString formatted = QString("[%1] [%2] %3\n")
        .arg(timestamp, level.toUpper().leftJustified(7, ' '), message);
    
    m_stream << formatted;
    m_stream.flush();
}

void Logger::rotateLogs()
{
    // Закрываем текущий файл
    m_stream.flush();
    m_logFile.close();
    
    // Переименовываем текущий файл
    QString baseName = QFileInfo(m_logFilePath).baseName();
    QString extension = QFileInfo(m_logFilePath).suffix();
    QString archiveName = QString("%1/%2_%3.%4")
        .arg(m_logDir, baseName, QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss"), extension);
    
    QFile::rename(m_logFilePath, archiveName);
    
    // Создаем новый файл
    m_logFile.setFileName(m_logFilePath);
    if (m_logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        m_stream.setDevice(&m_logFile);
        m_stream.setEncoding(QStringConverter::Utf8);
    }
}

void Logger::cleanOldLogs()
{
    QDir dir(m_logDir);
    QStringList filters;
    filters << "eso_update_tool_*.log";
    
    QStringList files = dir.entryList(filters, QDir::Files, QDir::Time | QDir::Reversed);
    
    // Удаляем старые файлы если их больше лимита
    while (files.size() > m_maxFiles) {
        QString oldFile = files.takeFirst();
        dir.remove(oldFile);
    }
}

QString Logger::generateLogFileName()
{
    return QString("%1/eso_update_tool_%2.log")
        .arg(m_logDir, QDateTime::currentDateTime().toString("yyyy-MM-dd"));
}

QString Logger::levelToString(QtMsgType type)
{
    switch (type) {
        case QtDebugMsg:    return "DEBUG";
        case QtInfoMsg:     return "INFO";
        case QtWarningMsg:  return "WARNING";
        case QtCriticalMsg: return "ERROR";
        case QtFatalMsg:    return "FATAL";
        default:            return "UNKNOWN";
    }
}

// LogMessage implementation
LogMessage::LogMessage(const QString &message, const QString &level)
    : m_message(message)
    , m_level(level)
{
    Logger::instance().log(message, level);
}

LogMessage::~LogMessage()
{
}
