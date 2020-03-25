#include <stdint.h>
#include <stdbool.h>

#include <Arduino.h>

#include <WifiEspNow.h>
#include <espnow.h>

#include "editline.h"
#include "cmdproc.h"

#define print Serial.printf

static char editline[256];
static const uint8_t bcast_mac[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

static int do_bc(int argc, char *argv[])
{
    char payload[16];
    
    int n = (argc > 1) ? atoi(argv[1]) : 1;
    print("Sending %d packets ...\n", n);
    unsigned long start_ms = millis();
    for (int i = 0; i < n; i++) {
        snprintf(payload, sizeof(payload), "p%d", i);
        bool res = esp_now_send((u8 *)bcast_mac, (uint8_t *)payload, strlen(payload));
        if (!res) {
            print("msg %d failed!\n", i);
        }
    }
    unsigned long end_ms = millis();
    print("Sending %d messages took %ld\n", n, end_ms - start_ms);
    
    return 0;
}

static int do_help(int argc, char *argv[]);
const cmd_t commands[] = {
    { "bc", do_bc, "Send broadcast" },
    { "help", do_help, "Show help" },
    { NULL, NULL, NULL }
};

static void show_help(const cmd_t * cmds)
{
    for (const cmd_t * cmd = cmds; cmd->cmd != NULL; cmd++) {
        print("%10s: %s\n", cmd->name, cmd->help);
    }
}

static int do_help(int argc, char *argv[])
{
    show_help(commands);
    return CMD_OK;
}

static void tx(const uint8_t* mac, uint8_t status)
{
    print("tx: stat=%d, %02X:%02X:%02X:%02X:%02X:%02X\n", status,
        mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

static void rx(const uint8_t* mac, uint8_t status)
{
    print("rx: stat=%d, %02X:%02X:%02X:%02X:%02X:%02X\n", status,
        mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

void setup(void)
{
    Serial.begin(115200);
    EditInit(editline, sizeof(editline));
    
    print("WifiEspNow.begin()...");
    bool result = WifiEspNow.begin();
    print(result ? "OK\n" : "FAIL\n");

    WifiEspNow.addPeer(bcast_mac, 1);
    
    // override rx/tx callbacks
//    esp_now_register_send_cb(reinterpret_cast<esp_now_send_cb_t>(tx));
    esp_now_register_recv_cb(reinterpret_cast<esp_now_recv_cb_t>(rx));
}


void loop(void)
{
    // parse command line
    bool haveLine = false;
    if (Serial.available()) {
        char c;
        haveLine = EditLine(Serial.read(), &c);
        Serial.write(c);
    }
    if (haveLine) {
        int result = cmd_process(commands, editline);
        switch (result) {
        case CMD_OK:
            print("OK\n");
            break;
        case CMD_NO_CMD:
            break;
        case CMD_UNKNOWN:
            print("Unknown command, available commands:\n");
            show_help(commands);
            break;
        case CMD_ARG:
            print("Invalid argument(s)\n");
            break;
        default:
            print("%d\n", result);
            break;
        }
        print(">");
    }
}

