/* === message protocol for car <-> service-node communication */
#include <string>
#include <vector>
#include <iostream>
#include <sstream>

#define DELIMITER  "|"
#define MSG_END  ";"
#define SET  "set"
#define STATUS  "status"
#define STATUS_DELIMITER  ":"

/* --- setter_msgs --- */
#define BRAKE  "brake"
#define CLAMP  "clamp"
#define COMPRESSOR  "compressor"
#define EBS_RELIEF  "ebs_relief"
#define EBS_SPEAKER  "ebs_speaker"
#define FINISHED  "finished"
#define HEART_BEAT  "heart_beat"
#define LIGHT  "light"
#define SERVICE_BREAK  "service_break"
#define SHUTDOWN  "shutdown"
#define STEERING  "steering"
#define STEER  "steer"

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

struct SystemStatus
{
	bool asms;
	bool clamped_sensor;
	int ebs_actuator;
	int ebs_line;
	bool ebs_ok;
	int positon_rack;
	int pressure_rag;
	int service_tank;
	int steer_pos;
};

std::vector<std::string> split(const std::string& s, const std::string& delimiter)
{
   std::vector<std::string> tokens;
   std::string token;
   std::istringstream tokenStream(s);
   while (std::getline(tokenStream, token, delimiter[0]))
   {
      tokens.push_back(token);
   }
   return tokens;
}

std::string encodeMessage(const std::string cmd_1, const std::string cmd_2, const std::string payload)
{
	std::string message = cmd_1 + DELIMITER + cmd_2 + DELIMITER + payload + MSG_END;	
	message = std::to_string(message.length()) + DELIMITER + message;
	return message;
}

SystemStatus decodeStatusMessage(const std::string payload)
{
	SystemStatus result = {};
	auto splited_payload = split(payload, DELIMITER);
	for (std::string reading : splited_payload)
	{
		auto splited_reading = split(reading, STATUS_DELIMITER);
		/* TODO: Define message contents and type */
		if (splited_reading.front() == ASMS) { result.asms = splited_reading.back() == "ON" ? 1 : 0; }
		// if (splited_reading.front() == CLAMPED_SENSOR) { result.clamped_sensor = static_cast<>(splited_reading.back()); }
		// if (splited_reading.front() == EBS_ACTUATOR) { result.ebs_actuator = static_cast<>(splited_reading.back()); }
		// if (splited_reading.front() == EBS_LINE) { result.ebs_line = static_cast<>(splited_reading.back()); }
		// if (splited_reading.front() == EBS_OK) { result.ebs_ok = static_cast<>(splited_reading.back()); }
		// if (splited_reading.front() == POSITON_RACK) { result.positon_rack = static_cast<>(splited_reading.back()); }
		// if (splited_reading.front() == PRESSURE_RAG) { result.pressure_rag = static_cast<>(splited_reading.back()); }
		// if (splited_reading.front() == SERVICE_TANK) { result.service_tank = static_cast<>(splited_reading.back()); }
		if (splited_reading.front() == STEER_POS) { result.steer_pos = std::stoi(splited_reading.back()); }
	}
	return result;
}


int main()
{
	// Encoding
	std::string payload = std::string(ASMS) + STATUS_DELIMITER + "ON" + DELIMITER + STEER_POS + STATUS_DELIMITER + "567";
	std::string status_message = encodeMessage(STATUS, "NONE", payload);
	std::cout << status_message << std::endl;

	// Decoding
	SystemStatus status = decodeStatusMessage(status_message);
	std::cout << "Asms:" << status.asms << std::endl;
	std::cout << "steer_pos:" << status.steer_pos << std::endl;
	return 0;
}

