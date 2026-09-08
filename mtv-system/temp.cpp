void PbxMtvSystem::draw_overlay(QImage * image, int offset_x, int offset_y)
{
        // 1. ЗАЩИТА ОТ NULL-УКАЗАТЕЛЕЙ
        if (!image) {
                qCritical() << "CRITICAL ERROR: QImage pointer is NULL!";
                return;
        }
        if (!buffer) {
                qCritical() << "CRITICAL ERROR: Output buffer pointer is NULL!";
                return;
        }
       
        // 3. ПРОВЕРКА СТРУКТУРЫ И ФОРМАТА QIMAGE
        if (image->format() != QImage::Format_ARGB32 && image->format() != QImage::Format_RGB32) {
                qCritical() << "WARNING: QImage format is not ARGB32/RGB32! Current format:" << image->format();
                // Если формат не 32-битный, constScanLine(y) + x*4 гарантированно вызовет краш
        }

        

        
        Q_ASSERT(image->width()+offset_x<=1920);// Данная конкретная строчка проверяет, что правая граница рисуемой или обрабатываемой картинки с учетом её смещения по оси X не выходит за пределы разрешения Full HD (1920 пикселей).
        Q_ASSERT(image->height()+offset_y<=1080);

        uint8_t* start_address = reinterpret_cast<uint8_t*>(buffer);
        int row_stride = 1920 * 2; // 3840 байт на строку YUYV
        // 2. Получаем реальные размеры картинки QImage
        int img_w = image->width();  // 480
        int img_h = image->height(); // 150
        // 3. БЕЗОПАСНОСТЬ: Проверяем, чтобы картинка со смещением не вылезла за границы Full HD экрана
        if ((offset_x + img_w) > 1920 || (offset_y + img_h) > 1080) {
        qCritical() << "CRITICAL ERROR: Image with offsets goes out of Full HD bounds!";
        return;
        }

         // ЦИКЛ ОТРИСОВКИ КАРТИНКИ СО СМЕЩЕНИЕМ
        for (int y = 0; y < img_h; ++y) {
        
                // Вычисляем глобальный Y на экране с учетом вертикального смещения
                int screen_y = y + offset_y;
                
                // Находим базовый адрес начала этой строки на экране
                uint8_t * row_start_address = start_address + (screen_y * row_stride);
                
                // Сдвигаем указатель вправо внутри строки на значение offset_x
                // Так как 1 пиксель YUYV = 2 байта, умножаем offset_x на 2
                uint8_t * current_row_with_offset = row_start_address + (offset_x * 2);
                
                // Передаем в NEON-конвертер реальную ширину картинки (480)
                convert_line(image, y, img_w, current_row_with_offset);
 
        }
        // frame is ready
        // set flag in position 1920*1080*3 = 6220800 +1024 = 6221824 - 8 = 6221816
        uint32_t *frame_ready_flag = reinterpret_cast<uint32_t*>(start_address + 6221816);
        *frame_ready_flag = 1; // Сигнализируем утилите: "Кадр готов, забирай дамп!"
}

void PbxMtvSystem::convert_line(QImage * img, int y, int width, uint8_t * buffer)
{
        // 2. ПРОВЕРКА ГЕОМЕТРИИ И ГРАНИЦ КАДРА
        if (y < 0 || y >= img->height()) {
                qCritical() << "CRITICAL ERROR: Requested 'y' line" << y 
                            << "is out of QImage bounds (height:" << img->height() << ")";
                return;
        }
        if (width <= 0 || width > img->width() || width > 1920) {
                qCritical() << "CRITICAL ERROR: Invalid width:" << width 
                            << "(QImage width:" << img->width() << ")";
                return;
        }

        uint8_t * dst = buffer;

        const uint8_t * line = img->constScanLine(y);
        if (!line) {
                qCritical() << "CRITICAL ERROR: constScanLine(" << y << ") returned NULL!";
                return;
        }

        // Коэффициенты BT.601
        int16x8_t y_r = vmovq_n_s16(77);
        int16x8_t y_g = vmovq_n_s16(150);
        int16x8_t y_b = vmovq_n_s16(29);
        int16x8_t cb_r = vmovq_n_s16(-43);
        int16x8_t cb_g = vmovq_n_s16(-85);
        int16x8_t cb_b = vmovq_n_s16(128);
        int16x8_t cr_r = vmovq_n_s16(128);
        int16x8_t cr_g = vmovq_n_s16(-107);
        int16x8_t cr_b = vmovq_n_s16(-21);

        for(int x = 0; x < width; x += 8) {
                uint8x8x4_t rgb = vld4_u8(line + x * 4);

                int16x8_t r = vreinterpretq_s16_u16(vmovl_u8(rgb.val[2]));
                int16x8_t g = vreinterpretq_s16_u16(vmovl_u8(rgb.val[1]));
                int16x8_t b = vreinterpretq_s16_u16(vmovl_u8(rgb.val[0]));

                int16x8_t y_acc = vmulq_s16(r, y_r);
                y_acc = vmlaq_s16(y_acc, g, y_g);
                y_acc = vmlaq_s16(y_acc, b, y_b);
                uint8x8_t y_val = vqmovun_s16(vshrq_n_s16(y_acc, 8));

                int16x8_t cb_acc = vmulq_s16(r, cb_r);
                cb_acc = vmlaq_s16(cb_acc, g, cb_g);
                cb_acc = vmlaq_s16(cb_acc, b, cb_b);
                uint8x8_t cb_val = vqmovun_s16(vaddq_s16(vshrq_n_s16(cb_acc, 8), vmovq_n_s16(128)));

                int16x8_t cr_acc = vmulq_s16(r, cr_r);
                cr_acc = vmlaq_s16(cr_acc, g, cr_g);
                cr_acc = vmlaq_s16(cr_acc, b, cr_b);
                uint8x8_t cr_val = vqmovun_s16(vaddq_s16(vshrq_n_s16(cr_acc, 8), vmovq_n_s16(128)));

                uint8x8x2_t cb_pairs = vuzp_u8(cb_val, cb_val); 
                uint8x8x2_t cr_pairs = vuzp_u8(cr_val, cr_val); 

                uint8x8x2_t uv_interleaved = vzip_u8(cb_pairs.val[0], cr_pairs.val[0]);
                uint8x8_t uv_val = uv_interleaved.val[0]; 

                uint8x8x2_t yuyv_struct;
                yuyv_struct.val[0] = y_val;
                yuyv_struct.val[1] = uv_val;

                uint8x8x2_t packed = vzip_u8(yuyv_struct.val[0], yuyv_struct.val[1]);

                vst1_u8(dst, packed.val[0]);
                vst1_u8(dst + 8, packed.val[1]);

                dst += 16;
        }
}