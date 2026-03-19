#include "LogWidget.h"
#include "core/Logger.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDateTime>
#include <QFileDialog>
#include <QMessageBox>
#include <QFile>
#include <QTextStream>
#include <QDesktopServices>
#include <QUrl>
#include <QStringConverter>

LogWidget::LogWidget(QWidget *parent)
    : QWidget(parent)
    , m_logEdit(nullptr)
    , m_clearBtn(nullptr)
    , m_saveBtn(nullptr)
    , m_openDirBtn(nullptr)
    , m_countLabel(nullptr)
    , m_messageCount(0)
{
    setupUi();
    
    // Начальное сообщение
    appendLog("ESO Update Tool готов к работе", "info");
    
    // Информация о файле лога
    if (Logger::instance().isInitialized()) {
        appendLog(QString("Файл лога: %1").arg(Logger::instance().logFilePath()), "info");
    }
}

void LogWidget::logToFile(const QString &message, const QString &level)
{
    Logger::instance().log(message, level);
}

void LogWidget::setupUi()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    // Текстовое поле лога
    m_logEdit = new QTextEdit(this);
    m_logEdit->setReadOnly(true);
    m_logEdit->setFont(QFont("Courier", 9));
    m_logEdit->setLineWrapMode(QTextEdit::NoWrap);
    m_logEdit->setStyleSheet(
        "QTextEdit {"
        "  background-color: #1e1e1e;"
        "  color: #d4d4d4;"
        "  border: 1px solid #3c3c3c;"
        "}"
    );
    
    mainLayout->addWidget(m_logEdit);
    
    // Кнопки
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    
    m_countLabel = new QLabel(tr("Сообщений: 0"), this);
    
    m_clearBtn = new QPushButton(tr("Очистить"), this);
    m_clearBtn->setIcon(QIcon::fromTheme("edit-clear"));
    
    m_saveBtn = new QPushButton(tr("Сохранить..."), this);
    m_saveBtn->setIcon(QIcon::fromTheme("document-save"));
    
    m_openDirBtn = new QPushButton(tr("Открыть папку логов"), this);
    m_openDirBtn->setIcon(QIcon::fromTheme("folder-open"));
    
    buttonLayout->addWidget(m_countLabel);
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_openDirBtn);
    buttonLayout->addWidget(m_saveBtn);
    buttonLayout->addWidget(m_clearBtn);
    
    mainLayout->addLayout(buttonLayout);
    
    // Подключение сигналов
    connect(m_clearBtn, &QPushButton::clicked, this, &LogWidget::clearLog);
    connect(m_saveBtn, &QPushButton::clicked, this, &LogWidget::saveLog);
    connect(m_openDirBtn, &QPushButton::clicked, this, &LogWidget::openLogDir);
}

void LogWidget::appendLog(const QString &message, const QString &level)
{
    m_messageCount++;
    
    QString formatted = formatMessage(message, level);
    m_logEdit->append(formatted);
    
    // Обновляем счетчик
    m_countLabel->setText(tr("Сообщений: %1").arg(m_messageCount));
    
    // Прокрутка вниз
    QTextCursor cursor = m_logEdit->textCursor();
    cursor.movePosition(QTextCursor::End);
    m_logEdit->setTextCursor(cursor);
    
    // Запись в файл лога
    logToFile(message, level);
}

void LogWidget::clearLog()
{
    m_logEdit->clear();
    m_messageCount = 0;
    m_countLabel->setText(tr("Сообщений: 0"));
    appendLog("Лог очищен", "info");
}

void LogWidget::saveLog()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Сохранить лог"),
        QString("eso_update_tool_%1.log")
            .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm-ss")),
        tr("Текстовые файлы (*.log *.txt);;Все файлы (*)"));
    
    if (fileName.isEmpty()) {
        return;
    }
    
    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream stream(&file);
        stream.setEncoding(QStringConverter::Utf8);
        stream << m_logEdit->toPlainText();
        file.close();
        
        QMessageBox::information(this, tr("Успех"), tr("Лог сохранен: %1").arg(fileName));
    } else {
        QMessageBox::warning(this, tr("Ошибка"), tr("Не удалось сохранить файл"));
    }
}
    
void LogWidget::openLogDir()
{
    QString logDir = Logger::instance().logDir();
    
    if (logDir.isEmpty()) {
        QMessageBox::warning(this, tr("Ошибка"), tr("Директория логов не найдена"));
        return;
    }
    
    QDir dir(logDir);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    
    // Открываем директорию в файловом менеджере
    QDesktopServices::openUrl(QUrl::fromLocalFile(logDir));
}
    
QString LogWidget::formatMessage(const QString &message, const QString &level)
{
    QString timestamp = QDateTime::currentDateTime().toString("HH:mm:ss");
    QString color;
    QString prefix;
    
    if (level == "success") {
        color = "#4ec9b0";
        prefix = "[OK]  ";
    } else if (level == "warning") {
        color = "#dcdcaa";
        prefix = "[WARN]";
    } else if (level == "error") {
        color = "#f14c4c";
        prefix = "[ERR] ";
    } else {
        color = "#d4d4d4";
        prefix = "[INFO]";
    }
    
    return QString("<span style=\"color: #808080;\">[%1]</span> "
                   "<span style=\"color: %2;\">%3</span> "
                   "<span style=\"color: #d4d4d4;\">%4</span>")
        .arg(timestamp, color, prefix, message.toHtmlEscaped());
}
