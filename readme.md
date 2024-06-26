# 宏打印模块功能说明

## 运行环境

​		qt create软件打开，适用于linux开发环境

## 解决问题

​	   封装抽象DEBUG调试打印宏，方便自定义调试，支持等级配置，各种不同独立模块显示，打印日志信息，日志文件写入等功能

## 支持的功能说明

#### 打印调用函数说明

```c
需开启宏 DEBUG_LOG_OPEN
init_debug("./log_print.txt");//传参默认打印信息保存位置
DEBUG_LOG(APP, ERROR, "----%d---%f--%s-\n", 1, 1.1, "hellow");//开始使用
```

#### 打印显示功能

  * 目前支持等级，模块，文件名，函数名，年月日，时分秒，行数信息的打印，全部打印如下所示

    ```c
    ERROR [app][main.cpp][thread_function][2024-6-26][20:23:13.224]-535:打印的自定内容
    ```

  * 支持上面的打印模块自定义显示，每块可以独立控制显示，控制枚举参数和默认参数配置如下

	```c
	typedef enum
	{
	    LOGO_NONE = 0,
	    LOGO_LEVEL_STRING = 1 << 0,
	    LOGO_MODULE_STRING = 1 << 1,
	    LOGO_FILE_STRING = 1 << 2,
	    LOGO_FUNCTION_STRING = 1 << 3,
	    LOGO_YEAR_MONTH_DAY_STRING = 1 << 4,
	    LOGO_MINUTES_AND_SECONDS_STRING = 1 << 5,
	    LOGO_ROWS_STRING = 1 << 6,
	} SHOW_HEAD_LOGO;
	全局结构体参数debugCtrlConfig：
	.debugShowHeadValue =
	            {
	                //    .debugShowHeadFlag = 				 LOGO_LEVEL_STRING|LOGO_FILE_STRING|LOGO_FUNCTION_STRING|LOGO_ROWS_STRING,
	                .debugShowHeadFlag = 0xFFFF,//默认全显示
	                // .debugShowHeadFlag = LOGO_LEVEL_STRING,
	            },
	```

* 支持自定义模块打印配置，当前配置app,fpga,sensor,net,other模块名字显示，默认显示配置开关如下

  ```c
  全局结构体参数debugCtrlConfig：
  .debugShowHeadValue =
  {
  //    .debugShowHeadFlag = LOGO_LEVEL_STRING|LOGO_FILE_STRING|LOGO_FUNCTION_STRING|LOGO_ROWS_STRING,
        .debugShowHeadFlag = 0xFFFF,
     // .debugShowHeadFlag = LOGO_LEVEL_STRING,
  },//目前可以扩充到16个模块含义打印，每个可以独立配置显示开关
  ```

* 支持默认等级配置，等级说明如下

  ```c
  ERROR>WARNING>INFO>DEBUG>DEBUG，配置的等级值越低打印越详细
  目前仅支持五个等级配置，可以扩充到16个等级配置
  ```

* 打印保存日志功能，可以进行调试打印进行终端显示和保存到文件，默认配置说明如下

  ```c
  typedef enum
  {
      PRINT_TERMINAL = 1 << 0,
      PRINT_FLASH = 2 << 0
  } LOG_STORAGE_METHOD;
  全局结构体参数debugCtrlConfig：
  .debugPrintExhibitValue =
  {
  	.debugPrintExhibitFlag = /*PRINT_TERMINAL|*/ PRINT_FLASH,
  },
  // .logFilePath = "./log_print.txt"
  ```

* 日志文件配置说明，目前支持日志文件的大小可配置(单位Kb)，解决过多的日志文件占用系统存储资源需求，日志文件支持循环擦除旧日志写入新日志信息

  ```c
  全局结构体参数debugCtrlConfig：
  .logFileSize = 1024000 * 2,
  ```

* 配置开关宏定义说明，对于debug程序可以开启`DEBUG_LOG_OPEN`宏,release程序可以去掉这个宏，并不占用程序任何资源文件

## 性能特色说明

* 实现打印写入端和显示端分离，写入端仅负责将需要打印信息放入共享内存池，减少调试端打印占用的程序的性能资源，显示端由独立的低优先级线程负责从共享内存池获取数据，并根据选择负责繁琐的IO调用显示
* 目前开辟512个队列，用于应对多线程和突发大量的调试信息写入，本地自测400个线程并发写入，10G左右的日志文件写入不丢失

### 代码配宏说明

```c
#define DEBUG_LOG_OPEN 			//配置宏开关
#define BUF_SIZE 1024 			//单条debug打印内容512字节
#define ARRAY_SIZE 512			//缓存池队列对象数量
#define FILE_NAME_LENGTH 20		//显示文件名字长度
#define FUNC_NAME_LENGTH 30		//显示函数名字
#define PRINT_MESS_LENGTH 512	//单次打印内容
#define LOG_FILE_PATH_LENGTH 50	//保存日志文件的路径长度
```

