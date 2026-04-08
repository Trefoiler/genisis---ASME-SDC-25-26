#include <cstdio>

#include <pico/time.h>

extern "C" {
#include "controller_state.h"
#include "input_monitor.h"
}

static constexpr uint32_t ANALOG_LOG_INTERVAL_MS = 100;

static bool s_was_connected = false;
static uint32_t s_last_analog_log_ms = 0;

static void print_signed_value(const char* label, float value) {
    int scaled = static_cast<int>(value * 100.0f);

    char sign = '+';
    if (scaled < 0) {
        sign = '-';
        scaled = -scaled;
    }

    std::printf("%s=%c%d.%02d", label, sign, scaled / 100, scaled % 100);
}

static void print_unsigned_value(const char* label, float value) {
    int scaled = static_cast<int>(value * 100.0f);
    if (scaled < 0) {
        scaled = 0;
    }

    std::printf("%s=%d.%02d", label, scaled / 100, scaled % 100);
}

extern "C" void input_monitor_init(void) {
    s_was_connected = false;
    s_last_analog_log_ms = 0;
}

extern "C" void input_monitor_update(void) {
    if (controller_connected() && !s_was_connected) {
        std::printf("input_monitor: controller connected\n");
    }
    if (!controller_connected() && s_was_connected) {
        std::printf("input_monitor: controller disconnected\n");
    }
    s_was_connected = controller_connected();

    if (!controller_connected()) {
        return;
    }

    // Print one-time button press events.
    for (int i = 0; i < CONTROLLER_BUTTON_COUNT; i++) {
        ControllerButton button = static_cast<ControllerButton>(i);

        if (controller_button_pressed(button)) {
            std::printf("pressed: %s\n", controller_button_name(button));
        }
    }

    // Read analog values.
    float lx = controller_left_x();
    float ly = controller_left_y();
    float rx = controller_right_x();
    float ry = controller_right_y();
    float lt = controller_left_trigger();
    float rt = controller_right_trigger();

    bool left_active  = (lx != 0.0f) || (ly != 0.0f);
    bool right_active = (rx != 0.0f) || (ry != 0.0f);
    bool lt_active    = (lt != 0.0f);
    bool rt_active    = (rt != 0.0f);

    if (!left_active && !right_active && !lt_active && !rt_active) {
        return;
    }

    uint32_t now_ms = to_ms_since_boot(get_absolute_time());
    if ((now_ms - s_last_analog_log_ms) < ANALOG_LOG_INTERVAL_MS) {
        return;
    }
    s_last_analog_log_ms = now_ms;

    bool printed_anything = false;

    if (left_active) {
        std::printf("left(");
        print_signed_value("x", lx);
        std::printf(" ");
        print_signed_value("y", ly);
        std::printf(")");
        printed_anything = true;
    }

    if (right_active) {
        if (printed_anything) std::printf(" ");
        std::printf("right(");
        print_signed_value("x", rx);
        std::printf(" ");
        print_signed_value("y", ry);
        std::printf(")");
        printed_anything = true;
    }

    if (lt_active) {
        if (printed_anything) std::printf(" ");
        print_unsigned_value("lt", lt);
        printed_anything = true;
    }

    if (rt_active) {
        if (printed_anything) std::printf(" ");
        print_unsigned_value("rt", rt);
        printed_anything = true;
    }

    if (printed_anything) {
        std::printf("\n");
    }
}
