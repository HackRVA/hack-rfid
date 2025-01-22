#include "sys.h"

#ifdef __PICO_BUILD__
#include "hardware/spi.h"
#include "pico/stdlib.h"
#include <stdio.h>

#include "fs.h"
#include "wifi.h"

void sys_init()
{
	stdio_init_all();

	sleep_ms(500);
#if WITH_FS
	fs_init();
#endif
	sleep_ms(5000);

	wifi_init();
}

void sys_run(callback_func update_callback)
{
	if (!update_callback) {
		fprintf(stderr, "Error: Callback function cannot be NULL\n");
		return;
	}

	printf("Waiting for a card...\n");

	while (true) {
		update_callback();
		sleep_ms(1000);
	}
}

#else // Linux

#include <stdio.h>
#include <unistd.h>

void sys_init()
{
	printf("System initialized for Linux.\n");
}

void sys_run(callback_func update_callback)
{
	if (!update_callback) {
		fprintf(stderr, "Error: Callback function cannot be NULL\n");
		return;
	}

	printf("Running on Linux...\n");

	while (1) {
		update_callback();
		sleep(1);
	}
}
#endif
