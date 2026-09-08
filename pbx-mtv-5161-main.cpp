#include <QApplication>
#include <signal.h>
#include <QCommandLineParser>
#include <QDebug>
#include <QObject>
#include "pbx-mtv-5161.h"
#include <QStringList>
#include <QLoggingCategory>

QT_USE_NAMESPACE

static QLoggingCategory category("\033[32m MAIN\033[0m"); // ign green

void app_exit(int)
{
        QCoreApplication::quit();
}



int main(int argc, char *argv[])
{
        QApplication a(argc, argv);
        
        #ifdef APP_VERSION
        QCoreApplication::setApplicationVersion(APP_VERSION);
        #else
        QCoreApplication::setApplicationVersion(VERSION);
        #endif
        
        QCommandLineParser parser;
        parser.setApplicationDescription("PBX-MTV-5161 application");
        
        parser.addVersionOption(); 
        parser.addHelpOption();

        // Опция watchdog
        QCommandLineOption watchdog_option(QStringList() << "w" << "watchdog", "Enable watchdog.");
        parser.addOption(watchdog_option);

        // Опция debug
        QCommandLineOption debug_option("debug", "Enable debug mode.");
        parser.addOption(debug_option);

        // ДОБАВЬТЕ ЭТИ СТРОКИ: Регистрируем флаг --jq
        QCommandLineOption jq_option("jq", "Enable JSON logging mode.");
        parser.addOption(jq_option);

        QCommandLineOption sn75_option("sn75", "Enable sn75 logging mode.");
        parser.addOption(sn75_option);

        QCommandLineOption scte_option("scte", "Enable scte logging mode.");        
        parser.addOption(scte_option);

        QCommandLineOption rw_option(QStringList() << "rw" << "rwdis", "Disable read write str-mem.");
        parser.addOption(rw_option);
        

        // Теперь парсер успешно обработает --jq и не будет выдавать ошибку
        parser.process(a);

        // (Опционально) Можно вывести лог о включении option здесь
        if (parser.isSet(debug_option)) {
                qDebug(category) << "PBX-MTV-5161 application Debug mode is enabled";
        }else if(parser.isSet(jq_option)){
                qDebug(category) << "PBX-MTV-5161 application JQ mode is enabled";
        }else if(parser.isSet(sn75_option)){
                qDebug(category) << "PBX-MTV-5161 application sn75 mode is enabled";
        }else if(parser.isSet(scte_option)){
                qDebug(category) << "PBX-MTV-5161 application scte mode is enabled";
        }else if(parser.isSet(rw_option)){
                qDebug(category) << "PBX-MTV-5161 application read write str-mem is disabled";
        }else{
                qDebug(category) << "PBX-MTV-5161 application";
        }


        signal(SIGINT, app_exit);
        signal(SIGPIPE, SIG_IGN);

        bool watchdog = parser.isSet(watchdog_option);
        PbxMtv508 mtv508(watchdog);

        return a.exec();
}


/*int main(int argc, char *argv[])
{
        QApplication a(argc, argv);
        QStringList args = QCoreApplication::arguments();
        if (args.size() > 1) {
                QString firstArg = args.at(1); // Get the first real argument
                qDebug() << "First argument passed:" << firstArg;
        }
        qDebug() << "PBX-MTV-5161 application";
        QCoreApplication::setApplicationVersion(APP_VERSION);
        QCommandLineParser parser;
        QCommandLineOption watchdog_option("w", "watchdog");
        parser.setApplicationDescription("PBX-MTV-5161 applicaton");
        parser.addVersionOption(); 
        parser.addOption(watchdog_option);
        parser.addHelpOption();
        parser.process(a);

        signal(SIGINT, app_exit);
        signal(SIGPIPE, SIG_IGN);

        bool watchdog = parser.isSet(watchdog_option);
        PbxMtv508 mtv508(watchdog);

        return a.exec();
}
*/