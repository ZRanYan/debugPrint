#include <QDebug>
#include <QDateTime>
#include <cstdarg> // 包含可变参数的头文件
#include <cstdio>

#define BUF_SIZE 300
char buf[BUF_SIZE];
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
    LOGO_ROWS_STRING = 1<<4,
}SHOW_HEAD_LOGO;

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
            unsigned short int bit5_rows : 1;
        }bitField;
    }debugShowHeadValue;

}DEBUG_CTRL_STRUCT;

static DEBUG_CTRL_STRUCT debugCtrlConfig =
{
    .level = DEBUG,
    .debugModuleFlagValue =
    {
        .debugModuleFlag = 0xFFFF,
    },
    .debugShowHeadValue =
    {
       // .debugShowHeadFlag = LOGO_LEVEL_STRING|LOGO_FILE_STRING|LOGO_FUNCTION_STRING|LOGO_ROWS_STRING,
        .debugShowHeadFlag = LOGO_LEVEL_STRING,
    },
};

static char *levelString[] = {"ERROR", "WARNING", "INFO", "DEBUG", " "};
static char *printModuleName[] = {"app","fpga","sensor","net","other"};
static SHOW_HEAD_LOGO headLog[] = {LOGO_LEVEL_STRING,LOGO_MODULE_STRING,LOGO_FILE_STRING,LOGO_FUNCTION_STRING,LOGO_ROWS_STRING};
unsigned int printHeadPack(char *buffer, int bufferSize, unsigned int *written, const char *format, ...)
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

static int firstCalculateBitPos(MODULE_NAME num)
{
    int pos = 0;
    while (((num) & (1 << pos)) == 0) pos++;
    return pos;
}

void logPrint(PRINT_LEVEL printLevel, MODULE_NAME moduleLevel, const char *fileName, const char *funcName, const int lineNum, const char *fmt, ...)
{
    memset(buf, 0, sizeof(buf));
    unsigned int length = 0;
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
    //判断显示的头部标识信息内容
    for(int i=0;i<sizeof(headLog)/sizeof(SHOW_HEAD_LOGO);i++)
    {
        switch(debugCtrlConfig.debugShowHeadValue.debugShowHeadFlag & headLog[i])
        {
            case LOGO_LEVEL_STRING:
                printHeadPack(buf, BUF_SIZE, &length, "%s ", levelString[printLevel]);
                break;
            case LOGO_MODULE_STRING:
                printHeadPack(buf, BUF_SIZE, &length, "[%s]", printModuleName[firstCalculateBitPos(moduleLevel)]);
                break;
            case LOGO_FILE_STRING:
                printHeadPack(buf, BUF_SIZE, &length, "[%s]", fileName);
                break;
            case LOGO_FUNCTION_STRING:
                printHeadPack(buf, BUF_SIZE, &length, "[%s]", funcName);
                break;
            case LOGO_ROWS_STRING:
                printHeadPack(buf, BUF_SIZE, &length, "%d:", lineNum);
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
    puts(buf);
    return;
}

#define LOG(...) \
            logPrint(INFO, OTHER, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)

#define DEBUG_LOG(level, module, ...) \
            logPrint(level, module, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)


int main() {

   // LOG("------===%s==%d--%f\n","debug",123,56.34);
    DEBUG_LOG(ERROR,APP,"=============%d\n", 1);
    DEBUG_LOG(WARNING,APP,"=============%d\n", 1);
    DEBUG_LOG(INFO,APP,"=============%d\n", 1);
    DEBUG_LOG(DEBUG,APP,"=============%d\n", 1);
    DEBUG_LOG(DEBUG,SENSOR,"=============%d\n", 1);
    DEBUG_LOG(DEBUG,NET,"=============%d\n", 1);
    return 0;
}
