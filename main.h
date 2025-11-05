#pragma once
#include <vector>
#include "rom.h"
#include "bus.h"

class nespc{
    public:
        bus nes;
        std::shared_ptr<rom> cart;
};