#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTabWidget>
#include <QStatusBar>
#include <QProgressBar>
#include <QLabel>

class KeyGeneratorWidget;
class PatchCreatorWidget;
class PckArchiverWidget;
class LauncherPatcherWidget;
class SettingsWidget;
class LogWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void logMessage(const QString &message, const QString &level = "info");
    void setStatus(const QString &status);
    void showProgress(int value, int max = 100);

private:
    void setupUi();
    void setupMenuBar();
    void setupToolBar();
    void setupStatusBar();

    QTabWidget *m_tabWidget;
    QStatusBar *m_statusBar;
    QProgressBar *m_progressBar;

    KeyGeneratorWidget *m_keyGenerator;
    PatchCreatorWidget *m_patchCreator;
    PckArchiverWidget *m_pckArchiver;
    LauncherPatcherWidget *m_launcherPatcher;
    SettingsWidget *m_settings;
    LogWidget *m_logWidget;
};

#endif // MAINWINDOW_H
