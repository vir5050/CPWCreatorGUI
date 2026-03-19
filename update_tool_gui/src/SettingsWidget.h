#ifndef SETTINGSWIDGET_H
#define SETTINGSWIDGET_H

#include <QWidget>
#include <QLineEdit>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>

class SettingsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SettingsWidget(QWidget *parent = nullptr);

signals:
    void logMessage(const QString &message, const QString &level);
    void settingsChanged();

private slots:
    void saveSettings();
    void resetSettings();
    void browseToolPath();

private:
    void setupUi();
    void loadSettings();
    
    QLineEdit *m_defaultGameDir;
    QLineEdit *m_defaultOutputDir;
    QLineEdit *m_defaultServerUrl;
    QLineEdit *m_compressionLevel;
    QCheckBox *m_signPatchCheck;
    QCheckBox *m_compressFilesCheck;
    QCheckBox *m_createJsonCheck;
};

#endif // SETTINGSWIDGET_H
