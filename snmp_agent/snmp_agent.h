#ifndef SNMP_AGENT_H
#define SNMP_AGENT_H

#include <QObject>
#include <QLocalSocket>
#include <qdebug.h>
#include <QTimer>

#define SERVER_NAME "/tmp/sock.unix"

struct oid_object_t {
    QString key;
    QString value;
};

typedef QList<oid_object_t> MIB_t;
typedef QPair<QString, QString> pair_t;
typedef MIB_t::iterator iterator_MIB_t;

class Snmp_Agent : public QObject
{
    Q_OBJECT
public:
    explicit Snmp_Agent(QObject *parent = 0);

    void set_device_ID(int device_ID);
    void set_mib_element_int(QString key, int value);
    void set_mib_element_int32(QString key,  qint32 value);
    void set_mib_element_str(QString key, QString str);
    QString get_oid_element(QString key);

private:
    QLocalSocket *socket;
    QTimer timer_reconnect;
    void parse_datagram(QString str);
    void parse_command(QString str);
    void write_to_socket(QString str);
    void cmd_get(QString str);
    void cmd_set(QString str);
    void cmd_get_next(QString str);
    iterator_MIB_t get_index(QString key);
    bool device_node(QString str);

    int cmd_type;
    QString OID_node, OID_node_dot;
    MIB_t MIB_map;
    iterator_MIB_t pnt_MIB_map, iterator_to_set;

    oid_object_t oid_object;

    void MIB_map_append(QString key, QString value);
    iterator_MIB_t find_index(QString key);
    QTextStream * in;

signals:
    void signal_set_val(QString key, QString val);

public slots:

    void slot_connected();
    void slot_disconnected();
    void slot_readData();
    void slot_timer_reconnect();
};

#endif // SNMP_AGENT_H
