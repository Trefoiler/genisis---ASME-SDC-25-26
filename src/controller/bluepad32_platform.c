#include <stdbool.h>

#include <pico/cyw43_arch.h>
#include <uni.h>

#include "controller_state.h"
#include "sdkconfig.h"
#include "test_input.h"

#ifndef CONFIG_BLUEPAD32_PLATFORM_CUSTOM
#error "Pico W must use BLUEPAD32_PLATFORM_CUSTOM"
#endif

static void my_platform_init(int argc, const char** argv) {
    (void)argc;
    (void)argv;

    logi("controller: init\n");
    controller_state_init();
    test_input_init();
}

static void my_platform_on_init_complete(void) {
    logi("controller: init complete, starting scan\n");

    uni_bt_start_scanning_and_autoconnect_unsafe();

    // Startup is done, so turn the onboard LED off.
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
    test_input_update();
}

static uni_error_t my_platform_on_device_ready(uni_hid_device_t* d) {
    logi("controller: ready (%p)\n", d);
    return UNI_ERROR_SUCCESS;
}

static void my_platform_on_controller_data(uni_hid_device_t* d, uni_controller_t* ctl) {
    (void)d;

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

    test_input_update();
}

static const uni_property_t* my_platform_get_property(uni_property_idx_t idx) {
    (void)idx;
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