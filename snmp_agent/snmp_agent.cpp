#include "snmp_agent.h"

#define OID_PROFITT ".1.3.6.1.4.1.52035"
#define PONG     "PONG"
#define PING     "PING"
#define GET      "get"
#define SET      "set"
#define GET_NEXT "getnext"

#define NONE    "none"
#define INTEGER "integer"
#define OK      "OK"

#define DO_RECONNECT_TIME 1000

enum {CMD_GET, CMD_SET, CMD_GET_NEXT};

Snmp_Agent::Snmp_Agent(QObject *parent) : QObject(parent)
{
    cmd_type = CMD_GET;

    qDebug() << "snmp agent created";

    socket = new QLocalSocket(this);
    in = new QTextStream(socket);
    connect(socket, SIGNAL(connected   ()), this, SLOT(slot_connected   ()));
    connect(socket, SIGNAL(disconnected()), this, SLOT(slot_disconnected()));
    connect(socket, SIGNAL(readyRead   ()), this, SLOT(slot_readData    ()));
    socket->connectToServer(SERVER_NAME);

    connect(&timer_reconnect, SIGNAL(timeout()), this, SLOT(slot_timer_reconnect()));
    timer_reconnect.setInterval(DO_RECONNECT_TIME);
    if(socket->state() == QLocalSocket::UnconnectedState)
        timer_reconnect.start();

}


void Snmp_Agent::slot_timer_reconnect()
{
    socket->connectToServer(SERVER_NAME);
}


void Snmp_Agent::slot_connected()
{
    timer_reconnect.stop();
}


void Snmp_Agent::slot_disconnected()
{
    timer_reconnect.start();
}

void Snmp_Agent::slot_readData()
{
    QString str;

    while(!in->atEnd())
    {
       str = in->readLine();
       parse_datagram(str);
    }
}


void Snmp_Agent::write_to_socket(QString str)
{
    str += "\n";
    QByteArray arr = str.toStdString().c_str();
    socket->write(arr);
}

void Snmp_Agent::MIB_map_append(QString key, QString value)
{
oid_object_t obj;
iterator_MIB_t it_MIB;

    obj.key   = key;
    obj.value = value;

    it_MIB = find_index(key);
    if(it_MIB == MIB_map.end()){
        MIB_map.append(obj);
        pnt_MIB_map = MIB_map.end();
    }
    else{
        *it_MIB = obj;
    }
}


void Snmp_Agent::set_mib_element_int32(QString key,  qint32 value)
/*  Добавляет в базу данных (Managment Information Base)
 *  длинное целое значение параметра или настройки устройства
 */
{
    QString t = "Integer32\n" + QString::number(value);
    MIB_map_append(key, t);
}

void Snmp_Agent::set_mib_element_int(QString key, int value)
/*  Добавляет в базу данных (Managment Information Base)
 *  целое значение параметра или настройки устройства
 */
{
//oid_object_t obj;

    QString t = "INTEGER\n" + QString::number(value);
    MIB_map_append(key, t);
}

void Snmp_Agent::set_mib_element_str(QString key, QString str)
/*  Добавляет в базу данных (Managment Information Base)
 *  строковое значение параметра устройства
 */
{
    QString t = "string\n" + str;
    MIB_map_append(key, t);
}


iterator_MIB_t Snmp_Agent::find_index(QString key)
{
oid_object_t elm;
iterator_MIB_t index;

    index = MIB_map.begin();

    foreach( elm,  MIB_map){
        if((elm.key.compare(key, Qt::CaseInsensitive) == 0)) return index;
        index++;
    }

    return MIB_map.end();
}

QString Snmp_Agent::get_oid_element(QString key)
{
iterator_MIB_t it_MIB;

    key.remove(OID_node + ".");

    it_MIB = find_index(key);
    if(it_MIB == MIB_map.end())
        return NONE;
    else
        return it_MIB->value;
}


void Snmp_Agent::set_device_ID(int device_ID)
{
    OID_node  = OID_PROFITT;
    OID_node += "." + QString::number(device_ID);

    OID_node_dot = OID_PROFITT;
    OID_node_dot += "." + QString::number(device_ID) + ".";
}


void Snmp_Agent::parse_datagram(QString str)
{
    if(str.compare(PING, Qt::CaseInsensitive) == 0){
        write_to_socket(PONG);
        return;
    }

    if(str.compare(GET, Qt::CaseInsensitive) == 0){
        cmd_type = CMD_GET;
        return;
    }

    if(str.compare(SET, Qt::CaseInsensitive) == 0){
        cmd_type = CMD_SET;
        return;
    }

    if(str.compare(GET_NEXT, Qt::CaseInsensitive) == 0){
        cmd_type = CMD_GET_NEXT;
        return;
    }

    parse_command(str);
}


void Snmp_Agent::parse_command(QString str)
{
    switch (cmd_type) {
        case CMD_GET:
            cmd_get(str);
            break;
        case CMD_SET:
            cmd_set(str);
            break;
        case CMD_GET_NEXT:
            cmd_get_next(str);
            break;
        default:
            break;
    }
}


void Snmp_Agent::cmd_get(QString str)
{
QString val;

    val = get_oid_element(str);
    if(val != NONE) write_to_socket(str);

    write_to_socket(val);
}


iterator_MIB_t Snmp_Agent::get_index(QString key)
{
    key.remove(OID_node_dot);
    return find_index(key);
//    return MIB_map.find(key);
}


bool Snmp_Agent::device_node(QString str)
{
    return str.contains(OID_node_dot, Qt::CaseInsensitive);
}


void Snmp_Agent::cmd_set(QString str)
{
    // установить указатель на элемент
    if(device_node(str)){
        iterator_to_set = get_index(str);
        return;
    }

    if(iterator_to_set == MIB_map.end()){
        write_to_socket(NONE);
        return;
    }

    write_to_socket(OK);
    emit signal_set_val(iterator_to_set->key, str);
}


void Snmp_Agent::cmd_get_next(QString str)
{
    if(pnt_MIB_map != MIB_map.end())
        pnt_MIB_map++;

    if(str.compare(OID_node, Qt::CaseInsensitive) == 0)
        pnt_MIB_map = MIB_map.begin();

    if(str.compare(OID_PROFITT, Qt::CaseInsensitive) == 0)
        pnt_MIB_map = MIB_map.begin();

    if(pnt_MIB_map == MIB_map.end()){
        write_to_socket(NONE);
        return;
    }

    QString  key = OID_node + "." + pnt_MIB_map->key;
    write_to_socket(key);
    write_to_socket(pnt_MIB_map->value);

}
