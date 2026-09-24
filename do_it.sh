#!/bin/bash

# Настройки подключения к плате
BOARD_IP="192.168.5.92"
BOARD_PORT="2222"
BOARD_USER="root"
BINARY_NAME="pbx-mtv-5161"
DEST_DIR="/home/root"
USRBIN_DIR="/usr/bin"


# Прерывать выполнение при ошибках до этапа сборки
set -e

echo "=== 1. Настройка окружения кросс-компиляции ==="
source /opt/poky/5.2.4/environment-setup-cortexa9t2hf-neon-poky-linux-gnueabi

echo "=== 2. Очистка и создание директории сборки ==="
rm -rf build
mkdir build
cd build

echo "=== 3. Конфигурация проекта через CMake ==="
cmake ..

echo "=== 4. Сборка проекта ==="
set +e
make -j$(nproc)
MAKE_RET=$?
set -e

# Если сборка завершилась ошибкой, выходим
if [ $MAKE_RET -ne 0 ]; then
    echo "=== Ошибка компиляции! Скрипт остановлен. ==="
    exit $MAKE_RET
fi

# Очищаем терминал при успешной сборке
clear
echo "=== SUCCSESS -> DEPLOY... ===   ip route del default via 192.168.0.1  "

# 5. Останавливаем старую службу на плате (Замените [имя_сервиса] на реальное!)
echo "-> STOP systemctl..."
ssh -p ${BOARD_PORT} ${BOARD_USER}@${BOARD_IP} "systemctl stop $BINARY_NAME"

# 6. Копируем новый бинарник на плату
echo "-> SCP..."
scp -P ${BOARD_PORT} "${BINARY_NAME}" ${BOARD_USER}@${BOARD_IP}:${DEST_DIR}/

# 7. Запускаем приложение на плате
echo "-> START application"
#ssh -p ${BOARD_PORT} ${BOARD_USER}@${BOARD_IP} "cd ${DEST_DIR} && chmod +x ${BINARY_NAME}"
ssh -p ${BOARD_PORT} ${BOARD_USER}@${BOARD_IP} "cp ${BINARY_NAME} /usr/bin/${BINARY_NAME}"
ssh -p ${BOARD_PORT} ${BOARD_USER}@${BOARD_IP} "systemctl start ${BINARY_NAME}"
echo "=== Стрим логов приложения (нажмите Ctrl+C для выхода) ==="
# Подключаемся и смотрим логи в реальном времени
ssh -p ${BOARD_PORT} ${BOARD_USER}@${BOARD_IP} "journalctl -u ${BINARY_NAME} -n 50 -f"
