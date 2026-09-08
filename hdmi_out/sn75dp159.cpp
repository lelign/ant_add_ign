#include <QLoggingCategory>
#include "sn75dp159.h"
#include <QCoreApplication>
#include <iostream>


// 1. Создаем кастомный обработчик логов в C++ (вместо qputenv)
void myMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg) {
    QByteArray localMsg = msg.toLocal8Bit();
    QString categoryStr = context.category ? QString(context.category) : "default";

    switch (type) {
    case QtInfoMsg:
        if (categoryStr == "SUCCESS") {
            // Зеленый цвет для категории SUCCESS
            std::cout << "\033[32m[" << localMsg.constData() << "]\033[0m\n";
        } else {
            // Бирюзовый (Cyan) цвет для категории SN75DP159
            // Используем std::cout, чтобы \n в начале строки не ломал вывод
            std::cout << "\033[36m[" << categoryStr.toStdString() << "]\033[0m" << localMsg.constData() << "\n";
        }
        break;

    case QtWarningMsg:
        // Желтый цвет для WARNING
        std::cerr << "\033[33m[" << categoryStr.toStdString() << "]\033[0m" << localMsg.constData() << "\n";
        break;

    default:
        // Все остальные типы сообщений (debug, critical) выводятся как обычно
        std::cout << localMsg.constData() << "\n";
        break;
    }
}

static bool initLogging = []() {
    qInstallMessageHandler(myMessageOutput);
    return true;
}();


/*// 2. Инициализируем обработчик один раз до main() через static it'work but log message duplicated
static bool initLogging = []() {
    qInstallMessageHandler(myMessageOutput);
    return true;
}();

static bool initLoggingPattern = []() {
    return qputenv("QT_MESSAGE_PATTERN", 
        // --- Секция INFO ---
        "%{if-info}"
            "%{if-category(SUCCESS)}"
                "\033[32m[%{category}]\033[0m %{message}\n"
            "%{else}"
                "\033[36m[%{category}]\033[0m %{message}\n"
            "%{endif}"
        "%{endif}"
        
        // --- Секция WARNING ---
        "%{if-warning}"
            "\033[33m[%{category}]\033[0m %{message}\n"
        "%{endif}"
    );
}();*/


static QLoggingCategory infoCategory("SN75DP159 HDMI Retimer");
static QLoggingCategory warningCategory("WARNING");
static QLoggingCategory successCategory("SUCCESS"); // Будет обрабатываться как Info, но краситься в зеленый


Sn75dp159::Sn75dp159(uint8_t i2c_addr){
    sn75_Mode = QCoreApplication::arguments().contains("--sn75");
    if (sn75_Mode) {
        qCInfo(infoCategory) << "initialization sn159dp on " << i2c_filename_sn75dp << "addr" << Qt::hex << i2c_addr;
    }
    
    sn75dp_addr = i2c_addr;
    sn75dp_i2c = new I2c(i2c_filename_sn75dp);

    timer_update_sn75dp = new QTimer;
    connect(timer_update_sn75dp, SIGNAL(timeout()), this, SLOT(sn75dp159_device_id_read()));
    timer_update_sn75dp->setInterval(1000);
    timer_update_sn75dp->start();
}

Sn75dp159::~Sn75dp159(){
    delete sn75dp_i2c;
}

void Sn75dp159::sn75dp159_write(uint8_t reg_addr, uint8_t reg_data){
    int ret = sn75dp_i2c->write(sn75dp_addr, reg_addr, reg_data);
    if (ret == -1) qCWarning(warningCategory) << "sn75dp159_write i2c error"<< Qt::hex << sn75dp_addr;
}

void Sn75dp159::sn75dp159_read(uint8_t reg_addr, uint8_t *reg_data){
    int ret = (sn75dp_i2c->read(sn75dp_addr, reg_addr));
    if (ret == -1){
        qCWarning(warningCategory) << "sn75dp159_read i2c error" << Qt::hex << sn75dp_addr;
        *reg_data = 0xFF;
    }
    else *reg_data = (uint8_t)ret;
}

void Sn75dp159::sn75dp159_device_id_read(){
    // 1. Чтение Device ID (Регистры 0x00 - 0x04)
    uint8_t id_bytes[5];
    sn75dp159_read(0x00, &id_bytes[0]); // 'D'
    sn75dp159_read(0x01, &id_bytes[1]); // 'P'
    sn75dp159_read(0x02, &id_bytes[2]); // '1'
    sn75dp159_read(0x03, &id_bytes[3]); // '5'
    sn75dp159_read(0x04, &id_bytes[4]); // '9'

    // 2. Чтение Link State и PLL Configuration (Регистры 0x1C - 0x1D)
    uint8_t reg_1C_data = 0;
    uint8_t reg_1D_data = 0;
    sn75dp159_read(0x1C, &reg_1C_data);
    sn75dp159_read(0x1D, &reg_1D_data);

    if (sn75_Mode) {
        // Конвертируем ID в строку QString
        QString deviceId = QString::fromLatin1(reinterpret_cast<const char*>(id_bytes), 5);
        
        // --- Валидация 1: Проверка модели чипа ---
        if (deviceId != "DP159") {
            qCritical() << "CRITICAL: Device ID mismatch! Expected 'DP159', got:" << deviceId;
            return;
        } 
        
        // Вывод сырых данных регистров линка
        qCInfo(infoCategory) << "\nsn75dp159_device_id_read:" << deviceId << "i2c"  << Qt::hex << sn75dp_addr <<
            "Link State 0x1C:" << Qt::hex << reg_1C_data << "PLL Config 0x1D:" << Qt::hex << reg_1D_data;

        // --- ИСПРАВЛЕНО: Правильные аппаратные маски Texas Instruments для SN75DP159 ---
        // Бит 7 (0x80) - SIG_DET_CH_CLK: Входящий Pixel Clock от FPGA физически обнаружен
        bool rx_clk_detected = (reg_1C_data & 0x80) != 0; 
        
        // Бит 3 (0x08) - VGA_PLL_LOCK: Внутренний PLL ретаймера успешно захватил частоту
        bool pll_locked = (reg_1C_data & 0x08) != 0; 

        // --- Валидация 2: Анализ стабильности видеосигнала ---
        if (!rx_clk_detected) {
            qCWarning(warningCategory) << "No input video clock detected on SN75DP159! Check layout or FPGA DMA trigger.";
            
            // Принудительно выключаем трансляцию, если клок пропал, чтобы защитить монитор
            sn75dp159_write(0x09, 0x00); 
        } else if (!pll_locked) {
            qCWarning(warningCategory) << "Video clock detected, but PLL failed to LOCK! Signal might be unstable.";
        } else {
            qCInfo(successCategory) << "SN75DP159 on "<< Qt::hex << sn75dp_addr<<" Link is stable. Clock detected and PLL locked.";
            
            // АВТОМАТИЧЕСКОЕ ПРОБУЖДЕНИЕ: Включаем TMDS линии, если клок и PLL стабильны
            // Регистр 0x09: 0x01 = Активация HDMI режима и открытие выходных каскадов
            sn75dp159_write(0x09, 0x01); 
            
            // Регистр 0x0A: 0x13 = Применение настроек задержек линий и запуск автокалибровки
            sn75dp159_write(0x0A, 0x13);
        }
    }
}

void Sn75dp159::slot_sn75dp159_update(int rate){
    uint8_t data;
	
    if (rate == RATE_12G_12170) sn75dp159_write(0x0C, 0x01);	//PRE_SEL = Reg0Ch[1:0] = 01 (labeled HDMI_TWPST)
    else sn75dp159_write(0x0C, 0xFC);							//VSWING_DATA & VSWING_CLK to -7% = Reg0Ch[7:2] = 111111
																//PRE_SEL = Reg0Ch[1:0] = 00: (Labeled HDMI_TWPST)
    sn75dp159_read(0x0B, &data);
    data &= (1 << 1);

    switch (rate){
        case RATE_12G_12170:
        case RATE_6G_12170:
            data |= 0x98;
            break;
		
        case RATE_3G_12170:
            data |= 0x88;
            break;
		
        case RATE_HD_12170:
            data |= 0x80;
            break;
    }
    sn75dp159_write(0x0B, data);	//TX_TERM_CTL = Reg0Bh[4:3] = 11, SLEW_CTL = Reg0Bh[7:6] = 10
    sn75dp159_write(0x0A, 0x3D);	//APPLY_RXTX_CHANGES set to 1 and other default set
}
