/*
 * ChibiOS/RT Application Layer for Miniature Cars
 * Copyright (C) 2013 - 2015 Christian Berger
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef PROTOCOL
#define PROTOCOL

/*===========================================================================*/
/* Docker-STM32 protocol related stuff.                                      */
/*===========================================================================*/
#define DELIMITER  "|"
#define MSG_END  ";"
#define SET  "set"
#define STATUS  "status"
#define STATUS_DELIMITER ":"

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

/* --- Request struct --- */
struct GpioRequest {
	char pin[32];
	bool value;
	};

#endif 
