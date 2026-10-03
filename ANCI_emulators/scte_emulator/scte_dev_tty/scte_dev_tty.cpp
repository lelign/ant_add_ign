#include <iostream>
#include <vector>
#include <cstring>
#include <fcntl.h>   
#include <unistd.h>  
#include <cstdint>   

int main() {
    const char* DEVICE_PATH = "/dev/tsin1"; 

    // Открываем FIFO в режиме R/W, чтобы избежать блокировки
    int fd = open(DEVICE_PATH, O_RDWR);
    if (fd == -1) {
        std::cerr << "Ошибка: Не удалось открыть " << DEVICE_PATH << std::endl;
        return 1;
    }

    // Оригинальные структуры пакетов (по 44 байта)
    // Индексы 0, 1, 2 — это SCTE_IN (вход в рекламу)
    // Индексы 3, 4, 5 — это SCTE_OUT (выход из рекламы)
    std::vector<std::vector<uint8_t>> raw_messages = {
        {0x41, 0x07, 0x28, 0x08, 0xff, 0xff, 0x00, 0x27, 0x00, 0x00, 0x33, 0x00, 0x00, 0x00, 0x00, 0x02, 0x01, 0x01, 0x00, 0x0e, 0x01, 0x04, 0x00, 0x00, 0x6e, 0x00, 0x00, 0x1f, 0x40, 0x00, 0x96, 0x00, 0x00, 0x01, 0x01, 0x0a, 0x00, 0x05, 0x01, 0x00, 0x00, 0x00, 0x01, 0x5d}, // IN 1
        {0x41, 0x07, 0x28, 0x08, 0xff, 0xff, 0x00, 0x27, 0x00, 0x00, 0x34, 0x00, 0x00, 0x00, 0x00, 0x02, 0x01, 0x01, 0x00, 0x0e, 0x01, 0x04, 0x00, 0x00, 0x6e, 0x00, 0x00, 0x17, 0x98, 0x00, 0x96, 0x00, 0x00, 0x01, 0x01, 0x0a, 0x00, 0x05, 0x01, 0x00, 0x00, 0x00, 0x01, 0xae}, // IN 2
        {0x41, 0x07, 0x28, 0x08, 0xff, 0xff, 0x00, 0x27, 0x00, 0x00, 0x35, 0x00, 0x00, 0x00, 0x00, 0x02, 0x01, 0x01, 0x00, 0x0e, 0x01, 0x04, 0x00, 0x00, 0x6e, 0x00, 0x00, 0x0f, 0xf0, 0x00, 0x96, 0x00, 0x00, 0x01, 0x01, 0x0a, 0x00, 0x05, 0x01, 0x00, 0x00, 0x00, 0x01, 0xff}, // IN 3
        
        {0x41, 0x07, 0x28, 0x08, 0xff, 0xff, 0x00, 0x27, 0x00, 0x00, 0x36, 0x00, 0x00, 0x00, 0x00, 0x02, 0x01, 0x01, 0x00, 0x0e, 0x03, 0x04, 0x00, 0x00, 0x6f, 0x00, 0x00, 0x1f, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x0a, 0x00, 0x05, 0x01, 0x00, 0x00, 0x00, 0x01, 0xcc}, // OUT 1
        {0x41, 0x07, 0x28, 0x08, 0xff, 0xff, 0x00, 0x27, 0x00, 0x00, 0x37, 0x00, 0x00, 0x00, 0x00, 0x02, 0x01, 0x01, 0x00, 0x0e, 0x03, 0x04, 0x00, 0x00, 0x6f, 0x00, 0x00, 0x17, 0x98, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x0a, 0x00, 0x05, 0x01, 0x00, 0x00, 0x00, 0x01, 0x1d}, // OUT 2
        {0x41, 0x07, 0x28, 0x08, 0xff, 0xff, 0x00, 0x27, 0x00, 0x00, 0x38, 0x00, 0x00, 0x00, 0x00, 0x02, 0x01, 0x01, 0x00, 0x0e, 0x03, 0x04, 0x00, 0x00, 0x6f, 0x00, 0x00, 0x0f, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x0a, 0x00, 0x05, 0x01, 0x00, 0x00, 0x00, 0x01, 0x6e}  // OUT 3
    };

    // === ШАГ 1: Прогрев буфера ридера (сброс readout_cnt) ===
    // Отправляем 260 пакетов для любого канала (например, 0), чтобы обнулить счетчик на плате
    std::cout << "Сброс readout_cnt в AncReader (отправка 260 стартовых пакетов)..." << std::endl;
    for (int i = 0; i < 260; ++i) {
        std::vector<uint8_t> warm_packet;
        warm_packet.push_back(0); // префикс канала 0
        warm_packet.insert(warm_packet.end(), raw_messages[0].begin(), raw_messages[0].end());
        write(fd, warm_packet.data(), warm_packet.size());
        usleep(500); 
    }
    std::cout << "Прогрев завершен! Переходим в режим бесконечной генерации." << std::endl;

    bool is_scte_in_phase = true; // Триггер фазы: чередуем IN и OUT
    uint32_t iteration_counter = 0;

    // === ШАГ 2: Бесконечный цикл генерации ===
    while (true) {
        iteration_counter++;
        std::cout << "\n--- Итерация #" << iteration_counter 
                  << " | Фаза: " << (is_scte_in_phase ? "SCTE_IN (Вход)" : "SCTE_OUT (Выход)") << " ---" << std::endl;

        // Обходим последовательно все 16 каналов (от 0 до 15)
        for (uint8_t channel = 0; channel < 16; ++channel) {
            std::vector<uint8_t> packet;
            
            // 1. Задаем байт канала динамически (от 0 до 15)
            packet.push_back(channel); 

            // 2. Выбираем шаблон сообщения в зависимости от текущей фазы и номера канала.
            // Чтобы у каналов были разные сообщения (разные Event ID/таймкоды), 
            // мы используем остаток от деления (channel % 3), выбирая один из трех вариантов IN или OUT.
            size_t message_index = 0;
            if (is_scte_in_phase) {
                message_index = channel % 3;       // Индексы 0, 1, 2
            } else {
                message_index = 3 + (channel % 3);   // Индексы 3, 4, 5
            }

            // 3. Копируем тело оригинального сообщения в пакет вслед за байтом канала
            packet.insert(packet.end(), raw_messages[message_index].begin(), raw_messages[message_index].end());

            // 4. Отправляем в /dev/tsin1
            write(fd, packet.data(), packet.size());
            
            // Маленькая микропауза в 5 миллисекунд между каналами, 
            // чтобы AncReader успевал разгребать очередь сигналов и не захлебнулся
            usleep(5000); 
        }

        std::cout << "Метки для всех 16 каналов успешно отправлены. Ожидание 15 секунд..." << std::endl;

        // Переключаем фазу для следующей итерации (если сейчас был вход, то через 15 секунд будет выход)
        is_scte_in_phase = !is_scte_in_phase;

        // Спим ровно 15 секунд перед повторением истории
        sleep(15);
    }

    close(fd); // Сюда код никогда не дойдет, прерывается через Ctrl+C
    return 0;
}







/*Чтобы пакеты идеально и без изменений кода на плате распарсились вашей функцией Scte_104::parse(), 
эмулятор должен отправлять ваши примеры байт-в-байт в их первозданном виде, вообще без модификации заголовков. 
Единственное, что мы добавим — это 1 байт канала (0x02) в самое начало при отправке в /dev/tsin1, 
чтобы удовлетворить AncReader.*/

/*
ок, это заработало, получил scte сообщения на экране а10 для 2-го канала
Эмулятор больше не пытается менять значение size (четвертый байт). 
На плату приходят оригинальные 41 07 28 08 ....
Идеальное смещение pnt: 
    Функция Scte_104::parse считает 
        DID=0x41, SDID=0x07, 
        tmp=0x28, 
        Payload=0x08. 
    Указатель pnt встанет ровно на 4-ю позицию, где начнется оригинальный блок SCTE-104 FF FF 00 27 ....
Корректные данные в логах: 
    Поля message_number прочитаются ровно как 33, 34, 35... 
    num_ops равен 1. 
    Цикл for сделает ровно 1 итерацию, найдет opID == 0x0101 и успешно вызовет splice_request_data().
*/

/*
#include <iostream>
#include <vector>
#include <cstring>
#include <fcntl.h>   
#include <unistd.h>  
#include <cstdint>   

int main() {
    const char* DEVICE_PATH = "/dev/tsin1"; 

    // Открываем FIFO в режиме R/W, чтобы избежать ошибки блокировки (допускается для mkfifo)
    int fd = open(DEVICE_PATH, O_RDWR);
    if (fd == -1) {
        std::cerr << "Ошибка: Не удалось открыть " << DEVICE_PATH << std::endl;
        return 1;
    }

    // Ваши оригинальные примеры из логов (каждая строка ровно 44 байта)
    std::vector<std::vector<uint8_t>> raw_scte_messages = {
        {0x41, 0x07, 0x28, 0x08, 0xff, 0xff, 0x00, 0x27, 0x00, 0x00, 0x33, 0x00, 0x00, 0x00, 0x00, 0x02, 0x01, 0x01, 0x00, 0x0e, 0x01, 0x04, 0x00, 0x00, 0x6e, 0x00, 0x00, 0x1f, 0x40, 0x00, 0x96, 0x00, 0x00, 0x01, 0x01, 0x0a, 0x00, 0x05, 0x01, 0x00, 0x00, 0x00, 0x01, 0x5d},
        {0x41, 0x07, 0x28, 0x08, 0xff, 0xff, 0x00, 0x27, 0x00, 0x00, 0x34, 0x00, 0x00, 0x00, 0x00, 0x02, 0x01, 0x01, 0x00, 0x0e, 0x01, 0x04, 0x00, 0x00, 0x6e, 0x00, 0x00, 0x17, 0x98, 0x00, 0x96, 0x00, 0x00, 0x01, 0x01, 0x0a, 0x00, 0x05, 0x01, 0x00, 0x00, 0x00, 0x01, 0xae},
        {0x41, 0x07, 0x28, 0x08, 0xff, 0xff, 0x00, 0x27, 0x00, 0x00, 0x35, 0x00, 0x00, 0x00, 0x00, 0x02, 0x01, 0x01, 0x00, 0x0e, 0x01, 0x04, 0x00, 0x00, 0x6e, 0x00, 0x00, 0x0f, 0xf0, 0x00, 0x96, 0x00, 0x00, 0x01, 0x01, 0x0a, 0x00, 0x05, 0x01, 0x00, 0x00, 0x00, 0x01, 0xff},
        {0x41, 0x07, 0x28, 0x08, 0xff, 0xff, 0x00, 0x27, 0x00, 0x00, 0x36, 0x00, 0x00, 0x00, 0x00, 0x02, 0x01, 0x01, 0x00, 0x0e, 0x03, 0x04, 0x00, 0x00, 0x6f, 0x00, 0x00, 0x1f, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x0a, 0x00, 0x05, 0x01, 0x00, 0x00, 0x00, 0x01, 0xcc},
        {0x41, 0x07, 0x28, 0x08, 0xff, 0xff, 0x00, 0x27, 0x00, 0x00, 0x37, 0x00, 0x00, 0x00, 0x00, 0x02, 0x01, 0x01, 0x00, 0x0e, 0x03, 0x04, 0x00, 0x00, 0x6f, 0x00, 0x00, 0x17, 0x98, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x0a, 0x00, 0x05, 0x01, 0x00, 0x00, 0x00, 0x01, 0x1d},
        {0x41, 0x07, 0x28, 0x08, 0xff, 0xff, 0x00, 0x27, 0x00, 0x00, 0x38, 0x00, 0x00, 0x00, 0x00, 0x02, 0x01, 0x01, 0x00, 0x0e, 0x03, 0x04, 0x00, 0x00, 0x6f, 0x00, 0x00, 0x0f, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x0a, 0x00, 0x05, 0x01, 0x00, 0x00, 0x00, 0x01, 0x6e}
    };

    uint8_t channel_byte = 2; // Канал 2 (< 16), чтобы пройти валидацию AncReader

    // === ШАГ 1: Прогрев буфера (сброс readout_cnt) ===
    std::cout << "Отправка 260 пакетов для прогрева буфера (сброс readout_cnt)..." << std::endl;
    for (int i = 0; i < 260; ++i) {
        std::vector<uint8_t> warm_packet;
        warm_packet.push_back(channel_byte); // Сначала пишем байт канала
        // Затем дописываем оригинальный массив первого примера
        warm_packet.insert(warm_packet.end(), raw_scte_messages[0].begin(), raw_scte_messages[0].end());
        
        write(fd, warm_packet.data(), warm_packet.size());
        usleep(1000); 
    }

    std::cout << "Буфер прогрет. Начинаем поочередную отправку оригинальных примеров..." << std::endl;

    // === ШАГ 2: Циклически отправляем ваши оригинальные пакеты ===
    for (size_t idx = 0; idx < raw_scte_messages.size(); ++idx) {
        std::vector<uint8_t> combat_packet;
        
        // 1. Добавляем префикс канала в начало (AncReader считает его как data.buf[0])
        combat_packet.push_back(channel_byte); 
        
        // 2. Добавляем оригинальные 44 байта без каких-либо изменений
        combat_packet.insert(combat_packet.end(), raw_scte_messages[idx].begin(), raw_scte_messages[idx].end());

        std::cout << "Отправка примера №" << idx + 1 << "..." << std::endl;
        
        // Отправляем результирующий пакет размером 45 байт (1 байт канала + 44 байта оригинального примера)
        ssize_t bytes_written = write(fd, combat_packet.data(), combat_packet.size());

        if (bytes_written < 0) {
            std::cerr << "Ошибка записи примера №" << idx + 1 << std::endl;
        } else {
            std::cout << "Успешно отправлено " << bytes_written << " байт в /dev/tsin1." << std::endl;
        }
        
        sleep(2); // Пауза 2 секунды между примерами
    }

    close(fd);
    std::cout << "Все примеры успешно обработаны." << std::endl;
    return 0;
}*/




/* работает, но парсинг неправильныйб 
#include <iostream>
#include <vector>
#include <cstring>
#include <fcntl.h>   
#include <unistd.h>  
#include <cstdint>   

int main() {
    const char* DEVICE_PATH = "/dev/tsin1"; 

    // Открываем FIFO в режиме R/W, чтобы избежать ошибки блокировки (допускается для mkfifo)
    int fd = open(DEVICE_PATH, O_RDWR);
    if (fd == -1) {
        std::cerr << "Ошибка: Не удалось открыть " << DEVICE_PATH << std::endl;
        return 1;
    }

    // Массив ваших реальных примеров (каждый пакет по 44 байта)
    std::vector<std::vector<uint8_t>> scte_examples = {
        // 1-3: Splice Start (Вход в рекламу, разные таймкоды/Event ID)
        {0x41, 0x07, 0x28, 0x08, 0xff, 0xff, 0x00, 0x27, 0x00, 0x00, 0x33, 0x00, 0x00, 0x00, 0x00, 0x02, 0x01, 0x01, 0x00, 0x0e, 0x01, 0x04, 0x00, 0x00, 0x6e, 0x00, 0x00, 0x1f, 0x40, 0x00, 0x96, 0x00, 0x00, 0x01, 0x01, 0x0a, 0x00, 0x05, 0x01, 0x00, 0x00, 0x00, 0x01, 0x5d},
        {0x41, 0x07, 0x28, 0x08, 0xff, 0xff, 0x00, 0x27, 0x00, 0x00, 0x34, 0x00, 0x00, 0x00, 0x00, 0x02, 0x01, 0x01, 0x00, 0x0e, 0x01, 0x04, 0x00, 0x00, 0x6e, 0x00, 0x00, 0x17, 0x98, 0x00, 0x96, 0x00, 0x00, 0x01, 0x01, 0x0a, 0x00, 0x05, 0x01, 0x00, 0x00, 0x00, 0x01, 0xae},
        {0x41, 0x07, 0x28, 0x08, 0xff, 0xff, 0x00, 0x27, 0x00, 0x00, 0x35, 0x00, 0x00, 0x00, 0x00, 0x02, 0x01, 0x01, 0x00, 0x0e, 0x01, 0x04, 0x00, 0x00, 0x6e, 0x00, 0x00, 0x0f, 0xf0, 0x00, 0x96, 0x00, 0x00, 0x01, 0x01, 0x0a, 0x00, 0x05, 0x01, 0x00, 0x00, 0x00, 0x01, 0xff},
        // 4-6: Splice End / Reset (Выход из рекламы)
        {0x41, 0x07, 0x28, 0x08, 0xff, 0xff, 0x00, 0x27, 0x00, 0x00, 0x36, 0x00, 0x00, 0x00, 0x00, 0x02, 0x01, 0x01, 0x00, 0x0e, 0x03, 0x04, 0x00, 0x00, 0x6f, 0x00, 0x00, 0x1f, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x0a, 0x00, 0x05, 0x01, 0x00, 0x00, 0x00, 0x01, 0xcc},
        {0x41, 0x07, 0x28, 0x08, 0xff, 0xff, 0x00, 0x27, 0x00, 0x00, 0x37, 0x00, 0x00, 0x00, 0x00, 0x02, 0x01, 0x01, 0x00, 0x0e, 0x03, 0x04, 0x00, 0x00, 0x6f, 0x00, 0x00, 0x17, 0x98, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x0a, 0x00, 0x05, 0x01, 0x00, 0x00, 0x00, 0x01, 0x1d},
        {0x41, 0x07, 0x28, 0x08, 0xff, 0xff, 0x00, 0x27, 0x00, 0x00, 0x38, 0x00, 0x00, 0x00, 0x00, 0x02, 0x01, 0x01, 0x00, 0x0e, 0x03, 0x04, 0x00, 0x00, 0x6f, 0x00, 0x00, 0x0f, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x0a, 0x00, 0x05, 0x01, 0x00, 0x00, 0x00, 0x01, 0x6e}
    };

    // Подготовка: AncReader отбрасывает первые 256 пакетов.
    // Сгенерируем 260 "прогревочных" пакетов, используя первый пример в качестве основы.
    std::cout << "Отправка 260 пакетов для прогрева буфера (сброс readout_cnt)..." << std::endl;
    std::vector<uint8_t> warm_packet = scte_examples[0];
    
    // В ваших примерах 3-й байт равен 0x28 (десятичное 40). 
    // Формула AncReader: size (buf[3]) == data.size - buf[0] - 4.
    // В примере buf[3] = 0x08. Чтобы проверка сошлась, мы подгоним байт канала buf[0] под формулу.
    // buf[0] = data.size - 4 - size = 44 - 4 - 8 = 32. 
    // Но так как проверка требует (channel < 16), мы жестко перезапишем заголовок, чтобы он ИДЕАЛЬНО прошел валидацию:
    warm_packet[0] = 2;    // channel = 2 (< 16)
    warm_packet[1] = 0x41; // S2010_DID
    warm_packet[2] = 0x07; // S2010_SDID
    warm_packet[3] = 39;   // size = 44 - 1 - 4 = 39

    for (int i = 0; i < 260; ++i) {
        // Будем слегка менять message_number (5-й байт), чтобы пакеты отличались
        warm_packet[4] = i & 0xFF; 
        write(fd, warm_packet.data(), warm_packet.size());
        usleep(1000); // 1 мс пауза
    }

    std::cout << "Буфер прогрет. Начинаем отправку боевых примеров..." << std::endl;

    // Циклически отправляем ваши 6 реальных пакетов
    for (size_t idx = 0; idx < scte_examples.size(); ++idx) {
        std::vector<uint8_t> combat_packet = scte_examples[idx];
        
        // Модифицируем первые 4 байта под жесткие условия валидации вашего AncReader
        combat_packet[0] = 2;    // channel = 2 (проходит проверку channel < 16)
        combat_packet[1] = 0x41; // DID = 0x41
        combat_packet[2] = 0x07; // SDID = 0x07
        combat_packet[3] = 39;   // size = 44 - 1 - 4 = 39 (убирает лог "Incorrect ANC size")

        std::cout << "Отправка примера №" << idx + 1 << "..." << std::endl;
        ssize_t bytes_written = write(fd, combat_packet.data(), combat_packet.size());

        if (bytes_written < 0) {
            std::cerr << "Ошибка записи примера №" << idx + 1 << std::endl;
        } else {
            std::cout << "Успешно отправлено " << bytes_written << " байт." << std::endl;
        }
        
        sleep(2); // Делаем паузу в 2 секунды между отправками меток
    }

    close(fd);
    std::cout << "Все примеры успешно отправлены." << std::endl;
    return 0;
}*/



/* работает, но нужно привести в полное соотвествие с примерами из /docs/scte-104.txt
#include <iostream>
#include <vector>
#include <cstring>
#include <fcntl.h>   
#include <unistd.h>  
#include <cstdint>   

// Константы из вашего AncReader
#define S2010_DID (0x41)
#define S2010_SDID (0x07)

// Вспомогательные функции для Big-Endian (сетевой порядок байт)
void appendUint16(std::vector<uint8_t>& buf, uint16_t val) {
    buf.push_back((val >> 8) & 0xFF);
    buf.push_back(val & 0xFF);
}

void appendUint32(std::vector<uint8_t>& buf, uint32_t val) {
    buf.push_back((val >> 24) & 0xFF);
    buf.push_back((val >> 16) & 0xFF);
    buf.push_back((val >> 8)  & 0xFF);
    buf.push_back(val & 0xFF);
}

// Функция генерации правильного сырого ANC-пакета для AncReader
std::vector<uint8_t> createAncPacket(uint8_t target_channel, uint32_t event_id) {
    std::vector<uint8_t> packet;

    // === 1. ЗАГОЛОВОК ANC ===
    packet.push_back(target_channel); // data.buf[0] = channel
    packet.push_back(S2010_DID);      // data.buf[1] = did (0x41)
    packet.push_back(S2010_SDID);     // data.buf[2] = sdid (0x07)
    
    // Место под размер (data.buf[3] = size)
    size_t anc_size_pos = packet.size();
    packet.push_back(0);              // placeholder для size

    // === 2. СТРУКТУРА SCTE-104 ===
    appendUint16(packet, 0xFFFF);     // opID / Magic Number
    packet.push_back(1);              // message_number
    packet.push_back(0);              // flags
    
    size_t msg_len_pos = packet.size();
    appendUint16(packet, 0);          // message_length (placeholder)
    
    packet.push_back(1);              // protocol_version
    packet.push_back(0);              // AS_index
    packet.push_back(1);              // message_count

    // Заголовок операции (Splice Request = 0x0101)
    size_t op_start_pos = packet.size();
    appendUint16(packet, 0x0101); 
    
    size_t op_len_pos = packet.size();
    appendUint16(packet, 0);          // op_data_length (placeholder)

    // Данные операции (Splice Start)
    packet.push_back(1);              // splice_insert_type: 1 = Вход в рекламу
    appendUint32(packet, event_id);   // уникальный ID события
    appendUint16(packet, 0);          // unique_program_id
    packet.push_back(5);              // pre_roll_time: 5 секунд
    packet.push_back(0);              // break_duration
    packet.push_back(0);              // avail_num
    packet.push_back(0);              // avails_expected

    // === 3. РАСЧЕТ ДЛИН (Fixups) ===
    // Внутренние длины SCTE-104
    uint16_t op_data_len = packet.size() - (op_len_pos + 2);
    packet[op_len_pos] = (op_data_len >> 8) & 0xFF;
    packet[op_len_pos + 1] = op_data_len & 0xFF;

    uint16_t total_msg_len = packet.size() - (msg_len_pos + 2);
    packet[msg_len_pos] = (total_msg_len >> 8) & 0xFF;
    packet[msg_len_pos + 1] = total_msg_len & 0xFF;

    // === ИСПРАВЛЕНИЕ РАЗМЕРА ПОД ФОРМУЛУ ANCREADER ===
    // Нам нужно, чтобы выполнялось равенство: size == data.size - 1 - 4
    // Полный текущий размер пакета (data.size) равен packet.size()
    uint8_t calculated_size = packet.size() - 1 - 4;
    packet[anc_size_pos] = calculated_size; 

    return packet;
}

int main() {
    const char* DEVICE_PATH = "/dev/tsin1"; 
    uint8_t CHANNEL = 2; 

    // int fd = open(DEVICE_PATH, O_WRONLY | O_NDELAY);
    int fd = open(DEVICE_PATH, O_RDWR); // чтобы генератор открывал файл абсолютно всегда, даже если AncReader еще не запущен и никто не слушает канал
    if (fd == -1) {
        std::cerr << "Ошибка: Не удалось открыть " << DEVICE_PATH << std::endl;
        return 1;
    }

    std::cout << "Отправка пакетов инициализации для сброса readout_cnt..." << std::endl;
    for (int i = 0; i < 260; ++i) {
        std::vector<uint8_t> warm_packet = createAncPacket(CHANNEL, i);
        write(fd, warm_packet.data(), warm_packet.size());
        usleep(1000); 
    }

    uint32_t TARGET_EVENT_ID = 777;
    std::vector<uint8_t> real_packet = createAncPacket(CHANNEL, TARGET_EVENT_ID);

    std::cout << "Отправка боевой метки SCTE-104 (Event ID: " << TARGET_EVENT_ID << ")..." << std::endl;
    ssize_t bytes_written = write(fd, real_packet.data(), real_packet.size());

    if (bytes_written < 0) {
        std::cerr << "Ошибка записи боевой метки!" << std::endl;
    } else {
        std::cout << "Успешно отправлено " << bytes_written << " байт боевого ANC пакета." << std::endl;
        std::cout << "Новое значение поля size (в HEX): " << (int)real_packet[3] << std::endl;
    }

    close(fd);
    return 0;
}*/


/* второй вариант , не проходит if(size != data.size-1-4)
#include <iostream>
#include <vector>
#include <cstring>
#include <fcntl.h>   
#include <unistd.h>  
#include <cstdint>   

// Константы из вашего AncReader
#define S2010_DID (0x41)
#define S2010_SDID (0x07)

// Вспомогательные функции для Big-Endian (сетевой порядок байт)
void appendUint16(std::vector<uint8_t>& buf, uint16_t val) {
    buf.push_back((val >> 8) & 0xFF);
    buf.push_back(val & 0xFF);
}

void appendUint32(std::vector<uint8_t>& buf, uint32_t val) {
    buf.push_back((val >> 24) & 0xFF);
    buf.push_back((val >> 16) & 0xFF);
    buf.push_back((val >> 8)  & 0xFF);
    buf.push_back(val & 0xFF);
}

// Функция генерации правильного сырого ANC-пакета для AncReader
std::vector<uint8_t> createAncPacket(uint8_t target_channel, uint32_t event_id) {
    std::vector<uint8_t> packet;

    // === 1. ЗАГОЛОВОК ANC (Ожидается структурой AncReader::process) ===
    packet.push_back(target_channel); // data.buf[0] = channel
    packet.push_back(S2010_DID);      // data.buf[1] = did (0x41)
    packet.push_back(S2010_SDID);     // data.buf[2] = sdid (0x07)
    
    // Место под размер ANC полезной нагрузки (data.buf[3] = size)
    size_t anc_size_pos = packet.size();
    packet.push_back(0);              // placeholder для size

    // === 2. СТРУКТУРА SCTE-104 (То, что упакуется в QByteArray и улетит в сигнал) ===
    // Глобальный заголовок SCTE-104
    appendUint16(packet, 0xFFFF);     // opID / Magic Number
    packet.push_back(1);              // message_number
    packet.push_back(0);              // flags
    
    size_t msg_len_pos = packet.size();
    appendUint16(packet, 0);          // message_length (placeholder)
    
    packet.push_back(1);              // protocol_version
    packet.push_back(0);              // AS_index
    packet.push_back(1);              // message_count

    // Заголовок операции (Splice Request = 0x0101)
    size_t op_start_pos = packet.size();
    appendUint16(packet, 0x0101); 
    
    size_t op_len_pos = packet.size();
    appendUint16(packet, 0);          // op_data_length (placeholder)

    // Данные операции (Splice Start)
    packet.push_back(1);              // splice_insert_type: 1 = Вход в рекламу
    appendUint32(packet, event_id);   // уникальный ID события
    appendUint16(packet, 0);          // unique_program_id
    packet.push_back(5);              // pre_roll_time: 5 секунд
    packet.push_back(0);              // break_duration
    packet.push_back(0);              // avail_num
    packet.push_back(0);              // avails_expected

    // === 3. РАСЧЕТ ДЛИН (Fixups) ===
    // Внутренние длины SCTE-104
    uint16_t op_data_len = packet.size() - (op_len_pos + 2);
    packet[op_len_pos] = (op_data_len >> 8) & 0xFF;
    packet[op_len_pos + 1] = op_data_len & 0xFF;

    uint16_t total_msg_len = packet.size() - (msg_len_pos + 2);
    packet[msg_len_pos] = (total_msg_len >> 8) & 0xFF;
    packet[msg_len_pos + 1] = total_msg_len & 0xFF;

    // Расчет поля size для заголовка ANC: size = data.size - 1 - 4
    // То есть размер всех байт, идущих ПОСЛЕ байта size
    uint8_t anc_payload_size = packet.size() - (anc_size_pos + 1);
    packet[anc_size_pos] = anc_payload_size;

    return packet;
}

int main() {
    const char* DEVICE_PATH = "/dev/tsin1"; 
    uint8_t CHANNEL = 2; // Канал должен быть < 16, чтобы пройти проверку

    // Открываем как стандартное символьное/потоковое устройство на запись
    int fd = open(DEVICE_PATH, O_WRONLY | O_NDELAY);
    if (fd == -1) {
        std::cerr << "Ошибка: Не удалось открыть " << DEVICE_PATH << ". Проверьте mknod или права доступа." << std::endl;
        return 1;
    }

    std::cout << "Подготовка к отправке потока в " << DEVICE_PATH << "..." << std::endl;

    // === ШАГ 1: Пробиваем "заглушку" readout_cnt ===
    // AncReader игнорирует первые 256 пакетов. Отправим 260 пакетов для прогрева буфера
    std::cout << "Отправка 260 пакетов инициализации для сброса readout_cnt..." << std::endl;
    for (int i = 0; i < 260; ++i) {
        std::vector<uint8_t> warm_packet = createAncPacket(CHANNEL, i);
        write(fd, warm_packet.data(), warm_packet.size());
        usleep(1000); // небольшая пауза (1 мс) между пакетами
    }

    // === ШАГ 2: Отправляем боевую метку SCTE-104 ===
    uint32_t TARGET_EVENT_ID = 777;
    std::vector<uint8_t> real_packet = createAncPacket(CHANNEL, TARGET_EVENT_ID);

    std::cout << "Отправка боевой метки SCTE-104 (Event ID: " << TARGET_EVENT_ID << ")..." << std::endl;
    ssize_t bytes_written = write(fd, real_packet.data(), real_packet.size());

    if (bytes_written < 0) {
        std::cerr << "Ошибка записи боевой метки!" << std::endl;
    } else {
        std::cout << "Успешно отправлено " << bytes_written << " байт боевого ANC пакета." << std::endl;
        std::cout << "HEX: ";
        for (uint8_t byte : real_packet) {
            printf("%02X ", byte);
        }
        std::cout << std::endl;
    }

    close(fd);
    return 0;
}*/


/*
first variant
#include <iostream>
#include <vector>
#include <cstring>
#include <fcntl.h>   // Префиксы O_RDWR, O_NOCTTY
#include <termios.h> // Конфигурация ком-порта (struct termios)
#include <unistd.h>  // write(), close()
#include <cstdint>  // <-- ДОБАВЛЕНО: исправляет ошибку ‘uint8_t’ was not declared in this scope


// Вспомогательные функции для Big-Endian (сетевой порядок байт)
void appendUint16(std::vector<uint8_t>& buf, uint16_t val) {
    buf.push_back((val >> 8) & 0xFF);
    buf.push_back(val & 0xFF);
}

void appendUint32(std::vector<uint8_t>& buf, uint32_t val) {
    buf.push_back((val >> 24) & 0xFF);
    buf.push_back((val >> 16) & 0xFF);
    buf.push_back((val >> 8)  & 0xFF);
    buf.push_back(val & 0xFF);
}

int main() {
    // Укажите ваш порт (например, /dev/ttyUSB0 или /dev/ttyS0)
    const char* SERIAL_PORT = "/dev/tsin1"; // /dev/tsin1

    // === 1. ОТКРЫТИЕ И НАСТРОЙКА КОМ-ПОРТА ===
    int fd = open(SERIAL_PORT, O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd == -1) {
        std::cerr << "Ошибка: Не удалось открыть порт " << SERIAL_PORT << ". Проверьте права доступа (sudo)." << std::endl;
        return 1;
    }
    fcntl(fd, F_SETFL, 0); // Сброс блокировок чтения

    // Структура для настройки параметров связи
    struct termios options;
    tcgetattr(fd, &options);

    // Установка скорости (115200 бод)
    cfsetispeed(&options, B115200);
    cfsetospeed(&options, B115200);

    // Настройка режима: 8 бит данных, нет четности, 1 стоп-бит (8N1)
    options.c_cflag &= ~PARENB;        // Без четности
    options.c_cflag &= ~CSTOPB;        // 1 стоп-бит
    options.c_cflag &= ~CSIZE;         // Сброс маски размера кадра
    options.c_cflag |= CS8;            // 8 бит данных
    options.c_cflag |= (CLOCAL | CREAD); // Включить приемник, локальный режим

    // Отключение управления потоком (Hardware & Software flow control)
    options.c_cflag &= ~CRTSCTS;
    options.c_iflag &= ~(IXON | IXOFF | IXANY);

    // Сырой режим ввода-вывода (Raw Mode)
    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    options.c_oflag &= ~OPOST;

    // Применение настроек немедленно
    tcsetattr(fd, TCSANOW, &options);

    // === 2. ФОРМИРОВАНИЕ ПАКЕТА SCTE-104 ===
    std::vector<uint8_t> packet;

    // Добавляем маркер начала кадра для последовательного порта (STX = 0x02), если требует протокол инжектора
    // packet.push_back(0x02); 

    // Глобальный заголовок
    appendUint16(packet, 0xFFFF); // opID / Magic Number
    packet.push_back(1);          // message_number
    packet.push_back(0);          // flags
    
    size_t msg_len_pos = packet.size();
    appendUint16(packet, 0);      // message_length (placeholder)
    
    packet.push_back(1);          // protocol_version
    packet.push_back(0);          // AS_index
    packet.push_back(1);          // message_count (1 команда)

    // Заголовок операции (Splice Request = 0x0101)
    size_t op_start_pos = packet.size();
    appendUint16(packet, 0x0101); 
    
    size_t op_len_pos = packet.size();
    appendUint16(packet, 0);      // op_data_length (placeholder)

    // Данные операции (Splice Start)
    packet.push_back(1);          // splice_insert_type: 1 = Вход в рекламу
    appendUint32(packet, 777);    // splice_event_id (ID события)
    appendUint16(packet, 0);      // unique_program_id
    packet.push_back(5);          // pre_roll_time: 5 секунд задержки
    packet.push_back(0);          // break_duration
    packet.push_back(0);          // avail_num
    packet.push_back(0);          // avails_expected

    // Расчет длин полей (Fixup)
    uint16_t op_data_len = packet.size() - (op_len_pos + 2);
    packet[op_len_pos] = (op_data_len >> 8) & 0xFF;
    packet[op_len_pos + 1] = op_data_len & 0xFF;

    uint16_t total_msg_len = packet.size() - (msg_len_pos + 2);
    packet[msg_len_pos] = (total_msg_len >> 8) & 0xFF;
    packet[msg_len_pos + 1] = total_msg_len & 0xFF;

    // Конец кадра для последовательного порта (ETX = 0x03), если требует протокол
    // packet.push_back(0x03); 

    // === 3. ОТПРАВКА В ПОРТ ===
    std::cout << "Отправка SCTE-104 в " << SERIAL_PORT << "..." << std::endl;
    ssize_t bytes_written = write(fd, packet.data(), packet.size());

    if (bytes_written < 0) {
        std::cerr << "Ошибка записи в порт!" << std::endl;
    } else {
        std::cout << "Успешно отправлено " << bytes_written << " байт." << std::endl;
        std::cout << "HEX: ";
        for (uint8_t byte : packet) {
            printf("%02X ", byte);
        }
        std::cout << std::endl;
    }

    // Закрытие порта
    close(fd);
    return 0;
}
*/