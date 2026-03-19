#!/bin/bash

# Скрипт сборки ESO Update Tool GUI

set -e

echo "================================"
echo "ESO Update Tool GUI Build Script"
echo "================================"

# Проверка зависимостей
check_dependencies() {
    echo "Проверка зависимостей..."
    
    local missing=()
    
    if ! command -v cmake &> /dev/null; then
        missing+=("cmake")
    fi
    
    if ! command -v qmake6 &> /dev/null && ! command -v qmake &> /dev/null; then
        missing+=("Qt6 (qmake)")
    fi
    
    # Проверка OpenSSL (разные методы для разных систем)
    local openssl_found=false
    
    # macOS с Homebrew
    if [[ "$OSTYPE" == "darwin"* ]]; then
        if [ -d "$(brew --prefix openssl@3 2>/dev/null)" ] || [ -d "$(brew --prefix openssl 2>/dev/null)" ]; then
            openssl_found=true
        fi
    fi
    
    # Linux через pkg-config
    if ! $openssl_found && pkg-config --exists openssl 2>/dev/null; then
        openssl_found=true
    fi
    
    # Проверка наличия заголовков
    if ! $openssl_found; then
        for path in /usr/include/openssl/ssl.h /usr/local/include/openssl/ssl.h; do
            if [ -f "$path" ]; then
                openssl_found=true
                break
            fi
        done
    fi
    
    if ! $openssl_found; then
        missing+=("openssl-dev")
    fi
    
    # Проверка zlib (разные методы для разных систем)
    local zlib_found=false
    
    # macOS с Homebrew
    if [[ "$OSTYPE" == "darwin"* ]]; then
        if [ -d "$(brew --prefix zlib 2>/dev/null)" ]; then
            zlib_found=true
        fi
    fi
    
    # Linux через pkg-config
    if ! $zlib_found && pkg-config --exists zlib 2>/dev/null; then
        zlib_found=true
    fi
    
    # Проверка наличия заголовков
    if ! $zlib_found; then
        for path in /usr/include/zlib.h /usr/local/include/zlib.h; do
            if [ -f "$path" ]; then
                zlib_found=true
                break
            fi
        done
    fi
    
    if ! $zlib_found; then
        missing+=("zlib-dev")
    fi
    
    if [ ${#missing[@]} -ne 0 ]; then
        echo "Отсутствуют зависимости: ${missing[*]}"
        echo ""
        echo "Установка зависимостей:"
        echo "  Ubuntu/Debian: sudo apt-get install build-essential cmake qt6-base-dev libssl-dev zlib1g-dev"
        echo "  Arch Linux:    sudo pacman -S base-devel cmake qt6-base openssl zlib"
        echo "  Fedora/RHEL:   sudo dnf install gcc-c++ cmake qt6-qtbase-devel openssl-devel zlib-devel"
        echo "  macOS:         brew install cmake qt@6 openssl@3 zlib"
        exit 1
    fi
    
    echo "✓ Все зависимости установлены"
}

# Сборка
build() {
    local build_type="${1:-Release}"
    local build_dir="build"
    local log_file="build_errors.log"
    
    echo ""
    echo "Конфигурация: $build_type"
    echo "Директория сборки: $build_dir"
    echo "Лог ошибок: $log_file"
    echo ""
    
    # Очистка старого лога
    rm -f "$log_file"
    
    # Создание директории сборки
    mkdir -p "$build_dir"
    cd "$build_dir"
    
    # CMake конфигурация
    echo "Запуск CMake..."
    
    # Определяем путь к Qt6
    if command -v qmake6 &> /dev/null; then
        QT_PATH=$(qmake6 -query QT_INSTALL_PREFIX)
    else
        QT_PATH=$(qmake -query QT_INSTALL_PREFIX)
    fi
    
    # Определяем дополнительные пути для macOS/Homebrew
    CMAKE_OPTS=""
    if [[ "$OSTYPE" == "darwin"* ]]; then
        # OpenSSL от Homebrew
        if [ -d "$(brew --prefix openssl@3 2>/dev/null)" ]; then
            OPENSSL_PATH=$(brew --prefix openssl@3)
            CMAKE_OPTS="-DOPENSSL_ROOT_DIR=$OPENSSL_PATH"
            echo "Используется OpenSSL из: $OPENSSL_PATH"
        elif [ -d "$(brew --prefix openssl 2>/dev/null)" ]; then
            OPENSSL_PATH=$(brew --prefix openssl)
            CMAKE_OPTS="-DOPENSSL_ROOT_DIR=$OPENSSL_PATH"
            echo "Используется OpenSSL из: $OPENSSL_PATH"
        fi
    fi
    
    # CMake с записью ошибок
    if ! cmake -DCMAKE_BUILD_TYPE="$build_type" \
               -DCMAKE_PREFIX_PATH="$QT_PATH" \
               $CMAKE_OPTS \
               .. 2>&1 | tee "../$log_file"; then
        echo ""
        echo "================================"
        echo "Ошибка конфигурации CMake!"
        echo "Лог ошибок: $log_file"
        echo "================================"
        cd ..
        exit 1
    fi
    
    # Компиляция
    echo ""
    echo "Компиляция..."
    local jobs=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
    
    if ! cmake --build . -j"$jobs" 2>&1 | tee -a "../$log_file"; then
        cd ..
        echo ""
        echo "================================"
        echo "Ошибка компиляции!"
        echo "Лог ошибок: $log_file"
        echo "================================"
        echo ""
        echo "Последние 20 строк ошибок:"
        echo "--------------------------------"
        grep -E "(error:|Error:|ERROR)" "$log_file" | tail -20
        exit 1
    fi
    
    cd ..
    
    echo ""
    echo "================================"
    echo "Сборка успешно завершена!"
    echo "Исполняемый файл: $build_dir/eso_update_tool_gui"
    echo "================================"
}

# Очистка
clean() {
    echo "Очистка директории сборки..."
    rm -rf build
    rm -f build_errors.log
    echo "✓ Очистка завершена"
}

# Показать лог ошибок
show_log() {
    if [ -f "build_errors.log" ]; then
        echo "Содержимое build_errors.log:"
        echo "================================"
        cat build_errors.log
    else
        echo "Файл лога не найден: build_errors.log"
    fi
}

# Справка
usage() {
    echo "Использование: $0 [команда]"
    echo ""
    echo "Команды:"
    echo "  build       Сборка в Release режиме (по умолчанию)"
    echo "  debug       Сборка в Debug режиме"
    echo "  clean       Очистка директории сборки"
    echo "  log         Показать последний лог ошибок"
    echo ""
    echo "Примеры:"
    echo "  $0              # Сборка в Release режиме"
    echo "  $0 debug        # Сборка в Debug режиме"
    echo "  $0 log          # Показать лог ошибок"
}

# Главная логика
case "${1:-build}" in
    build)
        check_dependencies
        build "Release"
        ;;
    debug)
        check_dependencies
        build "Debug"
        ;;
    clean)
        clean
        ;;
    log)
        show_log
        ;;
    help|--help|-h)
        usage
        ;;
    *)
        echo "Неизвестная команда: $1"
        usage
        exit 1
        ;;
esac
