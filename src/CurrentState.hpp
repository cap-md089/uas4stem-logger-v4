typedef struct CurrentState {
	int timeInAir;
	double latitude;
	double longtitude;
	bool armed;
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
} CurrentState;

CurrentState* parseCurrentState(string input) {

}
