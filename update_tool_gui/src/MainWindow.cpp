#include "MainWindow.h"
#include "KeyGeneratorWidget.h"
#include "PatchCreatorWidget.h"
#include "PckArchiverWidget.h"
#include "LauncherPatcherWidget.h"
#include "SettingsWidget.h"
#include "LogWidget.h"

#include <QMenuBar>
#include <QToolBar>
#include <QAction>
#include <QMessageBox>
#include <QApplication>
#include <QStyle>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_tabWidget(nullptr)
    , m_progressBar(nullptr)
    , m_keyGenerator(nullptr)
    , m_patchCreator(nullptr)
    , m_pckArchiver(nullptr)
    , m_launcherPatcher(nullptr)
    , m_settings(nullptr)
    , m_logWidget(nullptr)
{
    setupUi();
    setupMenuBar();
    setupToolBar();
    setupStatusBar();
    
    logMessage("ESO Update Tool запущен", "info");
}

MainWindow::~MainWindow()
{
}

void MainWindow::setupUi()
{
    m_tabWidget = new QTabWidget(this);
    m_tabWidget->setTabPosition(QTabWidget::North);
    m_tabWidget->setDocumentMode(true);
    m_tabWidget->setMovable(true);
    
    // Создание вкладок
    m_keyGenerator = new KeyGeneratorWidget(this);
    m_patchCreator = new PatchCreatorWidget(this);
    m_pckArchiver = new PckArchiverWidget(this);
    m_launcherPatcher = new LauncherPatcherWidget(this);
    m_settings = new SettingsWidget(this);
    m_logWidget = new LogWidget(this);
    
    // Добавление вкладок
    m_tabWidget->addTab(m_patchCreator, QIcon::fromTheme("document-save"), tr("Создание патча"));
    m_tabWidget->addTab(m_pckArchiver, QIcon::fromTheme("package-x-generic"), tr("PCK Архивы"));
    m_tabWidget->addTab(m_launcherPatcher, QIcon::fromTheme("applications-development"), tr("Патч лаунчера"));
    m_tabWidget->addTab(m_keyGenerator, QIcon::fromTheme("dialog-password"), tr("RSA Ключи"));
    m_tabWidget->addTab(m_settings, QIcon::fromTheme("preferences-system"), tr("Настройки"));
    m_tabWidget->addTab(m_logWidget, QIcon::fromTheme("text-x-generic"), tr("Лог"));
    
    setCentralWidget(m_tabWidget);
    
    // Подключение сигналов
    connect(m_keyGenerator, &KeyGeneratorWidget::logMessage, this, &MainWindow::logMessage);
    connect(m_patchCreator, &PatchCreatorWidget::logMessage, this, &MainWindow::logMessage);
    connect(m_pckArchiver, &PckArchiverWidget::logMessage, this, &MainWindow::logMessage);
    connect(m_launcherPatcher, &LauncherPatcherWidget::logMessage, this, &MainWindow::logMessage);
    connect(m_settings, &SettingsWidget::logMessage, this, &MainWindow::logMessage);
    
    connect(m_keyGenerator, &KeyGeneratorWidget::statusChanged, this, &MainWindow::setStatus);
    connect(m_patchCreator, &PatchCreatorWidget::statusChanged, this, &MainWindow::setStatus);
    connect(m_pckArchiver, &PckArchiverWidget::statusChanged, this, &MainWindow::setStatus);
    connect(m_launcherPatcher, &LauncherPatcherWidget::statusChanged, this, &MainWindow::setStatus);
    
    connect(m_patchCreator, &PatchCreatorWidget::progressChanged, this, &MainWindow::showProgress);
    connect(m_pckArchiver, &PckArchiverWidget::progressChanged, this, &MainWindow::showProgress);
}

void MainWindow::setupMenuBar()
{
    // Меню Файл
    QMenu *fileMenu = menuBar()->addMenu(tr("&Файл"));
    
    QAction *exitAction = fileMenu->addAction(tr("Выход"));
    exitAction->setShortcut(QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, this, &QMainWindow::close);
    
    // Меню Инструменты
    QMenu *toolsMenu = menuBar()->addMenu(tr("&Инструменты"));
    
    QAction *genKeysAction = toolsMenu->addAction(tr("Генерировать ключи"));
    connect(genKeysAction, &QAction::triggered, [this]() {
        m_tabWidget->setCurrentWidget(m_keyGenerator);
    });
    
    QAction *createPatchAction = toolsMenu->addAction(tr("Создать патч"));
    connect(createPatchAction, &QAction::triggered, [this]() {
        m_tabWidget->setCurrentWidget(m_patchCreator);
    });
    
    QAction *patchLauncherAction = toolsMenu->addAction(tr("Патч лаунчера"));
    connect(patchLauncherAction, &QAction::triggered, [this]() {
        m_tabWidget->setCurrentWidget(m_launcherPatcher);
    });
    
    // Меню Справка
    QMenu *helpMenu = menuBar()->addMenu(tr("&Справка"));
    
    QAction *aboutAction = helpMenu->addAction(tr("О программе"));
    connect(aboutAction, &QAction::triggered, [this]() {
        QMessageBox::about(this, tr("О программе"),
            tr("<h3>ESO Update Tool</h3>"
               "<p>Версия 1.0.0</p>"
               "<p>Утилита для создания обновлений игры Ether Saga Odyssey</p>"
               "<p>Функции:</p>"
               "<ul>"
               "<li>Генерация RSA ключей</li>"
               "<li>Создание списков патчей с MD5</li>"
               "<li>Работа с PCK архивами</li>"
               "<li>Патчинг лаунчера ключами</li>"
               "<li>Сжатие файлов</li>"
               "</ul>"
               "<p>© 2024 ESO Tools</p>"));
    });
    
    QAction *aboutQtAction = helpMenu->addAction(tr("О Qt"));
    connect(aboutQtAction, &QAction::triggered, qApp, &QApplication::aboutQt);
}

void MainWindow::setupToolBar()
{
    QToolBar *toolBar = addToolBar(tr("Главная"));
    toolBar->setMovable(false);
    
    QAction *patchAction = toolBar->addAction(QIcon::fromTheme("document-save"), tr("Патч"));
    connect(patchAction, &QAction::triggered, [this]() {
        m_tabWidget->setCurrentWidget(m_patchCreator);
    });
    
    QAction *pckAction = toolBar->addAction(QIcon::fromTheme("package-x-generic"), tr("PCK"));
    connect(pckAction, &QAction::triggered, [this]() {
        m_tabWidget->setCurrentWidget(m_pckArchiver);
    });
    
    QAction *launcherPatchAction = toolBar->addAction(QIcon::fromTheme("applications-development"), tr("Лаунчер"));
    connect(launcherPatchAction, &QAction::triggered, [this]() {
        m_tabWidget->setCurrentWidget(m_launcherPatcher);
    });
    
    toolBar->addSeparator();
    
    QAction *settingsAction = toolBar->addAction(QIcon::fromTheme("preferences-system"), tr("Настройки"));
    connect(settingsAction, &QAction::triggered, [this]() {
        m_tabWidget->setCurrentWidget(m_settings);
    });
}

void MainWindow::setupStatusBar()
{
    m_statusBar = statusBar();
    
    m_progressBar = new QProgressBar(this);
    m_progressBar->setTextVisible(true);
    m_progressBar->setFormat("%p%");
    m_progressBar->setFixedWidth(200);
    m_progressBar->hide();
    
    m_statusBar->addPermanentWidget(m_progressBar);
    m_statusBar->showMessage(tr("Готов"));
}

void MainWindow::logMessage(const QString &message, const QString &level)
{
    m_logWidget->appendLog(message, level);
}

void MainWindow::setStatus(const QString &status)
{
    m_statusBar->showMessage(status);
}

void MainWindow::showProgress(int value, int max)
{
    if (value < 0) {
        m_progressBar->hide();
        return;
    }
    
    m_progressBar->setMaximum(max);
    m_progressBar->setValue(value);
    m_progressBar->show();
}
