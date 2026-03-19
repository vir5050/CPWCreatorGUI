#include "SettingsWidget.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QSettings>
#include <QFileDialog>
#include <QMessageBox>
#include <QSpinBox>
#include <QLabel>

SettingsWidget::SettingsWidget(QWidget *parent)
    : QWidget(parent)
    , m_defaultGameDir(nullptr)
    , m_defaultOutputDir(nullptr)
    , m_defaultServerUrl(nullptr)
    , m_compressionLevel(nullptr)
    , m_signPatchCheck(nullptr)
    , m_compressFilesCheck(nullptr)
    , m_createJsonCheck(nullptr)
{
    setupUi();
    loadSettings();
}

void SettingsWidget::setupUi()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);
    
    // === Пути по умолчанию ===
    QGroupBox *pathsGroup = new QGroupBox(tr("Пути по умолчанию"), this);
    QFormLayout *pathsLayout = new QFormLayout(pathsGroup);
    
    // Директория игры
    QHBoxLayout *gameDirLayout = new QHBoxLayout();
    m_defaultGameDir = new QLineEdit(this);
    QPushButton *browseGameBtn = new QPushButton(tr("Обзор..."), this);
    gameDirLayout->addWidget(m_defaultGameDir);
    gameDirLayout->addWidget(browseGameBtn);
    pathsLayout->addRow(tr("Директория игры:"), gameDirLayout);
    
    // Директория вывода
    QHBoxLayout *outputDirLayout = new QHBoxLayout();
    m_defaultOutputDir = new QLineEdit(this);
    QPushButton *browseOutputBtn = new QPushButton(tr("Обзор..."), this);
    outputDirLayout->addWidget(m_defaultOutputDir);
    outputDirLayout->addWidget(browseOutputBtn);
    pathsLayout->addRow(tr("Директория вывода:"), outputDirLayout);
    
    mainLayout->addWidget(pathsGroup);
    
    // === Настройки сервера ===
    QGroupBox *serverGroup = new QGroupBox(tr("Настройки сервера"), this);
    QFormLayout *serverLayout = new QFormLayout(serverGroup);
    
    m_defaultServerUrl = new QLineEdit("http://yourserver.com", this);
    serverLayout->addRow(tr("URL сервера:"), m_defaultServerUrl);
    
    mainLayout->addWidget(serverGroup);
    
    // === Настройки сжатия ===
    QGroupBox *compressionGroup = new QGroupBox(tr("Сжатие"), this);
    QFormLayout *compressionLayout = new QFormLayout(compressionGroup);
    
    m_compressionLevel = new QLineEdit("9", this);
    m_compressionLevel->setMaximumWidth(50);
    compressionLayout->addRow(tr("Уровень сжатия (1-9):"), m_compressionLevel);
    
    m_compressFilesCheck = new QCheckBox(tr("Сжимать файлы при создании патча"), this);
    m_compressFilesCheck->setChecked(true);
    compressionLayout->addRow(m_compressFilesCheck);
    
    mainLayout->addWidget(compressionGroup);
    
    // === Настройки патча ===
    QGroupBox *patchGroup = new QGroupBox(tr("Патч"), this);
    QVBoxLayout *patchLayout = new QVBoxLayout(patchGroup);
    
    m_signPatchCheck = new QCheckBox(tr("Подписывать патч RSA"), this);
    m_signPatchCheck->setChecked(true);
    patchLayout->addWidget(m_signPatchCheck);
    
    m_createJsonCheck = new QCheckBox(tr("Создавать config.json"), this);
    m_createJsonCheck->setChecked(true);
    patchLayout->addWidget(m_createJsonCheck);
    
    mainLayout->addWidget(patchGroup);
    
    // === Кнопки ===
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    
    QPushButton *saveBtn = new QPushButton(tr("Сохранить"), this);
    saveBtn->setIcon(QIcon::fromTheme("document-save"));
    
    QPushButton *resetBtn = new QPushButton(tr("Сбросить"), this);
    resetBtn->setIcon(QIcon::fromTheme("edit-undo"));
    
    buttonLayout->addStretch();
    buttonLayout->addWidget(saveBtn);
    buttonLayout->addWidget(resetBtn);
    
    mainLayout->addLayout(buttonLayout);
    
    // Информация
    QLabel *infoLabel = new QLabel(tr(
        "<b>Уровень сжатия:</b><br>"
        "1 - Быстрое сжатие (меньшее сжатие)<br>"
        "6 - Баланс (рекомендуется)<br>"
        "9 - Максимальное сжатие (медленно)"
    ), this);
    infoLabel->setStyleSheet("color: gray; padding: 10px; background: #f5f5f5; border-radius: 5px;");
    mainLayout->addWidget(infoLabel);
    
    mainLayout->addStretch();
    
    // Подключение сигналов
    connect(saveBtn, &QPushButton::clicked, this, &SettingsWidget::saveSettings);
    connect(resetBtn, &QPushButton::clicked, this, &SettingsWidget::resetSettings);
    connect(browseGameBtn, &QPushButton::clicked, this, [this]() {
        QString dir = QFileDialog::getExistingDirectory(this, tr("Выберите директорию"));
        if (!dir.isEmpty()) m_defaultGameDir->setText(dir);
    });
    connect(browseOutputBtn, &QPushButton::clicked, this, [this]() {
        QString dir = QFileDialog::getExistingDirectory(this, tr("Выберите директорию"));
        if (!dir.isEmpty()) m_defaultOutputDir->setText(dir);
    });
}

void SettingsWidget::loadSettings()
{
    QSettings settings("ESO Tools", "ESO Update Tool");
    
    m_defaultGameDir->setText(settings.value("paths/gameDir").toString());
    m_defaultOutputDir->setText(settings.value("paths/outputDir", "./patch_output").toString());
    m_defaultServerUrl->setText(settings.value("server/url", "http://yourserver.com").toString());
    m_compressionLevel->setText(settings.value("compression/level", "9").toString());
    m_compressFilesCheck->setChecked(settings.value("compression/enabled", true).toBool());
    m_signPatchCheck->setChecked(settings.value("patch/sign", true).toBool());
    m_createJsonCheck->setChecked(settings.value("patch/createJson", true).toBool());
}

void SettingsWidget::saveSettings()
{
    QSettings settings("ESO Tools", "ESO Update Tool");
    
    settings.setValue("paths/gameDir", m_defaultGameDir->text());
    settings.setValue("paths/outputDir", m_defaultOutputDir->text());
    settings.setValue("server/url", m_defaultServerUrl->text());
    settings.setValue("compression/level", m_compressionLevel->text());
    settings.setValue("compression/enabled", m_compressFilesCheck->isChecked());
    settings.setValue("patch/sign", m_signPatchCheck->isChecked());
    settings.setValue("patch/createJson", m_createJsonCheck->isChecked());
    
    settings.sync();
    
    emit logMessage(tr("Настройки сохранены"), "success");
    emit settingsChanged();
    
    QMessageBox::information(this, tr("Успех"), tr("Настройки сохранены"));
}

void SettingsWidget::resetSettings()
{
    m_defaultGameDir->clear();
    m_defaultOutputDir->setText("./patch_output");
    m_defaultServerUrl->setText("http://yourserver.com");
    m_compressionLevel->setText("9");
    m_compressFilesCheck->setChecked(true);
    m_signPatchCheck->setChecked(true);
    m_createJsonCheck->setChecked(true);
    
    emit logMessage(tr("Настройки сброшены"), "info");
}

void SettingsWidget::browseToolPath()
{
    QString file = QFileDialog::getOpenFileName(this, tr("Выберите исполняемый файл"));
    // ...
}
