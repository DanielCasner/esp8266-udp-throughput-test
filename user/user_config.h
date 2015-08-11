#define SSID "UDPThroughput"
#define SSID_PASSWORD "2manysecrets"

/// Put format strings in flash rather than needing to load them into program memory
#define USE_OPTIMIZE_PRINTF
/// Add missing prototype of os_printf_plus
extern int os_printf_plus(const char * format, ...) __attribute__ ((format (printf, 1, 2)));
