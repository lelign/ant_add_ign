#include "led_class.h"
#include <QLoggingCategory>
#include <QFile>

#include <QRegularExpression>

#define INPUTS_ALL "/var/volatile/inputs_all"

static QLoggingCategory category("led_class");


const QStringList  path_to_led_sdi_list =
{
    #if (BOARD_REV==0)
    "/var/volatile/input_1", /* led 1 */
    "/var/volatile/input_2", /* led 2 */
    "/var/volatile/input_3", /* led 3 */
    "/var/volatile/input_4", /* led 4 */
    "/var/volatile/input_5", /* led 5 */
    "/var/volatile/input_6", /* led 6 */
    "/var/volatile/input_7", /* led 7 */
    "/var/volatile/input_8", /* led 8 */
    "/var/volatile/input_9", /* led 9 */
    "/var/volatile/input_10", /* led 10 */
    "/var/volatile/input_11", /* led 11 */
    "/var/volatile/input_12", /* led 12 */
    "/var/volatile/input_13", /* led 13 */
    "/var/volatile/input_14", /* led 14 */
    "/var/volatile/input_15", /* led 15 */
    "/var/volatile/input_16"  /* led 16 */
    #else
    "/var/volatile/input_1", /* led 1 */
    "/var/volatile/input_2", /* led 2 */
    "/var/volatile/input_3", /* led 3 */
    "/var/volatile/input_4", /* led 4 */
    "/var/volatile/input_5", /* led 5 */
    "/var/volatile/input_6", /* led 6 */
    "/var/volatile/input_7", /* led 7 */
    "/var/volatile/input_8",  /* led 8 */
    "/var/volatile/input_9", /* led 9 */
    "/var/volatile/input_10", /* led 10 */
    "/var/volatile/input_11", /* led 11 */
    "/var/volatile/input_12", /* led 12 */
    "/var/volatile/input_13", /* led 13 */
    "/var/volatile/input_14", /* led 14 */
    "/var/volatile/input_15", /* led 15 */
    "/var/volatile/input_16"  /* led 16 */
    #endif
};

Led_class::Led_class(QObject *parent) : QObject(parent)
{
    set_all_led_off();
}

/* -------------------------------------------------------------------- */
void Led_class::set_led_state(int num, int state)
{
    #if (BOARD_REV==0)
    if(state)
        set_state(path_to_led_sdi_list[num], "1");
    else
        set_state(path_to_led_sdi_list[num], "0");
    #else
    if(state)
        set_state(path_to_led_sdi_list[num], "0");
    else
        set_state(path_to_led_sdi_list[num], "1");
    #endif
}
/* -------------------------------------------------------------------- */
void Led_class::set_all_led_off()
{
    for(int i = 0; i < path_to_led_sdi_list.size(); i++)
        set_state(path_to_led_sdi_list[i], "0");
}

/* -------------------------------------------------------------------- */
void Led_class::set_all_led_on()
{
    for(int i = 0; i < path_to_led_sdi_list.size(); i++)
        set_state(path_to_led_sdi_list[i], "1");
}

/* -------------------------------------------------------------------- */
void Led_class::set_state(QString file_name, const char *state)
{
    QFile file(file_name);
    file.open(QIODevice::ReadWrite);
    file.write(state);
    file.close();
    // ign added this :
    intArg = read_value(file_name);
    QString rewrite_line;
    rewrite_line= file_name.replace("/var/volatile/", "");
    write_value(rewrite_line, intArg);
}

int Led_class::read_value(QString file_name) {
    QFile file(file_name);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug(category) << "Failed to open file for reading:" << file.errorString();
        return -1;
    }
    QString line;
    line = file.readAll();
    file.close();
    return  line.toInt();    
}

void Led_class::write_value(QString rewrite_line, int intArg) {
    QFile file(INPUTS_ALL);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Error opening file for reading:" << file.errorString();
        return;
    }

    QString fileContent = QTextStream(&file).readAll();
    file.close();
    QString newPattern = rewrite_line;
    QString newString = rewrite_line + " " + QString::number(intArg);
    QRegularExpression regex(QString(".*%1.*\\n").arg(QRegularExpression::escape(newPattern)));
    fileContent.replace(regex, newString + "\n");
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        qDebug() << "Error opening file for writing:" << file.errorString();
        return;
    }

    QTextStream out(&file);
    out << fileContent;
    file.close();
    // qDebug() << "Content replaced successfully.";
}