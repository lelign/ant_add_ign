/*
 * On operating systems each process is allocated a fixed table of file handles,
 * or file descriptors. This is typically about 1024 handles.
 * When you try to open 1025th file descriptor the operating system kills your process.
 *
 * https://oroboro.com/file-handle-leaks-server/
*/


#include "file_handle_leaks.h"
#include <QLoggingCategory>

#define MIN_TO_MSEC(min) ((min) * (60 * 1000))

static QLoggingCategory category("File_handle_leaks");

File_handle_leaks::File_handle_leaks(QObject *parent) : QObject(parent)
{
    qDebug(category) << "Creating...";

    timer_info_update = new QTimer;
    connect(timer_info_update, &QTimer::timeout, this, &File_handle_leaks::slot_info_update);
    timer_info_update->start(MIN_TO_MSEC(3));
}


void File_handle_leaks::slot_info_update()
{
static int error_set = 0;
int numHandles = 0;

    if(error_set) return;

    int maxNumHandles = getdtablesize();

    for(int i = 0; i < maxNumHandles; i++)
    {
        int fd_flags = fcntl(i, F_GETFD);
        if(fd_flags != -1) numHandles++;
    }

    if(numHandles > maxNumHandles * 0.75){
        emit signal_too_many_open_files("Too many open files!");
        error_set = 1;
    }
}

