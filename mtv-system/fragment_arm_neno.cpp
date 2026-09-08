        // Обрабатываем по 8 пикселей за итерацию
        /*        for(int x = 0; x < width; x += 8) {
                // Загружаем ARGB (4 канала). val[3] содержит Alpha, если он нужен драйверу в будущем
                uint8x8x4_t rgb = vld4_u8(line + x * 4);

                int16x8_t r = vreinterpretq_s16_u16(vmovl_u8(rgb.val[2]));
                int16x8_t g = vreinterpretq_s16_u16(vmovl_u8(rgb.val[1]));
                int16x8_t b = vreinterpretq_s16_u16(vmovl_u8(rgb.val[0]));

                // Расчет компоненты Y
                int16x8_t y_acc = vmulq_s16(r, y_r);
                y_acc = vmlaq_s16(y_acc, g, y_g);
                y_acc = vmlaq_s16(y_acc, b, y_b);
                uint8x8_t y_val = vqmovun_s16(vshrq_n_s16(y_acc, 8));

                // Расчет компоненты Cb
                int16x8_t cb_acc = vmulq_s16(r, cb_r);
                cb_acc = vmlaq_s16(cb_acc, g, cb_g);
                cb_acc = vmlaq_s16(cb_acc, b, cb_b);
                uint8x8_t cb_val = vqmovun_s16(vaddq_s16(vshrq_n_s16(cb_acc, 8), vmovq_n_s16(128)));

                // Расчет компоненты Cr
                int16x8_t cr_acc = vmulq_s16(r, cr_r);
                cr_acc = vmlaq_s16(cr_acc, g, cr_g);
                cr_acc = vmlaq_s16(cr_acc, b, cr_b);
                uint8x8_t cr_val = vqmovun_s16(vaddq_s16(vshrq_n_s16(cr_acc, 8), vmovq_n_s16(128)));

                // Упаковываем в формат 3 байта на пиксель: [Y][Cr][Cb]
                // Примечание: Если вашему драйверу нужен порядок Y, Cb, Cr — просто поменяйте местами присвоения ниже
                uint8x8x3_t ycrcb_struct;
                ycrcb_struct.val[0] = y_val;
                ycrcb_struct.val[1] = cr_val;
                ycrcb_struct.val[2] = cb_val;

                // Сохраняем 8 пикселей в память интерливом (8 пикселей * 3 байта = 24 байта)
                vst3_u8(dst, ycrcb_struct);

                // Сдвигаем указатель назначения на 24 байта вперед
                dst += 24;
        }
*/
        // Исправленный цикл внутри convert_line