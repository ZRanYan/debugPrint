#include <QDebug>
#include <QDateTime>
#include <cstdarg> // 包含可变参数的头文件
#include <cstdio>
#include <stdio.h>

#include <QDateTime>
#include <cstring>
#include <QThread>
#include <pthread.h>
#include <unistd.h>

#include <pthread.h>
#include <mqueue.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sched.h>

#define BUF_SIZE 512

#define ARRAY_SIZE 512

#define DEBUG_LOG_OPEN

#define INIT_MEMBER(a) {a, #a}
// char buf[BUF_SIZE];
typedef enum
{
    ERROR,
    WARNING,
    INFO,
    DEBUG,
    PRINT_LEVEL_END
}PRINT_LEVEL;
typedef enum
{
    APP = 1<<0,
    FPGA = 1<<1,
    SENSOR = 1<<2,
    NET = 1<<3,
    OTHER = 1<<4
}MODULE_NAME;
typedef enum
{
    LOGO_NONE = 0,
    LOGO_LEVEL_STRING = 1<<0,
    LOGO_MODULE_STRING = 1<<1,
    LOGO_FILE_STRING = 1<<2,
    LOGO_FUNCTION_STRING = 1<<3,
    LOGO_YEAR_MONTH_DAY_STRING = 1<<4,
    LOGO_MINUTES_AND_SECONDS_STRING = 1<<5,
    LOGO_ROWS_STRING = 1<<6,
}SHOW_HEAD_LOGO;
typedef struct
{
    SHOW_HEAD_LOGO headLog;
    char           headLogString[35];
}SHOW_HEAD_CONFIG_STRUCT;
typedef enum
{
    PRINT_TERMINAL = 1<<0,
    PRINT_FLASH = 2<<0
}LOG_STORAGE_METHOD;
typedef struct
{
    PRINT_LEVEL level;
    union {
        unsigned short int debugModuleFlag; //
        struct {
            unsigned short int bit1_app : 1;
            unsigned short int bit2_fpga : 1;
            unsigned short int bit3_sensor : 1;
            unsigned short int bit4_net : 1;
            unsigned short int bit5_other : 1;
        }bitField;
    }debugModuleFlagValue;
    union {
        unsigned short int debugShowHeadFlag; // 整数成员
        struct {
            unsigned short int bit1_level : 1;
            unsigned short int bit2_module : 1;
            unsigned short int bit3_file : 1;
            unsigned short int bit4_function : 1;
            unsigned short int bit5_year_month_day : 1;
            unsigned short int bit6_minutes_seconds : 1;
            unsigned short int bit7_rows : 1;
        }bitField;
    }debugShowHeadValue;
    union {
        unsigned short int debugPrintExhibitFlag; //
        struct {
            unsigned short int bit1_terminal : 1;
            unsigned short int bit2_flash : 1;
        }bitField;
    }debugPrintExhibitValue;
    char logFilePath[30];
}DEBUG_CTRL_STRUCT;
typedef struct
{
    int year;
    int month;
    int day;
    int hour;
    int minute;
    int second;
    int msec;
}DEBUG_TIME_INFO;

typedef struct
{
    pthread_mutex_t mutex;
    PRINT_LEVEL printLevel;
    MODULE_NAME printModuleName;
    DEBUG_TIME_INFO mTime;
    char fileName[30];
    char funcName[20];
    unsigned int lineNum;
    char printMess[256];//需要的打印信息
}DEBUG_MQ_MESSAGE_STRUCT;


typedef struct
{
    volatile unsigned char wirteIndex;//only
    volatile unsigned char readIndex;//only
    pthread_mutex_t mutex_wirte;
    DEBUG_MQ_MESSAGE_STRUCT data[ARRAY_SIZE];
}DEBUG_MQ_STRUCT;


DEBUG_MQ_STRUCT debugMqData;

#define MAX_MSG_SIZE 256
#define MSG_QUEUE_NAME "/asgdsgasg"


pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
int thread_ready = 0;

static DEBUG_CTRL_STRUCT debugCtrlConfig =
{
    .level = DEBUG,
    .debugModuleFlagValue =
    {
        .debugModuleFlag = 0xFFFF,
    },
    .debugShowHeadValue =
    {
    //    .debugShowHeadFlag = LOGO_LEVEL_STRING|LOGO_FILE_STRING|LOGO_FUNCTION_STRING|LOGO_ROWS_STRING,
        .debugShowHeadFlag = 0xFFFF,
        // .debugShowHeadFlag = LOGO_LEVEL_STRING,
    },
    .debugPrintExhibitValue =
    {
        .debugPrintExhibitFlag = PRINT_TERMINAL|PRINT_FLASH,
    },
    // .logFilePath = "./log_print.txt"
};


static char *levelString[] = {"ERROR", "WARNING", "INFO", "DEBUG", " "};
static char *printModuleName[] = {"app","fpga","sensor","net","other"};
static SHOW_HEAD_CONFIG_STRUCT headLog[] = {INIT_MEMBER(LOGO_LEVEL_STRING),INIT_MEMBER(LOGO_MODULE_STRING),INIT_MEMBER(LOGO_FILE_STRING),\
                                            INIT_MEMBER(LOGO_FUNCTION_STRING),INIT_MEMBER(LOGO_YEAR_MONTH_DAY_STRING),\
                                            INIT_MEMBER(LOGO_MINUTES_AND_SECONDS_STRING),INIT_MEMBER(LOGO_ROWS_STRING)};

/*打印当前的参数配置情况*/
void logPrintSettingInfo()
{
    int i,j;
    qDebug("-------------------current debug setting---------------------\n");
    qDebug("debug level:\n");
    qDebug("\t%s\n", levelString[debugCtrlConfig.level]);
    qDebug("debug module:\n\t");
    for(i=0; i<sizeof(printModuleName)/sizeof(printModuleName[0]);i++)
    {
        if((1<<i) == (debugCtrlConfig.debugModuleFlagValue.debugModuleFlag & (1<<i)))
        {
            qDebug("%s ", printModuleName[i]);
        }
    }
    qDebug("\ndebug show head:\n\t");
    for(i=0,j=0; i<sizeof(headLog)/sizeof(headLog[0]);i++)
    {
        if((headLog[i].headLog) == (debugCtrlConfig.debugShowHeadValue.debugShowHeadFlag & (headLog[i].headLog)))
        {
            j++;
            qDebug("%s_string ", headLog[i].headLogString);
            if(j%4 == 0)
            {
                qDebug("\n\t");
            }
        }
    }
    qDebug("\ndebug print exhibit:\n\t");
    if(PRINT_TERMINAL == (debugCtrlConfig.debugPrintExhibitValue.debugPrintExhibitFlag & PRINT_TERMINAL))
    {
        qDebug("PRINT_TERMINAL ");
    }
    if(PRINT_FLASH == (debugCtrlConfig.debugPrintExhibitValue.debugPrintExhibitFlag & PRINT_FLASH))
    {
        qDebug("PRINT_TERMINAL:%s", debugCtrlConfig.logFilePath);
    }
    qDebug("\n----------------------------end----------------------------\n");
}

static unsigned int debugPrintHeadPack(char *buffer, int bufferSize, unsigned int *written, const char *format, ...)
{
    static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_lock(&mutex);
    va_list args;
    va_start(args, format);
    unsigned int remaining = bufferSize - *written;
    unsigned int length = vsnprintf(buffer + *written, remaining, format, args);
    va_end(args);
    if (length >= remaining) {
        qDebug("Output was truncated.\n");
    }
    *written += length;
    pthread_mutex_unlock(&mutex);
    return length;
    
}
static int firstCalculateBitPos(unsigned int num)
{
    int pos = 0;
    while (((num) & (1 << pos)) == 0) pos++;
    return pos;
}
static void logGetCurrentTimeInfo(DEBUG_TIME_INFO *times)
{
    QDateTime currentDateTime = QDateTime::currentDateTime();
    // 提取年月日时分秒信息
    times->year = currentDateTime.date().year();
    times->month = currentDateTime.date().month();
    times->day = currentDateTime.date().day();
    times->hour = currentDateTime.time().hour();
    times->minute = currentDateTime.time().minute();
    times->second = currentDateTime.time().second();
    times->msec = currentDateTime.time().msec();
    return;
}

static void debugPrintSavePosition(const char *debugBuf)
{
    static FILE *file = NULL;
    if(PRINT_TERMINAL == (debugCtrlConfig.debugPrintExhibitValue.debugPrintExhibitFlag&PRINT_TERMINAL))
    {
        // qDebug("%s",debugBuf);
        puts(debugBuf);
        // qDebug(debugBuf);
    }
    if(PRINT_FLASH == (debugCtrlConfig.debugPrintExhibitValue.debugPrintExhibitFlag&PRINT_FLASH))
    {
        if(NULL == file)
        {
            file = fopen(debugCtrlConfig.logFilePath, "a+");
            if (file == NULL) {
                perror("Failed to open file");
                return ;
            }
        }
        fseek(file, 0, SEEK_END);
        // long fileSize = ftell(file);
        // if (fileSize == 0) {
        // }
        if (fputs(debugBuf, file) == EOF) {
            perror("Failed to write to file");
            fclose(file);
            return ;
        }
        return;
    }
    if(NULL != file)
    {
        fclose(file);
        file = NULL;
    }
    return;
}


static void debug_save_data(DEBUG_MQ_MESSAGE_STRUCT *mqMessage)
{
    //static int i=0;
    pthread_mutex_lock(&debugMqData.mutex_wirte);
    pthread_mutex_lock(&debugMqData.data[debugMqData.wirteIndex].mutex);
    DEBUG_MQ_MESSAGE_STRUCT *mMessage = NULL;
    /*保存信息到共享数据池里*/
    mMessage = &debugMqData.data[debugMqData.wirteIndex];
    memcpy(mMessage, mqMessage, sizeof(DEBUG_MQ_MESSAGE_STRUCT));
    debugMqData.wirteIndex++;
    debugMqData.wirteIndex = debugMqData.wirteIndex & (ARRAY_SIZE-1);
    pthread_mutex_unlock(&debugMqData.data[debugMqData.wirteIndex].mutex);
    pthread_mutex_unlock(&debugMqData.mutex_wirte);
    return;
}


void debug_send_log_message(DEBUG_MQ_MESSAGE_STRUCT *mqMessage)
{
    static int i = 0;
    debug_save_data(mqMessage);
    // if((++i % 10) == 0)
    // {
    //     i=0;
    //     usleep(100);
    // }
    // usleep(100);
    // usleep(10);
    pthread_mutex_lock(&mutex);
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);
    return;
}


void logPrint(PRINT_LEVEL printLevel, MODULE_NAME moduleLevel, const char *fileName, const char *funcName, const int lineNum, const char *fmt, ...)
{
    unsigned int length = 0;
    DEBUG_MQ_MESSAGE_STRUCT mqMessageData;
    va_list valist;
    memset(&mqMessageData, 0, sizeof(mqMessageData));
    // memset(&currentTimeInfo, 0, sizeof(currentTimeInfo));
    //判断打印优先级，是否进行显示
    if(printLevel > debugCtrlConfig.level)
    {
        return;
    }
    //判断某些模块是否要显示
    if(moduleLevel != (debugCtrlConfig.debugModuleFlagValue.debugModuleFlag & moduleLevel))
    {
        return;
    }
    if(LOGO_YEAR_MONTH_DAY_STRING == (debugCtrlConfig.debugShowHeadValue.debugShowHeadFlag & LOGO_YEAR_MONTH_DAY_STRING) || \
            LOGO_MINUTES_AND_SECONDS_STRING == (debugCtrlConfig.debugShowHeadValue.debugShowHeadFlag & LOGO_MINUTES_AND_SECONDS_STRING))
    {
        logGetCurrentTimeInfo(&mqMessageData.mTime);
    }
    //封装数据包
    mqMessageData.mutex = PTHREAD_MUTEX_INITIALIZER;
    mqMessageData.printLevel = printLevel;
    mqMessageData.printModuleName = moduleLevel;
    const char *tmpfilename = strrchr(fileName, '/');
    tmpfilename+=1;
    // tmpfilename = strrchr(funcName, '/');

    strncpy(mqMessageData.fileName, tmpfilename, sizeof(mqMessageData.fileName));
    strncpy(mqMessageData.funcName, funcName, sizeof(mqMessageData.funcName));
    mqMessageData.lineNum = lineNum;
    va_start(valist, fmt);
    vsnprintf(mqMessageData.printMess, sizeof(mqMessageData.printMess), fmt, valist);
    va_end(valist);
    debug_send_log_message(&mqMessageData);
    return;
}

void logDataProcess(DEBUG_MQ_MESSAGE_STRUCT *data)
{
    unsigned int length = 0;
    static char buf[BUF_SIZE];
    DEBUG_MQ_MESSAGE_STRUCT mTmpData;
     while(pthread_mutex_lock(&data->mutex)!=0)
     {
        usleep(100);
     }
    memcpy(&mTmpData, data, sizeof(DEBUG_MQ_MESSAGE_STRUCT));
    pthread_mutex_unlock(&data->mutex);
    memset(buf, 0, sizeof(buf));
     if(data->printMess == NULL || *data->printMess == '\0')
    {
        return;
    }
    // qDebug("%s:%s:%d %s\n", mTmpData.fileName, mTmpData.funcName, mTmpData.lineNum,mTmpData.printMess);
    //判断显示的头部标识信息内容
    if(debugCtrlConfig.debugShowHeadValue.bitField.bit1_level)
    {
        debugPrintHeadPack(buf, BUF_SIZE, &length, "%s ", levelString[mTmpData.printLevel]);
    }
    if(debugCtrlConfig.debugShowHeadValue.bitField.bit2_module)
    {
        if(debugCtrlConfig.debugModuleFlagValue.bitField.bit1_app)
        {
            debugPrintHeadPack(buf, BUF_SIZE, &length, "[%s]", printModuleName[0]);
        }
        else if(debugCtrlConfig.debugModuleFlagValue.bitField.bit2_fpga)
        {
            debugPrintHeadPack(buf, BUF_SIZE, &length, "[%s]", printModuleName[1]);
        }
        else if(debugCtrlConfig.debugModuleFlagValue.bitField.bit3_sensor)
        {
            debugPrintHeadPack(buf, BUF_SIZE, &length, "[%s]", printModuleName[2]);
        }
        else if(debugCtrlConfig.debugModuleFlagValue.bitField.bit4_net)
        {
            debugPrintHeadPack(buf, BUF_SIZE, &length, "[%s]", printModuleName[3]);
        }
        else if(debugCtrlConfig.debugModuleFlagValue.bitField.bit5_other)
        {
            debugPrintHeadPack(buf, BUF_SIZE, &length, "[%s]", printModuleName[4]);
        }
    }
    if(debugCtrlConfig.debugShowHeadValue.bitField.bit3_file)
    {
        debugPrintHeadPack(buf, BUF_SIZE, &length, "[%s]", mTmpData.fileName);
    }
    if(debugCtrlConfig.debugShowHeadValue.bitField.bit4_function)
    {
        debugPrintHeadPack(buf, BUF_SIZE, &length, "[%s]", mTmpData.funcName);
    }
    if(debugCtrlConfig.debugShowHeadValue.bitField.bit5_year_month_day)
    {
        debugPrintHeadPack(buf, BUF_SIZE, &length, "[%d-%d-%d]", mTmpData.mTime.year, mTmpData.mTime.month, mTmpData.mTime.day);
    }
    if(debugCtrlConfig.debugShowHeadValue.bitField.bit6_minutes_seconds)
    {
        debugPrintHeadPack(buf, BUF_SIZE, &length, "[%d:%d:%d.%d]", mTmpData.mTime.hour, mTmpData.mTime.minute, mTmpData.mTime.second, mTmpData.mTime.msec);
    }
    if(debugCtrlConfig.debugShowHeadValue.bitField.bit7_rows)
    {
        debugPrintHeadPack(buf, BUF_SIZE, &length, "-%d:", mTmpData.lineNum);
    }
    
    if(5 > (BUF_SIZE - length))
    {
        qDebug("buf too small\n");
        return;
    }
    strncpy(buf+length, mTmpData.printMess, BUF_SIZE - length);
    debugPrintSavePosition(buf);
    return;
}


void *receiver_thread(void *arg) {
    int j = 0;
    unsigned int tmpWirteIndex = 0;
    int difference = 0;
    struct timespec wait_time;
    while(1)
    {
        // pthread_mutex_lock(&mutex);
        // clock_gettime(CLOCK_REALTIME, &wait_time);
        // wait_time.tv_sec += 1;
        // pthread_cond_timedwait(&cond, &mutex, &wait_time);
        pthread_cond_wait(&cond, &mutex);
        // pthread_mutex_unlock(&mutex);

         while(pthread_mutex_lock(&debugMqData.mutex_wirte)!=0)
        {
            usleep(100);
        }
        // pthread_mutex_lock(&debugMqData.mutex_wirte);
        tmpWirteIndex = debugMqData.wirteIndex;
        pthread_mutex_unlock(&debugMqData.mutex_wirte);

        difference = tmpWirteIndex - debugMqData.readIndex;
        // if (difference < 0) {
        //     difference += ARRAY_SIZE;
        // }
        if(0 == difference)
        {
            continue;
        }
        else if (difference < 0)
        {
            for (unsigned int i = debugMqData.readIndex; i < ARRAY_SIZE; ++i)
            {
                logDataProcess(&debugMqData.data[i]);
            }
            for (unsigned int i = 0; i < tmpWirteIndex; ++i)
            {
                logDataProcess(&debugMqData.data[i]);
            }
        }
        else
        {
            for(unsigned int i = debugMqData.readIndex; i < tmpWirteIndex; i++)
            {
                logDataProcess(&debugMqData.data[i]);
            }
        }
        debugMqData.readIndex = tmpWirteIndex;//Update the coordinates
    }
}

#define LOG(...) \
    logPrint(INFO, OTHER, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)

#ifdef DEBUG_LOG_OPEN
#define DEBUG_LOG(level, module, ...) \
    logPrint(level, module, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
#else
    #define DEBUG_LOG(level, module, ...)
#endif

void init_debug(char *path)
{
    static pthread_t receiver_tid;
    strcpy(debugCtrlConfig.logFilePath, path);
    pthread_create(&receiver_tid, NULL, receiver_thread, NULL);
    debugMqData.readIndex = 0;
    debugMqData.wirteIndex = 0;
    debugMqData.mutex_wirte = PTHREAD_MUTEX_INITIALIZER;
    for(int i=0;i<ARRAY_SIZE;i++)
    {
        debugMqData.data[i].mutex = PTHREAD_MUTEX_INITIALIZER;
    }
    // DEBUG_LOG(ERROR,APP,"start\n");
}

int main() {

    // Main process sends messages to the queue
    init_debug("./log_print.txt");

    // qint64 startTime = QDateTime::currentMSecsSinceEpoch();
    // DEBUG_LOG(ERROR,APP,"==1=======regwer\n");
    // DEBUG_LOG(ERROR,APP,"==2=====dssadfasder\n");
    // DEBUG_LOG(ERROR,APP,"==3=======regwer\n");
    // DEBUG_LOG(ERROR,APP,"==4=====dssadfasder\n");
    // DEBUG_LOG(ERROR,APP,"==5=======regwer\n");
    // DEBUG_LOG(ERROR,APP,"===6====dssadfasder\n");
    // DEBUG_LOG(ERROR,APP,"====7=====regwer\n");
    // DEBUG_LOG(ERROR,APP,"====8===dssadfasder\n");
    // DEBUG_LOG(ERROR,APP,"====9=====regwer\n");
    // DEBUG_LOG(ERROR,APP,"===10====dssadfasder\n");
    // DEBUG_LOG(ERROR,APP,"===11====dssadfasder\n");
    for(int i=0;i<100000000;i++)
    {
        DEBUG_LOG(ERROR,APP,"==============================++++++++++++++++++++++++++++%d\n", i);
    }
    // DEBUG_LOG(ERROR,APP,"=======dssadfasder\n");
    sleep(1);
    // while(1)
    // {
    //     // DEBUG_LOG(ERROR,APP,"=======dssadfasder\n");
    //     sleep(2);
    //     // qDebug("Function took %d milliseconds to execute.\n", elapsedTime);
    // }
#if 0
    init_debug("./log_print.txt");
   // LOG("------===%s==%d--%f\n","debug",123,56.34);
    unsigned int i = 0;
    qint64 startTime = QDateTime::currentMSecsSinceEpoch();
    for(i=0;i<10000;i++)
    {
        DEBUG_LOG(ERROR,APP,"=============%d\n", 1);
       // qDebug("=============%d\n", 1);
    }
    qint64 endTime = QDateTime::currentMSecsSinceEpoch();
    // 计算函数调用占用的时间（毫秒）
    qint64 elapsedTime = endTime - startTime;
    qDebug("Function took %d milliseconds to execute.\n", elapsedTime);
    // qDebug() << "Function took" << elapsedTime << "milliseconds to execute.";
   // logPrintSettingInfo();

#endif

    return 0;
}
