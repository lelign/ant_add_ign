#pragma once

#include <QObject>
#include <QVector>
#include <random>

class AudioEmulatorWorker : public QObject
{
    Q_OBJECT

public:
    explicit AudioEmulatorWorker(QObject *parent = nullptr);

public slots:
    void startEmulation(); // Основной цикл, который будет крутиться в потоке
    void stopEmulation();  // Слот для безопасной остановки

signals:
    // Сигнал, который будет раз в 100 мс передавать новые уровни в основной поток
    void levelsUpdated(const QVector<int> &levels);
    void finished();

private:
    bool m_running;
    const int m_channels = 64;
    const int m_intervalMs = 100;
};
