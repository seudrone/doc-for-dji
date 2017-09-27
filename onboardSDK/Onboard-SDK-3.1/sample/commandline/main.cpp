#include <iostream>

#include "DJIHardDriverManifold.h"
#include "conboardsdktask.h"
#include "APIThread.h"

using namespace std;

void FrontFly(FlightData& data, float32_t speed, float32_t height)
{
	data.flag = 0x53;
	data.x = speed;
	data.y = 0;
	data.z = height;
	data.yaw = 0;
}


int main(int argc, char *argv[])
{
	//! @note replace these two lines below to change to an other hard-driver level.
	HardDriverManifold driver("/dev/ttyTHS1", 230400);
	driver.init();

	CoreAPI api(&driver);
	ConboardSDKScript sdkScript(&api);

	//@data
	FlightData data;

	ScriptThread st(&sdkScript);

	//! @note replace these four lines below to change to an other hard-driver level.
	APIThread send(&api, 1);
	APIThread read(&api, 2);
	send.createThread();
	read.createThread();

	loadSS(st.script, (UserData)"--SS load ./key.txt");
	sleep(1);
	st.script->getApi()->activate(&st.script->adata);
	sleep(1);
	st.script->getApi()->setControl(true);
	sleep(1);

	int flag;
	flag = 4;
	st.script->getFlight()->task((DJI::onboardSDK::Flight::TASK)flag);
	sleep(8);

	struct timeval start, end;

	gettimeofday(&start, NULL);
	gettimeofday(&end, NULL);

	while(1000000 * (end.tv_sec - start.tv_sec) + end.tv_usec - start.tv_usec < 10000000)
	{
		FrontFly(data, 3, 10);
		usleep(20000);
		gettimeofday(&end, NULL);
	}
	return 0;
}


