#include "KeyGeneratorWidget.h"
#include "core/Crypto.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QApplication>
#include <QClipboard>
#include <QSpinBox>
#include <QProgressDialog>
#include <QCoreApplication>

KeyGeneratorWidget::KeyGeneratorWidget(QWidget *parent)
    : QWidget(parent)
    , m_outputDirEdit(nullptr)
    , m_keySizeCombo(nullptr)
    , m_publicKeyEdit(nullptr)
    , m_privateKeyEdit(nullptr)
    , m_generateBtn(nullptr)
    , m_saveBtn(nullptr)
    , m_statusLabel(nullptr)
{
    setupUi();
}

void KeyGeneratorWidget::setupUi()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);
    
    // Группа настроек
    QGroupBox *settingsGroup = new QGroupBox(tr("Настройки"), this);
    QFormLayout *settingsLayout = new QFormLayout(settingsGroup);
    
    // Директория вывода
    QHBoxLayout *outputLayout = new QHBoxLayout();
    m_outputDirEdit = new QLineEdit("./keys", this);
    QPushButton *browseBtn = new QPushButton(tr("Обзор..."), this);
    outputLayout->addWidget(m_outputDirEdit);
    outputLayout->addWidget(browseBtn);
    settingsLayout->addRow(tr("Директория:"), outputLayout);
    
    // Размер ключа
    m_keySizeCombo = new QComboBox(this);
    m_keySizeCombo->addItems({"1024", "2048", "4096"});
    m_keySizeCombo->setCurrentText("2048");
    settingsLayout->addRow(tr("Размер ключа:"), m_keySizeCombo);
    
    mainLayout->addWidget(settingsGroup);
    
    // Кнопки
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    m_generateBtn = new QPushButton(tr("Генерировать ключи"), this);
    m_generateBtn->setIcon(QIcon::fromTheme("dialog-password"));
    m_generateBtn->setMinimumHeight(40);
    m_generateBtn->setStyleSheet("font-weight: bold;");
    
    m_saveBtn = new QPushButton(tr("Сохранить в файлы"), this);
    m_saveBtn->setIcon(QIcon::fromTheme("document-save"));
    m_saveBtn->setEnabled(false);
    
    buttonLayout->addWidget(m_generateBtn);
    buttonLayout->addWidget(m_saveBtn);
    buttonLayout->addStretch();
    
    mainLayout->addLayout(buttonLayout);
    
    // Статус
    m_statusLabel = new QLabel(tr("Нажмите 'Генерировать' для создания ключей"), this);
    m_statusLabel->setStyleSheet("color: gray; font-style: italic;");
    mainLayout->addWidget(m_statusLabel);
    
    // Группа публичного ключа
    QGroupBox *publicGroup = new QGroupBox(tr("Публичный ключ (для лаунчера)"), this);
    QVBoxLayout *publicLayout = new QVBoxLayout(publicGroup);
    
    m_publicKeyEdit = new QTextEdit(this);
    m_publicKeyEdit->setReadOnly(true);
    m_publicKeyEdit->setFont(QFont("Courier", 9));
    m_publicKeyEdit->setMaximumHeight(150);
    
    QPushButton *copyPublicBtn = new QPushButton(tr("Копировать"), this);
    connect(copyPublicBtn, &QPushButton::clicked, this, &KeyGeneratorWidget::copyPublicKey);
    
    publicLayout->addWidget(m_publicKeyEdit);
    publicLayout->addWidget(copyPublicBtn);
    
    mainLayout->addWidget(publicGroup);
    
    // Группа приватного ключа
    QGroupBox *privateGroup = new QGroupBox(tr("Приватный ключ (секретный, для подписи патчей)"), this);
    QVBoxLayout *privateLayout = new QVBoxLayout(privateGroup);
    
    m_privateKeyEdit = new QTextEdit(this);
    m_privateKeyEdit->setReadOnly(true);
    m_privateKeyEdit->setFont(QFont("Courier", 9));
    m_privateKeyEdit->setMaximumHeight(150);
    
    QHBoxLayout *privateButtonLayout = new QHBoxLayout();
    QPushButton *copyPrivateBtn = new QPushButton(tr("Копировать"), this);
    connect(copyPrivateBtn, &QPushButton::clicked, this, &KeyGeneratorWidget::copyPrivateKey);
    
    QLabel *warningLabel = new QLabel(tr("⚠️ Не передавайте приватный ключ третьим лицам!"), this);
    warningLabel->setStyleSheet("color: red; font-weight: bold;");
    
    privateButtonLayout->addWidget(copyPrivateBtn);
    privateButtonLayout->addWidget(warningLabel);
    privateButtonLayout->addStretch();
    
    privateLayout->addWidget(m_privateKeyEdit);
    privateLayout->addLayout(privateButtonLayout);
    
    mainLayout->addWidget(privateGroup);
    
    mainLayout->addStretch();
    
    // Подключение сигналов
    connect(browseBtn, &QPushButton::clicked, this, &KeyGeneratorWidget::browseOutputDir);
    connect(m_generateBtn, &QPushButton::clicked, this, &KeyGeneratorWidget::generateKeys);
    connect(m_saveBtn, &QPushButton::clicked, this, &KeyGeneratorWidget::saveKeys);
}

void KeyGeneratorWidget::generateKeys()
{
    int keySize = m_keySizeCombo->currentText().toInt();
    
    m_statusLabel->setText(tr("Генерация ключей (%1 бит)...").arg(keySize));
    m_statusLabel->setStyleSheet("color: blue;");
    emit statusChanged(tr("Генерация RSA ключей..."));
    emit logMessage(tr("Начата генерация RSA-%1 ключей").arg(keySize), "info");
    
    QApplication::setOverrideCursor(Qt::WaitCursor);
    
    Crypto crypto;
    if (crypto.generateRSAKey(keySize)) {
        m_publicKey = crypto.getPublicKeyPEM();
        m_privateKey = crypto.getPrivateKeyPEM();
        
        m_publicKeyEdit->setText(QString::fromUtf8(m_publicKey));
        m_privateKeyEdit->setText(QString::fromUtf8(m_privateKey));
        
        m_saveBtn->setEnabled(true);
        m_statusLabel->setText(tr("Ключи успешно сгенерированы"));
        m_statusLabel->setStyleSheet("color: green; font-weight: bold;");
        
        emit logMessage(tr("RSA ключи успешно сгенерированы (%1 бит)").arg(keySize), "success");
        emit statusChanged(tr("Ключи сгенерированы"));
    } else {
        m_statusLabel->setText(tr("Ошибка генерации ключей"));
        m_statusLabel->setStyleSheet("color: red;");
        
        emit logMessage(tr("Ошибка генерации RSA ключей: %1").arg(crypto.lastError()), "error");
        emit statusChanged(tr("Ошибка"));
    }
    
    QApplication::restoreOverrideCursor();
}

void KeyGeneratorWidget::browseOutputDir()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("Выберите директорию"));
    if (!dir.isEmpty()) {
        m_outputDirEdit->setText(dir);
    }
}

void KeyGeneratorWidget::saveKeys()
{
    QString outputDir = m_outputDirEdit->text();
    
    if (outputDir.isEmpty()) {
        QMessageBox::warning(this, tr("Ошибка"), tr("Укажите директорию для сохранения"));
        return;
    }
    
    // Разрешаем путь относительно директории приложения
    QDir appDir(QCoreApplication::applicationDirPath());
    QString absolutePath = appDir.absoluteFilePath(outputDir);
    
    // Создаём директорию если не существует
    if (!QDir(absolutePath).exists()) {
        if (!QDir().mkpath(absolutePath)) {
            QMessageBox::critical(this, tr("Ошибка"), tr("Не удалось создать директорию: %1").arg(absolutePath));
            return;
        }
    }
    
    QString pubPath = QDir(absolutePath).filePath("pub_key.txt");
    QString privPath = QDir(absolutePath).filePath("priv_key.txt");
    
    // Сохранение публичного ключа
    QFile pubFile(pubPath);
    if (!pubFile.open(QIODevice::WriteOnly)) {
        QMessageBox::critical(this, tr("Ошибка"), tr("Не удалось создать файл: %1").arg(pubPath));
        return;
    }
    pubFile.write(m_publicKey);
    pubFile.close();
    
    // Сохранение приватного ключа
    QFile privFile(privPath);
    if (!privFile.open(QIODevice::WriteOnly)) {
        QMessageBox::critical(this, tr("Ошибка"), tr("Не удалось создать файл: %1").arg(privPath));
        return;
    }
    privFile.write(m_privateKey);
    privFile.close();
    
    emit logMessage(tr("Ключи сохранены в:\n  %1\n  %2").arg(pubPath, privPath), "success");
    
    QMessageBox::information(this, tr("Успех"), 
        tr("Ключи успешно сохранены:\n\nПубличный: %1\nПриватный: %2")
        .arg(pubPath, privPath));
}

void KeyGeneratorWidget::copyPublicKey()
{
    QApplication::clipboard()->setText(m_publicKeyEdit->toPlainText());
    emit statusChanged(tr("Публичный ключ скопирован"));
}

void KeyGeneratorWidget::copyPrivateKey()
{
    QApplication::clipboard()->setText(m_privateKeyEdit->toPlainText());
    emit statusChanged(tr("Приватный ключ скопирован"));
}
