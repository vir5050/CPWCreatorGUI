#ifndef PCKARCHIVERWIDGET_H
#define PCKARCHIVERWIDGET_H

#include <QWidget>
#include <QLineEdit>
#include <QTextEdit>
#include <QProgressBar>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>

class PckArchiverWidget : public QWidget
{
    Q_OBJECT

public:
    explicit PckArchiverWidget(QWidget *parent = nullptr);

signals:
    void logMessage(const QString &message, const QString &level);
    void statusChanged(const QString &status);
    void progressChanged(int value, int max = 100);

private slots:
    void browseInputFile();
    void browseOutputFile();
    void browseInputDir();
    void browseOutputDir();
    void packDirectory();
    void unpackArchive();
    void updateGameKeys(int index);

private:
    void setupUi();
    void setKeyValues(int algorithmId, int maskDword, unsigned int guardByte0, unsigned int guardByte1);
    
    // Упаковка
    QLineEdit *m_packInputDir;
    QLineEdit *m_packOutputFile;
    QProgressBar *m_packProgress;
    
    // Распаковка
    QLineEdit *m_unpackInputFile;
    QLineEdit *m_unpackOutputDir;
    QProgressBar *m_unpackProgress;
    
    // Настройки игры
    QComboBox *m_gameSelector;
    QLineEdit *m_key1Edit;
    QLineEdit *m_key2Edit;
    QLineEdit *m_asig1Edit;
    QLineEdit *m_asig2Edit;
};

#endif // PCKARCHIVERWIDGET_H
