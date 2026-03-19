/**
 * ESO Update Tool GUI - Графическая утилита для создания обновлений
 * Ether Saga Odyssey
 */

#include <QApplication>
#include <QIcon>
#include <QStyleFactory>
#include <QDir>
#include <QSysInfo>
#include "MainWindow.h"
#include "core/Logger.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // Настройка приложения
    app.setApplicationName("ESO Update Tool");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("ESO Tools");
    
    // Инициализация логгера в директории приложения
    QString logDir = QCoreApplication::applicationDirPath() + "/logs";
    if (!Logger::instance().init(logDir)) {
        // Fallback
        Logger::instance().init();
    }
    
    // Логирование запуска
    Logger::instance().log("Приложение запущено", "INFO");
    Logger::instance().log(QString("Версия: %1").arg(app.applicationVersion()), "INFO");
    Logger::instance().log(QString("Платформа: %1").arg(QSysInfo::prettyProductName()), "INFO");
    
    // Установка стиля
#ifdef Q_OS_WIN
    app.setStyle(QStyleFactory::create("Fusion"));
#endif
    
    // Создание главного окна
    MainWindow window;
    window.setWindowTitle("ESO Update Tool v1.0.0");
    window.setMinimumSize(900, 650);
    window.resize(1000, 700);
    window.show();
    
    int result = app.exec();
    
    // Логирование завершения
    Logger::instance().log("Приложение завершено", "INFO");
    
    return result;
}
