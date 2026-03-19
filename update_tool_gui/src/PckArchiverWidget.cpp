#include "PckArchiverWidget.h"
#include "core/PckArchiver.h"
#include "core/FileSystem.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QtConcurrent>
#include <QPushButton>
#include <QLabel>
#include <QSpinBox>
#include <QCoreApplication>

PckArchiverWidget::PckArchiverWidget(QWidget *parent)
    : QWidget(parent)
    , m_packInputDir(nullptr)
    , m_packOutputFile(nullptr)
    , m_packProgress(nullptr)
    , m_unpackInputFile(nullptr)
    , m_unpackOutputDir(nullptr)
    , m_unpackProgress(nullptr)
    , m_gameSelector(nullptr)
    , m_key1Edit(nullptr)
    , m_key2Edit(nullptr)
    , m_asig1Edit(nullptr)
    , m_asig2Edit(nullptr)
{
    setupUi();
}

void PckArchiverWidget::setupUi()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);
    
    // === Выбор игры ===
    QGroupBox *gameGroup = new QGroupBox(tr("Игра"), this);
    QFormLayout *gameLayout = new QFormLayout(gameGroup);
    
    m_gameSelector = new QComboBox(this);
    m_gameSelector->addItem(tr("Ether Saga Odyssey (ESO)"), "eso");
    m_gameSelector->addItem(tr("Forsaken World (FW)"), "fw");
    m_gameSelector->addItem(tr("Perfect World (PW)"), "pw");
    m_gameSelector->addItem(tr("Jade Dynasty (JD)"), "jd");
    m_gameSelector->addItem(tr("Пользовательские ключи"), "custom");
    gameLayout->addRow(tr("Игра:"), m_gameSelector);
    
    // Ключи
    QHBoxLayout *keysLayout = new QHBoxLayout();
    
    QFormLayout *algoLayout = new QFormLayout();
    m_key1Edit = new QLineEdit("0", this);
    m_key1Edit->setFont(QFont("Courier", 9));
    m_key1Edit->setToolTip(tr("ID алгоритма:\n0 = Jade Dynasty / Perfect World (по умолчанию)\n111 = Hot Dance Party\n121 = Ether Saga Odyssey\n131 = Forsaken World\n161 = Saint Seiya / Swordsman Online"));
    algoLayout->addRow(tr("Algorithm ID:"), m_key1Edit);
    
    QFormLayout *maskLayout = new QFormLayout();
    m_key2Edit = new QLineEdit("0xA8937462", this);
    m_key2Edit->setFont(QFont("Courier", 9));
    m_key2Edit->setToolTip(tr("MaskDword (вычисляется автоматически)"));
    maskLayout->addRow(tr("MaskDword:"), m_key2Edit);
    
    QFormLayout *guard0Layout = new QFormLayout();
    m_asig1Edit = new QLineEdit("0xFDFDFEEE", this);
    m_asig1Edit->setFont(QFont("Courier", 9));
    m_asig1Edit->setToolTip(tr("GuardByte0 (вычисляется автоматически)"));
    guard0Layout->addRow(tr("GuardByte0:"), m_asig1Edit);
    
    QFormLayout *guard1Layout = new QFormLayout();
    m_asig2Edit = new QLineEdit("0xF00DBEEF", this);
    m_asig2Edit->setFont(QFont("Courier", 9));
    m_asig2Edit->setToolTip(tr("GuardByte1 (вычисляется автоматически)"));
    guard1Layout->addRow(tr("GuardByte1:"), m_asig2Edit);
    
    keysLayout->addLayout(algoLayout);
    keysLayout->addLayout(maskLayout);
    keysLayout->addLayout(guard0Layout);
    keysLayout->addLayout(guard1Layout);
    
    gameLayout->addRow(tr("Ключи:"), keysLayout);
    
    // Включение/выключение полей ключей
    bool isCustom = (m_gameSelector->currentData().toString() == "custom");
    m_key1Edit->setEnabled(isCustom);
    m_key2Edit->setEnabled(isCustom);
    m_asig1Edit->setEnabled(isCustom);
    m_asig2Edit->setEnabled(isCustom);
    
    mainLayout->addWidget(gameGroup);
    
    // === Упаковка ===
    QGroupBox *packGroup = new QGroupBox(tr("Упаковка в PCK"), this);
    QVBoxLayout *packLayout = new QVBoxLayout(packGroup);
    
    // Входная директория
    QHBoxLayout *packInputLayout = new QHBoxLayout();
    packInputLayout->addWidget(new QLabel(tr("Директория:")));
    m_packInputDir = new QLineEdit(this);
    m_packInputDir->setPlaceholderText(tr("Директория с файлами для упаковки"));
    packInputLayout->addWidget(m_packInputDir);
    QPushButton *browsePackInput = new QPushButton(tr("Обзор..."), this);
    packInputLayout->addWidget(browsePackInput);
    packLayout->addLayout(packInputLayout);
    
    // Выходной файл
    QHBoxLayout *packOutputLayout = new QHBoxLayout();
    packOutputLayout->addWidget(new QLabel(tr("Выходной файл:")));
    m_packOutputFile = new QLineEdit(this);
    m_packOutputFile->setPlaceholderText(tr("Имя выходного PCK файла"));
    packOutputLayout->addWidget(m_packOutputFile);
    QPushButton *browsePackOutput = new QPushButton(tr("Обзор..."), this);
    packOutputLayout->addWidget(browsePackOutput);
    packLayout->addLayout(packOutputLayout);
    
    // Прогресс и кнопка
    QHBoxLayout *packActionLayout = new QHBoxLayout();
    m_packProgress = new QProgressBar(this);
    m_packProgress->hide();
    QPushButton *packBtn = new QPushButton(tr("Упаковать"), this);
    packBtn->setIcon(QIcon::fromTheme("package-x-generic"));
    packActionLayout->addWidget(m_packProgress, 1);
    packActionLayout->addWidget(packBtn);
    packLayout->addLayout(packActionLayout);
    
    mainLayout->addWidget(packGroup);
    
    // === Распаковка ===
    QGroupBox *unpackGroup = new QGroupBox(tr("Распаковка PCK"), this);
    QVBoxLayout *unpackLayout = new QVBoxLayout(unpackGroup);
    
    // Входной файл
    QHBoxLayout *unpackInputLayout = new QHBoxLayout();
    unpackInputLayout->addWidget(new QLabel(tr("PCK файл:")));
    m_unpackInputFile = new QLineEdit(this);
    m_unpackInputFile->setPlaceholderText(tr("Файл PCK для распаковки"));
    unpackInputLayout->addWidget(m_unpackInputFile);
    QPushButton *browseUnpackInput = new QPushButton(tr("Обзор..."), this);
    unpackInputLayout->addWidget(browseUnpackInput);
    unpackLayout->addLayout(unpackInputLayout);
    
    // Выходная директория
    QHBoxLayout *unpackOutputLayout = new QHBoxLayout();
    unpackOutputLayout->addWidget(new QLabel(tr("Директория:")));
    m_unpackOutputDir = new QLineEdit(this);
    m_unpackOutputDir->setPlaceholderText(tr("Директория для распаковки"));
    unpackOutputLayout->addWidget(m_unpackOutputDir);
    QPushButton *browseUnpackOutput = new QPushButton(tr("Обзор..."), this);
    unpackOutputLayout->addWidget(browseUnpackOutput);
    unpackLayout->addLayout(unpackOutputLayout);
    
    // Прогресс и кнопка
    QHBoxLayout *unpackActionLayout = new QHBoxLayout();
    m_unpackProgress = new QProgressBar(this);
    m_unpackProgress->hide();
    QPushButton *unpackBtn = new QPushButton(tr("Распаковать"), this);
    unpackBtn->setIcon(QIcon::fromTheme("extract-archive"));
    unpackActionLayout->addWidget(m_unpackProgress, 1);
    unpackActionLayout->addWidget(unpackBtn);
    unpackLayout->addLayout(unpackActionLayout);
    
    mainLayout->addWidget(unpackGroup);
    
    // Информация
    QLabel *infoLabel = new QLabel(tr(
        "<b>Поддерживаемые игры:</b><br>"
        "• ESO - Ether Saga Odyssey<br>"
        "• FW - Forsaken World<br>"
        "• PW - Perfect World / Jade Dynasty<br>"
        "<br>"
        "<b>Формат PCK:</b> Angelica File Package"
    ), this);
    infoLabel->setStyleSheet("color: gray; padding: 10px; background: #f5f5f5; border-radius: 5px;");
    mainLayout->addWidget(infoLabel);
    
    mainLayout->addStretch();
    
    // Подключение сигналов
    connect(m_gameSelector, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &PckArchiverWidget::updateGameKeys);
    connect(browsePackInput, &QPushButton::clicked, this, &PckArchiverWidget::browseInputDir);
    connect(browsePackOutput, &QPushButton::clicked, this, &PckArchiverWidget::browseOutputFile);
    connect(browseUnpackInput, &QPushButton::clicked, this, &PckArchiverWidget::browseInputFile);
    connect(browseUnpackOutput, &QPushButton::clicked, this, &PckArchiverWidget::browseOutputDir);
    
    connect(packBtn, &QPushButton::clicked, this, &PckArchiverWidget::packDirectory);
    connect(unpackBtn, &QPushButton::clicked, this, &PckArchiverWidget::unpackArchive);
}

void PckArchiverWidget::updateGameKeys(int index)
{
    QString game = m_gameSelector->itemData(index).toString();
    bool isCustom = (game == "custom");
    
    m_key1Edit->setEnabled(isCustom);
    m_key2Edit->setEnabled(isCustom);
    m_asig1Edit->setEnabled(isCustom);
    m_asig2Edit->setEnabled(isCustom);
    
    if (!isCustom) {
        // Устанавливаем ключи для выбранной игры
        // Ключи вычисляются по формулам из WinPCK
        // GuardByte0 = 0xFDFDFEEE + id * 0x72341F2
        // GuardByte1 = 0xF00DBEEF + id * 0x1237A73
        // MaskDword  = 0xA8937462 + id * 0xAB2321F
        // CheckMask  = 0x59374231 + id * 0x987A223
        
        if (game == "eso") {
            // ESO - id=121
            setKeyValues(121, 0x29FDE0C1, 0x541C3C66, 0xFE4A1B2A);
        } else if (game == "fw") {
            // FW - id=131
            setKeyValues(131, 0x384E02E0, 0x6B5A7D88, 0x091A8D7F);
        } else if (game == "pw" || game == "jd") {
            // PW/JD - id=0 (по умолчанию)
            setKeyValues(0, 0xA8937462, 0xFDFDFEEE, 0xF00DBEEF);
        }
    }
}

void PckArchiverWidget::setKeyValues(int algorithmId, int maskDword, unsigned int guardByte0, unsigned int guardByte1)
{
    m_key1Edit->setText(QString::number(algorithmId));
    m_key2Edit->setText(QString("0x%1").arg(maskDword, 8, 16, QChar('0')).toUpper());
    m_asig1Edit->setText(QString("0x%1").arg(guardByte0, 8, 16, QChar('0')).toUpper());
    m_asig2Edit->setText(QString("0x%1").arg(guardByte1, 8, 16, QChar('0')).toUpper());
}

void PckArchiverWidget::browseInputFile()
{
    QString file = QFileDialog::getOpenFileName(this, tr("Выберите PCK файл"),
        QString(), tr("PCK файлы (*.pck *.pkx);;Все файлы (*)"));
    if (!file.isEmpty()) {
        m_unpackInputFile->setText(file);
        
        // Автоматически устанавливаем выходную директорию
        if (m_unpackOutputDir->text().isEmpty()) {
            QFileInfo fi(file);
            m_unpackOutputDir->setText(fi.absolutePath() + "/" + fi.completeBaseName() + ".files");
        }
    }
}

void PckArchiverWidget::browseOutputFile()
{
    QString file = QFileDialog::getSaveFileName(this, tr("Сохранить как"),
        QString(), tr("PCK файлы (*.pck)"));
    if (!file.isEmpty()) {
        m_packOutputFile->setText(file);
    }
}

void PckArchiverWidget::browseInputDir()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("Выберите директорию"));
    if (!dir.isEmpty()) {
        m_packInputDir->setText(dir);
        
        // Автоматически устанавливаем выходной файл
        if (m_packOutputFile->text().isEmpty()) {
            m_packOutputFile->setText(dir + ".pck");
        }
    }
}

void PckArchiverWidget::browseOutputDir()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("Выберите директорию"));
    if (!dir.isEmpty()) {
        m_unpackOutputDir->setText(dir);
    }
}

void PckArchiverWidget::packDirectory()
{
    QString inputDir = m_packInputDir->text();
    QString outputFile = m_packOutputFile->text();
    
    if (inputDir.isEmpty() || outputFile.isEmpty()) {
        QMessageBox::warning(this, tr("Ошибка"), tr("Укажите входную директорию и выходной файл"));
        return;
    }
    
    // Преобразуем пути относительно директории приложения
    QDir appDir(QCoreApplication::applicationDirPath());
    QString absInputDir = appDir.absoluteFilePath(inputDir);
    QString absOutputFile = appDir.absoluteFilePath(outputFile);
    
    if (!QDir(absInputDir).exists()) {
        QMessageBox::warning(this, tr("Ошибка"), tr("Директория не существует: %1").arg(absInputDir));
        return;
    }
    
    emit statusChanged(tr("Упаковка PCK..."));
    emit logMessage(tr("Начата упаковка: %1 -> %2").arg(absInputDir, absOutputFile), "info");
    
    m_packProgress->setRange(0, 0);
    m_packProgress->show();
    
    // Получаем ID алгоритма или пользовательские ключи
    QString game = m_gameSelector->currentData().toString();
    bool isCustom = (game == "custom");
    
    (void)QtConcurrent::run([this, absInputDir, absOutputFile, game, isCustom]() {
        PckArchiver archiver;
        
        if (isCustom) {
            // Пользовательские ключи
            bool ok;
            int algorithmId = m_key1Edit->text().toInt(&ok, 0);
            archiver.setAlgorithmId(algorithmId);
        } else {
            // Предустановленные игры
            if (game == "eso") {
                archiver.setAlgorithmId(121);
            } else if (game == "fw") {
                archiver.setAlgorithmId(131);
            } else if (game == "pw" || game == "jd") {
                archiver.setAlgorithmId(0);
            }
        }
        
        bool success = archiver.pack(absInputDir, absOutputFile);
        
        QMetaObject::invokeMethod(this, [this, success, absOutputFile]() {
            m_packProgress->hide();
            
            if (success) {
                emit statusChanged(tr("Упаковка завершена"));
                emit logMessage(tr("PCK архив создан: %1").arg(absOutputFile), "success");
                emit progressChanged(-1, 0);
                
                QMessageBox::information(this, tr("Успех"), tr("Архив успешно создан:\n%1").arg(absOutputFile));
            } else {
                emit statusChanged(tr("Ошибка"));
                emit logMessage(tr("Ошибка упаковки"), "error");
                QMessageBox::warning(this, tr("Ошибка"), tr("Не удалось создать архив"));
            }
        });
    });
}

void PckArchiverWidget::unpackArchive()
{
    QString inputFile = m_unpackInputFile->text();
    QString outputDir = m_unpackOutputDir->text();
    
    if (inputFile.isEmpty() || outputDir.isEmpty()) {
        QMessageBox::warning(this, tr("Ошибка"), tr("Укажите входной файл и выходную директорию"));
        return;
    }
    
    // Преобразуем пути относительно директории приложения
    QDir appDir(QCoreApplication::applicationDirPath());
    QString absInputFile = appDir.absoluteFilePath(inputFile);
    QString absOutputDir = appDir.absoluteFilePath(outputDir);
    
    if (!QFile::exists(absInputFile)) {
        QMessageBox::warning(this, tr("Ошибка"), tr("Файл не существует: %1").arg(absInputFile));
        return;
    }
    
    // Создаём выходную директорию
    if (!QDir(absOutputDir).exists()) {
        QDir().mkpath(absOutputDir);
    }
    
    emit statusChanged(tr("Распаковка PCK..."));
    emit logMessage(tr("Начата распаковка: %1 -> %2").arg(absInputFile, absOutputDir), "info");
    
    m_unpackProgress->setRange(0, 0);
    m_unpackProgress->show();
    
    // Получаем ID алгоритма или пользовательские ключи
    QString game = m_gameSelector->currentData().toString();
    bool isCustom = (game == "custom");
    
    (void)QtConcurrent::run([this, absInputFile, absOutputDir, game, isCustom]() {
        PckArchiver archiver;
        
        if (isCustom) {
            // Пользовательские ключи
            bool ok;
            int algorithmId = m_key1Edit->text().toInt(&ok, 0);
            archiver.setAlgorithmId(algorithmId);
        } else {
            // Предустановленные игры
            if (game == "eso") {
                archiver.setAlgorithmId(121);
            } else if (game == "fw") {
                archiver.setAlgorithmId(131);
            } else if (game == "pw" || game == "jd") {
                archiver.setAlgorithmId(0);
            }
        }
        
        bool success = archiver.unpack(absInputFile, absOutputDir);
        
        QMetaObject::invokeMethod(this, [this, success, absOutputDir]() {
            m_unpackProgress->hide();
            
            if (success) {
                emit statusChanged(tr("Распаковка завершена"));
                emit logMessage(tr("Архив распакован в: %1").arg(absOutputDir), "success");
                emit progressChanged(-1, 0);
                
                QMessageBox::information(this, tr("Успех"), tr("Архив успешно распакован в:\n%1").arg(absOutputDir));
            } else {
                emit statusChanged(tr("Ошибка"));
                emit logMessage(tr("Ошибка распаковки"), "error");
                QMessageBox::warning(this, tr("Ошибка"), tr("Не удалось распаковать архив"));
            }
        });
    });
}
