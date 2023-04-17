/*
 * dynamixel-xl330.h
 *
 *  Created on: Apr 11, 2023
 *      Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#ifndef _DYNAMIXEL_XL330_H_
#define _DYNAMIXEL_XL330_H_

struct xl330_control_table
{
	/* EEPROM */
	uint16_t model_number;
	uint32_t model_information;
	uint8_t firmware_version;
	uint8_t id;
	uint8_t baud_rate;
	uint8_t return_delay_time;
	uint8_t drive_mode;
	uint8_t operating_mode;
	uint8_t secondary_id;
	uint8_t protocol_type;
	uint8_t reserved_1[6];
	uint32_t home_offset;
	uint32_t moving_threshold;
	uint8_t reserved_2[3];
	uint8_t temperature_limit;
	uint16_t max_voltage_limit;
	uint16_t min_voltage_limit;
	uint16_t pwm_limit;
	uint16_t current_limit;
	uint8_t reserved_3[4];
	uint32_t velocity_limit;
	uint32_t max_position_limit;
	uint32_t min_position_limit;
	uint8_t reserved_4[4];
	uint8_t startup_configuration;
	uint8_t reserved_5[1];
	uint8_t pwm_slope;
	uint8_t shutdown;

	/* Ram */
	uint8_t torque_enable;
	uint8_t led;
	uint8_t reserved_6[2];
	uint8_t status_return_level;
	uint8_t registered_instruction;
	uint8_t hardware_error_status;
	uint8_t reserved_7[5];
	uint16_t velocity_i_gain;
	uint16_t velocity_p_gain;
	uint16_t position_d_gain;
	uint16_t position_i_gain;
	uint16_t position_p_gain;
	uint8_t reserved_8[2];
	uint16_t feedforward_2nd_gain;
	uint16_t feedforward_1nd_gain;
	uint8_t reserved_9[6];
	uint8_t bus_watchdog;
	uint8_t reserved_10[1];
	uint16_t goal_pwm;
	uint16_t goal_current;
	uint32_t goal_velocity;
	uint32_t profile_acceleration;
	uint32_t profile_velocity;
	uint32_t goal_position;
	uint16_t realtime_tick;
	uint8_t moving;
	uint8_t moving_status;
	uint16_t present_pwm;
	uint16_t present_current;
	uint32_t present_velocity;
	uint32_t present_position;
	uint32_t velocity_trajectory;
	uint32_t position_trajectory;
	uint16_t present_input_voltage;
	uint8_t present_temperature;
	uint8_t backup_ready;
	uint8_t reserved_11[20];
	uint16_t indirect_address[20];
	uint8_t indirect_data[20];
} __packed;

#define dynamixel_xl330_addr(field) (offsetof(struct xl330_control_table, field))
#define dynamixel_xl330_sizeof(field) (sizeof(((struct xl330_control_table *)0)->field))

#define MODEL_NUMBER_ADDR dynamixel_xl330_addr(model_number)
#define MODEL_INFORMATION_ADDR dynamixel_xl330_addr(model_information)
#define FIRMWARE_VERSION_ADDR dynamixel_xl330_addr(firmware_version)
#define ID_ADDR dynamixel_xl330_addr(id)
#define BAUD_RATE_ADDR dynamixel_xl330_addr(baud_rate)
#define RETURN_DELAY_TIME_ADDR dynamixel_xl330_addr(return_delay_time)
#define DRIVE_MODE_ADDR dynamixel_xl330_addr(drive_mode)
#define OPERATING_MODE_ADDR dynamixel_xl330_addr(operating_mode)
#define SECONDARY_ID_ADDR dynamixel_xl330_addr(secondary_id)
#define PROTOCOL_TYPE_ADDR dynamixel_xl330_addr(protocol_type)
#define HOME_OFFSET_ADDR dynamixel_xl330_addr(home_offset)
#define MOVING_THRESHOLD_ADDR dynamixel_xl330_addr(moving_threshold)
#define TEMPERATURE_LIMIT_ADDR dynamixel_xl330_addr(temperature_limit)
#define MAX_VOLTAGE_LIMIT_ADDR dynamixel_xl330_addr(max_voltage_limit)
#define MIN_VOLTAGE_LIMIT_ADDR dynamixel_xl330_addr(min_voltage_limit)
#define PWM_LIMIT_ADDR dynamixel_xl330_addr(pwm_limit)
#define CURRENT_LIMIT_ADDR dynamixel_xl330_addr(current_limit)
#define VELOCITY_LIMIT_ADDR dynamixel_xl330_addr(velocity_limit)
#define MAX_POSITION_LIMIT_ADDR dynamixel_xl330_addr(max_position_limit)
#define MIN_POSITION_LIMIT_ADDR dynamixel_xl330_addr(min_position_limit)
#define STARTUP_CONFIGURATION_ADDR dynamixel_xl330_addr(startup_configuration)
#define PWM_SLOPE_ADDR dynamixel_xl330_addr(pwm_slope)
#define SHUTDOWN_ADDR dynamixel_xl330_addr(shutdown)
#define TORQUE_ENABLE_ADDR dynamixel_xl330_addr(torque_enable)
#define LED_ADDR dynamixel_xl330_addr(led)
#define STATUS_RETURN_LEVEL_ADDR dynamixel_xl330_addr(status_return_level)
#define REGISTERED_INSTRUCTION_ADDR dynamixel_xl330_addr(registered_instruction)
#define HARDWARE_ERROR_STATUS_ADDR dynamixel_xl330_addr(hardware_error_status)
#define VELOCITY_I_GAIN_ADDR dynamixel_xl330_addr(velocity_i_gain)
#define VELOCITY_P_GAIN_ADDR dynamixel_xl330_addr(velocity_p_gain)
#define POSITION_D_GAIN_ADDR dynamixel_xl330_addr(position_d_gain)
#define POSITION_I_GAIN_ADDR dynamixel_xl330_addr(position_i_gain)
#define POSITION_P_GAIN_ADDR dynamixel_xl330_addr(position_p_gain)
#define FEEDFORWARD_2ND_GAIN_ADDR dynamixel_xl330_addr(feedforward_2nd_gain)
#define FEEDFORWARD_1ND_GAIN_ADDR dynamixel_xl330_addr(feedforward_1nd_gain)
#define BUS_WATCHDOG_ADDR dynamixel_xl330_addr(bus_watchdog)
#define GOAL_PWM_ADDR dynamixel_xl330_addr(goal_pwm)
#define GOAL_CURRENT_ADDR dynamixel_xl330_addr(goal_current)
#define GOAL_VELOCITY_ADDR dynamixel_xl330_addr(goal_velocity)
#define PROFILE_ACCELERATION_ADDR dynamixel_xl330_addr(profile_acceleration)
#define PROFILE_VELOCITY_ADDR dynamixel_xl330_addr(profile_velocity)
#define GOAL_POSITION_ADDR dynamixel_xl330_addr(goal_position)
#define REALTIME_TICK_ADDR dynamixel_xl330_addr(realtime_tick)
#define MOVING_ADDR dynamixel_xl330_addr(moving)
#define MOVING_STATUS_ADDR dynamixel_xl330_addr(moving_status)
#define PRESENT_PWM_ADDR dynamixel_xl330_addr(present_pwm)
#define PRESENT_CURRENT_ADDR dynamixel_xl330_addr(present_current)
#define PRESENT_VELOCITY_ADDR dynamixel_xl330_addr(present_velocity)
#define PRESENT_POSITION_ADDR dynamixel_xl330_addr(present_position)
#define VELOCITY_TRAJECTORY_ADDR dynamixel_xl330_addr(velocity_trajectory)
#define POSITION_TRAJECTORY_ADDR dynamixel_xl330_addr(position_trajectory)
#define PRESENT_INPUT_VOLTAGE_ADDR dynamixel_xl330_addr(present_input_voltage)
#define PRESENT_TEMPERATURE_ADDR dynamixel_xl330_addr(present_temperature)
#define BACKUP_READY_ADDR dynamixel_xl330_addr(backup_ready)
#define INDIRECT_ADDRESS_ADDR dynamixel_xl330_addr(indirect_address)
#define INDIRECT_DATA_ADDR dynamixel_xl330_addr(indirect_data)

#define MODEL_NUMBER_SIZE dynamixel_xl330_sizeof(model_number)
#define MODEL_INFORMATION_SIZE dynamixel_xl330_sizeof(model_information)
#define FIRMWARE_VERSION_SIZE dynamixel_xl330_sizeof(firmware_version)
#define ID_SIZE dynamixel_xl330_sizeof(id)
#define BAUD_RATE_SIZE dynamixel_xl330_sizeof(baud_rate)
#define RETURN_DELAY_TIME_SIZE dynamixel_xl330_sizeof(return_delay_time)
#define DRIVE_MODE_SIZE dynamixel_xl330_sizeof(drive_mode)
#define OPERATING_MODE_SIZE dynamixel_xl330_sizeof(operating_mode)
#define SECONDARY_ID_SIZE dynamixel_xl330_sizeof(secondary_id)
#define PROTOCOL_TYPE_SIZE dynamixel_xl330_sizeof(protocol_type)
#define HOME_OFFSET_SIZE dynamixel_xl330_sizeof(home_offset)
#define MOVING_THRESHOLD_SIZE dynamixel_xl330_sizeof(moving_threshold)
#define TEMPERATURE_LIMIT_SIZE dynamixel_xl330_sizeof(temperature_limit)
#define MAX_VOLTAGE_LIMIT_SIZE dynamixel_xl330_sizeof(max_voltage_limit)
#define MIN_VOLTAGE_LIMIT_SIZE dynamixel_xl330_sizeof(min_voltage_limit)
#define PWM_LIMIT_SIZE dynamixel_xl330_sizeof(pwm_limit)
#define CURRENT_LIMIT_SIZE dynamixel_xl330_sizeof(current_limit)
#define VELOCITY_LIMIT_SIZE dynamixel_xl330_sizeof(velocity_limit)
#define MAX_POSITION_LIMIT_SIZE dynamixel_xl330_sizeof(max_position_limit)
#define MIN_POSITION_LIMIT_SIZE dynamixel_xl330_sizeof(min_position_limit)
#define STARTUP_CONFIGURATION_SIZE dynamixel_xl330_sizeof(startup_configuration)
#define PWM_SLOPE_SIZE dynamixel_xl330_sizeof(pwm_slope)
#define SHUTDOWN_SIZE dynamixel_xl330_sizeof(shutdown)
#define TORQUE_ENABLE_SIZE dynamixel_xl330_sizeof(torque_enable)
#define LED_SIZE dynamixel_xl330_sizeof(led)
#define STATUS_RETURN_LEVEL_SIZE dynamixel_xl330_sizeof(status_return_level)
#define REGISTERED_INSTRUCTION_SIZE dynamixel_xl330_sizeof(registered_instruction)
#define HARDWARE_ERROR_STATUS_SIZE dynamixel_xl330_sizeof(hardware_error_status)
#define VELOCITY_I_GAIN_SIZE dynamixel_xl330_sizeof(velocity_i_gain)
#define VELOCITY_P_GAIN_SIZE dynamixel_xl330_sizeof(velocity_p_gain)
#define POSITION_D_GAIN_SIZE dynamixel_xl330_sizeof(position_d_gain)
#define POSITION_I_GAIN_SIZE dynamixel_xl330_sizeof(position_i_gain)
#define POSITION_P_GAIN_SIZE dynamixel_xl330_sizeof(position_p_gain)
#define FEEDFORWARD_2ND_GAIN_SIZE dynamixel_xl330_sizeof(feedforward_2nd_gain)
#define FEEDFORWARD_1ND_GAIN_SIZE dynamixel_xl330_sizeof(feedforward_1nd_gain)
#define BUS_WATCHDOG_SIZE dynamixel_xl330_sizeof(bus_watchdog)
#define GOAL_PWM_SIZE dynamixel_xl330_sizeof(goal_pwm)
#define GOAL_CURRENT_SIZE dynamixel_xl330_sizeof(goal_current)
#define GOAL_VELOCITY_SIZE dynamixel_xl330_sizeof(goal_velocity)
#define PROFILE_ACCELERATION_SIZE dynamixel_xl330_sizeof(profile_acceleration)
#define PROFILE_VELOCITY_SIZE dynamixel_xl330_sizeof(profile_velocity)
#define GOAL_POSITION_SIZE dynamixel_xl330_sizeof(goal_position)
#define REALTIME_TICK_SIZE dynamixel_xl330_sizeof(realtime_tick)
#define MOVING_SIZE dynamixel_xl330_sizeof(moving)
#define MOVING_STATUS_SIZE dynamixel_xl330_sizeof(moving_status)
#define PRESENT_PWM_SIZE dynamixel_xl330_sizeof(present_pwm)
#define PRESENT_CURRENT_SIZE dynamixel_xl330_sizeof(present_current)
#define PRESENT_VELOCITY_SIZE dynamixel_xl330_sizeof(present_velocity)
#define PRESENT_POSITION_SIZE dynamixel_xl330_sizeof(present_position)
#define VELOCITY_TRAJECTORY_SIZE dynamixel_xl330_sizeof(velocity_trajectory)
#define POSITION_TRAJECTORY_SIZE dynamixel_xl330_sizeof(position_trajectory)
#define PRESENT_INPUT_VOLTAGE_SIZE dynamixel_xl330_sizeof(present_input_voltage)
#define PRESENT_TEMPERATURE_SIZE dynamixel_xl330_sizeof(present_temperature)
#define BACKUP_READY_SIZE dynamixel_xl330_sizeof(backup_ready)
#define INDIRECT_ADDRESS_SIZE dynamixel_xl330_sizeof(indirect_address)
#define INDIRECT_DATA_SIZE dynamixel_xl330_sizeof(indirect_data)

#endif
