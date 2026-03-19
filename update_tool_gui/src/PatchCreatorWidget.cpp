#include "PatchCreatorWidget.h"
#include "core/FileSystem.h"
#include "core/PatchGenerator.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QProgressDialog>
#include <QThread>
#include <QtConcurrent>
#include <QHeaderView>
#include <QCoreApplication>

PatchCreatorWidget::PatchCreatorWidget(QWidget *parent)
    : QWidget(parent)
    , m_gameDirEdit(nullptr)
    , m_outputDirEdit(nullptr)
    , m_privateKeyEdit(nullptr)
    , m_versionEdit(nullptr)
    , m_serverUrlEdit(nullptr)
    , m_fileList(nullptr)
    , m_fileCountLabel(nullptr)
    , m_totalSizeLabel(nullptr)
    , m_progressBar(nullptr)
    , m_scanBtn(nullptr)
    , m_createBtn(nullptr)
    , m_cancelBtn(nullptr)
    , m_watcher(nullptr)
    , m_cancelled(false)
{
    setupUi();
    
    m_watcher = new QFutureWatcher<void>();
    connect(m_watcher, &QFutureWatcher<void>::finished, this, &PatchCreatorWidget::onScanFinished);
}

void PatchCreatorWidget::setupUi()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);
    
    // === Группа путей ===
    QGroupBox *pathsGroup = new QGroupBox(tr("Пути"), this);
    QFormLayout *pathsLayout = new QFormLayout(pathsGroup);
    
    // Директория игры
    QHBoxLayout *gameDirLayout = new QHBoxLayout();
    m_gameDirEdit = new QLineEdit(this);
    m_gameDirEdit->setPlaceholderText(tr("Директория с файлами игры"));
    QPushButton *browseGameBtn = new QPushButton(tr("Обзор..."), this);
    gameDirLayout->addWidget(m_gameDirEdit);
    gameDirLayout->addWidget(browseGameBtn);
    pathsLayout->addRow(tr("Директория игры:"), gameDirLayout);
    
    // Директория вывода
    QHBoxLayout *outputDirLayout = new QHBoxLayout();
    m_outputDirEdit = new QLineEdit("./patch_output", this);
    m_outputDirEdit->setPlaceholderText(tr("Директория для сохранения патча"));
    QPushButton *browseOutputBtn = new QPushButton(tr("Обзор..."), this);
    outputDirLayout->addWidget(m_outputDirEdit);
    outputDirLayout->addWidget(browseOutputBtn);
    pathsLayout->addRow(tr("Директория вывода:"), outputDirLayout);
    
    // Приватный ключ
    QHBoxLayout *keyLayout = new QHBoxLayout();
    m_privateKeyEdit = new QLineEdit(this);
    m_privateKeyEdit->setPlaceholderText(tr("Оставьте пустым для создания неподписанного патча"));
    QPushButton *browseKeyBtn = new QPushButton(tr("Обзор..."), this);
    keyLayout->addWidget(m_privateKeyEdit);
    keyLayout->addWidget(browseKeyBtn);
    pathsLayout->addRow(tr("Приватный ключ:"), keyLayout);
    
    mainLayout->addWidget(pathsGroup);
    
    // === Группа настроек ===
    QGroupBox *settingsGroup = new QGroupBox(tr("Настройки патча"), this);
    QFormLayout *settingsLayout = new QFormLayout(settingsGroup);
    
    m_versionEdit = new QLineEdit("1.0.0", this);
    settingsLayout->addRow(tr("Версия:"), m_versionEdit);
    
    m_serverUrlEdit = new QLineEdit("http://yourserver.com", this);
    settingsLayout->addRow(tr("URL сервера:"), m_serverUrlEdit);
    
    mainLayout->addWidget(settingsGroup);
    
    // === Кнопки сканирования и создания ===
    QHBoxLayout *actionLayout = new QHBoxLayout();
    
    m_scanBtn = new QPushButton(tr("Сканировать файлы"), this);
    m_scanBtn->setIcon(QIcon::fromTheme("search"));
    m_scanBtn->setMinimumHeight(35);
    
    m_createBtn = new QPushButton(tr("Создать патч"), this);
    m_createBtn->setIcon(QIcon::fromTheme("document-save"));
    m_createBtn->setMinimumHeight(35);
    m_createBtn->setStyleSheet("font-weight: bold;");
    m_createBtn->setEnabled(false);
    
    m_cancelBtn = new QPushButton(tr("Отмена"), this);
    m_cancelBtn->setEnabled(false);
    
    actionLayout->addWidget(m_scanBtn);
    actionLayout->addWidget(m_createBtn);
    actionLayout->addWidget(m_cancelBtn);
    actionLayout->addStretch();
    
    mainLayout->addLayout(actionLayout);
    
    // === Прогресс ===
    m_progressBar = new QProgressBar(this);
    m_progressBar->setTextVisible(true);
    m_progressBar->setFormat("%p% - %v/%m файлов");
    m_progressBar->hide();
    mainLayout->addWidget(m_progressBar);
    
    // === Статистика ===
    QHBoxLayout *statsLayout = new QHBoxLayout();
    m_fileCountLabel = new QLabel(tr("Файлов: 0"), this);
    m_totalSizeLabel = new QLabel(tr("Размер: 0 B"), this);
    statsLayout->addWidget(m_fileCountLabel);
    statsLayout->addWidget(m_totalSizeLabel);
    statsLayout->addStretch();
    mainLayout->addLayout(statsLayout);
    
    // === Список файлов ===
    QGroupBox *filesGroup = new QGroupBox(tr("Список файлов"), this);
    QVBoxLayout *filesLayout = new QVBoxLayout(filesGroup);
    
    m_fileList = new QTreeWidget(this);
    m_fileList->setHeaderLabels({tr("Файл"), tr("MD5"), tr("Размер"), tr("Статус")});
    m_fileList->setRootIsDecorated(false);
    m_fileList->setSortingEnabled(true);
    m_fileList->setAlternatingRowColors(true);
    m_fileList->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_fileList->header()->setSectionResizeMode(1, QHeaderView::Fixed);
    m_fileList->header()->setSectionResizeMode(2, QHeaderView::Fixed);
    m_fileList->setColumnWidth(1, 250);
    m_fileList->setColumnWidth(2, 100);
    
    filesLayout->addWidget(m_fileList);
    mainLayout->addWidget(filesGroup);
    
    // Подключение сигналов
    connect(browseGameBtn, &QPushButton::clicked, this, &PatchCreatorWidget::browseGameDir);
    connect(browseOutputBtn, &QPushButton::clicked, this, &PatchCreatorWidget::browseOutputDir);
    connect(browseKeyBtn, &QPushButton::clicked, this, &PatchCreatorWidget::browsePrivateKey);
    connect(m_scanBtn, &QPushButton::clicked, this, &PatchCreatorWidget::scanDirectory);
    connect(m_createBtn, &QPushButton::clicked, this, &PatchCreatorWidget::createPatch);
    connect(m_cancelBtn, &QPushButton::clicked, this, &PatchCreatorWidget::cancelOperation);
}

void PatchCreatorWidget::browseGameDir()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("Выберите директорию игры"));
    if (!dir.isEmpty()) {
        m_gameDirEdit->setText(dir);
    }
}

void PatchCreatorWidget::browseOutputDir()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("Выберите директорию вывода"));
    if (!dir.isEmpty()) {
        m_outputDirEdit->setText(dir);
    }
}

void PatchCreatorWidget::browsePrivateKey()
{
    QString file = QFileDialog::getOpenFileName(this, tr("Выберите приватный ключ"),
        QString(), tr("PEM файлы (*.pem *.txt);;Все файлы (*)"));
    if (!file.isEmpty()) {
        m_privateKeyEdit->setText(file);
    }
}

void PatchCreatorWidget::scanDirectory()
{
    QString gameDir = m_gameDirEdit->text();
    
    if (gameDir.isEmpty()) {
        QMessageBox::warning(this, tr("Ошибка"), tr("Укажите директорию игры"));
        return;
    }
    
    if (!QDir(gameDir).exists()) {
        QMessageBox::warning(this, tr("Ошибка"), tr("Директория не существует: %1").arg(gameDir));
        return;
    }
    
    m_files.clear();
    m_fileList->clear();
    m_createBtn->setEnabled(false);
    m_cancelled = false;
    
    m_scanBtn->setEnabled(false);
    m_progressBar->setRange(0, 0); // Неопределенный прогресс
    m_progressBar->show();
    emit statusChanged(tr("Сканирование директории..."));
    emit logMessage(tr("Начато сканирование: %1").arg(gameDir), "info");
    
    // Запуск сканирования в отдельном потоке
    QFuture<void> future = QtConcurrent::run([this, gameDir]() {
        FileSystem fs;
        m_files = fs.scanDirectory(gameDir, [this](int current, int total, const QString &file) {
            if (m_cancelled) return;
            QMetaObject::invokeMethod(this, [this, current, total, file]() {
                emit progressChanged(current, total);
                emit statusChanged(tr("Сканирование: %1").arg(file));
            }, Qt::QueuedConnection);
        });
    });
    
    m_watcher->setFuture(future);
}

void PatchCreatorWidget::onScanFinished()
{
    m_progressBar->hide();
    m_scanBtn->setEnabled(true);
    
    if (m_cancelled) {
        emit statusChanged(tr("Сканирование отменено"));
        emit logMessage(tr("Сканирование отменено пользователем"), "warning");
        return;
    }
    
    // Обновление списка файлов
    updateFileList();
    
    m_createBtn->setEnabled(!m_files.isEmpty());
    
    emit statusChanged(tr("Сканирование завершено"));
    emit logMessage(tr("Найдено файлов: %1").arg(m_files.size()), "success");
    emit progressChanged(-1, 0);
}

void PatchCreatorWidget::updateFileList()
{
    m_fileList->clear();
    
    qint64 totalSize = 0;
    
    for (const auto &file : m_files) {
        QTreeWidgetItem *item = new QTreeWidgetItem(m_fileList);
        item->setText(0, file.path);
        item->setText(1, file.md5);
        item->setText(2, FileSystem::formatSize(file.size));
        item->setText(3, tr("Новый"));
        item->setData(0, Qt::UserRole, file.path);
        
        totalSize += file.size;
    }
    
    m_fileCountLabel->setText(tr("Файлов: %1").arg(m_files.size()));
    m_totalSizeLabel->setText(tr("Размер: %1").arg(FileSystem::formatSize(totalSize)));
}

void PatchCreatorWidget::createPatch()
{
    QString gameDir = m_gameDirEdit->text();
    QString outputDir = m_outputDirEdit->text();
    QString privateKey = m_privateKeyEdit->text();
    QString version = m_versionEdit->text();
    QString serverUrl = m_serverUrlEdit->text();
    
    if (gameDir.isEmpty() || outputDir.isEmpty()) {
        QMessageBox::warning(this, tr("Ошибка"), tr("Заполните все обязательные поля"));
        return;
    }
    
    if (m_files.isEmpty()) {
        QMessageBox::warning(this, tr("Ошибка"), tr("Список файлов пуст. Сначала выполните сканирование."));
        return;
    }
    
    // Преобразуем пути относительно директории приложения
    QDir appDir(QCoreApplication::applicationDirPath());
    QString absOutputDir = appDir.absoluteFilePath(outputDir);
    
    // Создаём выходную директорию
    if (!QDir(absOutputDir).exists()) {
        if (!QDir().mkpath(absOutputDir)) {
            QMessageBox::critical(this, tr("Ошибка"), tr("Не удалось создать директорию: %1").arg(absOutputDir));
            return;
        }
    }
    
    m_cancelled = false;
    m_createBtn->setEnabled(false);
    m_scanBtn->setEnabled(false);
    m_cancelBtn->setEnabled(true);
    
    m_progressBar->setRange(0, m_files.size());
    m_progressBar->setValue(0);
    m_progressBar->show();
    
    emit statusChanged(tr("Создание патча..."));
    emit logMessage(tr("Начато создание патча в: %1").arg(absOutputDir), "info");
    
    // Запуск в отдельном потоке
    QFuture<void> future = QtConcurrent::run([this, gameDir, absOutputDir, privateKey, version, serverUrl]() {
        PatchGenerator generator;
        generator.setVersion(version);
        generator.setServerUrl(serverUrl);
        
        if (!privateKey.isEmpty()) {
            generator.loadPrivateKey(privateKey);
        }
        
        int processed = 0;
        bool success = generator.generate(gameDir, absOutputDir, m_files,
            [this, &processed](const QString &file, bool success) {
                if (m_cancelled) return;
                processed++;
                QMetaObject::invokeMethod(this, [this, processed, file, success]() {
                    emit progressChanged(processed, m_files.size());
                    emit statusChanged(tr("Обработка: %1").arg(file));
                    if (!success) {
                        emit logMessage(tr("Ошибка обработки: %1").arg(file), "error");
                    }
                }, Qt::QueuedConnection);
            });
        
        QMetaObject::invokeMethod(this, [this, success, absOutputDir]() {
            m_progressBar->hide();
            m_createBtn->setEnabled(true);
            m_scanBtn->setEnabled(true);
            m_cancelBtn->setEnabled(false);
            
            if (success && !m_cancelled) {
                emit statusChanged(tr("Патч создан успешно"));
                emit logMessage(tr("Патч успешно создан"), "success");
                emit progressChanged(-1, 0);
                
                QMessageBox::information(this, tr("Успех"),
                    tr("Патч успешно создан!\n\nВыходная директория: %1").arg(absOutputDir));
            } else if (m_cancelled) {
                emit statusChanged(tr("Операция отменена"));
                emit logMessage(tr("Создание патча отменено"), "warning");
            } else {
                emit statusChanged(tr("Ошибка"));
                emit logMessage(tr("Ошибка создания патча"), "error");
            }
        }, Qt::QueuedConnection);
    });
}

void PatchCreatorWidget::cancelOperation()
{
    m_cancelled = true;
    m_cancelBtn->setEnabled(false);
    emit statusChanged(tr("Отмена операции..."));
    emit logMessage(tr("Запрошена отмена операции"), "warning");
}

void PatchCreatorWidget::onPatchFinished()
{
    m_progressBar->hide();
    m_createBtn->setEnabled(true);
    m_scanBtn->setEnabled(true);
    m_cancelBtn->setEnabled(false);
}
