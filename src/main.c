#include <btstack_run_loop.h>
#include <pico/cyw43_arch.h>
#include <pico/stdlib.h>
#include <uni.h>

#include "controller/controller_state.h"
#include "controller/sdkconfig.h"

#ifndef CONFIG_BLUEPAD32_PLATFORM_CUSTOM
#error "Pico W must use BLUEPAD32_PLATFORM_CUSTOM"
#endif

// Defined in src/controller/bluepad32_platform.c
struct uni_platform* get_my_platform(void);

int main(void) {
    // Enable USB serial output so logs show up in the serial monitor.
    stdio_init_all();

    // Start with a clean controller state.
    controller_state_init();

    // Initialize the Pico 2 W wireless chip.
    if (cyw43_arch_init()) {
        printf("Failed to initialize CYW43.\n");
        return -1;
    }

    // Turn the onboard LED on during startup.
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);

    // Tell Bluepad32 to use our custom platform callbacks.
    uni_platform_set_custom(get_my_platform());

    // Initialize Bluepad32.
    uni_init(0, NULL);

    // Hand off to the Bluetooth event loop.
    btstack_run_loop_execute();

    return 0;
}