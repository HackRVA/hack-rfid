#include <pico/cyw43_arch.h>
#include <string.h>

/*
 * we should make this more robust.
 * it currently just connects an has no recovery if we disconnect for some
 * reason
 *
 * also, i think it breaks if we fail to connect to wifi.
 * if we fail to connect, the door lock should still operate.
 * see: https://www.raspberrypi.com/documentation/pico-sdk/networking.html
 * */
int wifi_init()
{
	/* skip if we don't have ssid or password set */
	if (strlen(WIFI_SSID) == 0 || strlen(WIFI_PASSWORD) == 0) {
		return 0;
	}
	if (cyw43_arch_init_with_country(CYW43_COUNTRY_USA)) {
		printf("failed to initialize wifi\n");
		return 1;
	}
	printf("wifi initialized\n");
	printf("connecting to ssid: %s\n", WIFI_SSID);
	cyw43_arch_enable_sta_mode();

	int status = cyw43_arch_wifi_connect_timeout_ms(
		WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 10000);
	if (status < 0) {
		printf("failed to connect: %d\n", status);
		return 1;
	}
	return 0;
}
