// Copyright 2015-2016 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#include "iot_controllable_obj.h"

CControllable::~CControllable() {};

CPowerCtrl::CPowerCtrl(gpio_num_t io)
{
    io_num = io;
    gpio_config_t conf;
    conf.pin_bit_mask = (1ULL << io_num);
    conf.mode = GPIO_MODE_OUTPUT;
    conf.pull_up_en = (gpio_pullup_t) 0;
    conf.pull_down_en = (gpio_pulldown_t) 0;
    conf.intr_type = (gpio_int_type_t) 0;
    gpio_config(&conf);
}

void CPowerCtrl::on()
{
    gpio_set_level(io_num, 0);
}

void CPowerCtrl::off()
{
    gpio_set_level(io_num, 1);
}
