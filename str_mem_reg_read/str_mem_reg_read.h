#ifndef STR_MEM_REG_READ_H
#define STR_MEM_REG_READ_H

#include <stdint.h>

// Чистый C++ класс, не привязанный к архитектуре Qt
class StrMemRegRead 
{
public:
    StrMemRegRead(); // Обычный конструктор без параметров
    
    uint32_t reg_read(uint32_t block, uint32_t addr);
};

#endif // STR_MEM_REG_READ_H
