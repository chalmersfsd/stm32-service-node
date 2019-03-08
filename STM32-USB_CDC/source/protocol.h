/*
 * protocol.h
 *
 *  Created on: Feb 18, 2019
 *      Author: mvshv
 */

#ifndef SOURCE_PROTOCOL_H_
#define SOURCE_PROTOCOL_H_

#define DELIMITER  "|"
#define MSG_END  ";"
#define SET  "set"
#define GET "get"
#define STATUS  "status"
#define STATUS_DELIMITER ":"
#define ACK "ACK"
#define NACK "NACK"

/* --- setter_msgs --- */
#define CLAMP_SET  "clamp_set"
#define COMPRESSOR  "compressor"
#define EBS_RELIEF  "ebs_relief"
#define EBS_SPEAKER  "ebs_speaker"
#define FINISHED  "finished"
#define HEART_BEAT  "heart_beat"
#define SERVICE_BREAK  "service_break"
#define SHUTDOWN  "shutdown"
#define RACK_LEFT  "rack_left"
#define RACK_RIGHT "rack_right"
#define REDUNDENCY "redundency"
#define SPARE "spare"

#define STEER_SPEED "steer_speed"
#define BRAKE_PRESSURE "brake_pressure"
#define ASSI_BLUE "assi_blue"
#define ASSI_RED "assi_red"
#define ASSI_GREEN "assi_green"

/* --- status_msgs --- */
#define ASMS  "asms"
#define CLAMPED_SENSOR  "clamped_sensor"
#define EBS_ACTUATOR  "ebs_actuator"
#define EBS_LINE  "ebs_line"
#define EBS_OK  "ebs_ok"
#define POSITION_RACK  "position_rack"
#define PRESSURE_RAG  "pressure_rag"
#define SERVICE_TANK  "service_tank"
#define STEER_POS  "steer_pos"

#endif /* SOURCE_PROTOCOL_H_ */
