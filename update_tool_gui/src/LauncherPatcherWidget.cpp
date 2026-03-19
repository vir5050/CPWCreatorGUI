#include "LauncherPatcherWidget.h"
#include "core/LauncherPatcher.h"
#include "core/Logger.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QFileInfo>
#include <QDir>
#include <QDateTime>
#include <QApplication>
#include <QClipboard>
#include <QListWidgetItem>

LauncherPatcherWidget::LauncherPatcherWidget(QWidget *parent)
    : QWidget(parent)
    , m_launcherEdit(nullptr)
    , m_oldKeyEdit(nullptr)
    , m_newKeyEdit(nullptr)
    , m_extractedKeyEdit(nullptr)
    , m_keyListWidget(nullptr)
    , m_fileInfoLabel(nullptr)
    , m_statusLabel(nullptr)
    , m_progressBar(nullptr)
    , m_loadBtn(nullptr)
    , m_scanBtn(nullptr)
    , m_extractBtn(nullptr)
    , m_patchBtn(nullptr)
    , m_backupBtn(nullptr)
{
    setupUi();
}

void LauncherPatcherWidget::setupUi()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);
    
    // === Группа выбора файлов ===
    QGroupBox *filesGroup = new QGroupBox(tr("Файлы"), this);
    QFormLayout *filesLayout = new QFormLayout(filesGroup);
    
    // Лаунчер
    QHBoxLayout *launcherLayout = new QHBoxLayout();
    m_launcherEdit = new QLineEdit(this);
    m_launcherEdit->setPlaceholderText(tr("Исполняемый файл лаунчера (.exe)"));
    QPushButton *browseLauncherBtn = new QPushButton(tr("Обзор..."), this);
    launcherLayout->addWidget(m_launcherEdit);
    launcherLayout->addWidget(browseLauncherBtn);
    filesLayout->addRow(tr("Лаунчер:"), launcherLayout);
    
    // Старый ключ
    QHBoxLayout *oldKeyLayout = new QHBoxLayout();
    m_oldKeyEdit = new QLineEdit(this);
    m_oldKeyEdit->setPlaceholderText(tr("Текущий публичный ключ (PEM)"));
    QPushButton *browseOldKeyBtn = new QPushButton(tr("Обзор..."), this);
    oldKeyLayout->addWidget(m_oldKeyEdit);
    oldKeyLayout->addWidget(browseOldKeyBtn);
    filesLayout->addRow(tr("Старый ключ:"), oldKeyLayout);
    
    // Новый ключ
    QHBoxLayout *newKeyLayout = new QHBoxLayout();
    m_newKeyEdit = new QLineEdit(this);
    m_newKeyEdit->setPlaceholderText(tr("Новый публичный ключ (PEM)"));
    QPushButton *browseNewKeyBtn = new QPushButton(tr("Обзор..."), this);
    newKeyLayout->addWidget(m_newKeyEdit);
    newKeyLayout->addWidget(browseNewKeyBtn);
    filesLayout->addRow(tr("Новый ключ:"), newKeyLayout);
    
    mainLayout->addWidget(filesGroup);
    
    // === Информация о файле ===
    QGroupBox *infoGroup = new QGroupBox(tr("Информация"), this);
    QVBoxLayout *infoLayout = new QVBoxLayout(infoGroup);
    
    m_fileInfoLabel = new QLabel(tr("Файл не загружен"), this);
    m_fileInfoLabel->setWordWrap(true);
    m_fileInfoLabel->setStyleSheet("color: gray;");
    infoLayout->addWidget(m_fileInfoLabel);
    
    m_statusLabel = new QLabel(this);
    m_statusLabel->setStyleSheet("font-weight: bold;");
    infoLayout->addWidget(m_statusLabel);
    
    m_progressBar = new QProgressBar(this);
    m_progressBar->hide();
    infoLayout->addWidget(m_progressBar);
    
    mainLayout->addWidget(infoGroup);
    
    // === Кнопки действий ===
    QHBoxLayout *actionLayout = new QHBoxLayout();
    
    m_loadBtn = new QPushButton(tr("Загрузить"), this);
    m_loadBtn->setIcon(QIcon::fromTheme("document-open"));
    
    m_scanBtn = new QPushButton(tr("Сканировать"), this);
    m_scanBtn->setIcon(QIcon::fromTheme("search"));
    m_scanBtn->setEnabled(false);
    
    m_patchBtn = new QPushButton(tr("Патчить"), this);
    m_patchBtn->setIcon(QIcon::fromTheme("document-save"));
    m_patchBtn->setEnabled(false);
    m_patchBtn->setStyleSheet("font-weight: bold;");
    
    m_backupBtn = new QPushButton(tr("Резервная копия"), this);
    m_backupBtn->setIcon(QIcon::fromTheme("document-save-as"));
    m_backupBtn->setEnabled(false);
    
    actionLayout->addWidget(m_loadBtn);
    actionLayout->addWidget(m_scanBtn);
    actionLayout->addWidget(m_patchBtn);
    actionLayout->addWidget(m_backupBtn);
    actionLayout->addStretch();
    
    mainLayout->addLayout(actionLayout);
    
    // === Найденные ключи ===
    QGroupBox *keysGroup = new QGroupBox(tr("Найденные ключи"), this);
    QVBoxLayout *keysLayout = new QVBoxLayout(keysGroup);
    
    m_keyListWidget = new QListWidget(this);
    m_keyListWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    keysLayout->addWidget(m_keyListWidget);
    
    m_extractBtn = new QPushButton(tr("Извлечь выбранный ключ"), this);
    m_extractBtn->setEnabled(false);
    keysLayout->addWidget(m_extractBtn);
    
    mainLayout->addWidget(keysGroup);
    
    // === Извлечённый ключ ===
    QGroupBox *extractedGroup = new QGroupBox(tr("Извлечённый ключ"), this);
    QVBoxLayout *extractedLayout = new QVBoxLayout(extractedGroup);
    
    m_extractedKeyEdit = new QTextEdit(this);
    m_extractedKeyEdit->setReadOnly(true);
    m_extractedKeyEdit->setFont(QFont("Courier", 9));
    m_extractedKeyEdit->setMaximumHeight(150);
    extractedLayout->addWidget(m_extractedKeyEdit);
    
    QHBoxLayout *extractButtons = new QHBoxLayout();
    QPushButton *copyExtractedBtn = new QPushButton(tr("Копировать"), this);
    QPushButton *useAsOldKeyBtn = new QPushButton(tr("Использовать как старый"), this);
    extractButtons->addWidget(copyExtractedBtn);
    extractButtons->addWidget(useAsOldKeyBtn);
    extractButtons->addStretch();
    extractedLayout->addLayout(extractButtons);
    
    mainLayout->addWidget(extractedGroup);
    
    mainLayout->addStretch();
    
    // === Подключение сигналов ===
    connect(browseLauncherBtn, &QPushButton::clicked, this, &LauncherPatcherWidget::browseLauncher);
    connect(browseOldKeyBtn, &QPushButton::clicked, this, &LauncherPatcherWidget::browseOldKey);
    connect(browseNewKeyBtn, &QPushButton::clicked, this, &LauncherPatcherWidget::browseNewKey);
    connect(m_loadBtn, &QPushButton::clicked, this, &LauncherPatcherWidget::loadLauncher);
    connect(m_scanBtn, &QPushButton::clicked, this, &LauncherPatcherWidget::scanForKeys);
    connect(m_extractBtn, &QPushButton::clicked, this, &LauncherPatcherWidget::extractSelectedKey);
    connect(m_patchBtn, &QPushButton::clicked, this, &LauncherPatcherWidget::patchLauncher);
    connect(m_backupBtn, &QPushButton::clicked, this, &LauncherPatcherWidget::createBackup);
    
    connect(copyExtractedBtn, &QPushButton::clicked, this, [this]() {
        QApplication::clipboard()->setText(m_extractedKeyEdit->toPlainText());
        emit statusChanged(tr("Ключ скопирован"));
    });
    
    connect(useAsOldKeyBtn, &QPushButton::clicked, this, [this]() {
        m_oldKeyEdit->setText(m_extractedKeyEdit->toPlainText());
        emit statusChanged(tr("Ключ установлен как старый"));
    });
    
    connect(m_keyListWidget, &QListWidget::itemSelectionChanged, this, [this]() {
        m_extractBtn->setEnabled(m_keyListWidget->currentRow() >= 0);
    });
}

void LauncherPatcherWidget::browseLauncher()
{
    QString file = QFileDialog::getOpenFileName(this, tr("Выберите лаунчер"),
        QString(), tr("Исполняемые файлы (*.exe);;Все файлы (*)"));
    if (!file.isEmpty()) {
        m_launcherEdit->setText(file);
    }
}

void LauncherPatcherWidget::browseOldKey()
{
    QString file = QFileDialog::getOpenFileName(this, tr("Выберите старый ключ"),
        QString(), tr("PEM файлы (*.pem *.txt);;Все файлы (*)"));
    if (!file.isEmpty()) {
        QFile keyFile(file);
        if (keyFile.open(QIODevice::ReadOnly)) {
            m_oldKeyEdit->setText(QString::fromUtf8(keyFile.readAll()));
            keyFile.close();
        }
    }
}

void LauncherPatcherWidget::browseNewKey()
{
    QString file = QFileDialog::getOpenFileName(this, tr("Выберите новый ключ"),
        QString(), tr("PEM файлы (*.pem *.txt);;Все файлы (*)"));
    if (!file.isEmpty()) {
        QFile keyFile(file);
        if (keyFile.open(QIODevice::ReadOnly)) {
            m_newKeyEdit->setText(QString::fromUtf8(keyFile.readAll()));
            keyFile.close();
        }
    }
}

void LauncherPatcherWidget::loadLauncher()
{
    m_launcherPath = m_launcherEdit->text();
    
    if (m_launcherPath.isEmpty()) {
        QMessageBox::warning(this, tr("Ошибка"), tr("Укажите путь к лаунчеру"));
        return;
    }
    
    QFile file(m_launcherPath);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, tr("Ошибка"), tr("Не удалось открыть файл: %1").arg(m_launcherPath));
        return;
    }
    
    m_launcherData = file.readAll();
    file.close();
    
    QFileInfo info(m_launcherPath);
    QString sizeStr = QString("%1 MB").arg(m_launcherData.size() / (1024.0 * 1024.0), 0, 'f', 2);
    
    QString typeStr;
    LauncherPatcher patcher;
    patcher.loadExecutable(m_launcherPath);
    
    if (patcher.isPEFile()) {
        typeStr = "Windows PE";
    } else if (patcher.isELFFile()) {
        typeStr = "Linux ELF";
    } else if (patcher.isMachOFile()) {
        typeStr = "macOS Mach-O";
    } else {
        typeStr = tr("Неизвестный формат");
    }
    
    m_fileInfoLabel->setText(QString("%1\nРазмер: %2\nТип: %3")
        .arg(info.fileName(), sizeStr, typeStr));
    m_fileInfoLabel->setStyleSheet("color: green;");
    
    m_scanBtn->setEnabled(true);
    m_backupBtn->setEnabled(true);
    
    emit logMessage(tr("Загружен файл: %1 (%2)").arg(info.fileName(), sizeStr), "success");
    emit statusChanged(tr("Файл загружен"));
}

void LauncherPatcherWidget::scanForKeys()
{
    m_keyListWidget->clear();
    m_foundKeyOffsets.clear();
    
    LauncherPatcher patcher;
    if (!patcher.loadExecutable(m_launcherPath)) {
        QMessageBox::warning(this, tr("Ошибка"), patcher.lastError());
        return;
    }
    
    emit statusChanged(tr("Сканирование..."));
    emit logMessage(tr("Поиск RSA ключей в файле"), "info");
    
    // Ищем PEM ключи
    QList<int> offsets = patcher.findRSAPublicKeyMarkers();
    
    if (offsets.isEmpty()) {
        m_statusLabel->setText(tr("Ключи не найдены"));
        m_statusLabel->setStyleSheet("color: orange; font-weight: bold;");
        emit logMessage(tr("RSA ключи не найдены в файле"), "warning");
    } else {
        updateKeyList(offsets);
        m_statusLabel->setText(tr("Найдено ключей: %1").arg(offsets.size()));
        m_statusLabel->setStyleSheet("color: green; font-weight: bold;");
        emit logMessage(tr("Найдено %1 ключей").arg(offsets.size()), "success");
        
        m_patchBtn->setEnabled(true);
    }
    
    emit statusChanged(tr("Сканирование завершено"));
}

void LauncherPatcherWidget::updateKeyList(const QList<int> &offsets)
{
    m_foundKeyOffsets = offsets;
    
    LauncherPatcher patcher;
    patcher.loadExecutable(m_launcherPath);
    
    for (int i = 0; i < offsets.size(); ++i) {
        QString itemText = tr("Ключ #%1 (смещение: 0x%2)")
            .arg(i + 1)
            .arg(offsets[i], 8, 16, QChar('0'));
        
        QListWidgetItem *item = new QListWidgetItem(itemText, m_keyListWidget);
        item->setData(Qt::UserRole, offsets[i]);
    }
}

void LauncherPatcherWidget::extractSelectedKey()
{
    int row = m_keyListWidget->currentRow();
    if (row < 0 || row >= m_foundKeyOffsets.size()) {
        return;
    }
    
    int offset = m_foundKeyOffsets[row];
    
    LauncherPatcher patcher;
    patcher.loadExecutable(m_launcherPath);
    
    QByteArray key = patcher.extractPublicKey(offset);
    
    if (!key.isEmpty()) {
        m_extractedKeyEdit->setText(QString::fromUtf8(key));
        emit logMessage(tr("Ключ извлечён из смещения 0x%1").arg(offset, 0, 16), "success");
        emit statusChanged(tr("Ключ извлечён"));
    } else {
        emit logMessage(tr("Не удалось извлечь ключ"), "error");
    }
}

void LauncherPatcherWidget::patchLauncher()
{
    QString oldKey = m_oldKeyEdit->text();
    QString newKey = m_newKeyEdit->text();
    
    if (oldKey.isEmpty()) {
        QMessageBox::warning(this, tr("Ошибка"), tr("Укажите старый ключ"));
        return;
    }
    
    if (newKey.isEmpty()) {
        QMessageBox::warning(this, tr("Ошибка"), tr("Укажите новый ключ"));
        return;
    }
    
    // Подтверждение
    QMessageBox::StandardButton reply = QMessageBox::question(this, tr("Подтверждение"),
        tr("Вы уверены, что хотите пропатчить лаунчер?\n\n"
           "Рекомендуется создать резервную копию перед патчингом."),
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply != QMessageBox::Yes) {
        return;
    }
    
    LauncherPatcher patcher;
    if (!patcher.loadExecutable(m_launcherPath)) {
        QMessageBox::warning(this, tr("Ошибка"), patcher.lastError());
        return;
    }
    
    emit statusChanged(tr("Патчинг..."));
    emit logMessage(tr("Начат патчинг лаунчера"), "info");
    
    if (patcher.findAndReplaceKey(oldKey.toUtf8(), newKey.toUtf8())) {
        // Сохраняем
        if (patcher.saveExecutable(m_launcherPath)) {
            emit logMessage(tr("Лаунчер успешно пропатчен!"), "success");
            emit statusChanged(tr("Патчинг завершён"));
            
            QMessageBox::information(this, tr("Успех"), 
                tr("Лаунчер успешно пропатчен!\n\n"
                   "Старый ключ заменён на новый."));
        } else {
            QMessageBox::warning(this, tr("Ошибка"), patcher.lastError());
            emit logMessage(tr("Ошибка сохранения: %1").arg(patcher.lastError()), "error");
        }
    } else {
        QMessageBox::warning(this, tr("Ошибка"), patcher.lastError());
        emit logMessage(tr("Ошибка патчинга: %1").arg(patcher.lastError()), "error");
    }
}

void LauncherPatcherWidget::createBackup()
{
    if (m_launcherPath.isEmpty()) {
        QMessageBox::warning(this, tr("Ошибка"), tr("Сначала загрузите лаунчер"));
        return;
    }
    
    QString backupPath = m_launcherPath + ".backup";
    
    // Добавляем метку времени
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
    backupPath = m_launcherPath + "." + timestamp + ".bak";
    
    if (QFile::copy(m_launcherPath, backupPath)) {
        emit logMessage(tr("Резервная копия создана: %1").arg(backupPath), "success");
        emit statusChanged(tr("Резервная копия создана"));
        
        QMessageBox::information(this, tr("Успех"), 
            tr("Резервная копия создана:\n%1").arg(backupPath));
    } else {
        emit logMessage(tr("Ошибка создания резервной копии"), "error");
        QMessageBox::warning(this, tr("Ошибка"), tr("Не удалось создать резервную копию"));
    }
}
