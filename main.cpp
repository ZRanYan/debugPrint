
#include "debug.h"


int main()
{
    init_debug("./log_print.txt");
    // debugTest();
   /* for(int i=0;i<61;i++)
    {
        sleep(1);
        logPrintSettingInfo();
    }
    */
    // sleep(2);
    // logPrintSettingInfo();
    DEBUG_LOG(APP, ERROR,"start");
    DEBUG_LOG(NET, WARNING,"this is fpga!");
    DEBUG_LOG(SENSOR, INFO,"this is net");
    DEBUG_LOG(FPGA, DEBUG,"sdsaga app");
    DEBUG_LOG(ALG, ERROR, "alg is here");
    DEBUG_LOG(PIC, WARNING,"sensor is here");
    DEBUG_LOG(OTHER, INFO, "that is pic");
    sleep(1);
    logPrintSettingInfo();
    return 0;
}
