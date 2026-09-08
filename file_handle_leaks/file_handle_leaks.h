#ifndef FILE_HANDLE_LEAKS_H
#define FILE_HANDLE_LEAKS_H

#include <QObject>
#include <QTimer>
#include <unistd.h>
#include <fcntl.h>

class File_handle_leaks : public QObject
{
    Q_OBJECT
public:
    explicit File_handle_leaks(QObject *parent = nullptr);
    QTimer *timer_info_update;

public slots:
    void slot_info_update();

signals:
    void signal_too_many_open_files(QString);
private:
};

#endif // FILE_HANDLE_LEAKS_H
