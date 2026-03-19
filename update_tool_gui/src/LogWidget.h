#ifndef LOGWIDGET_H
#define LOGWIDGET_H

#include <QWidget>
#include <QTextEdit>
#include <QPushButton>
#include <QLabel>

class LogWidget : public QWidget
{
    Q_OBJECT

public:
    explicit LogWidget(QWidget *parent = nullptr);

    static void logToFile(const QString &message, const QString &level = "info");

public slots:
    void appendLog(const QString &message, const QString &level = "info");
    void clearLog();
    void saveLog();
    void openLogDir();

private:
    void setupUi();
    QString formatMessage(const QString &message, const QString &level);
    
    QTextEdit *m_logEdit;
    QPushButton *m_clearBtn;
    QPushButton *m_saveBtn;
    QPushButton *m_openDirBtn;
    QLabel *m_countLabel;
    int m_messageCount;
};

#endif // LOGWIDGET_H
