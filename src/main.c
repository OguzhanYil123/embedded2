#include <zephyr.h>
#include <device.h>
#include <drivers/sensor.h>
#include <stdio.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/uuid.h>
#include <tinycrypt/aes.h>
#include <tinycrypt/constants.h>
#include <stdio.h>






#define SERVICE_DATA_LEN        9
#define SERVICE_UUID            0xfcd2     
#define IDX_TEMPL               4          
#define IDX_TEMPH               5          
#define IDX_HUML                7
#define IDX_HUMH                8

#define ADV_PARAM BT_LE_ADV_PARAM(BT_LE_ADV_OPT_USE_IDENTITY, BT_GAP_ADV_SLOW_INT_MIN, BT_GAP_ADV_SLOW_INT_MAX, NULL)

const uint8_t key[16] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF, 0xFE, 0xDC, 0xBA, 0x98, 0x76, 0x54, 0x32, 0x10};

static const char *now_str(void) {
	static char buf[16];
	uint32_t now = k_uptime_get_32();
	unsigned int ms = now % MSEC_PER_SEC;
	unsigned int s;
	unsigned int min;
	unsigned int h;
	now /= MSEC_PER_SEC;
	s = now % 60U;
	now /= 60U;
	min = now % 60U;
	now /= 60U;
	h = now;
	snprintf(buf, sizeof(buf), "%u:%02u:%02u.%03u",
		 h, min, s, ms);
	return buf;
}

static uint8_t service_data[SERVICE_DATA_LEN] = {
	BT_UUID_16_ENCODE(SERVICE_UUID),
	0x40,
	0x02,	/* Temperature */
	0xc4,	/* Low byte */
	0x00,   /* High byte */
	0x03,	/* Humidity */
	0xbf,	/* hum  low byte*/
	0x13,   /* hum high byte*/
};

static struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR),
	BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1),
	BT_DATA(BT_DATA_SVC_DATA16, service_data, ARRAY_SIZE(service_data))
};

static void bt_ready(int err) {
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}
	printk("Bluetooth initialized\n");
	err = bt_le_adv_start(ADV_PARAM, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err) {
		printk("Advertising failed to start (err %d)\n", err);
		return;
	}
}


void main(void) {
	const char *const label = DT_LABEL(DT_INST(0, aosong_dht));
	const struct device *dht22 = device_get_binding(label);
	if (!dht22) {
		printf("Failed to find sensor %s\n", label);
		return;
	}

    struct tc_aes_key_sched_struct sched;
	uint8_t cipherTextTemp[16];
	uint8_t cipherTextHum[16];

    tc_aes128_set_encrypt_key(&sched, key);

	int err;
	int temp;
	int hum;

	/*
	uint8_t temp;
	uint8_t hum;
	*/

	err = bt_enable(bt_ready);
	if (err) {
		printf("Bluetooth init failed (err %d)\n", err);
		return 0;
	}
	
	while (true) {
	int rc = sensor_sample_fetch(dht22);
	struct sensor_value temperature;
	struct sensor_value humidity;
	rc = sensor_channel_get(dht22, SENSOR_CHAN_AMBIENT_TEMP, &temperature);
	    if (rc == 0) {
			rc = sensor_channel_get(dht22, SENSOR_CHAN_HUMIDITY,
						&humidity);
		}
		if (rc != 0) {
			printf("get failed: %d\n", rc);
			break;
		}




	uint8_t plainTextTemp[16] = {sensor_value_to_double(&temperature)};
	uint8_t plainTextHum[16] = {sensor_value_to_double(&humidity)};
	
	tc_aes_encrypt(cipherTextTemp, plainTextTemp, &sched);
	tc_aes_encrypt(cipherTextHum, plainTextHum, &sched);
	
    printf("\n");
	printf("[%s]: %.1f Cel ; %.1f %%RH\n", now_str(), sensor_value_to_double(&temperature), sensor_value_to_double(&humidity));
	printf("TEMP: ");
	for (int i = 0; i < sizeof(cipherTextTemp); i++) {
		printf("%02X ", cipherTextTemp[i]);
	}
	printf("\n");
	printf("HUM: ");
	for (int i = 0; i < sizeof(cipherTextHum); i++) {
		printf("%02X ", cipherTextHum[i]);
		}
	printf("\n");
	printf("******************************************************");


	service_data[IDX_TEMPH] = (temp * 100) >> 8;
	service_data[IDX_TEMPL] = (temp * 100) & 0xff;
	service_data[IDX_HUMH] = (hum * 100) >> 8;
	service_data[IDX_HUML] = (hum * 100) & 0xff;

	temp = sensor_value_to_double(&temperature);
	hum = sensor_value_to_double(&humidity);




    err = bt_le_adv_update_data(ad, ARRAY_SIZE(ad), NULL, 0);
	if (err) {printk("Failed to update advertising data (err %d)\n", err);}
	
	k_sleep(K_SECONDS(2));
	
	
		


}
}


	
