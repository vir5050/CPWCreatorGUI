#ifndef LOGGER_H
#define LOGGER_H

#include <QString>
#include <QFile>
#include <QTextStream>
#include <QMutex>
#include <QDateTime>

class Logger
{
public:
    static Logger& instance();
    
    bool init(const QString &logDir = QString());
    void log(const QString &message, const QString &level = "INFO");
    void setMaxFileSize(qint64 maxSize) { m_maxFileSize = maxSize; }
    void setMaxFiles(int count) { m_maxFiles = count; }
    
    QString logFilePath() const { return m_logFilePath; }
    QString logDir() const { return m_logDir; }
    bool isInitialized() const { return m_initialized; }
    
    static QString levelToString(QtMsgType type);
    
private:
    Logger();
    ~Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    
    void rotateLogs();
    void cleanOldLogs();
    QString generateLogFileName();
    
    QFile m_logFile;
    QTextStream m_stream;
    QString m_logDir;
    QString m_logFilePath;
    QMutex m_mutex;
    qint64 m_maxFileSize;
    int m_maxFiles;
    bool m_initialized;
};

// Утилита для автоматического логирования
class LogMessage
{
public:
    LogMessage(const QString &message, const QString &level = "INFO");
    ~LogMessage();
    
    QString message() const { return m_message; }
    
private:
    QString m_message;
    QString m_level;
};

#endif // LOGGER_H
