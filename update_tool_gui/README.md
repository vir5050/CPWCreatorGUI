# Update Tool GUI

Графическая утилита для создания обновлений игр на движке Angelica.

![Screenshot](docs/screenshot.png)

## Возможности

- 🎮 **Создание патчей** — сканирование файлов, вычисление MD5, сжатие
- 📦 **Работа с PCK архивами** — упаковка и распаковка игровых ресурсов
- 🔐 **RSA подпись** — генерация ключей и подписание патчей
- ⚙️ **Настройки** — гибкая конфигурация параметров
- 📝 **Логирование** — подробный журнал операций

## Поддерживаемые игры

| Игра | Код | Статус |
|------|-----|--------|
| Ether Saga Odyssey | ESO | ✅ Полная поддержка |
| Forsaken World | FW | ✅ Полная поддержка |
| Perfect World | PW | ✅ Полная поддержка |
| Jade Dynasty | JD | ✅ Полная поддержка |

## Требования

- **Qt 6.2+**
- **OpenSSL 1.1+**
- **zlib 1.2+**
- **C++17** совместимый компилятор

## Сборка

### Linux (Ubuntu/Debian)

```bash
# Установка зависимостей
sudo apt-get install build-essential cmake qt6-base-dev libssl-dev zlib1g-dev

# Сборка
./build.sh
```

### Linux (Arch Linux)

```bash
# Установка зависимостей
sudo pacman -S base-devel cmake qt6-base openssl zlib

# Сборка
./build.sh
```

### Linux (Fedora/RHEL)

```bash
# Установка зависимостей
sudo dnf install gcc-c++ cmake qt6-qtbase-devel openssl-devel zlib-devel

# Сборка
./build.sh
```

### macOS (Homebrew)

```bash
# Установка зависимостей
brew install cmake qt@6 openssl@3 zlib

# Сборка
export OPENSSL_ROOT_DIR=$(brew --prefix openssl@3)
./build.sh
```

### Windows

1. Установите Qt 6 через онлайн-установщик
2. Установите OpenSSL для Windows
3. Откройте проект в Qt Creator или соберите через CMake

```cmd
mkdir build
cd build
cmake -DCMAKE_PREFIX_PATH=C:/Qt/6.5.0/msvc2019_64 ..
cmake --build . --config Release
```

## Использование

### Создание патча

1. Перейдите на вкладку **"Создание патча"**
2. Укажите директорию с файлами игры
3. Укажите директорию для сохранения патча
4. (Опционально) Укажите приватный ключ для подписи
5. Нажмите **"Сканировать файлы"**
6. После сканирования нажмите **"Создать патч"**

### Работа с PCK архивами

1. Перейдите на вкладку **"PCK Архивы"**
2. Выберите игру из списка или укажите пользовательские ключи
3. Для упаковки: укажите директорию и выходной файл
4. Для распаковки: укажите PCK файл и директорию назначения

### Генерация RSA ключей

1. Перейдите на вкладку **"RSA Ключи"**
2. Укажите размер ключа (1024-4096 бит)
3. Укажите директорию для сохранения
4. Нажмите **"Генерировать ключи"**

## Структура выходных данных

```
patch_output/
├── config.json              # JSON конфигурация для лаунчера
├── lists/
│   └── files.md5           # Список файлов с MD5 хешами и подписью
├── compressed/
│   ├── abc123...zip        # Сжатые файлы (имя = MD5 хеш)
│   └── ...
└── newlauncher/            # Структура для веб-сервера
    ├── files/
    └── files_compressed/
```

## Ключи для игр

### Ether Saga Odyssey (ESO)
```
KEY_1 = 0xB6A0D9D9
KEY_2 = 0x6C9FC1B5
ASIG_1 = 0x5DA82A50
ASIG_2 = 0x79D29F4A
```

### Forsaken World (FW)
```
KEY_1 = 0x21C31A3F
KEY_2 = 0x185C2025
ASIG_1 = 0xA5253CA4
ASIG_2 = 0x859E8718
```

### Perfect World / Jade Dynasty (PW/JD)
```
KEY_1 = 0xA8A0A8A2
KEY_2 = 0x0F1A0E8B
ASIG_1 = 0xFE00BEBE
ASIG_2 = 0xF004BEBE
```

## Формат PCK архива

PCK (Angelica File Package) — формат игровых архивов от Perfect World Entertainment.

Структура:
```
[Заголовок 12 байт]
  - FSIG_1: 4 байта
  - Размер файла: 4 байта
  - FSIG_2: 4 байта

[Сжатые данные файлов]

[Таблица файлов с XOR шифрованием]
  - Размер записи XOR KEY_1: 4 байта
  - Размер записи XOR KEY_2: 4 байта
  - Путь файла: 260 байт
  - Позиция: 4 байта
  - Оригинальный размер: 4 байта
  - Сжатый размер: 4 байта
  - Unknown: 4 байта

[Footer]
  - ASIG_1: 4 байта
  - Версия: 4 байта
  - Позиция таблицы XOR KEY_1: 4 байта
  - Reserved: 4 байта
  - Сигнатура "Angelica File Package, Perfect World.": 37 байт
  - Padding: 215 байт
  - ASIG_2: 4 байта
  - Количество файлов: 4 байта
  - Версия: 4 байта
```

## Разработка

### Структура проекта

```
update_tool_gui/
├── CMakeLists.txt          # Конфигурация CMake
├── build.sh                # Скрипт сборки
├── src/
│   ├── main.cpp            # Точка входа
│   ├── MainWindow.*        # Главное окно
│   ├── KeyGeneratorWidget.* # Виджет генерации ключей
│   ├── PatchCreatorWidget.* # Виджет создания патча
│   ├── PckArchiverWidget.*  # Виджет работы с PCK
│   ├── SettingsWidget.*     # Виджет настроек
│   ├── LogWidget.*          # Виджет лога
│   └── core/
│       ├── Crypto.*        # Криптография (RSA, MD5)
│       ├── FileSystem.*    # Работа с файлами
│       ├── PckArchiver.*   # Работа с PCK
│       └── PatchGenerator.* # Генерация патча
└── resources/
    └── resources.qrc       # Ресурсы Qt
```

## Лицензия

MIT License
