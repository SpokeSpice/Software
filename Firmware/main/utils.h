#pragma once

static inline int angle_normalize(int input) {
    int output = input;

    while (output < 0) {
        output += 360;
    }
 
    return output % 360;
}