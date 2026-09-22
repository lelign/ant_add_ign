#include "emulator.h"
#include <QThread>
#include <QDebug>

AudioEmulatorWorker::AudioEmulatorWorker(QObject *parent)
    : QObject(parent), m_running(false)
{
}

void AudioEmulatorWorker::startEmulation()
{
    m_running = true;

    // Инициализация генераторов (как в вашем коде)
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> dist_init(0.0, 1.0);
    std::uniform_real_distribution<double> dist_drift(-0.15, 0.15);
    std::uniform_real_distribution<double> dist_drop(0.0, 1.0);

    std::vector<double> current_levels(m_channels);
    for (int i = 0; i < m_channels; ++i) {
        current_levels[i] = dist_init(gen);
    }

    // Создаем QVector для передачи через Qt-сигнал
    QVector<int> int_levels(m_channels);

    qDebug() << "Audio Emulator thread started.";

    while (m_running) {
        // Симулируем поведение звука
        for (int i = 0; i < m_channels; ++i) {
            double drift = dist_drift(gen);
            current_levels[i] += drift;

            if (current_levels[i] < 0.0) current_levels[i] = 0.0;
            if (current_levels[i] > 1.0) current_levels[i] = 1.0;

            if (dist_drop(gen) < 0.02) {
                current_levels[i] *= 0.2;
            }

            // Масштабируем double (0.0 - 1.0) в int (0 - 100%).
            // int_levels[i] = static_cast<int>(current_levels[i] * 100.0); // 0-100
            int_levels[i] = static_cast<int>(current_levels[i] * 255.0); //диапазон 0-255
        }
        // check generating
        // qDebug() << "[Workers: Ch1 =" << int_levels[0] << "Ch2 =" << int_levels[1];

        // Эмиттим сигнал с данными. Qt сам позаботится о потокобезопасной доставке
        emit levelsUpdated(int_levels);

        // Вместо std::this_thread::sleep используем потокобезопасный сон Qt,
        // который позволяет корректно обрабатывать события остановки
        QThread::msleep(m_intervalMs);
    }

    emit finished();
}




void AudioEmulatorWorker::stopEmulation()
{
    m_running = false;
}

