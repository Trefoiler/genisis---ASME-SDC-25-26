#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include <pico/cyw43_arch.h>
#include <pico/time.h>
#include <uni.h>

#include "controller_state.h"
#include "sdkconfig.h"

#ifndef CONFIG_BLUEPAD32_PLATFORM_CUSTOM
#error "Pico W must use BLUEPAD32_PLATFORM_CUSTOM"
#endif

#define LOG_INTERVAL_MS 150

// Button bits from your DS4 testing
#define BTN_X               0x0001
#define BTN_CIRCLE          0x0002
#define BTN_SQUARE          0x0004
#define BTN_TRIANGLE        0x0008
#define BTN_L1              0x0010
#define BTN_R1              0x0020
#define BTN_L2_BUTTON       0x0040
#define BTN_R2_BUTTON       0x0080
#define BTN_L3              0x0100
#define BTN_R3              0x0200

// Misc bits from your DS4 testing
#define MISC_PS             0x01
#define MISC_SHARE          0x02
#define MISC_OPTIONS        0x04

static void format_signed_unit(char* out, size_t len, float value) {
    int scaled = (int)(value * 100.0f);

    char sign = '+';
    if (scaled < 0) {
        sign = '-';
        scaled = -scaled;
    }

    snprintf(out, len, "%c%d.%02d", sign, scaled / 100, scaled % 100);
}

static void format_unsigned_unit(char* out, size_t len, float value) {
    int scaled = (int)(value * 100.0f);
    if (scaled < 0) {
        scaled = 0;
    }

    snprintf(out, len, "%d.%02d", scaled / 100, scaled % 100);
}

static void append_label(char* buffer, size_t len, const char* label, bool* first) {
    if (!*first) {
        strncat(buffer, ", ", len - strlen(buffer) - 1);
    }
    strncat(buffer, label, len - strlen(buffer) - 1);
    *first = false;
}

static void decode_buttons(uint16_t buttons, char* out, size_t len) {
    bool first = true;
    out[0] = '\0';

    if (buttons & BTN_X)         append_label(out, len, "x", &first);
    if (buttons & BTN_CIRCLE)    append_label(out, len, "circle", &first);
    if (buttons & BTN_SQUARE)    append_label(out, len, "square", &first);
    if (buttons & BTN_TRIANGLE)  append_label(out, len, "triangle", &first);
    if (buttons & BTN_L1)        append_label(out, len, "l1", &first);
    if (buttons & BTN_R1)        append_label(out, len, "r1", &first);
    if (buttons & BTN_L2_BUTTON) append_label(out, len, "l2", &first);
    if (buttons & BTN_R2_BUTTON) append_label(out, len, "r2", &first);
    if (buttons & BTN_L3)        append_label(out, len, "l3", &first);
    if (buttons & BTN_R3)        append_label(out, len, "r3", &first);

    if (first) {
        snprintf(out, len, "none");
    }
}

static void decode_misc(uint8_t misc, char* out, size_t len) {
    bool first = true;
    out[0] = '\0';

    if (misc & MISC_PS)      append_label(out, len, "ps", &first);
    if (misc & MISC_SHARE)   append_label(out, len, "share", &first);
    if (misc & MISC_OPTIONS) append_label(out, len, "options", &first);

    if (first) {
        snprintf(out, len, "none");
    }
}

static const char* decode_dpad(uint8_t dpad) {
    switch (dpad) {
        case 0x00: return "none";
        case 0x01: return "up";
        case 0x02: return "down";
        case 0x04: return "right";
        case 0x08: return "left";
        case 0x05: return "up-right";
        case 0x09: return "up-left";
        case 0x06: return "down-right";
        case 0x0a: return "down-left";
        default:   return "unknown";
    }
}

static void my_platform_init(int argc, const char** argv) {
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    logi("controller: init\n");
    controller_state_init();
}

static void my_platform_on_init_complete(void) {
    logi("controller: init complete, starting scan\n");

    uni_bt_start_scanning_and_autoconnect_unsafe();

    // Keep keys so reconnecting is easier.
    uni_bt_list_keys_unsafe();

    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
}

static uni_error_t my_platform_on_device_discovered(bd_addr_t addr, const char* name, uint16_t cod, uint8_t rssi) {
    (void)addr;
    (void)name;
    (void)rssi;

    if (((cod & UNI_BT_COD_MINOR_MASK) & UNI_BT_COD_MINOR_KEYBOARD) == UNI_BT_COD_MINOR_KEYBOARD) {
        logi("controller: ignoring keyboard\n");
        return UNI_ERROR_IGNORE_DEVICE;
    }

    return UNI_ERROR_SUCCESS;
}

static void my_platform_on_device_connected(uni_hid_device_t* d) {
    logi("controller: connected (%p)\n", d);
}

static void my_platform_on_device_disconnected(uni_hid_device_t* d) {
    logi("controller: disconnected (%p)\n", d);
    controller_state_set_disconnected();
}

static uni_error_t my_platform_on_device_ready(uni_hid_device_t* d) {
    logi("controller: ready (%p)\n", d);
    return UNI_ERROR_SUCCESS;
}

static void my_platform_on_controller_data(uni_hid_device_t* d, uni_controller_t* ctl) {
    ARG_UNUSED(d);

    static uint32_t last_log_ms = 0;

    if (ctl->klass != UNI_CONTROLLER_CLASS_GAMEPAD) {
        return;
    }

    uni_gamepad_t* gp = &ctl->gamepad;

    controller_state_update(
        gp->dpad,
        gp->axis_x,
        gp->axis_y,
        gp->axis_rx,
        gp->axis_ry,
        gp->brake,
        gp->throttle,
        gp->buttons,
        gp->misc_buttons,
        gp->gyro[0],
        gp->gyro[1],
        gp->gyro[2],
        gp->accel[0],
        gp->accel[1],
        gp->accel[2],
        ctl->battery
    );

    uint32_t now_ms = to_ms_since_boot(get_absolute_time());
    if ((now_ms - last_log_ms) < LOG_INTERVAL_MS) {
        return;
    }
    last_log_ms = now_ms;

    const ControllerState* state = controller_state_get();

    char lx_str[8];
    char ly_str[8];
    char rx_str[8];
    char ry_str[8];
    char lt_str[8];
    char rt_str[8];
    char buttons_str[96];
    char misc_str[48];

    format_signed_unit(lx_str, sizeof(lx_str), controller_left_x());
    format_signed_unit(ly_str, sizeof(ly_str), controller_left_y());
    format_signed_unit(rx_str, sizeof(rx_str), controller_right_x());
    format_signed_unit(ry_str, sizeof(ry_str), controller_right_y());
    format_unsigned_unit(lt_str, sizeof(lt_str), controller_left_trigger());
    format_unsigned_unit(rt_str, sizeof(rt_str), controller_right_trigger());

    decode_buttons(state->buttons, buttons_str, sizeof(buttons_str));
    decode_misc(state->misc, misc_str, sizeof(misc_str));

    unsigned int battery_pct = (state->battery * 100u) / 255u;

    logi("---------------- CONTROLLER ----------------\n");
    logi("connected: yes\n");
    logi("battery: raw=%u/255  pct=%u%%\n", state->battery, battery_pct);

    logi("raw:\n");
    logi("  dpad=0x%02x\n", state->dpad);
    logi("  x=%4ld  y=%4ld  rx=%4ld  ry=%4ld\n",
         (long)state->x, (long)state->y, (long)state->rx, (long)state->ry);
    logi("  brake=%4ld  throttle=%4ld\n",
         (long)state->brake, (long)state->throttle);
    logi("  buttons=0x%04x\n", state->buttons);
    logi("  misc=0x%02x\n", state->misc);

    logi("clean:\n");
    logi("  left_stick:   x=%s  y=%s\n", lx_str, ly_str);
    logi("  right_stick:  x=%s  y=%s\n", rx_str, ry_str);
    logi("  triggers:     left=%s  right=%s\n", lt_str, rt_str);
    logi("  dpad:         %s\n", decode_dpad(state->dpad));
    logi("  buttons:      %s\n", buttons_str);
    logi("  misc:         %s\n", misc_str);
    logi("--------------------------------------------\n");
}

static const uni_property_t* my_platform_get_property(uni_property_idx_t idx) {
    ARG_UNUSED(idx);
    return NULL;
}

static void my_platform_on_oob_event(uni_platform_oob_event_t event, void* data) {
    switch (event) {
        case UNI_PLATFORM_OOB_BLUETOOTH_ENABLED:
            logi("controller: bluetooth enabled = %d\n", (bool)(data));
            break;

        default:
            break;
    }
}

struct uni_platform* get_my_platform(void) {
    static struct uni_platform plat = {
        .name = "Genesis Controller Platform",
        .init = my_platform_init,
        .on_init_complete = my_platform_on_init_complete,
        .on_device_discovered = my_platform_on_device_discovered,
        .on_device_connected = my_platform_on_device_connected,
        .on_device_disconnected = my_platform_on_device_disconnected,
        .on_device_ready = my_platform_on_device_ready,
        .on_oob_event = my_platform_on_oob_event,
        .on_controller_data = my_platform_on_controller_data,
        .get_property = my_platform_get_property,
    };

    return &plat;
}