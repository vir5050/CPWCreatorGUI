#ifndef KEYGENERATORWIDGET_H
#define KEYGENERATORWIDGET_H

#include <QWidget>
#include <QLineEdit>
#include <QTextEdit>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>

class KeyGeneratorWidget : public QWidget
{
    Q_OBJECT

public:
    explicit KeyGeneratorWidget(QWidget *parent = nullptr);

signals:
    void logMessage(const QString &message, const QString &level);
    void statusChanged(const QString &status);

private slots:
    void generateKeys();
    void browseOutputDir();
    void saveKeys();
    void copyPublicKey();
    void copyPrivateKey();

private:
    void setupUi();

    QLineEdit *m_outputDirEdit;
    QComboBox *m_keySizeCombo;
    QTextEdit *m_publicKeyEdit;
    QTextEdit *m_privateKeyEdit;
    QPushButton *m_generateBtn;
    QPushButton *m_saveBtn;
    QLabel *m_statusLabel;
    
    QByteArray m_publicKey;
    QByteArray m_privateKey;
};

#endif // KEYGENERATORWIDGET_H
