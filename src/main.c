#include <btstack_run_loop.h>
#include <pico/cyw43_arch.h>
#include <pico/stdlib.h>
#include <uni.h>

#include "controller_state.h"
#include "sdkconfig.h"

// Pico W / Pico 2 W Bluepad32 projects should use the custom platform.
#ifndef CONFIG_BLUEPAD32_PLATFORM_CUSTOM
#error "Pico W must use BLUEPAD32_PLATFORM_CUSTOM"
#endif

// Defined in my_platform.c
struct uni_platform* get_my_platform(void);

int main(void) {
    // Enable USB serial output so log messages show up in the serial monitor.
    stdio_init_all();

    // Start with a clean controller state.
    controller_state_init();

    // Initialize the wireless chip.
    // Bluepad32 depends on this for Bluetooth support on the Pico 2 W.
    if (cyw43_arch_init()) {
        printf("Failed to initialize CYW43.\n");
        return -1;
    }

    // Turn the onboard LED on during startup.
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);

    // Tell Bluepad32 to use the callback functions in my_platform.c.
    // This must happen before uni_init().
    uni_platform_set_custom(get_my_platform());

    // Initialize Bluepad32.
    uni_init(0, NULL);

    // Hand control to the Bluetooth event loop.
    // From this point on, callbacks in my_platform.c drive the behavior.
    btstack_run_loop_execute();

    return 0;
}