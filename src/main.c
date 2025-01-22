#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "acl.h"
#include "sys/rfid_reader.h"
#include "sys/sys.h"

struct rfid_reader reader;
struct access_control_list acl;

void init_acl()
{
	acl.file_path = "acl";
	acl.user_count = 0;

	acl_append_user(&acl, "fakefob1");
	acl_append_user(&acl, "fakefob2");
	acl_append_user(&acl, "fakefob3");
	acl_append_user(&acl, "dbe8893f85");
}

void load_acl()
{
	struct access_control_list loaded_acl;
	acl_load(&loaded_acl, "acl");

	/*acl_print(&loaded_acl);*/
	uint32_t hash = acl_hash(&loaded_acl);
	printf("ACL Hash: %u\n", hash);
}

#ifdef __PICO_BUILD__
#include "pico/stdlib.h"
#include "sys/device/relay.h"

void test_uid(struct access_control_list *acl, char *uid)
{
	if (acl_has_user(acl, uid)) {
		printf("user %s exists\n", uid);

		// can we open the relay without stopping the universe?
		relay_enable();
		sleep_ms(3000);
		relay_disable();
	} else {
		printf("user %s doesn't exist\n", uid);
	}
}
void init()
{
	init_acl();
	relay_init();
	rfid_reader_init(&reader);
}

void update_reader()
{
	uint32_t hash = acl_hash(&acl);
	printf("ACL Hash: %u\n", hash);
	if (rfid_reader_wait_for_card(&reader, 500) == 0) {
		printf("Card detected. Reading UID...\n");

		char uid[11];
		if (rfid_reader_read(&reader, uid) == 0) {
			test_uid(&acl, uid);
		} else {
			printf("Failed to read card.\n");
		}
	} else {
		/*printf("No card detected.\n");*/
	}
}
#else
void init()
{
	init_acl();
}
void update_reader()
{
	acl_print(&acl);
	uint32_t hash = acl_hash(&acl);
	printf("ACL Hash: %u\n", hash);
}
#endif

int counter = 0;

/*
 * we need to be able to check if systems are running.
 * If they aren't running, we need to restart them.
 *
 * e.g. If we determine that we aren't connected to wifi, reconnect.
 *
 * Things should run on an interval and not block.
 */
void update()
{
	counter++;

	if (counter > 100) {
		counter = 0;
	}
	printf("counter: %d\n", counter);

	// connect to wifi
	// initialize event manager
	//
	// loop on an interval
	// we should check if a card is being read
	update_reader();
	//
	// on a less frequent interval we should check that we still have wifi
	// connection
	//
	// do heartbeat
	//
	// check wifi connection
}

int main()
{
	sys_init();
	init();
	sys_run(update);
	return 0;
}
