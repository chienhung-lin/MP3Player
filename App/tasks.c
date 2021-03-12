/************************************************************************************

Copyright (c) 2001-2016  University of Washington Extension.

Module Name:

    tasks.c

Module Description:

    The tasks that are executed by the test application.

2016/2 Nick Strathy adapted it for NUCLEO-F401RE 

************************************************************************************/
#include <stdarg.h>

#include "bsp.h"
#include "print.h"
#include "mp3Util.h"

#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ILI9341.h>
#include <Adafruit_FT6206.h>
#include "SD.h"

Adafruit_ILI9341 lcdCtrl = Adafruit_ILI9341(); // The LCD controller

Adafruit_FT6206 touchCtrl = Adafruit_FT6206(); // The touch controller

#define PENRADIUS 3

long MapTouchToScreen(long x, long in_min, long in_max, long out_min, long out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}


//#include "train_crossing.h"

#define BUFSIZE 256

/************************************************************************************

   Allocate the stacks for each task.
   The maximum number of tasks the application can have is defined by OS_MAX_TASKS in os_cfg.h

************************************************************************************/

static OS_STK   TouchTaskStk[APP_CFG_TASK_START_STK_SIZE];
static OS_STK   Mp3DemoTaskStk[APP_CFG_TASK_START_STK_SIZE];
static OS_STK   LcdDisplayTaskStk[APP_CFG_TASK_START_STK_SIZE];

// Task prototypes
void TouchTask(void* pdata);
void Mp3DemoTask(void* pdata);
void LcdDisplayTask(void* pdata);

// Useful functions
void PrintToLcdWithBuf(char *buf, int size, char *format, ...);

// Globals
BOOLEAN nextSong = OS_FALSE;

/*******************************************************************************

UI Objects

*******************************************************************************/

/* Button Geomertic Infomation */
#define PLAY_X (10U)
#define PLAY_Y (170U)
#define PLAY_W (60U)
#define PLAY_H (40U)

#define PAUSE_X (80U)
#define PAUSE_Y (170U)
#define PAUSE_W (60U)
#define PAUSE_H (40U)

#define STOP_X (150U)
#define STOP_Y (170U)
#define STOP_W (60U)
#define STOP_H (40U)

#define VOL_UP_X (10U)
#define VOL_UP_Y (230U)
#define VOL_UP_W (40U)
#define VOL_UP_H (40U)

#define VOL_DW_X (60U)
#define VOL_DW_Y (230U)
#define VOL_DW_W (40U)
#define VOL_DW_H (40U)

#define PREV_X (110U)
#define PREV_Y (230U)
#define PREV_W (40U)
#define PREV_H (40U)

#define NEXT_X (160U)
#define NEXT_Y (230U)
#define NEXT_W (40U)
#define NEXT_H (40U)

#define UI_BUTTON_SIZES (sizeof(button_array)/sizeof(button_array[0]))

typedef void (*bu_e_cb_t)(Adafruit_ILI9341 *, void *);

typedef struct ui_button_type {
    Adafruit_GFX_Button *button;
    bu_e_cb_t bu_event_cb;
} ui_button_t;

static Adafruit_GFX_Button play_bu;
static Adafruit_GFX_Button pause_bu;
static Adafruit_GFX_Button stop_bu;
static Adafruit_GFX_Button vol_up_bu;
static Adafruit_GFX_Button vol_down_bu;
static Adafruit_GFX_Button prev_bu;
static Adafruit_GFX_Button next_bu;

static void play_bu_e_cb(Adafruit_ILI9341 *, void *);
static void pause_bu_e_cb(Adafruit_ILI9341 *, void *);
static void stop_bu_e_cb(Adafruit_ILI9341 *, void *);
static void vol_up_bu_e_cb(Adafruit_ILI9341 *, void *);
static void vol_down_bu_e_cb(Adafruit_ILI9341 *, void *);
static void prev_bu_e_cb(Adafruit_ILI9341 *, void *);
static void next_bu_e_cb(Adafruit_ILI9341 *, void *);

static ui_button_t button_array[] =
{
    { &play_bu, play_bu_e_cb }, 
    { &pause_bu, pause_bu_e_cb }, 
    { &stop_bu, stop_bu_e_cb },
    { &vol_up_bu, vol_up_bu_e_cb },
    { &vol_down_bu, vol_down_bu_e_cb },
    { &prev_bu, prev_bu_e_cb },
    { &next_bu, next_bu_e_cb }
};

void play_bu_e_cb(Adafruit_ILI9341 *_gfx, void *_arg)
{
    char *buf = (char *)_arg;
    _gfx->fillRect(10, 10, 220, 80, ILI9341_BLACK);
    _gfx->setCursor(40, 60);
    _gfx->setTextColor(ILI9341_WHITE);  
    _gfx->setTextSize(2);
    PrintToLcdWithBuf(buf, BUFSIZE, "PLAY");
}

void pause_bu_e_cb(Adafruit_ILI9341 *_gfx, void *_arg)
{    
    char *buf = (char *)_arg;
    _gfx->fillRect(10, 10, 220, 80, ILI9341_BLACK);
    _gfx->setCursor(40, 60);
    _gfx->setTextColor(ILI9341_WHITE);  
    _gfx->setTextSize(2);
    PrintToLcdWithBuf(buf, BUFSIZE, "PAUSE");
}

void stop_bu_e_cb(Adafruit_ILI9341 *_gfx, void *_arg)
{
    char *buf = (char *)_arg;
    _gfx->fillRect(10, 10, 220, 80, ILI9341_BLACK);
    _gfx->setCursor(40, 60);
    _gfx->setTextColor(ILI9341_WHITE);  
    _gfx->setTextSize(2);
    PrintToLcdWithBuf(buf, BUFSIZE, "STOP");
}

void vol_up_bu_e_cb(Adafruit_ILI9341 *_gfx, void *_arg)
{
    char *buf = (char *)_arg;
    _gfx->fillRect(10, 10, 220, 80, ILI9341_BLACK);
    _gfx->setCursor(40, 60);
    _gfx->setTextColor(ILI9341_WHITE);  
    _gfx->setTextSize(2);
    PrintToLcdWithBuf(buf, BUFSIZE, "VOL UP");
}

void vol_down_bu_e_cb(Adafruit_ILI9341 *_gfx, void *_arg)
{
    char *buf = (char *)_arg;
    _gfx->fillRect(10, 10, 220, 80, ILI9341_BLACK);
    _gfx->setCursor(40, 60);
    _gfx->setTextColor(ILI9341_WHITE);  
    _gfx->setTextSize(2);
    PrintToLcdWithBuf(buf, BUFSIZE, "VOL DOWN");
}

void prev_bu_e_cb(Adafruit_ILI9341 *_gfx, void *_arg)
{
    char *buf = (char *)_arg;
    _gfx->fillRect(10, 10, 220, 80, ILI9341_BLACK);
    _gfx->setCursor(40, 60);
    _gfx->setTextColor(ILI9341_WHITE);  
    _gfx->setTextSize(2);
    PrintToLcdWithBuf(buf, BUFSIZE, "PREV");
}

void next_bu_e_cb(Adafruit_ILI9341 *_gfx, void *_arg)
{
    char *buf = (char *)_arg;
    _gfx->fillRect(10, 10, 220, 80, ILI9341_BLACK);
    _gfx->setCursor(40, 60);
    _gfx->setTextColor(ILI9341_WHITE);  
    _gfx->setTextSize(2);
    PrintToLcdWithBuf(buf, BUFSIZE, "NEXT");
}

/************************************************************************************

   This task is the initial task running, started by main(). It starts
   the system tick timer and creates all the other tasks. Then it deletes itself.

************************************************************************************/
void StartupTask(void* pdata)
{
	char buf[BUFSIZE];

    PjdfErrCode pjdfErr;
    INT32U length;
    static HANDLE hSD = 0;
    static HANDLE hSPI = 0;

    PrintWithBuf(buf, BUFSIZE, "StartupTask: Begin\n");
    PrintWithBuf(buf, BUFSIZE, "StartupTask: Starting timer tick\n");

    // Start the system tick
    SetSysTick(OS_TICKS_PER_SEC);
    
    // Initialize SD card
    PrintWithBuf(buf, PRINTBUFMAX, "Opening handle to SD driver: %s\n", PJDF_DEVICE_ID_SD_ADAFRUIT);
    hSD = Open(PJDF_DEVICE_ID_SD_ADAFRUIT, 0);
    if (!PJDF_IS_VALID_HANDLE(hSD)) while(1);


    PrintWithBuf(buf, PRINTBUFMAX, "Opening SD SPI driver: %s\n", SD_SPI_DEVICE_ID);
    // We talk to the SD controller over a SPI interface therefore
    // open an instance of that SPI driver and pass the handle to 
    // the SD driver.
    hSPI = Open(SD_SPI_DEVICE_ID, 0);
    if (!PJDF_IS_VALID_HANDLE(hSPI)) while(1);
    
    length = sizeof(HANDLE);
    pjdfErr = Ioctl(hSD, PJDF_CTRL_SD_SET_SPI_HANDLE, &hSPI, &length);
    if(PJDF_IS_ERROR(pjdfErr)) while(1);

    if (!SD.begin(hSD)) {
        PrintWithBuf(buf, PRINTBUFMAX, "Attempt to initialize SD card failed.\n");
    }
    
    // Create the test tasks
    PrintWithBuf(buf, BUFSIZE, "StartupTask: Creating the application tasks\n");

    // The maximum number of tasks the application can have is defined by OS_MAX_TASKS in os_cfg.h
    OSTaskCreate(Mp3DemoTask, (void*)0, &Mp3DemoTaskStk[APP_CFG_TASK_START_STK_SIZE-1], MP3PLAY_TASKK_PRIO);
    OSTaskCreate(TouchTask, (void*)0, &TouchTaskStk[APP_CFG_TASK_START_STK_SIZE-1], TOUCH_TASKK_PRIO);
    OSTaskCreate(LcdDisplayTask, (void*)0, &LcdDisplayTaskStk[APP_CFG_TASK_START_STK_SIZE-1], DISPLAY_TASK_PRIO);

    // Delete ourselves, letting the work be done in the new tasks.
    PrintWithBuf(buf, BUFSIZE, "StartupTask: deleting self\n");
    
    OSTaskDel(OS_PRIO_SELF);
}

static void DrawLcdContents()
{
    char buf[BUFSIZE];
    OS_CPU_SR cpu_sr;
    
    // allow slow lower pri drawing operation to finish without preemption
    OS_ENTER_CRITICAL(); 
    
    lcdCtrl.fillScreen(ILI9341_BLACK);
    
    // Print a message on the LCD
    lcdCtrl.setCursor(40, 60);
    lcdCtrl.setTextColor(ILI9341_WHITE);  
    lcdCtrl.setTextSize(2);
    PrintToLcdWithBuf(buf, BUFSIZE, "Hello World!");

    OS_EXIT_CRITICAL();

}

static void DrawGrid(Adafruit_ILI9341 *gfx, 
                     uint32_t u32_x, uint32_t u32_y,
                     uint32_t u32_w, uint32_t u32_h,
                     uint32_t u32_x_grid, uint32_t u32_y_grid,
                     uint32_t u32_color)
{

    uint32_t u32_i, u32_j;
    
    // draw horizontal line
    for (u32_j = u32_y; u32_j < u32_y+u32_h; u32_j += u32_y_grid) {
        gfx->drawFastHLine(u32_x, u32_j, u32_w, u32_color);
    }
    
    // draw vertical line
    for (u32_i = u32_x; u32_i < u32_x+u32_w; u32_i += u32_x_grid) {
        gfx->drawFastVLine(u32_i, u32_y, u32_h, ILI9341_WHITE);
    }
}

/************************************************************************************

   Runs Lcd Display code

************************************************************************************/
void LcdDisplayTask(void* pdata)
{
    PjdfErrCode pjdfErr;
    INT8U  device_address, u8_error;
    INT32U length;

    char buf[BUFSIZE];
    PrintWithBuf(buf, BUFSIZE, "LcdDisplayTask: starting\n");

    PrintWithBuf(buf, BUFSIZE, "Opening LCD driver: %s\n", PJDF_DEVICE_ID_LCD_ILI9341);
    // Open handle to the LCD driver
    HANDLE hLcd = Open(PJDF_DEVICE_ID_LCD_ILI9341, 0);
    if (!PJDF_IS_VALID_HANDLE(hLcd)) while(1);

    PrintWithBuf(buf, BUFSIZE, "Opening LCD SPI driver: %s\n", LCD_SPI_DEVICE_ID);
    // We talk to the LCD controller over a SPI interface therefore
    // open an instance of that SPI driver and pass the handle to 
    // the LCD driver.
    HANDLE hSPI = Open(LCD_SPI_DEVICE_ID, 0);
    if (!PJDF_IS_VALID_HANDLE(hSPI)) while(1);

    length = sizeof(HANDLE);
    pjdfErr = Ioctl(hLcd, PJDF_CTRL_LCD_SET_SPI_HANDLE, &hSPI, &length);
    if(PJDF_IS_ERROR(pjdfErr)) while(1);

    PrintWithBuf(buf, BUFSIZE, "Initializing LCD controller\n");
    lcdCtrl.setPjdfHandle(hLcd);
    lcdCtrl.begin();

    // UI Initialize
    //DrawLcdContents();
    
    lcdCtrl.fillScreen(ILI9341_BLACK);
    
    DrawGrid(&lcdCtrl, 0, 0, ILI9341_TFTWIDTH, ILI9341_TFTHEIGHT,
             20, 20, ILI9341_WHITE);
    
    lcdCtrl.drawRect(10, 10, 220, 80, ILI9341_MAROON);
    
    lcdCtrl.drawRect(10, 110, 220, 40, ILI9341_MAROON);
    
#if 1
    play_bu.initButton(&lcdCtrl,
                           PLAY_X + PLAY_W/2, \
                           PLAY_Y + PLAY_H/2, \
                           PLAY_W, PLAY_H, \
                           ILI9341_WHITE, ILI9341_BLACK, ILI9341_WHITE, \
                           "Play", 2);
    
    pause_bu.initButton(&lcdCtrl,
                           PAUSE_X + PAUSE_W/2, \
                           PAUSE_Y + PAUSE_H/2, \
                           PAUSE_W, PAUSE_H, \
                           ILI9341_WHITE, ILI9341_BLACK, ILI9341_WHITE, \
                           "Pause", 2);
    
    stop_bu.initButton(&lcdCtrl,
                           STOP_X + STOP_W/2, \
                           STOP_Y + STOP_H/2, \
                           STOP_W, STOP_H, \
                           ILI9341_WHITE, ILI9341_BLACK, ILI9341_WHITE, \
                           "Stop", 2);
    
    vol_up_bu.initButton(&lcdCtrl,
                           VOL_UP_X + VOL_UP_W/2, \
                           VOL_UP_Y + VOL_UP_H/2, \
                           VOL_DW_W, VOL_UP_H, \
                           ILI9341_WHITE, ILI9341_BLACK, ILI9341_WHITE, \
                           "+", 2);
    
    vol_down_bu.initButton(&lcdCtrl,
                           VOL_DW_X + VOL_DW_W/2, \
                           VOL_DW_Y + VOL_DW_H/2, \
                           VOL_DW_W, VOL_DW_H, \
                           ILI9341_WHITE, ILI9341_BLACK, ILI9341_WHITE, \
                           "-", 2);

    prev_bu.initButton(&lcdCtrl,
                           PREV_X + PREV_W/2, \
                           PREV_Y + PREV_H/2, \
                           PREV_W, PREV_H, \
                           ILI9341_WHITE, ILI9341_BLACK, ILI9341_WHITE, \
                           "<", 2);
    
    next_bu.initButton(&lcdCtrl,
                           NEXT_X + NEXT_W/2, \
                           NEXT_Y + NEXT_H/2, \
                           NEXT_W, NEXT_H, \
                           ILI9341_WHITE, ILI9341_BLACK, ILI9341_WHITE, \
                           ">", 2);

    play_bu.drawButton();
    pause_bu.drawButton();
    stop_bu.drawButton();
    vol_up_bu.drawButton();
    vol_down_bu.drawButton();
    prev_bu.drawButton();
    next_bu.drawButton();
#endif
    
    u8_error = OSTaskResume(DISPLAY_TASK_PRIO);
    if (u8_error != OS_ERR_NONE) {
        PrintWithBuf(buf, BUFSIZE, "Display Task resume error: %d\n", u8_error);
    }
    u8_error = OSTaskSuspend(OS_PRIO_SELF);
    while (1) {
//        swtich () {
//        case ;
//        case ;
//        default;
//        }
    }
    
}

/************************************************************************************

   Runs Touch code

************************************************************************************/
void TouchTask(void* pdata)
{
    PjdfErrCode pjdfErr;
    INT8U  device_address, u8_error;
    INT32U length;
    uint32_t u32_i;
    ui_button_t *bu = NULL;

    char buf[BUFSIZE];
    PrintWithBuf(buf, BUFSIZE, "TouchTask: starting\n");
    
    PrintWithBuf(buf, BUFSIZE, "Initializing FT6206 touchscreen controller\n");
    
    // DRIVER TODO-done
    // Open a HANDLE for accessing device PJDF_DEVICE_ID_I2C1
    // <your code here>    
    HANDLE hI2C1 = Open(PJDF_DEVICE_ID_I2C1, 0);
    if (!PJDF_IS_VALID_HANDLE(hI2C1)) while(1);
    
    // Call Ioctl on that handle to set the I2C device address to FT6206_ADDR
    // <your code here>
    device_address = FT6206_ADDR;
    length = sizeof(device_address);
    pjdfErr = Ioctl(hI2C1, PJDF_CTRL_I2C_SET_DEVICE_ADDRESS, &device_address, &length);
    if(PJDF_IS_ERROR(pjdfErr)) while(1);

    // Call setPjdfHandle() on the touch contoller to pass in the I2C handle
    // <your code here>
    touchCtrl.setPjdfHandle(hI2C1);
        
    if (! touchCtrl.begin(40)) {  // pass in 'sensitivity' coefficient
        PrintWithBuf(buf, BUFSIZE, "Couldn't start FT6206 touchscreen controller\n");
        while (1);
    }
    
    // wait for lcd initialize finished
    PrintWithBuf(buf, BUFSIZE, "Wait for LcdDisplay task...\n");
    u8_error = OSTaskSuspend(OS_PRIO_SELF);
    if (u8_error != OS_ERR_NONE) {
        PrintWithBuf(buf, BUFSIZE, "Touch Task exit suspend error: %d\n", u8_error);
    }
    

    while (1) { 
        boolean touched;
        
        touched = touchCtrl.touched();
        
        if (! touched) {
            OSTimeDly(5);
        } else {
            TS_Point touch_p, convert_p = TS_Point();
            
            touch_p = touchCtrl.getPoint();
            convert_p.x = MapTouchToScreen(touch_p.x, 0, ILI9341_TFTWIDTH, ILI9341_TFTWIDTH, 0);
            convert_p.y = MapTouchToScreen(touch_p.y, 0, ILI9341_TFTHEIGHT, ILI9341_TFTHEIGHT, 0);
            
            for (u32_i = 0;u32_i < UI_BUTTON_SIZES;++u32_i) {
                bu = &button_array[u32_i];
                if (bu->button->contains(convert_p.x, convert_p.y)) {
                    bu->bu_event_cb(&lcdCtrl, (void *)buf);
                    break;
                }
            }
        }
    }
}

/************************************************************************************

   Runs MP3 demo code

************************************************************************************/
void Mp3DemoTask(void* pdata)
{
    PjdfErrCode pjdfErr;
    INT32U length;
    
    char buf[BUFSIZE];
    PrintWithBuf(buf, BUFSIZE, "Mp3DemoTask: starting\n");

    PrintWithBuf(buf, BUFSIZE, "Opening MP3 driver: %s\n", PJDF_DEVICE_ID_MP3_VS1053);
    // Open handle to the MP3 decoder driver
    HANDLE hMp3 = Open(PJDF_DEVICE_ID_MP3_VS1053, 0);
    if (!PJDF_IS_VALID_HANDLE(hMp3)) while(1);

    PrintWithBuf(buf, BUFSIZE, "Opening MP3 SPI driver: %s\n", MP3_SPI_DEVICE_ID);
    // We talk to the MP3 decoder over a SPI interface therefore
    // open an instance of that SPI driver and pass the handle to 
    // the MP3 driver.
    HANDLE hSPI = Open(MP3_SPI_DEVICE_ID, 0);
    if (!PJDF_IS_VALID_HANDLE(hSPI)) while(1);

    length = sizeof(HANDLE);
    pjdfErr = Ioctl(hMp3, PJDF_CTRL_MP3_SET_SPI_HANDLE, &hSPI, &length);
    if(PJDF_IS_ERROR(pjdfErr)) while(1);

    // Send initialization data to the MP3 decoder and run a test
    PrintWithBuf(buf, BUFSIZE, "Starting MP3 device test\n");
    
    Mp3Init(hMp3);
    int count = 0;
    
    while (1)
    {
        OSTimeDly(500);
        PrintWithBuf(buf, BUFSIZE, "Begin streaming sound file  count=%d\n", ++count);
        // Mp3Stream(hMp3, (INT8U*)Train_Crossing, sizeof(Train_Crossing)); 
        Mp3StreamSDFile(hMp3, "TRAIN001.mp3");
        PrintWithBuf(buf, BUFSIZE, "Done streaming sound file  count=%d\n", count);
    }
}


// Renders a character at the current cursor position on the LCD
static void PrintCharToLcd(char c)
{
    lcdCtrl.write(c);
}

/************************************************************************************

   Print a formated string with the given buffer to LCD.
   Each task should use its own buffer to prevent data corruption.

************************************************************************************/
void PrintToLcdWithBuf(char *buf, int size, char *format, ...)
{
    va_list args;
    va_start(args, format);
    PrintToDeviceWithBuf(PrintCharToLcd, buf, size, format, args);
    va_end(args);
}




