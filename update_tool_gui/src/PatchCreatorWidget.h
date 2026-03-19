#ifndef PATCHCREATORWIDGET_H
#define PATCHCREATORWIDGET_H

#include <QWidget>
#include <QLineEdit>
#include <QTextEdit>
#include <QProgressBar>
#include <QTreeWidget>
#include <QLabel>
#include <QPushButton>
#include <QFutureWatcher>
#include "core/FileSystem.h"

class PatchCreatorWidget : public QWidget
{
    Q_OBJECT

public:
    explicit PatchCreatorWidget(QWidget *parent = nullptr);

signals:
    void logMessage(const QString &message, const QString &level);
    void statusChanged(const QString &status);
    void progressChanged(int value, int max = 100);

private slots:
    void browseGameDir();
    void browseOutputDir();
    void browsePrivateKey();
    void scanDirectory();
    void createPatch();
    void cancelOperation();
    
    void onScanFinished();
    void onPatchFinished();

private:
    void setupUi();
    void updateFileList();
    
    QLineEdit *m_gameDirEdit;
    QLineEdit *m_outputDirEdit;
    QLineEdit *m_privateKeyEdit;
    QLineEdit *m_versionEdit;
    QLineEdit *m_serverUrlEdit;
    
    QTreeWidget *m_fileList;
    QLabel *m_fileCountLabel;
    QLabel *m_totalSizeLabel;
    
    QProgressBar *m_progressBar;
    QPushButton *m_scanBtn;
    QPushButton *m_createBtn;
    QPushButton *m_cancelBtn;
    
    QList<FileSystem::FileInfo> m_files;
    QFutureWatcher<void> *m_watcher;
    bool m_cancelled;
};

#endif // PATCHCREATORWIDGET_H
