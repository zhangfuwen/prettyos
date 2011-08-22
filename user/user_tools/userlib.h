#ifndef USERLIB_H
#define USERLIB_H

#include "types.h"


// syscalls (only non-standard functions, because we do not want to include stdio.h here.
FS_ERROR execute(const char* path, size_t argc, char* argv[]);
void exit();
bool wait(BLOCKERTYPE reason, void* data, uint32_t timeout);
uint32_t getMyPID();

void* userheapAlloc(size_t increase);

FS_ERROR partition_format(const char* path, FS_t type, const char* name);

bool waitForEvent(uint32_t timeout);
void event_enable(bool b);
EVENT_t event_poll(void* destination, size_t maxLength, EVENT_t filter);

uint32_t getCurrentMilliseconds();

void systemControl(SYSTEM_CONTROL todo);

void textColor(uint8_t color);
void setScrollField(uint8_t top, uint8_t bottom);
void setCursor(position_t pos);
void getCursor(position_t* pos);
void clearScreen(uint8_t backgroundColor);
void console_setProperties(console_properties_t properties);
void refreshScreen();

bool keyPressed(KEY_t key);

void beep(uint32_t frequency, uint32_t duration);

uint32_t getMyIP();
void dns_setServer(IP_t server);
void dns_getServer(IP_t* server);

uint32_t tcp_connect(IP_t IP, uint16_t port);
bool     tcp_send(uint32_t ID, void* data, size_t length);
bool     tcp_close(uint32_t ID);

bool udp_bind(uint16_t port);
bool udp_unbind(uint16_t port);
bool udp_send(void* data, uint32_t length, IP_t destIP, uint16_t srcPort, uint16_t destPort);

 // deprecated
int32_t floppy_dir();
void printLine(const char* message, uint32_t line, uint8_t attribute);


// user functions
void event_flush(EVENT_t filter);
void sleep(uint32_t milliseconds);
bool waitForTask(uint32_t pid, uint32_t timeout);

void iSetCursor(uint16_t x, uint16_t y);
uint32_t getCurrentSeconds();

void snprintf(char *buffer, size_t length, const char *args, ...);
void vsnprintf(char *buffer, size_t length, const char *args, va_list ap);

char* stoupper(char* s);
char* stolower(char* s);

void  reverse(char* s);
char* itoa(int32_t n, char* s);
char* utoa(uint32_t n, char* s);
void  ftoa(float f, char* buffer);
void  i2hex(uint32_t val, char* dest, uint32_t len);

void showInfo(uint8_t val);

IP_t stringToIP(char* str);

uint16_t TextGUI_ShowMSG(char* title, char* message);
uint16_t TextGUI_AskYN(char* title, char* message, uint8_t defaultselected);


#endif
