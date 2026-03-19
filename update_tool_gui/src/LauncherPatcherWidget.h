#ifndef LAUNCHERPATCHERWIDGET_H
#define LAUNCHERPATCHERWIDGET_H

#include <QWidget>
#include <QLineEdit>
#include <QTextEdit>
#include <QPushButton>
#include <QLabel>
#include <QListWidget>
#include <QProgressBar>

class LauncherPatcherWidget : public QWidget
{
    Q_OBJECT

public:
    explicit LauncherPatcherWidget(QWidget *parent = nullptr);

signals:
    void logMessage(const QString &message, const QString &level);
    void statusChanged(const QString &status);
    void progressChanged(int value, int max = 100);

private slots:
    void browseLauncher();
    void browseOldKey();
    void browseNewKey();
    void loadLauncher();
    void scanForKeys();
    void extractSelectedKey();
    void patchLauncher();
    void createBackup();

private:
    void setupUi();
    void updateKeyList(const QList<int> &offsets);
    
    QLineEdit *m_launcherEdit;
    QLineEdit *m_oldKeyEdit;
    QLineEdit *m_newKeyEdit;
    
    QTextEdit *m_extractedKeyEdit;
    QListWidget *m_keyListWidget;
    
    QLabel *m_fileInfoLabel;
    QLabel *m_statusLabel;
    QProgressBar *m_progressBar;
    
    QPushButton *m_loadBtn;
    QPushButton *m_scanBtn;
    QPushButton *m_extractBtn;
    QPushButton *m_patchBtn;
    QPushButton *m_backupBtn;
    
    QByteArray m_launcherData;
    QString m_launcherPath;
    QList<int> m_foundKeyOffsets;
};

#endif // LAUNCHERPATCHERWIDGET_H
