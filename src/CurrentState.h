#include <cstring>
#include <string>

typedef struct CurrentState {
	int timeInAir;
	double latitude;
	double longitude;
	float batteryVoltage;
	float batteryRemaining;
	float altitude;
	float groundspeed;
	float throttle;
	float distToHome;
	float verticalSpeed;
	float rtlSpeed;
	float rtlLandSpeed;
	double roll;
	double pitch;
	double yaw;
	bool armed;
} CurrentState;

/**
 * Updates state to match input
 *
 * The return value contains the status code:
 *	0 for success
 *  1 for incorrect length
 *  2 for invalid header
 */
int parseCurrentState(CurrentState* state, std::string input);
