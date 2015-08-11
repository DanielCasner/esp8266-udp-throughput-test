#include "c_types.h"
#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "os_type.h"
#include "mem.h"
#include "user_config.h"
#include "user_interface.h"
#include "espconn.h"
#include "driver/uart.h"

static volatile os_timer_t userTimer;
static struct espconn *pUdpServer;

//Called when new packet comes in.
static void udpserver_recv(void *arg, char *pusrdata, unsigned short len)
{
  espconn_send((struct espconn *)arg, (uint8*)pusrdata, len);
}

//Init function
void ICACHE_FLASH_ATTR
user_init()
{
    int8 err;

    REG_SET_BIT(0x3ff00014, BIT(0));
    system_update_cpu_freq(160);

    uart_init(BIT_RATE_115200, BIT_RATE_115200);

    os_printf("\r\nBooting up...\r\n");

    // Create ip config
    struct ip_info ipinfo;
    wifi_softap_dhcps_stop();
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

    // Create config for Wifi AP
    struct softap_config ap_config;

    wifi_softap_get_config(&ap_config);
    os_sprintf((char*)ap_config.ssid, SSID);
    os_sprintf((char*)ap_config.password, SSID_PASSWORD);
    ap_config.ssid_len = 0;
    ap_config.channel = 7;
    ap_config.authmode = AUTH_WPA2_PSK;
    ap_config.max_connection = 4;

    // Setup ESP module to AP mode and apply settings
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
    if (wifi_set_opmode(SOFTAP_MODE)) {
      os_printf("Successfully set up OPMODE\r\n");
    }
    else {
      os_printf("FAILED to set OPMODE\r\n");
    }

    pUdpServer = (struct espconn *)os_zalloc(sizeof(struct espconn));
    ets_memset( pUdpServer, 0, sizeof( struct espconn ) );
    espconn_create( pUdpServer );
    pUdpServer->type = ESPCONN_UDP;
    pUdpServer->proto.udp = (esp_udp *)os_zalloc(sizeof(esp_udp));
    pUdpServer->proto.udp->local_port = 5551;
    espconn_regist_recvcb(pUdpServer, udpserver_recv);

    err = espconn_create( pUdpServer );
    if (err) {
      os_printf("FAILED to create UDP server\r\n");
    }
    else {
      os_printf("Successfully created UDP server\r\n");
    }
}
