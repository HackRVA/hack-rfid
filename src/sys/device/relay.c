#include "pico/cyw43_arch.h"

/*using the builtin LED until we actually hook up a relay*/
void relay_init()
{
	/*if (cyw43_arch_init()) {*/
	/*  printf("Failed to initialize Wi-Fi chip\n");*/
	/*  return;*/
	/*}*/
	/*cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);*/
}
void relay_enable()
{
	cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
}
void relay_disable()
{
	cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
}
