#pragma once

void hall_tracker_init();
void hall_tracker_trigger(int angle);
int hall_tracker_current_angle();
int hall_tracker_current_frequency();
int hall_tracker_last_angle_delta();
