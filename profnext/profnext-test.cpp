#include <QDebug>
#include "cpu-cmd.h"
#include "profnext-test.h"

void ProfnextTest::timer_timeout()
{
        static uint8_t i = 0, ch = 0;
        // qDebug() << "timer";

        union{
                char data[VAL_MAX_SIZE];
                uint16_t val;
        } profnext_test_status;
        memset(&profnext_test_status.data, 0, VAL_MAX_SIZE);
        T_NODE_DYN_ATTR cmd_attr;
        cmd_attr.fDisabled = 0;
        static uint8_t prolex_header = 0xF0;

        if (i == 0){
                //статус модуля в profnext
                profnext_test_status.val = PERIPH_STATUS_OK;
                panel->cmd_profnext(TYPE_STATUS_P, STATUS_IDX, &cmd_attr, profnext_test_status.data);
                //enable input and bars for proflex header status
                panel->cmd_profnext(TYPE_STATUS_PX_P, STATUS_PX_IDX, &cmd_attr, profnext_test_status.data);
                i ++;
        }
        else if (i == 1)
        {
                //status -> ready
                profnext_test_status.val = 1;
                panel->cmd_profnext(TYPE_INDEXED_STRING_P, CMD_0, &cmd_attr, profnext_test_status.data);
                i ++;
        }

}

void ProfnextTest::indexed_string_event(uint8_t idx, uint16_t val)
{
        union{
                char data[VAL_MAX_SIZE];
                uint16_t val;
        } profnext_test_status;
        memset(&profnext_test_status.data, 0, VAL_MAX_SIZE);
        T_NODE_DYN_ATTR cmd_attr;
        cmd_attr.fDisabled = 0;


        //обработка комманды типа ресет
        if (idx == CMD_1 && val){
                qDebug() << "reset recieved";
                profnext_test_status.val = 0;
                panel->cmd_profnext(TYPE_INDEXED_STRING_P, (T_CMD_NUM)idx, &cmd_attr, profnext_test_status.data);
        }
}

ProfnextTest::ProfnextTest()
{
        panel = new ProfnextFrontPanel("/dev/ttyS1");
        timer = new QTimer();
        connect(timer, &QTimer::timeout, this, &ProfnextTest::timer_timeout);
        connect(panel, &ProfnextFrontPanel::indexed_string_event, this, &ProfnextTest::indexed_string_event);
        timer->start(TIMER_TEST_VAL);        
}

ProfnextTest::~ProfnextTest()
{
        delete panel;
}
