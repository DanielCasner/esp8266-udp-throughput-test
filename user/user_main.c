#include "mem.h"
#include "c_types.h"
#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "os_type.h"
#include "user_config.h"
#include "user_interface.h"
#include "espconn.h"


#define user_procTaskPrio        0
#define user_procTaskQueueLen    1
os_event_t    user_procTaskQueue[user_procTaskQueueLen];
static void loop(os_event_t *events);

static volatile os_timer_t some_timer;
static struct espconn *pUdpServer;
struct espconn *pespconn = NULL;

//Idle task
static void ICACHE_FLASH_ATTR
loop(os_event_t *events)
{
  char msg[1400]="I--DOWN THE RABBIT-HOLE\n\n\nAlice was beginning to get very tired of sitting by her sister on the\nbank, and of having nothing to do. Once or twice she had peeped into the\nbook her sister was reading, but it had no pictures or conversations in\nit, 'and what is the use of a book,' thought Alice, 'without pictures or\nconversations?'\n\nSo she was considering in her own mind (as well as she could, for the\nday made her feel very sleepy and stupid), whether the pleasure of\nmaking a daisy-chain would be worth the trouble of getting up and\npicking the daisies, when suddenly a White Rabbit with pink eyes ran\nclose by her.\n\nThere was nothing so very remarkable in that, nor did Alice think it so\nvery much out of the way to hear the Rabbit say to itself, 'Oh dear! Oh\ndear! I shall be too late!' But when the Rabbit actually took a watch\nout of its waistcoat-pocket and looked at it and then hurried on, Alice\nstarted to her feet, for it flashed across her mind that she had never\nbefore seen a rabbit with either a waistcoat-pocket, or a watch to take\nout of it, and, burning with curiosity, she ran across the field after\nit and was just in time to see it pop down a large rabbit-hole, under\nthe hedge. In another moment, down went Alice after it!\n\n[Illustration]\n\nThe rabbit-hole went straight on like a tunnel for some way and then\ndipped suddenly down, so suddenly that Alice had not a moment to think";

  if (pespconn) {
    espconn_sent(pespconn, msg, 1400);
  }

  system_os_post(user_procTaskPrio, 0, 0 );
}

//Called when new packet comes in.
static void ICACHE_FLASH_ATTR
udpserver_recv(void *arg, char *pusrdata, unsigned short len)
{
  char err = 0;
  pespconn = (struct espconn *)arg;

  REG_SET_BIT(0x3ff00014, BIT(0)); // Make sure we're still at speed
  //os_update_cpu_frequency(160);

  err = espconn_sent(pespconn, pusrdata, len);

  //os_printf("Received %d bytes and responded %d\r\n", len, err);
}

//Init function
void ICACHE_FLASH_ATTR
user_init()
{
    char err;

    REG_SET_BIT(0x3ff00014, BIT(0));
    os_update_cpu_frequency(160);

    uart_div_modify(0, UART_CLK_FREQ / 115200);

    os_delay_us(1000000);

    os_printf("\r\nBooting up...\r\n");

    // Create config for Wifi AP
    struct softap_config ap_config;

    os_strcpy(ap_config.ssid, "AnkiEspressif");

    os_strcpy(ap_config.password, "2manysecrets");
    ap_config.ssid_len = 0;
    ap_config.channel = 7;
    ap_config.authmode = AUTH_WPA2_PSK;
    ap_config.max_connection = 4;

    // Setup ESP module to AP mode and apply settings
    if (wifi_set_opmode(SOFTAP_MODE)) {
      os_printf("Successfully set up OPMODE\r\n");
    }
    else {
      os_printf("FAILED to set OPMODE\r\n");
    }
    if (wifi_softap_set_config(&ap_config)) {
      os_printf("Successfully set softap config\r\n");
    }
    else {
      os_printf("FAILED to set softap config\r\n");
    }
    if (wifi_set_phy_mode(PHY_MODE_11G)) {
      os_printf("Successfully set phy mode\r\n");
    }
    else {
      os_printf("FAILED to set phy mode\r\n");
    }

    // Create ip config
    struct ip_info ipinfo;
    ipinfo.gw.addr = ipaddr_addr("172.31.1.1");
    ipinfo.ip.addr = ipaddr_addr("172.31.1.1");
    ipinfo.netmask.addr = ipaddr_addr("255.255.255.0");

    // Assign ip config
    if (wifi_set_ip_info(SOFTAP_IF, &ipinfo)) {
      os_printf("Successfully set IP info\r\n");
    }
    else {
      os_printf("FAILED to set IP info\r\n");
    }

    // Start DHCP server
    if (wifi_softap_dhcps_start()) {
      os_printf("Successfully started DHCP server\r\n");
    }
    else {
      os_printf("FAILED to start DHCP server\r\n");
    }

    pUdpServer = (struct espconn *)os_zalloc(sizeof(struct espconn));
    ets_memset( pUdpServer, 0, sizeof( struct espconn ) );
    espconn_create( pUdpServer );
    pUdpServer->type = ESPCONN_UDP;
    pUdpServer->proto.udp = (esp_udp *)os_zalloc(sizeof(esp_udp));
    pUdpServer->proto.udp->local_port = 6661;
    espconn_regist_recvcb(pUdpServer, udpserver_recv);

    wifi_station_dhcpc_start();

    err = espconn_create( pUdpServer );
    if (err) {
      os_printf("FAILED to create UDP server, err %d\r\n", err);
    }
    else {
      os_printf("Successfully created UDP server\r\n");
    }

    //Start os task
    system_os_task(loop, user_procTaskPrio,user_procTaskQueue, user_procTaskQueueLen);
    system_os_post(user_procTaskPrio, 0, 0 );
}
