
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
    sleep(2);
    logPrintSettingInfo();
    return 0;
}
