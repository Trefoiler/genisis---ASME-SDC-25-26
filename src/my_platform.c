#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include <pico/cyw43_arch.h>
#include <pico/time.h>
#include <uni.h>

#include "controller_state.h"
#include "sdkconfig.h"

// Pico W / Pico 2 W Bluepad32 projects should use the custom platform.
#ifndef CONFIG_BLUEPAD32_PLATFORM_CUSTOM
#error "Pico W must use BLUEPAD32_PLATFORM_CUSTOM"
#endif

// Log controller state at most once every 100 ms.
// This keeps the serial monitor readable.
#define LOG_INTERVAL_MS 100

// Forward declaration
static void trigger_event_on_gamepad(uni_hid_device_t* d);

static void my_platform_init(int argc, const char** argv) {
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    logi("my_platform: init()\n");

    // Start with a clean controller state every boot.
    controller_state_init();
}

static void my_platform_on_init_complete(void) {
    logi("my_platform: on_init_complete()\n");

    // Start scanning and automatically connect to supported controllers.
    uni_bt_start_scanning_and_autoconnect_unsafe();

    // Leave stored Bluetooth keys alone so reconnecting is easier across reboots.
    // If you ever want to force a fresh re-pair, temporarily switch this to:
    // uni_bt_del_keys_unsafe();
    uni_bt_list_keys_unsafe();

    // Startup is done, so turn the onboard LED off.
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);

    // Print current Bluepad32 properties for debugging.
    uni_property_dump_all();
}

static uni_error_t my_platform_on_device_discovered(bd_addr_t addr, const char* name, uint16_t cod, uint8_t rssi) {
    ARG_UNUSED(addr);
    ARG_UNUSED(name);
    ARG_UNUSED(rssi);

    // Ignore keyboards. We only want controllers for this robot project.
    if (((cod & UNI_BT_COD_MINOR_MASK) & UNI_BT_COD_MINOR_KEYBOARD) == UNI_BT_COD_MINOR_KEYBOARD) {
        logi("Ignoring keyboard\n");
        return UNI_ERROR_IGNORE_DEVICE;
    }

    return UNI_ERROR_SUCCESS;
}

static void my_platform_on_device_connected(uni_hid_device_t* d) {
    logi("Controller connected: %p\n", d);
}

static void my_platform_on_device_disconnected(uni_hid_device_t* d) {
    logi("Controller disconnected: %p\n", d);

    // Clear the shared controller state so the rest of the robot sees "no controller".
    controller_state_set_disconnected();
}

static uni_error_t my_platform_on_device_ready(uni_hid_device_t* d) {
    logi("Controller ready: %p\n", d);

    // Accept the controller.
    return UNI_ERROR_SUCCESS;
}

static void my_platform_on_controller_data(uni_hid_device_t* d, uni_controller_t* ctl) {
    static uint32_t last_log_ms = 0;

    // Right now we only care about gamepads.
    if (ctl->klass != UNI_CONTROLLER_CLASS_GAMEPAD) {
        return;
    }

    uni_gamepad_t* gp = &ctl->gamepad;

    // Copy the newest controller data into our shared state.
    // Other robot files can read this without knowing anything about Bluepad32.
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

    // Print a readable status line occasionally for debugging.
    uint32_t now_ms = to_ms_since_boot(get_absolute_time());
    if ((now_ms - last_log_ms) >= LOG_INTERVAL_MS) {
        last_log_ms = now_ms;

        logi(
            "id=%d dpad=0x%02x x=%4ld y=%4ld rx=%4ld ry=%4ld brake=%4ld throttle=%4ld buttons=0x%04x misc=0x%02x battery=%u\n",
            uni_hid_device_get_idx_for_instance(d),
            gp->dpad,
            (long)gp->axis_x,
            (long)gp->axis_y,
            (long)gp->axis_rx,
            (long)gp->axis_ry,
            (long)gp->brake,
            (long)gp->throttle,
            gp->buttons,
            gp->misc_buttons,
            ctl->battery
        );
    }

    // Optional controller-side debug features from the Bluepad32 example.
    // Leave them in for now since they are handy for testing.
    if ((gp->buttons & BUTTON_A) && d->report_parser.play_dual_rumble != NULL) {
        d->report_parser.play_dual_rumble(d, 0, 250, 128, 0);
    }

    if ((gp->buttons & BUTTON_B) && d->report_parser.play_dual_rumble != NULL) {
        d->report_parser.play_dual_rumble(d, 0, 250, 0, 128);
    }

    if ((gp->buttons & BUTTON_X) && d->report_parser.set_player_leds != NULL) {
        static uint8_t leds = 0;
        d->report_parser.set_player_leds(d, leds++ & 0x0f);
    }

    if ((gp->buttons & BUTTON_Y) && d->report_parser.set_lightbar_color != NULL) {
        uint8_t r = (gp->axis_x * 256) / 512;
        uint8_t g = (gp->axis_y * 256) / 512;
        uint8_t b = (gp->axis_rx * 256) / 512;
        d->report_parser.set_lightbar_color(d, r, g, b);
    }
}

static const uni_property_t* my_platform_get_property(uni_property_idx_t idx) {
    ARG_UNUSED(idx);
    return NULL;
}

static void my_platform_on_oob_event(uni_platform_oob_event_t event, void* data) {
    switch (event) {
        case UNI_PLATFORM_OOB_GAMEPAD_SYSTEM_BUTTON:
            // The DS4 system button can trigger a quick feedback event.
            trigger_event_on_gamepad((uni_hid_device_t*)data);
            break;

        case UNI_PLATFORM_OOB_BLUETOOTH_ENABLED:
            logi("Bluetooth enabled: %d\n", (bool)(data));
            break;

        default:
            logi("Unsupported OOB event: 0x%04x\n", event);
            break;
    }
}

static void trigger_event_on_gamepad(uni_hid_device_t* d) {
    // Give a quick feedback pulse when the system button is pressed.
    if (d->report_parser.play_dual_rumble != NULL) {
        d->report_parser.play_dual_rumble(d, 0, 50, 128, 40);
    }

    // Advance the player LEDs.
    if (d->report_parser.set_player_leds != NULL) {
        static uint8_t led = 0;
        led = (led + 1) & 0x0f;
        d->report_parser.set_player_leds(d, led);
    }

    // Cycle the light bar color.
    if (d->report_parser.set_lightbar_color != NULL) {
        static uint8_t red = 0x10;
        static uint8_t green = 0x20;
        static uint8_t blue = 0x40;

        red += 0x10;
        green -= 0x20;
        blue += 0x40;

        d->report_parser.set_lightbar_color(d, red, green, blue);
    }
}

struct uni_platform* get_my_platform(void) {
    static struct uni_platform plat = {
        .name = "Genesis Platform",
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