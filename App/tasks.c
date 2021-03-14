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

#define BUFSIZE 128

/*******************************************************************************

    Display Task Command

*******************************************************************************/

typedef enum display_tk_cmd_type {
    DISPLAY_TK_NONE = 0,
    DISPLAY_TK_CLICK,
    DISPLAY_TK_UI_CB
} display_tk_cmd_t;

/*******************************************************************************

    MP3 Task Command

*******************************************************************************/

typedef enum mp3_tk_cmd_type {
    MP3_TK_NONE = 0,
    MP3_TK_PLAY,
    MP3_TK_PAUSE,
    MP3_TK_STOP,
    MP3_TK_VOL_UP,
    MP3_TK_VOL_DOWN,
    MP3_TK_PREV,
    MP3_TK_NEXT
} mp3_tk_cmd_t;

typedef enum mp3_dr_cmd_type {
    MP3_DR_NONE = 0,
    MP3_DR_PLAY_CHUNK,
    MP3_DR_VOL_CHANGE,
    MP3_DR_SOFT_RESET
} mp3_dr_cmd_t;

/*******************************************************************************

    UI structure

*******************************************************************************/

typedef void (*ui_event_cb_t)(Adafruit_ILI9341 *, void *);

#define UI_BUTTON_INFO_INIT(x, y, w, h, outline, fill, t_color, str, t_size) \
    { x, y, w, h, outline, fill, t_color, str, t_size }

// 32 bytes
typedef struct ui_button_info_type {
    uint16_t x;
    uint16_t y;
    uint16_t w;
    uint16_t h;
    uint16_t outline;
    uint16_t fill;
    uint16_t textcolor;
    char label[16];
    uint8_t textsize;
    char dummy[1]; // for aliagn
} ui_button_info_t;

#define TEXT_INFO_INIT(r_x, r_y, r_w, r_h, t_x, t_y, fill, t_color, t_size, str) \
    { r_x, r_y, r_w, r_h, t_x, t_y, fill, t_color, t_size, str }

// 36 bytes
typedef struct ui_textbox_info_type {
    uint16_t rec_x;
    uint16_t rec_y;
    uint16_t rec_w;
    uint16_t rec_h;
    uint16_t text_x;
    uint16_t text_y;
    uint16_t fill;
    uint16_t text_color;
    uint16_t text_size;
    char content[16];
    char dummy[2]; // for aliagn
} ui_textbox_info_t;

typedef struct ui_button_type {
    Adafruit_GFX_Button *button;
    ui_button_info_t button_info;
    ui_event_cb_t event_callback;
    mp3_tk_cmd_t mp3_tk_cmd;
} ui_button_t;

typedef struct ui_textbox_type {
    ui_textbox_info_t text_info;
    ui_event_cb_t draw_callback;
    ui_event_cb_t click_callback;
} ui_textbox_t ;

/*******************************************************************************

    Display command structure

*******************************************************************************/

typedef struct cmd_arg_type {
    uint8_t *print_buf;
    uint32_t print_buf_len;
    union {
        char buf[16];
        uint32_t u32_value;
    };
    uint8_t dummy[3];
} cmd_arg_t;

typedef struct point_type {
    uint32_t x;
    uint32_t y;
} point_t;

typedef struct display_tk_cmd_msg_type {
    display_tk_cmd_t command;
    union {
        point_t point;
        ui_event_cb_t event_callback;
    };
    cmd_arg_t argu;
} display_tk_cmd_msg_t;

/*******************************************************************************

    MP3 32bits data structure

*******************************************************************************/

#if 0

#define MASK(bitlen, offset) (((1<<(bitlen))-1) << offset)

#define MPEG_VERSION_OFFSET (19)
#define MPEG_BITS (2)
#define MPEG_VERSION_MASK MASK(MPEG_BITS, MPEG_VERSION_OFFSET)
#define MPEG_VERSION(n) ((n) & MPEG_VERSION_MASK)

#define LAYER_OFFSET (17)
#define LAYER_BITS (2)
#define LAYER_MASK MASK(LAYER_BITS, LAYER_OFFSET)
#define LAYER(n) ((n) & LAYER_MASK)

#define BIT_RATES_OFFSET (12)
#define BIT_RATES_BITS (4)
#define BIT_RATES_MASK MASK(BIT_RATES_BITS, BIT_RATES_OFFSET)
#define BIT_RATES(n) ((n) & BIT_RATES_MASK)

typedef uint32_t mp3_chunk_t;

#define FREE_BIT_RATE (0)
#define BAD_BIT_RATE (0xFFFF)

uint16_t bit_rates_tbl[2][3][16] =
{
  {
    { 0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448, 0xFFFF },
    { 0, 32, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384, 0xFFFF },
    { 0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 0xFFFF }
  },
  {
    { 0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448, 0xFFFF },
    { 0, 32, 48, 56, 64, 80, 96, 112, 128, 144, 160, 176, 192, 224, 256, 0xFFFF },
    { 0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, 0xFFFF }
  }
}

static uint16_t get_bit_rates(uint32_t u32_mp3_)
{
}

#endif

/************************************************************************************

   Allocate the stacks for each task.
   The maximum number of tasks the application can have is defined by OS_MAX_TASKS in os_cfg.h

************************************************************************************/


static OS_STK   Mp3DriverTaskStk[MP3_DRIVER_TASK_STK_SIZE];
static OS_STK   LcdDisplayTaskStk[DISPLAY_TASK_STK_SIZE];
static OS_STK   Mp3PlayerTaskStk[MP3PLAY_TASK_STK_SIZE];
static OS_STK   TouchTaskStk[TOUCH_TASK_STK_SIZE];

// Task prototypes
void Mp3DriverTask(void* pdata);
void LcdDisplayTask(void* pdata);
void Mp3PlayerTask(void* pdata);
void TouchTask(void* pdata);

// Useful functions
void PrintToLcdWithBuf(char *buf, int size, char *format, ...);

// Globals
BOOLEAN nextSong = OS_FALSE;

/*******************************************************************************

    OS Event

********************************************************************************/
           
static OS_EVENT *mp3_tk_cmd_mb = NULL;
//static OS_EVENT *display_tk_cmd_mb = NULL;

#define DISPLAY_TK_CMD_Q_SIZE 10
static OS_EVENT *diplay_tk_cmd_queue = NULL;
static void *display_tk_cmd_q_tbl[DISPLAY_TK_CMD_Q_SIZE];

/*******************************************************************************

    OS Memory

********************************************************************************/

#define CMD_MSG_SIZES 10

static OS_MEM *cmd_msg_mem = NULL;
static display_tk_cmd_msg_t cmd_msg_tbl[CMD_MSG_SIZES];

/*******************************************************************************

UI Objects

*******************************************************************************/
// UI_OBJS macro just used for unfolding in IDE...
#define UI_OBJS
#ifdef UI_OBJS
/* Button Geomertic Infomation */
#define PLAY_X (20U)
#define PLAY_Y (170U)
#define PLAY_W (60U)
#define PLAY_H (40U)

#define PAUSE_X (90U)
#define PAUSE_Y (170U)
#define PAUSE_W (60U)
#define PAUSE_H (40U)

#define STOP_X (160U)
#define STOP_Y (170U)
#define STOP_W (60U)
#define STOP_H (40U)

#define VOL_UP_X (160U)
#define VOL_UP_Y (220U)
#define VOL_UP_W (60U)
#define VOL_UP_H (40U)

#define VOL_DW_X (160U)
#define VOL_DW_Y (270U)
#define VOL_DW_W (60U)
#define VOL_DW_H (40U)

#define PREV_X (20U)
#define PREV_Y (220U)
#define PREV_W (60U)
#define PREV_H (40U)

#define NEXT_X (90U)
#define NEXT_Y (220U)
#define NEXT_W (60U)
#define NEXT_H (40U)

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


#define UI_BUTTON_SIZES (sizeof(button_array)/sizeof(button_array[0]))
    
static ui_button_t button_array[] =
{
    {
        &play_bu, 
        UI_BUTTON_INFO_INIT(PLAY_X, PLAY_Y, PLAY_W, PLAY_H, 
                            ILI9341_WHITE, ILI9341_BLACK, ILI9341_WHITE,
                            "Play", 2),
        play_bu_e_cb, 
        MP3_TK_PLAY 
    }, 
    { 
        &pause_bu, 
        UI_BUTTON_INFO_INIT(PAUSE_X, PAUSE_Y, PAUSE_W, PAUSE_H, 
                            ILI9341_WHITE, ILI9341_BLACK, ILI9341_WHITE,
                            "Pause", 2),
        pause_bu_e_cb, 
        MP3_TK_PAUSE 
    }, 
    { 
        &stop_bu,
        UI_BUTTON_INFO_INIT(STOP_X, STOP_Y, STOP_W, STOP_H, 
                            ILI9341_WHITE, ILI9341_BLACK, ILI9341_WHITE,
                            "Stop", 2),
        stop_bu_e_cb, 
        MP3_TK_STOP
    },
    {
        &vol_up_bu,
        UI_BUTTON_INFO_INIT(VOL_UP_X, VOL_UP_Y, VOL_UP_W, VOL_UP_H, 
                            ILI9341_WHITE, ILI9341_BLACK, ILI9341_WHITE,
                            "+", 2),
        vol_up_bu_e_cb,
        MP3_TK_VOL_UP
    },
    {
        &vol_down_bu, 
        UI_BUTTON_INFO_INIT(VOL_DW_X, VOL_DW_Y, VOL_DW_W, VOL_DW_H, 
                            ILI9341_WHITE, ILI9341_BLACK, ILI9341_WHITE,
                            "-", 2),        
        vol_down_bu_e_cb, 
        MP3_TK_VOL_DOWN 
    },
    { 
        &prev_bu,
        UI_BUTTON_INFO_INIT(PREV_X, PREV_Y, PREV_W, PREV_H, 
                            ILI9341_WHITE, ILI9341_BLACK, ILI9341_WHITE,
                            "<", 2),
        prev_bu_e_cb, 
        MP3_TK_PREV
    },
    { 
        &next_bu,
        UI_BUTTON_INFO_INIT(NEXT_X, NEXT_Y, NEXT_W, NEXT_H, 
                            ILI9341_WHITE, ILI9341_BLACK, ILI9341_WHITE,
                            ">", 2),
        next_bu_e_cb, 
        MP3_TK_NEXT  
    }
};
#endif


#define SONG_REC_X (10U)
#define SONG_REC_Y (10U)
#define SONG_REC_W (110U)
#define SONG_REC_H (40U)
#define SONG_TEXT_X (10U)
#define SONG_TEXT_Y (20U)
#define SONG_TEXT_FILL ILI9341_NAVY
#define SONG_TEXT_COLOR ILI9341_WHITE
#define SONG_TEXT_SIZE (2)

#define BU_TEXT_RECT_X (120U)
#define BU_TEXT_RECT_Y (10U)
#define BU_TEXT_RECT_W (110U)
#define BU_TEXT_RECT_H (40U)
#define BU_TEXT_X (10U)
#define BU_TEXT_Y (20U)
#define BUTTON_TEXT_FILL ILI9341_MAROON
#define BUTTON_TEXT_COLOR ILI9341_WHITE
#define BU_TEXT_SIZE (2)

#define VOL_TEXT_RECT_X (120U)
#define VOL_TEXT_RECT_Y (10U)
#define VOL_TEXT_RECT_W (110U)
#define VOL_TEXT_RECT_H (40U)
#define VOL_TEXT_X (120U)
#define VOL_TEXT_Y (20U)
#define VOL_TEXT_FILL ILI9341_MAGENTA
#define VOL_TEXT_COLOR ILI9341_WHITE
#define VOL_TEXT_SIZE (2)

static void textbox_draw_callback(Adafruit_ILI9341 *_gfx, void *_arg);

static ui_textbox_t ui_textbox_tbl[] = 
{
    {
        TEXT_INFO_INIT(SONG_REC_X, SONG_REC_Y, SONG_REC_W, SONG_REC_H, 
                     SONG_TEXT_X, SONG_TEXT_Y, SONG_TEXT_FILL, SONG_TEXT_COLOR,
                     SONG_TEXT_SIZE, ""),
        textbox_draw_callback,
        NULL
    },
    {
        TEXT_INFO_INIT(BU_TEXT_RECT_X, BU_TEXT_RECT_Y, BU_TEXT_RECT_W, BU_TEXT_RECT_H, 
                     BU_TEXT_X, BU_TEXT_Y, BUTTON_TEXT_FILL, BUTTON_TEXT_COLOR,
                     BU_TEXT_SIZE, ""),
        textbox_draw_callback,
        NULL
    }
};


void play_bu_e_cb(Adafruit_ILI9341 *_gfx, void *_arg)
{
    cmd_arg_t *cmd_arg_p = (cmd_arg_t *)_arg;
    _gfx->fillRect(BU_TEXT_RECT_X, BU_TEXT_RECT_Y, BU_TEXT_RECT_W, BU_TEXT_RECT_H, BUTTON_TEXT_FILL);
    _gfx->setCursor(BU_TEXT_X, BU_TEXT_Y);
    _gfx->setTextColor(BUTTON_TEXT_COLOR);  
    _gfx->setTextSize(BU_TEXT_SIZE);
    PrintToLcdWithBuf((char *)cmd_arg_p->print_buf, cmd_arg_p->print_buf_len , "PLAY");
}

void pause_bu_e_cb(Adafruit_ILI9341 *_gfx, void *_arg)
{    
    cmd_arg_t *cmd_arg_p = (cmd_arg_t *)_arg;
    _gfx->fillRect(BU_TEXT_RECT_X, BU_TEXT_RECT_Y, BU_TEXT_RECT_W, BU_TEXT_RECT_H, BUTTON_TEXT_FILL);
    _gfx->setCursor(BU_TEXT_X, BU_TEXT_Y);
    _gfx->setTextColor(BUTTON_TEXT_COLOR);  
    _gfx->setTextSize(BU_TEXT_SIZE);
    PrintToLcdWithBuf((char *)cmd_arg_p->print_buf, cmd_arg_p->print_buf_len , "PAUSE");
}

void stop_bu_e_cb(Adafruit_ILI9341 *_gfx, void *_arg)
{
    cmd_arg_t *cmd_arg_p = (cmd_arg_t *)_arg;
    _gfx->fillRect(BU_TEXT_RECT_X, BU_TEXT_RECT_Y, BU_TEXT_RECT_W, BU_TEXT_RECT_H, BUTTON_TEXT_FILL);
    _gfx->setCursor(BU_TEXT_X, BU_TEXT_Y);
    _gfx->setTextColor(BUTTON_TEXT_COLOR);  
    _gfx->setTextSize(BU_TEXT_SIZE);
    PrintToLcdWithBuf((char *)cmd_arg_p->print_buf, cmd_arg_p->print_buf_len , "STOP");
}

void vol_up_bu_e_cb(Adafruit_ILI9341 *_gfx, void *_arg)
{
    cmd_arg_t *cmd_arg_p = (cmd_arg_t *)_arg;
    _gfx->fillRect(BU_TEXT_RECT_X, BU_TEXT_RECT_Y, BU_TEXT_RECT_W, BU_TEXT_RECT_H, BUTTON_TEXT_FILL);
    _gfx->setCursor(BU_TEXT_X, BU_TEXT_Y);
    _gfx->setTextColor(BUTTON_TEXT_COLOR);  
    _gfx->setTextSize(BU_TEXT_SIZE);
    PrintToLcdWithBuf((char *)cmd_arg_p->print_buf, cmd_arg_p->print_buf_len , "VOLUP");
}

void vol_down_bu_e_cb(Adafruit_ILI9341 *_gfx, void *_arg)
{
    cmd_arg_t *cmd_arg_p = (cmd_arg_t *)_arg;
    _gfx->fillRect(BU_TEXT_RECT_X, BU_TEXT_RECT_Y, BU_TEXT_RECT_W, BU_TEXT_RECT_H, BUTTON_TEXT_FILL);
    _gfx->setCursor(BU_TEXT_X, BU_TEXT_Y);
    _gfx->setTextColor(BUTTON_TEXT_COLOR);  
    _gfx->setTextSize(BU_TEXT_SIZE);
    PrintToLcdWithBuf((char *)cmd_arg_p->print_buf, cmd_arg_p->print_buf_len , "VOLDW");
}

void prev_bu_e_cb(Adafruit_ILI9341 *_gfx, void *_arg)
{
    cmd_arg_t *cmd_arg_p = (cmd_arg_t *)_arg;
    _gfx->fillRect(BU_TEXT_RECT_X, BU_TEXT_RECT_Y, BU_TEXT_RECT_W, BU_TEXT_RECT_H, BUTTON_TEXT_FILL);
    _gfx->setCursor(BU_TEXT_X, BU_TEXT_Y);
    _gfx->setTextColor(BUTTON_TEXT_COLOR);  
    _gfx->setTextSize(BU_TEXT_SIZE);
    PrintToLcdWithBuf((char *)cmd_arg_p->print_buf, cmd_arg_p->print_buf_len , "PREV");
}

void next_bu_e_cb(Adafruit_ILI9341 *_gfx, void *_arg)
{
    cmd_arg_t *cmd_arg_p = (cmd_arg_t *)_arg;
    _gfx->fillRect(BU_TEXT_RECT_X, BU_TEXT_RECT_Y, BU_TEXT_RECT_W, BU_TEXT_RECT_H, BUTTON_TEXT_FILL);
    _gfx->setCursor(BU_TEXT_X, BU_TEXT_Y);
    _gfx->setTextColor(BUTTON_TEXT_COLOR);  
    _gfx->setTextSize(BU_TEXT_SIZE);
    PrintToLcdWithBuf((char *)cmd_arg_p->print_buf, cmd_arg_p->print_buf_len , "NEXT");
}

static void textbox_draw_callback(Adafruit_ILI9341 *_gfx, void *_arg)
{
    ui_textbox_info_t * text_p;
    const char *p;
    uint32_t u32_count;
    
    text_p = (ui_textbox_info_t *)_arg;
    p = (const char *)text_p->content;
    u32_count = 0;
    
    _gfx->fillRect(text_p->rec_x, text_p->rec_y, text_p->rec_w, text_p->rec_h, 
                   text_p->fill);
    _gfx->setCursor(text_p->text_x, text_p->text_y);
    _gfx->setTextColor(text_p->text_color);
    _gfx->setTextSize(text_p->text_size);
    
    while (u32_count < 16 && *p != '\0') {
        _gfx->write(*p);
        ++p;
        ++u32_count;
    }
}

static void song_text_e_cb(Adafruit_ILI9341 *_gfx, void *_arg)
{
    cmd_arg_t *cmd_arg_p = (cmd_arg_t *)_arg;
    char *p;
    uint32_t u32_count;
    
    cmd_arg_p = (cmd_arg_t *)_arg;
    p = cmd_arg_p->buf;
    u32_count = 0;
    
    _gfx->fillRect(SONG_REC_X, SONG_REC_Y, SONG_REC_W, SONG_REC_H, SONG_TEXT_FILL);
    _gfx->setCursor(SONG_TEXT_X, SONG_TEXT_Y);
    _gfx->setTextColor(SONG_TEXT_COLOR);  
    _gfx->setTextSize(SONG_TEXT_SIZE);
    
    while (u32_count < 16 && *p != '\0') {
        _gfx->write(*p);
        ++p;
        ++u32_count;
    }
    //PrintToLcdWithBuf((char *)cmd_arg_p->print_buf, cmd_arg_p->print_buf_len , cmd_arg_p->buf);
}

static void progress_bar_e_cb(Adafruit_ILI9341 *_gfx, void *_arg)
{
    cmd_arg_t *cmd_arg_p = (cmd_arg_t *)_arg;
    uint32_t u32_progress_bar = 220 * cmd_arg_p->u32_value / 100;
    _gfx->fillRect(10, 120, 220, 20, ILI9341_BLACK);
    _gfx->fillRect(10, 120, u32_progress_bar, 20, ILI9341_LIGHTGREY);
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
    uint8_t u8_error;

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
 
    // Create uCOS memory partitions
    PrintWithBuf(buf, BUFSIZE, "StartupTask: Creating the event objects\n");
    
    cmd_msg_mem = OSMemCreate(
                      cmd_msg_tbl, CMD_MSG_SIZES, 
                      sizeof(display_tk_cmd_msg_t), &u8_error);
    
    // Create the event objects
    PrintWithBuf(buf, BUFSIZE, "StartupTask: Creating the event objects\n");
    
    // Create mail box
    mp3_tk_cmd_mb = OSMboxCreate((void *)0);
    diplay_tk_cmd_queue = OSQCreate(display_tk_cmd_q_tbl,DISPLAY_TK_CMD_Q_SIZE);
    
    // Create the test tasks
    PrintWithBuf(buf, BUFSIZE, "StartupTask: Creating the application tasks\n");

    // The maximum number of tasks the application can have is defined by OS_MAX_TASKS in os_cfg.h
    OSTaskCreate(Mp3DriverTask, (void*)0, &Mp3DriverTaskStk[MP3_DRIVER_TASK_STK_SIZE-1], MP3_DRIVER_TASK_PRIO);
    OSTaskCreate(LcdDisplayTask, (void*)0, &LcdDisplayTaskStk[DISPLAY_TASK_STK_SIZE-1], DISPLAY_TASK_PRIO);
    OSTaskCreate(Mp3PlayerTask, (void*)0, &Mp3PlayerTaskStk[MP3PLAY_TASK_STK_SIZE-1], MP3PLAY_TASK_PRIO);
    OSTaskCreate(TouchTask, (void*)0, &TouchTaskStk[TOUCH_TASK_STK_SIZE-1], TOUCH_TASKK_PRIO);

    // Delete ourselves, letting the work be done in the new tasks.
    PrintWithBuf(buf, BUFSIZE, "StartupTask: deleting self\n");
    
    OSTaskDel(OS_PRIO_SELF);
}

//static void DrawLcdContents()
//{
//    char buf[BUFSIZE];
//    OS_CPU_SR cpu_sr;
//    
//    // allow slow lower pri drawing operation to finish without preemption
//    OS_ENTER_CRITICAL(); 
//    
//    lcdCtrl.fillScreen(ILI9341_BLACK);
//    
//    // Print a message on the LCD
//    lcdCtrl.setCursor(40, 60);
//    lcdCtrl.setTextColor(ILI9341_WHITE);  
//    lcdCtrl.setTextSize(2);
//    PrintToLcdWithBuf(buf, BUFSIZE, "Hello World!");
//
//    OS_EXIT_CRITICAL();
//}

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

static void DrawButtons(Adafruit_ILI9341 *gfx)
{
    uint32_t u32_i;
    ui_button_info_t *bu_info;
    Adafruit_GFX_Button *bu;
    
    for (u32_i = 0; u32_i < UI_BUTTON_SIZES; ++u32_i) {
        bu = button_array[u32_i].button;
        bu_info = &button_array[u32_i].button_info;
        
        bu->initButton(gfx,
                       bu_info->x + bu_info->w / 2, bu_info->y + bu_info->h / 2, 
                       bu_info->w, bu_info->h, 
                       bu_info->outline, bu_info->fill, bu_info->textcolor, 
                       bu_info->label, bu_info->textsize);
    }
    
    for (u32_i = 0; u32_i < UI_BUTTON_SIZES; ++u32_i) {
        bu = button_array[u32_i].button;
        bu->drawButton();
    } 
}

/************************************************************************************

   Runs Lcd Display code

************************************************************************************/

#define TIMEOUT_INFINITE 0

void LcdDisplayTask(void* pdata)
{
    PjdfErrCode pjdfErr;
    uint8_t  u8_error;
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
    
    // draw x axis and y axis grid
    DrawGrid(&lcdCtrl, 0, 0, ILI9341_TFTWIDTH, ILI9341_TFTHEIGHT,
             20, 20, ILI9341_WHITE);
    
    // draw a rectangular for song name space
    lcdCtrl.drawRect(10, 10, 220, 80, ILI9341_MAROON);
    
    // draw a retangular for process bar
    lcdCtrl.drawRect(10, 110, 220, 40, ILI9341_MAROON);
    
    DrawButtons(&lcdCtrl);
    
    PrintWithBuf(buf, BUFSIZE, "Display Task waits for Touch task");
    
    u8_error = OSTaskSuspend(OS_PRIO_SELF);
    
    if (u8_error != OS_ERR_NONE) {
        PrintWithBuf(buf, BUFSIZE, "Error: Display Task returned from suspend %d\n", u8_error);
    }
    
    display_tk_cmd_msg_t *cmd_msg_p;
    display_tk_cmd_msg_t cmd_msg;

//    uint32_t u32_i;
//    ui_button_t *bu;
//    point_t *point_p;
//    
    while (1) {
        
        // display task is event trigger task
        cmd_msg_p = (display_tk_cmd_msg_t *)OSQPend(diplay_tk_cmd_queue, TIMEOUT_INFINITE, &u8_error);
        
        // copy msg
        cmd_msg = *cmd_msg_p;
        
        // release resource to memory pool(uCOS memory heap)
        u8_error = OSMemPut(cmd_msg_mem, cmd_msg_p);
        cmd_msg_p = (display_tk_cmd_msg_t *)0;
        
        if (u8_error) {
        }
        
        switch (cmd_msg.command) {
        case DISPLAY_TK_CLICK:
//            point_p = &cmd_msg.point;
//            
//            // find first match button
//            for (u32_i = 0;u32_i < UI_BUTTON_SIZES;++u32_i) {
//                bu = &button_array[u32_i];
//                if (bu->button->contains(point_p->x, point_p->y)) {
//                    
//                    // This is ui event
//                    if (bu->event_callback) {
//                        cmd_msg.argu.print_buf = (uint8_t *)buf;
//                        cmd_msg.argu.print_buf_len = BUFSIZE;
//                        bu->event_callback(&lcdCtrl, (void *)&cmd_msg.argu);
//                    }
//                    
//                    // TODO: for sending a copy, use uCOS memory heap
//                    u8_error = OSMboxPost(mp3_tk_cmd_mb, &bu->mp3_tk_cmd);
//                    
//                    break;
//                }
//            }
            break;
        case DISPLAY_TK_UI_CB:
            if (cmd_msg.event_callback) {
                cmd_msg.argu.print_buf = (uint8_t *)buf;
                cmd_msg.argu.print_buf_len = BUFSIZE;
                cmd_msg.event_callback(&lcdCtrl, (void *)&cmd_msg.argu);
            }
            break;
        case DISPLAY_TK_NONE:
            break;
        default:
            ;
        }
        
        // no delay here because display task is event driven task
    }
}

/*******************************************************************************

    UI Event Task for external resource or tasks

*******************************************************************************/

void Mp3PlayerTask(void* pdata)
{
    ;
}

/************************************************************************************

   Touch Task

************************************************************************************/
void TouchTask(void* pdata)
{
    PjdfErrCode pjdfErr;
    INT8U  device_address, u8_error;
    INT32U length;

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
    
    u8_error = OSTaskResume(DISPLAY_TASK_PRIO);
        
    if (u8_error != OS_ERR_NONE) {
        PrintWithBuf(buf, BUFSIZE, "Touch Task resume display task error: %d\n", u8_error);
    }
    
    display_tk_cmd_msg_t *cmd_msg_p;
    uint32_t button_match = 0, u32_i;
    ui_button_t *bu;

    while (1) { 
        boolean touched;
        
        touched = touchCtrl.touched();
        
        button_match = 0;
        
        if (touched) {
            TS_Point touch_p, convert_p = TS_Point();
            
            touch_p = touchCtrl.getPoint();
            convert_p.x = MapTouchToScreen(touch_p.x, 0, ILI9341_TFTWIDTH, ILI9341_TFTWIDTH, 0);
            convert_p.y = MapTouchToScreen(touch_p.y, 0, ILI9341_TFTHEIGHT, ILI9341_TFTHEIGHT, 0);
            
            // find first match button
            for (u32_i = 0;u32_i < UI_BUTTON_SIZES;++u32_i) {
                bu = &button_array[u32_i];
                
                // if button_match
                if (bu->button->contains(convert_p.x, convert_p.y )) {
                    
                    button_match = 1;
                    
                    // TODO: for sending a copy, use uCOS memory heap
                    u8_error = OSMboxPost(mp3_tk_cmd_mb, &bu->mp3_tk_cmd);
                    
                    // This is ui event
                    if (bu->event_callback) {
                        
                        // get memory block
                        cmd_msg_p = (display_tk_cmd_msg_t *)OSMemGet(cmd_msg_mem, &u8_error);
            
                        if (!cmd_msg_p) {
                            PrintWithBuf(buf, BUFSIZE, "Touch Task: cmd mem allocate fails: %d\n", u8_error);
                        }
                        
                        // if get memory, send click event to display task
                        if (cmd_msg_p) {
            
                            cmd_msg_p->command = DISPLAY_TK_UI_CB;
                            cmd_msg_p->event_callback = bu->event_callback;
            
                            u8_error = OSQPost(diplay_tk_cmd_queue, cmd_msg_p);
                
                            if (u8_error != OS_ERR_NONE) {
                                PrintWithBuf(buf, BUFSIZE, "Touch Task: cmd post fails: %d\n", u8_error);
                            }
                        }
                    }                 
                    break;
                }
            }
        }
        
        if (!touched || !button_match) {
            OSTimeDly(30);
        } else {
            OSTimeDly(100);
        }
    }
}

/************************************************************************************

   Runs MP3 demo code

************************************************************************************/

#define MP3_BUF_SIZE 2048

static uint8_t u8_mp3_buff[MP3_BUF_SIZE];

void Mp3DriverTask(void* pdata)
{
    PjdfErrCode pjdfErr;
    INT32U length;
    uint32_t nbytes;
    File dir, file;
    
    char buf[BUFSIZE];
    PrintWithBuf(buf, BUFSIZE, "Mp3DriverTask: starting\n");

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
    PrintWithBuf(buf, BUFSIZE, "Starting MP3 Task\n");

    //int count = 0;
    //int play_song_count = 0;
    
    dir = SD.open("/", O_READ);
    
    PrintWithBuf(buf, BUFSIZE, "Open root directory: %s\n", dir.name());
    
    if (!dir) {
        PrintWithBuf(buf, BUFSIZE, "Error: could not open SD card file '%s'\n", "/");
    }
    
    dir.close();
    
    Mp3Init(hMp3);
    Mp3StreamInit(hMp3);
    
//    // Test file close
//    file = SD.open("TRAIN001.mp3", O_READ);
//    if (file) {
//        PrintWithBuf(buf, BUFSIZE, "Print file: %s\n", file.name());
//    }
//    file.close();
//    if (file) {
//        PrintWithBuf(buf, BUFSIZE, "Error: file should have been closed\n");
//    }
    
    static char filename[4][13] = { "TRAIN001.MP3", "FINAL.MP3", "LEVEL_5.MP3", "SISTER.MP3" };
    static uint32_t u32_name_index = 0, u32_filesize = 0, isplay = 0;
    static uint32_t u32_playsize, u32_new_chunk_id, u32_old_chunk_id;
    static uint8_t mp3_volumn = 0x10, u8_error;
    static mp3_tk_cmd_t *mp3_tk_command_p = NULL;
    static display_tk_cmd_msg_t *cmd_msg_p = NULL;
    static mp3_tk_cmd_t mp3_tk_command = MP3_TK_NONE;
    static mp3_dr_cmd_t mp3_dr_command = MP3_DR_NONE;
    
#define CHUNCK_SIZE 100
    
    nbytes = MP3_BUF_SIZE;

    // get memory block
    cmd_msg_p = (display_tk_cmd_msg_t *)OSMemGet(cmd_msg_mem, &u8_error);
            
    if (!cmd_msg_p) {
        PrintWithBuf(buf, BUFSIZE, "Mp3 Task: cmd mem allocate fails: %d\n", u8_error);
    }
            
    cmd_msg_p->command = DISPLAY_TK_UI_CB;
    cmd_msg_p->event_callback = song_text_e_cb;
    memset(cmd_msg_p->argu.buf, 0, 16);
    memcpy(cmd_msg_p->argu.buf, filename[u32_name_index], 5);
                
    u8_error = OSQPost(diplay_tk_cmd_queue, cmd_msg_p);
                
    if (u8_error != OS_ERR_NONE) {
        PrintWithBuf(buf, BUFSIZE, "Touch Task: cmd post fails: %d\n", u8_error);
    }
    
    while (1)
    {
        mp3_tk_command_p = (mp3_tk_cmd_t *)OSMboxAccept(mp3_tk_cmd_mb);
        
        mp3_tk_command = MP3_TK_NONE;
        mp3_dr_command = MP3_DR_NONE;
        
        // copy command from mailbox
        // even though no task will modify mailbox message, copy it to avoid sharing data
        if (mp3_tk_command_p) {
            mp3_tk_command = *mp3_tk_command_p;
        }
        
        switch (mp3_tk_command) {
        case MP3_TK_PLAY:
            if (!isplay) {
                if (!file) {
                    file = SD.open(filename[u32_name_index], O_READ);
                    u32_filesize = file.size();
                    u32_playsize = 0;
                    u32_new_chunk_id = u32_old_chunk_id = 0;
                    
                    cmd_msg_p = (display_tk_cmd_msg_t *)OSMemGet(cmd_msg_mem, &u8_error);
            
                    if (!cmd_msg_p) {
                        PrintWithBuf(buf, BUFSIZE, "Mp3 Task: cmd mem allocate fails: %d\n", u8_error);
                    }
            
                    cmd_msg_p->command = DISPLAY_TK_UI_CB;
                    cmd_msg_p->event_callback = progress_bar_e_cb;
                    cmd_msg_p->argu.u32_value = u32_old_chunk_id;
                
                    u8_error = OSQPost(diplay_tk_cmd_queue, cmd_msg_p);
                
                    if (u8_error != OS_ERR_NONE) {
                        PrintWithBuf(buf, BUFSIZE, "Mp3 Task: cmd post fails: %d\n", u8_error);
                    }
                    
                }
                isplay = 1;
            }
            break;
        case MP3_TK_PAUSE:
            if (isplay) {
                isplay = 0;
            }
            break;
        case MP3_TK_STOP:
            if (file) {
                file.close();
                
                // get memory block
                cmd_msg_p = (display_tk_cmd_msg_t *)OSMemGet(cmd_msg_mem, &u8_error);
                
                // to clear rest data in vs1053
                if (!cmd_msg_p) {
                    PrintWithBuf(buf, BUFSIZE, "Mp3 Task: cmd mem allocate fails: %d\n", u8_error);
                }
            
                cmd_msg_p->command = DISPLAY_TK_UI_CB;
                cmd_msg_p->event_callback = progress_bar_e_cb;
                cmd_msg_p->argu.u32_value = 0;
                
                u8_error = OSQPost(diplay_tk_cmd_queue, cmd_msg_p);
                
                if (u8_error != OS_ERR_NONE) {
                    PrintWithBuf(buf, BUFSIZE, "Mp3 Task: cmd post fails: %d\n", u8_error);
                }
                
                mp3_dr_command = MP3_DR_SOFT_RESET;
            }
            isplay = 0;
            break;
        case MP3_TK_VOL_UP:
            if (mp3_volumn != MP3_VOL_MAX) {
                --mp3_volumn;
                mp3_dr_command = MP3_DR_VOL_CHANGE;
            }
            break;
        case MP3_TK_VOL_DOWN:
            if (mp3_volumn != MP3_VOL_MIN) {
                ++mp3_volumn;
                mp3_dr_command = MP3_DR_VOL_CHANGE;
            }
            break;
        case MP3_TK_PREV:
            if (u32_name_index > 0) {
                --u32_name_index;
                
                if (file) {
                    
                    file.close();
                                  
                    isplay = 0;
                    // to clear rest data in vs1053
                    mp3_dr_command = MP3_DR_SOFT_RESET;
                }
                
                // get memory block
                cmd_msg_p = (display_tk_cmd_msg_t *)OSMemGet(cmd_msg_mem, &u8_error);
            
                if (!cmd_msg_p) {
                    PrintWithBuf(buf, BUFSIZE, "Mp3 Task: cmd mem allocate fails: %d\n", u8_error);
                }
            
                cmd_msg_p->command = DISPLAY_TK_UI_CB;
                cmd_msg_p->event_callback = song_text_e_cb;
                memset(cmd_msg_p->argu.buf, 0, 16);
                memcpy(cmd_msg_p->argu.buf, filename[u32_name_index], 5);
                
                u8_error = OSQPost(diplay_tk_cmd_queue, cmd_msg_p);
                
                if (u8_error != OS_ERR_NONE) {
                    PrintWithBuf(buf, BUFSIZE, "Touch Task: cmd post fails: %d\n", u8_error);
                }
                
                cmd_msg_p = (display_tk_cmd_msg_t *)OSMemGet(cmd_msg_mem, &u8_error);
            
                if (!cmd_msg_p) {
                    PrintWithBuf(buf, BUFSIZE, "Mp3 Task: cmd mem allocate fails: %d\n", u8_error);
                }
            
                cmd_msg_p->command = DISPLAY_TK_UI_CB;
                cmd_msg_p->event_callback = progress_bar_e_cb;
                cmd_msg_p->argu.u32_value = 0;
                
                u8_error = OSQPost(diplay_tk_cmd_queue, cmd_msg_p);
                
                if (u8_error != OS_ERR_NONE) {
                    PrintWithBuf(buf, BUFSIZE, "Mp3 Task: cmd post fails: %d\n", u8_error);
                }
                
            }
            break;
        case MP3_TK_NEXT:
            if (u32_name_index < 3) {
                ++u32_name_index;
                
                if (file) {
                    
                    file.close();
                                   
                    isplay = 0;
                    // to clear rest data in vs1053
                    mp3_dr_command = MP3_DR_SOFT_RESET;                  
                }
                
                // get memory block
                cmd_msg_p = (display_tk_cmd_msg_t *)OSMemGet(cmd_msg_mem, &u8_error);
            
                if (!cmd_msg_p) {
                    PrintWithBuf(buf, BUFSIZE, "Mp3 Task: cmd mem allocate fails: %d\n", u8_error);
                }
            
                cmd_msg_p->command = DISPLAY_TK_UI_CB;
                cmd_msg_p->event_callback = song_text_e_cb;
                memset(cmd_msg_p->argu.buf, 0, 16);
                memcpy(cmd_msg_p->argu.buf, filename[u32_name_index], 5);
                
                u8_error = OSQPost(diplay_tk_cmd_queue, cmd_msg_p);
                
                if (u8_error != OS_ERR_NONE) {
                    PrintWithBuf(buf, BUFSIZE, "Mp3 Task: cmd post fails: %d\n", u8_error);
                }
                
                cmd_msg_p = (display_tk_cmd_msg_t *)OSMemGet(cmd_msg_mem, &u8_error);
            
                if (!cmd_msg_p) {
                    PrintWithBuf(buf, BUFSIZE, "Mp3 Task: cmd mem allocate fails: %d\n", u8_error);
                }
            
                cmd_msg_p->command = DISPLAY_TK_UI_CB;
                cmd_msg_p->event_callback = progress_bar_e_cb;
                cmd_msg_p->argu.u32_value = 0;
                
                u8_error = OSQPost(diplay_tk_cmd_queue, cmd_msg_p);
                
                if (u8_error != OS_ERR_NONE) {
                    PrintWithBuf(buf, BUFSIZE, "Mp3 Task: cmd post fails: %d\n", u8_error);
                }
                
            }
            break;
        case MP3_TK_NONE:
            break;
        default:
            ;
        }

        if (mp3_dr_command == MP3_DR_NONE && isplay) {
            if (file.available()) {
                nbytes = MP3_BUF_SIZE;
                nbytes = file.read(u8_mp3_buff, nbytes);
                mp3_dr_command = MP3_DR_PLAY_CHUNK;
                
                u32_playsize += nbytes;
                u32_new_chunk_id = u32_playsize * CHUNCK_SIZE / u32_filesize;
                if (u32_new_chunk_id != u32_old_chunk_id) {
                    u32_old_chunk_id = u32_new_chunk_id;
                    // get memory block
                    cmd_msg_p = (display_tk_cmd_msg_t *)OSMemGet(cmd_msg_mem, &u8_error);
            
                    if (!cmd_msg_p) {
                        PrintWithBuf(buf, BUFSIZE, "Mp3 Task: cmd mem allocate fails: %d\n", u8_error);
                    }
            
                    cmd_msg_p->command = DISPLAY_TK_UI_CB;
                    cmd_msg_p->event_callback = progress_bar_e_cb;
                    cmd_msg_p->argu.u32_value = u32_old_chunk_id;
                
                    u8_error = OSQPost(diplay_tk_cmd_queue, cmd_msg_p);
                
                    if (u8_error != OS_ERR_NONE) {
                        PrintWithBuf(buf, BUFSIZE, "Mp3 Task: cmd post fails: %d\n", u8_error);
                    }
                }
            } else {
                cmd_msg_p = (display_tk_cmd_msg_t *)OSMemGet(cmd_msg_mem, &u8_error);
            
                if (!cmd_msg_p) {
                    PrintWithBuf(buf, BUFSIZE, "Mp3 Task: cmd mem allocate fails: %d\n", u8_error);
                }
            
                cmd_msg_p->command = DISPLAY_TK_UI_CB;
                cmd_msg_p->event_callback = progress_bar_e_cb;
                cmd_msg_p->argu.u32_value = 0;
                
                u8_error = OSQPost(diplay_tk_cmd_queue, cmd_msg_p);
                
                if (u8_error != OS_ERR_NONE) {
                    PrintWithBuf(buf, BUFSIZE, "Mp3 Task: cmd post fails: %d\n", u8_error);
                }
                
                file.close();
                isplay = 0;
            }
        }
        
        switch (mp3_dr_command) {
        case MP3_DR_PLAY_CHUNK:
            Mp3Stream(hMp3, u8_mp3_buff, nbytes);
            break;
        case MP3_DR_VOL_CHANGE:
            Mp3SetVol(hMp3, mp3_volumn, mp3_volumn);
            break;
        case MP3_DR_SOFT_RESET:
            Mp3SoftReset(hMp3);
            break;
        case MP3_DR_NONE:
            break;
        default:
            ;
        }
        OSTimeDly(30);
        
//        file = SD.open(filename, O_READ);
//        PrintWithBuf(buf, BUFSIZE, "Play file: %d %s\n", count++, file.name());
//        play_song_count = 0;
//        while (file.available() && play_song_count++ < 1000) {
//        
//            
//            nbytes = MP3_BUF_SIZE;
//            
//            nbytes = file.read(u8_mp3_buff, nbytes);
//
//            Mp3Stream(hMp3, u8_mp3_buff, nbytes);
//            
//        }
//        file.close();
        
//        OSTimeDly(500);
//        PrintWithBuf(buf, BUFSIZE, "Begin streaming sound file  count=%d\n", ++count);
//        // Mp3Stream(hMp3, (INT8U*)Train_Crossing, sizeof(Train_Crossing)); 
//        Mp3StreamSDFile(hMp3, "TRAIN001.mp3");
//        PrintWithBuf(buf, BUFSIZE, "Done streaming sound file  count=%d\n", count);
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




