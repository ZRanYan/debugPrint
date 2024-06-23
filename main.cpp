#include <QDebug>
#include <QDateTime>
#include <cstdarg> // 包含可变参数的头文件
#include <cstdio>
#include <stdio.h>
#include <sys/time.h>
#include <QDateTime>
#include <cstring>

#define BUF_SIZE 512
char buf[BUF_SIZE];

#define DEBUG_LOG_OPEN

#define INIT_MEMBER(a) {a, #a}

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
static DEBUG_CTRL_STRUCT debugCtrlConfig =
{
    .level = DEBUG,
    .debugModuleFlagValue =
    {
        .debugModuleFlag = 0xFFFF,
    },
    .debugShowHeadValue =
    {
       .debugShowHeadFlag = LOGO_LEVEL_STRING|LOGO_FILE_STRING|LOGO_FUNCTION_STRING|LOGO_ROWS_STRING,
        // .debugShowHeadFlag = 0xFFFF,
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
static unsigned int debugPrintHeadPack(char *buffer, int bufferSize, unsigned int *written, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    unsigned int remaining = bufferSize - *written;
    unsigned int length = vsnprintf(buffer + *written, remaining, format, args);
    va_end(args);
    if (length >= remaining) {
        printf("Output was truncated.\n");
    }
    *written += length;
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
        qDebug(debugBuf);
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
void logPrint(PRINT_LEVEL printLevel, MODULE_NAME moduleLevel, const char *fileName, const char *funcName, const int lineNum, const char *fmt, ...)
{
    unsigned int length = 0;
    DEBUG_TIME_INFO currentTimeInfo;
    memset(buf, 0, sizeof(buf));
    memset(&currentTimeInfo, 0, sizeof(currentTimeInfo));
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
        logGetCurrentTimeInfo(&currentTimeInfo);
    }
    //判断显示的头部标识信息内容
    for(int i=0;i<sizeof(headLog)/sizeof(SHOW_HEAD_LOGO);i++)
    {
        switch(debugCtrlConfig.debugShowHeadValue.debugShowHeadFlag & headLog[i].headLog)
        {
            case LOGO_LEVEL_STRING:
                debugPrintHeadPack(buf, BUF_SIZE, &length, "%s ", levelString[printLevel]);
                break;
            case LOGO_MODULE_STRING:
                debugPrintHeadPack(buf, BUF_SIZE, &length, "[%s]", printModuleName[firstCalculateBitPos((unsigned int)moduleLevel)]);
                break;
            case LOGO_FILE_STRING:
                debugPrintHeadPack(buf, BUF_SIZE, &length, "[%s]", fileName);
                break;
            case LOGO_FUNCTION_STRING:
                debugPrintHeadPack(buf, BUF_SIZE, &length, "[%s]", funcName);
                break;
            case LOGO_ROWS_STRING:
                debugPrintHeadPack(buf, BUF_SIZE, &length, "-%d:", lineNum);
                break;
            case LOGO_YEAR_MONTH_DAY_STRING:
                debugPrintHeadPack(buf, BUF_SIZE, &length, "[%d-%d-%d]", currentTimeInfo.year, currentTimeInfo.month, currentTimeInfo.day);
                break;
            case LOGO_MINUTES_AND_SECONDS_STRING:
                debugPrintHeadPack(buf, BUF_SIZE, &length, "[%d:%d:%d.%d]", currentTimeInfo.hour, currentTimeInfo.minute, currentTimeInfo.second,currentTimeInfo.msec);
                break;
            default:
                break;
        }
    }
    if(5 > length)
    {
        printf("buf too small\n");
        return;
    }
    char *data = buf+length;
    va_list valist;
    va_start(valist, fmt);
    vsnprintf(data, BUF_SIZE - length, fmt, valist);
    va_end(valist);
   // printf("%s\n", buf);
    // puts(buf);
    // qDebug(buf);
    debugPrintSavePosition(buf);
    return;
}
void init_debug(char *path)
{
    strcpy(debugCtrlConfig.logFilePath, path);
}
/*打印当前的参数配置情况*/
void logPrintSettingInfo()
{
    int i,j;
    printf("-------------------current debug setting---------------------\n");
    printf("debug level:\n");
    printf("\t%s\n", levelString[debugCtrlConfig.level]);
    printf("debug module:\n\t");
    for(i=0; i<sizeof(printModuleName)/sizeof(printModuleName[0]);i++)
    {
        if((1<<i) == (debugCtrlConfig.debugModuleFlagValue.debugModuleFlag & (1<<i)))
        {
            printf("%s ", printModuleName[i]);
        }
    }
    printf("\ndebug show head:\n\t");
    for(i=0,j=0; i<sizeof(headLog)/sizeof(headLog[0]);i++)
    {
        if((headLog[i].headLog) == (debugCtrlConfig.debugShowHeadValue.debugShowHeadFlag & (headLog[i].headLog)))
        {
            j++;
            printf("%s_string ", headLog[i].headLogString);
            if(j%4 == 0)
            {
                printf("\n\t");
            }
        }
    }
    printf("\ndebug print exhibit:\n\t");
    if(PRINT_TERMINAL == (debugCtrlConfig.debugPrintExhibitValue.debugPrintExhibitFlag & PRINT_TERMINAL))
    {
        printf("PRINT_TERMINAL ");
    }
    if(PRINT_FLASH == (debugCtrlConfig.debugPrintExhibitValue.debugPrintExhibitFlag & PRINT_FLASH))
    {
        printf("PRINT_TERMINAL:%s", debugCtrlConfig.logFilePath);
    }
    printf("\n----------------------------end----------------------------\n");
}


#define LOG(...) \
logPrint(INFO, OTHER, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)

#ifdef DEBUG_LOG_OPEN
#define DEBUG_LOG(level, module, ...) \
    logPrint(level, module, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
#else
    #define DEBUG_LOG(level, module, ...)
#endif

int main() {
    init_debug("./log_print.txt");
   // LOG("------===%s==%d--%f\n","debug",123,56.34);
    DEBUG_LOG(ERROR,APP,"=============%d\n", 1);
    DEBUG_LOG(WARNING,APP,"=============%d\n", 1);
    DEBUG_LOG(INFO,APP,"=============%d\n", 1);
    DEBUG_LOG(DEBUG,APP,"=============%d\n", 1);
    DEBUG_LOG(DEBUG,SENSOR,"=============%d\n", 1);
    DEBUG_LOG(DEBUG,NET,"=============%d\n", 1);
    logPrintSettingInfo();
    return 0;
}
