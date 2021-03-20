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
    MP3_TK_NEXT,    
    MP3_TK_BKWARD,
    MP3_TK_FRWARD,
    MP3_TK_AUTOPLAY
} mp3_tk_cmd_t;

typedef enum mp3_dr_cmd_type {
    MP3_DR_NONE = 0,
    MP3_DR_PLAY_CHUNK,
    MP3_DR_VOL_CHANGE,
    MP3_DR_SOFT_RESET
} mp3_dr_cmd_t;

typedef enum ui_type_type {
    UI_NONE = 0,
    UI_TEXTBOX,
    UI_BUTTON,
    UI_PROGRESSBAR
} ui_type_t;

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

#define PROGRESSBAR_INFO_INIT(x, y, w, h, bar_fill, bg_fill) {x, y, w, h, bar_fill, bg_fill}

typedef struct ui_progressbar_info_type {
    uint16_t rec_x;
    uint16_t rec_y;
    uint16_t rec_w;
    uint16_t rec_h;
    uint16_t bar_fill;
    uint16_t bg_fill;
} ui_progressbar_info_t;

typedef struct ui_button_type {
    ui_type_t ui_type;
    Adafruit_GFX_Button *button;
    ui_button_info_t button_info;
    ui_event_cb_t draw_callback;;
    ui_event_cb_t click_callback;
} ui_button_t;

typedef struct ui_textbox_type {
    ui_type_t ui_type;
    ui_textbox_info_t text_info;
    ui_event_cb_t draw_callback;
} ui_textbox_t ;

typedef struct ui_progressbar_type {
    ui_type_t ui_type;
    ui_progressbar_info_t progress_info;
    ui_event_cb_t draw_callback;
    uint16_t curr;
    uint16_t total;
} ui_progressbar_t;

/*******************************************************************************

    Display command structure

*******************************************************************************/

typedef struct ui_callback_arg_type {
    char *print_buf;
    uint32_t len;
    void *ui_obj;
    union {
        struct {
            uint32_t u32_value0;
            uint32_t u32_value1;
        };
    };
} ui_callback_arg_t;

typedef struct display_tk_cmd_msg_type {
    display_tk_cmd_t command;
    ui_type_t ui_type;
    void *ui_obj;
    union {
        char buf[16];
        struct {
            uint32_t u32_value0;
            uint32_t u32_value1;
        };
    };
} display_tk_cmd_msg_t;

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
static OS_EVENT *diplay_tk_cmd_queue = NULL;
static OS_FLAG_GRP *init_event_group = NULL;

#define DISPLAY_TK_CMD_Q_SIZE 20
static void *display_tk_cmd_q_tbl[DISPLAY_TK_CMD_Q_SIZE];

/*******************************************************************************

    OS Memory

********************************************************************************/

#define CMD_MSG_SIZES 20

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

#define BACKWARD_X (20U)
#define BACKWARD_Y (270U)
#define BACKWARD_W (60U)
#define BACKWARD_H (40U)

#define FORWARD_X (90U)
#define FORWARD_Y (270U)
#define FORWARD_W (60U)
#define FORWARD_H (40U)

#define AUTO_X (20U)
#define AUTO_Y (120U)
#define AUTO_W (60U)
#define AUTO_H (40U)

static Adafruit_GFX_Button play_bu;
static Adafruit_GFX_Button pause_bu;
static Adafruit_GFX_Button stop_bu;
static Adafruit_GFX_Button vol_up_bu;
static Adafruit_GFX_Button vol_down_bu;
static Adafruit_GFX_Button prev_bu;
static Adafruit_GFX_Button next_bu;
static Adafruit_GFX_Button backd_bu;
static Adafruit_GFX_Button forwd_bu;
static Adafruit_GFX_Button auto_bu;

static void button_draw_callback(Adafruit_ILI9341 *, void *);

#define UI_BUTTON_SIZES (sizeof(button_array)/sizeof(button_array[0]))

#define BU_AUTO_ID 9

static mp3_tk_cmd_t ui_button_mp3_cmd_tbl[] = { 
    MP3_TK_PLAY, 
    MP3_TK_PAUSE,
    MP3_TK_STOP,
    MP3_TK_VOL_UP,
    MP3_TK_VOL_DOWN, 
    MP3_TK_PREV,
    MP3_TK_NEXT,
    MP3_TK_BKWARD,
    MP3_TK_FRWARD,
    MP3_TK_AUTOPLAY
};

static char button_text_tbl[][16] = {
    "PLAY", "PAUSE", "STOP", "V_UP", "V_DW", "PREV", "NEXT", "BACK", "FORW", "AUTO"
};

static ui_button_t button_array[] =
{
    {
        UI_BUTTON, &play_bu, 
        UI_BUTTON_INFO_INIT(PLAY_X, PLAY_Y, PLAY_W, PLAY_H, 
                            ILI9341_WHITE, ILI9341_BLACK, ILI9341_WHITE,
                            "Play", 2),
        button_draw_callback, NULL
    }, 
    { 
        UI_BUTTON, &pause_bu, 
        UI_BUTTON_INFO_INIT(PAUSE_X, PAUSE_Y, PAUSE_W, PAUSE_H, 
                            ILI9341_WHITE, ILI9341_BLACK, ILI9341_WHITE,
                            "Pause", 2),
        button_draw_callback, NULL
    }, 
    { 
        UI_BUTTON, &stop_bu,
        UI_BUTTON_INFO_INIT(STOP_X, STOP_Y, STOP_W, STOP_H, 
                            ILI9341_WHITE, ILI9341_BLACK, ILI9341_WHITE,
                            "Stop", 2),
        button_draw_callback, NULL
    },
    {
        UI_BUTTON, &vol_up_bu,
        UI_BUTTON_INFO_INIT(VOL_UP_X, VOL_UP_Y, VOL_UP_W, VOL_UP_H, 
                            ILI9341_WHITE, ILI9341_BLACK, ILI9341_WHITE,
                            "+", 2),
        button_draw_callback, NULL
    },
    {
        UI_BUTTON, &vol_down_bu, 
        UI_BUTTON_INFO_INIT(VOL_DW_X, VOL_DW_Y, VOL_DW_W, VOL_DW_H, 
                            ILI9341_WHITE, ILI9341_BLACK, ILI9341_WHITE,
                            "-", 2), 
        button_draw_callback, NULL
    },
    { 
        UI_BUTTON, &prev_bu,
        UI_BUTTON_INFO_INIT(PREV_X, PREV_Y, PREV_W, PREV_H, 
                            ILI9341_WHITE, ILI9341_BLACK, ILI9341_WHITE,
                            "<", 2),
        button_draw_callback, NULL
    },
    { 
        UI_BUTTON, &next_bu,
        UI_BUTTON_INFO_INIT(NEXT_X, NEXT_Y, NEXT_W, NEXT_H, 
                            ILI9341_WHITE, ILI9341_BLACK, ILI9341_WHITE,
                            ">", 2),
        button_draw_callback, NULL
    },
    { 
        UI_BUTTON, &backd_bu,
        UI_BUTTON_INFO_INIT(BACKWARD_X, BACKWARD_Y, BACKWARD_W, BACKWARD_H, 
                            ILI9341_WHITE, ILI9341_BLACK, ILI9341_WHITE,
                            "<<", 2),
        button_draw_callback, NULL
    },
    { 
        UI_BUTTON, &forwd_bu,
        UI_BUTTON_INFO_INIT(FORWARD_X, FORWARD_Y, FORWARD_W, FORWARD_H, 
                            ILI9341_WHITE, ILI9341_BLACK, ILI9341_WHITE,
                            ">>", 2),
        button_draw_callback, NULL
    },
    { 
        UI_BUTTON, &auto_bu,
        UI_BUTTON_INFO_INIT(AUTO_X, AUTO_Y, AUTO_W, AUTO_H, 
                            ILI9341_WHITE, ILI9341_BLACK, ILI9341_WHITE,
                            "AUTO", 2),
        button_draw_callback, NULL
    }
};
#endif


#define SONG_REC_X (10U)
#define SONG_REC_Y (10U)
#define SONG_REC_W (160U)
#define SONG_REC_H (40U)
#define SONG_TEXT_X (10U)
#define SONG_TEXT_Y (24U)
#define SONG_TEXT_FILL ILI9341_NAVY
#define SONG_TEXT_COLOR ILI9341_WHITE
#define SONG_TEXT_SIZE (2)

#define BU_TEXT_RECT_X (170U)
#define BU_TEXT_RECT_Y (10U)
#define BU_TEXT_RECT_W (60U)
#define BU_TEXT_RECT_H (40U)
#define BU_TEXT_X (170U)
#define BU_TEXT_Y (24U)
#define BUTTON_TEXT_FILL ILI9341_MAROON
#define BUTTON_TEXT_COLOR ILI9341_WHITE
#define BU_TEXT_SIZE (2)

#define VOL_TEXT_RECT_X (10U)
#define VOL_TEXT_RECT_Y (50U)
#define VOL_TEXT_RECT_W (160U)
#define VOL_TEXT_RECT_H (40U)
#define VOL_TEXT_X (10U)
#define VOL_TEXT_Y (64U)
#define VOL_TEXT_FILL ILI9341_MAGENTA
#define VOL_TEXT_COLOR ILI9341_WHITE
#define VOL_TEXT_SIZE (2)

#define PRO_TEXT_RECT_X (170U)
#define PRO_TEXT_RECT_Y (50U)
#define PRO_TEXT_RECT_W (60U)
#define PRO_TEXT_RECT_H (40U)
#define PRO_TEXT_X (170U)
#define PRO_TEXT_Y (64U)
#define PRO_TEXT_FILL ILI9341_DARKGREEN
#define PRO_TEXT_COLOR ILI9341_WHITE
#define PRO_TEXT_SIZE (2)

#define AUTO_TEXT_RECT_X (170U)
#define AUTO_TEXT_RECT_Y (120U)
#define AUTO_TEXT_RECT_W (60U)
#define AUTO_TEXT_RECT_H (40U)
#define AUTO_TEXT_X (174U)
#define AUTO_TEXT_Y (124U)
#define AUTO_TEXT_FILL ILI9341_MAROON
#define AUTO_TEXT_COLOR ILI9341_WHITE
#define AUTO_TEXT_SIZE (2)

#define SONG_TEXTBOX_ID 0
#define BU_TEXTBOX_ID 1
#define VOL_TEXTBOX_ID 2
#define PRO_BAR_TEXTBOX_ID 3
#define AUTO_TEXTBOX_ID 4

static void textbox_draw_callback(Adafruit_ILI9341 *_gfx, void *_arg);

static ui_textbox_t ui_textbox_tbl[] = 
{
    {
        UI_TEXTBOX,
        TEXT_INFO_INIT(SONG_REC_X, SONG_REC_Y, SONG_REC_W, SONG_REC_H, 
                     SONG_TEXT_X, SONG_TEXT_Y, SONG_TEXT_FILL, SONG_TEXT_COLOR,
                     SONG_TEXT_SIZE, ""),
        textbox_draw_callback
    },
    {
        UI_TEXTBOX,
        TEXT_INFO_INIT(BU_TEXT_RECT_X, BU_TEXT_RECT_Y, BU_TEXT_RECT_W, BU_TEXT_RECT_H, 
                     BU_TEXT_X, BU_TEXT_Y, BUTTON_TEXT_FILL, BUTTON_TEXT_COLOR,
                     BU_TEXT_SIZE, ""),
        textbox_draw_callback
    },
    {
        UI_TEXTBOX,
        TEXT_INFO_INIT(VOL_TEXT_RECT_X, VOL_TEXT_RECT_Y, VOL_TEXT_RECT_W, VOL_TEXT_RECT_H, 
                     VOL_TEXT_X, VOL_TEXT_Y, VOL_TEXT_FILL, VOL_TEXT_COLOR,
                     VOL_TEXT_SIZE, ""),
        textbox_draw_callback
    },
    {
        UI_TEXTBOX,
        TEXT_INFO_INIT(PRO_TEXT_RECT_X, PRO_TEXT_RECT_Y, PRO_TEXT_RECT_W, PRO_TEXT_RECT_H, 
                     PRO_TEXT_X, PRO_TEXT_Y, PRO_TEXT_FILL, PRO_TEXT_COLOR,
                     PRO_TEXT_SIZE, ""),
        textbox_draw_callback
    },
    {
        UI_TEXTBOX,
        TEXT_INFO_INIT(AUTO_TEXT_RECT_X, AUTO_TEXT_RECT_Y, AUTO_TEXT_RECT_W, AUTO_TEXT_RECT_H, 
                     AUTO_TEXT_X, AUTO_TEXT_Y, AUTO_TEXT_FILL, AUTO_TEXT_COLOR,
                     AUTO_TEXT_SIZE, "AUTO\nOFF"),
        textbox_draw_callback
    }
};

#define SONG_PROBAR_REC_X (10U)
#define SONG_PROBAR_REC_Y (100U)
#define SONG_PROBAR_REC_W (220U)
#define SONG_PROBAR_REC_H (10U)
#define SONG_PROBAR_BAR ILI9341_WHITE
#define SONG_PROBAR_BG ILI9341_BLACK

#define SONG_PROBAR_ID 0

static void progress_bar_draw_callback(Adafruit_ILI9341 *_gfx, void *_arg);

static ui_progressbar_t ui_progressbar_tbl[] = 
{
    {
        UI_PROGRESSBAR,
        PROGRESSBAR_INFO_INIT(SONG_PROBAR_REC_X, SONG_PROBAR_REC_Y, 
                              SONG_PROBAR_REC_W, SONG_PROBAR_REC_H, 
                              SONG_PROBAR_BAR, SONG_PROBAR_BG),
        progress_bar_draw_callback,
        0, 0
    }
};

static void progress_bar_draw_callback(Adafruit_ILI9341 *_gfx, void *_arg)
{
    ui_callback_arg_t *cmd_arg_p = (ui_callback_arg_t *)_arg;
    ui_progressbar_t *progressbar_p;
    ui_progressbar_info_t *info_p;
    uint32_t new_curr, new_total;
    uint32_t new_bar_w, old_bar_w = 0;
    
    cmd_arg_p = (ui_callback_arg_t *)_arg;
    progressbar_p = (ui_progressbar_t *)cmd_arg_p->ui_obj;
    info_p = &progressbar_p->progress_info;
    new_curr = cmd_arg_p->u32_value0;
    new_total = cmd_arg_p->u32_value1;
    
    old_bar_w = info_p->rec_w * progressbar_p->curr / progressbar_p->total;
    new_bar_w = info_p->rec_w * new_curr / new_total;
    
    // if need to update
    if (new_bar_w != old_bar_w) {
        // progress bar increase
        if (new_bar_w > old_bar_w) {
            _gfx->fillRect(info_p->rec_x + old_bar_w, info_p->rec_y, new_bar_w - old_bar_w, info_p->rec_h, info_p->bar_fill);
        
        // progress bar decrease
        } else if (new_bar_w < old_bar_w) {
            _gfx->fillRect(info_p->rec_x + new_bar_w, info_p->rec_y, old_bar_w - new_bar_w, info_p->rec_h, info_p->bg_fill);
        }
    // edge condition for initial bar
    } else if (new_bar_w == 0){
        _gfx->fillRect(info_p->rec_x + new_bar_w, info_p->rec_y, info_p->rec_w, info_p->rec_h, info_p->bg_fill);
    }
    
    // update curr progress and total progress
    progressbar_p->curr = new_curr;
    progressbar_p->total =  new_total;
}

static void button_draw_callback(Adafruit_ILI9341 *_gfx, void *_arg)
{
    ui_callback_arg_t *callback_arg;
    ui_button_t *bu;
    Adafruit_GFX_Button *button;
    ui_button_info_t *bu_info;
    
    callback_arg = (ui_callback_arg_t *)_arg;
    bu = (ui_button_t *)callback_arg->ui_obj;
    button = bu->button;
    bu_info = &bu->button_info;
    
    button->initButton(_gfx,
                   bu_info->x + bu_info->w / 2, bu_info->y + bu_info->h / 2, 
                   bu_info->w, bu_info->h, 
                   bu_info->outline, bu_info->fill, bu_info->textcolor, 
                   bu_info->label, bu_info->textsize);
    button->drawButton();
}

static void textbox_draw_callback(Adafruit_ILI9341 *_gfx, void *_arg)
{
    ui_callback_arg_t *cmd_arg_p = (ui_callback_arg_t *)_arg;
    ui_textbox_t * textbox_p;
    ui_textbox_info_t *info_p;
    const char *p;
    uint32_t u32_count;
    
    cmd_arg_p = (ui_callback_arg_t *)_arg;
    textbox_p = (ui_textbox_t *)cmd_arg_p->ui_obj;
    info_p = &textbox_p->text_info;
    p = (const char *)info_p->content;
    u32_count = 0;
    
    _gfx->fillRect(info_p->rec_x, info_p->rec_y, info_p->rec_w, info_p->rec_h, 
                   info_p->fill);
    _gfx->setCursor(info_p->text_x, info_p->text_y);
    _gfx->setTextColor(info_p->text_color);
    _gfx->setTextSize(info_p->text_size);
    
    while (u32_count < 16 && *p != '\0') {
        _gfx->write(*p);
        
        // after printting '\n', x cursor will be set as 0
        // reset x cursor to alian first textline x
        if (*p == '\n') {        
            _gfx->setCursor(info_p->text_x, _gfx->getCursorY());
        }
        
        ++p;
        ++u32_count;
    }
}

//static void progress_bar_e_cb(Adafruit_ILI9341 *_gfx, void *_arg)
//{
//    cmd_arg_t *cmd_arg_p = (cmd_arg_t *)_arg;
//    uint32_t u32_progress_bar = 220 * cmd_arg_p->u32_value / 100;
//    _gfx->fillRect(10, 120, 220, 20, ILI9341_BLACK);
//    _gfx->fillRect(10, 120, u32_progress_bar, 20, ILI9341_LIGHTGREY);
//}

uint8_t button_draw(ui_button_t *button, char *buf, uint32_t buf_len)
{

    display_tk_cmd_msg_t *cmd_msg_p = NULL;
    uint8_t u8_error = OS_ERR_NONE;
    
    // get memory block
    cmd_msg_p = (display_tk_cmd_msg_t *)OSMemGet(cmd_msg_mem, &u8_error);
            
    if (!cmd_msg_p) {
        PrintWithBuf(buf, buf_len, "Mp3 Task: cmd mem allocate fails: %d\n", u8_error);
    }
            
    cmd_msg_p->command = DISPLAY_TK_UI_CB;
    cmd_msg_p->ui_type = UI_BUTTON;
    cmd_msg_p->ui_obj = (void *)button;
                
    u8_error = OSQPost(diplay_tk_cmd_queue, cmd_msg_p);
                
    if (u8_error != OS_ERR_NONE) {
        PrintWithBuf(buf, buf_len, "Mp3 Task: cmd post fails: %d\n", u8_error);
    }
    
    return u8_error;
}

uint8_t textbox_update(ui_textbox_t *textbox, char *label, uint32_t len, char *buf, uint32_t buf_len)
{

    display_tk_cmd_msg_t *cmd_msg_p = NULL;
    uint8_t u8_error = OS_ERR_NONE;
    
    // get memory block
    cmd_msg_p = (display_tk_cmd_msg_t *)OSMemGet(cmd_msg_mem, &u8_error);
            
    if (!cmd_msg_p) {
        PrintWithBuf(buf, buf_len, "Mp3 Task: cmd mem allocate fails: %d\n", u8_error);
    }
            
    cmd_msg_p->command = DISPLAY_TK_UI_CB;
    cmd_msg_p->ui_type = UI_TEXTBOX;
    cmd_msg_p->ui_obj = (void *)textbox;
    
    memset(cmd_msg_p->buf, 0, 16);
    memcpy(cmd_msg_p->buf, label, len);
                
    u8_error = OSQPost(diplay_tk_cmd_queue, cmd_msg_p);
                
    if (u8_error != OS_ERR_NONE) {
        PrintWithBuf(buf, buf_len, "Mp3 Task: cmd post fails: %d\n", u8_error);
    }

    return u8_error;
}

uint8_t progessbar_draw(ui_progressbar_t *progressbar, uint32_t curr, uint32_t total,char *buf, uint32_t buf_len)
{

    display_tk_cmd_msg_t *cmd_msg_p = NULL;
    uint8_t u8_error = OS_ERR_NONE;
    
    // get memory block
    cmd_msg_p = (display_tk_cmd_msg_t *)OSMemGet(cmd_msg_mem, &u8_error);
            
    if (!cmd_msg_p) {
        PrintWithBuf(buf, buf_len, "Mp3 Task: cmd mem allocate fails: %d\n", u8_error);
    }
            
    cmd_msg_p->command = DISPLAY_TK_UI_CB;
    cmd_msg_p->ui_type = UI_PROGRESSBAR;
    cmd_msg_p->ui_obj = (void *)progressbar;
    
    cmd_msg_p->u32_value0 = curr;
    cmd_msg_p->u32_value1 = total;
                
    u8_error = OSQPost(diplay_tk_cmd_queue, cmd_msg_p);
                
    if (u8_error != OS_ERR_NONE) {
        PrintWithBuf(buf, buf_len, "Mp3 Task: cmd post fails: %d\n", u8_error);
    }
    
    return u8_error;
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
    
    // Create message mail box
    mp3_tk_cmd_mb = OSMboxCreate((void *)0);
    
    // Create message queue
    diplay_tk_cmd_queue = OSQCreate(display_tk_cmd_q_tbl,DISPLAY_TK_CMD_Q_SIZE);
    
    // Create event flags for synchornize three task initialzation process
    init_event_group = OSFlagCreate(0x00, &u8_error);
    
    // Create the test tasks
    PrintWithBuf(buf, BUFSIZE, "StartupTask: Creating the application tasks\n");

    // The maximum number of tasks the application can have is defined by OS_MAX_TASKS in os_cfg.h
    OSTaskCreate(Mp3DriverTask, (void*)0, &Mp3DriverTaskStk[MP3_DRIVER_TASK_STK_SIZE-1], MP3_DRIVER_TASK_PRIO);
    OSTaskCreate(LcdDisplayTask, (void*)0, &LcdDisplayTaskStk[DISPLAY_TASK_STK_SIZE-1], DISPLAY_TASK_PRIO);
    //OSTaskCreate(Mp3PlayerTask, (void*)0, &Mp3PlayerTaskStk[MP3PLAY_TASK_STK_SIZE-1], MP3PLAY_TASK_PRIO);
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

static void DrawButtons2(char *print_buf, uint32_t u32_len)
{
    uint32_t u32_i;
    
    for (u32_i = 0; u32_i < UI_BUTTON_SIZES; ++u32_i) {        
        button_draw(&button_array[u32_i], print_buf, u32_len);
        OSTimeDly(10);
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
    uint16_t u16_grp_flag;
    

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
    PrintWithBuf(buf, BUFSIZE, "LcdDisplayTask post event group to mp3 task for notifying\n");
    
    // tell mp3 task that lcd initialization is finished
    u16_grp_flag = OSFlagPost(init_event_group, 0x02, OS_FLAG_SET, &u8_error);
    
    if (u8_error != OS_ERR_NONE) {
        PrintWithBuf(buf, BUFSIZE, "Error: LcdDisplayTask fails to set 0x02 in event group %d\n", u8_error);
    }
    
    lcdCtrl.fillScreen(ILI9341_BLACK);
    
    // draw x axis and y axis grid
    DrawGrid(&lcdCtrl, 0, 0, ILI9341_TFTWIDTH, ILI9341_TFTHEIGHT, 20, 20, ILI9341_WHITE);
    
    // draw a rectangular for song name space
    //lcdCtrl.drawRect(10, 10, 220, 80, ILI9341_MAROON);
    
    // draw a retangular for process bar
    //lcdCtrl.drawRect(10, 110, 220, 40, ILI9341_MAROON);
    
    //DrawButtons(&lcdCtrl);
    
    display_tk_cmd_msg_t *cmd_msg_p;
    display_tk_cmd_msg_t cmd_msg;
    ui_callback_arg_t ui_callback_arg;

    while (1) {
        
        // display task is event trigger task
        cmd_msg_p = (display_tk_cmd_msg_t *)OSQPend(diplay_tk_cmd_queue, TIMEOUT_INFINITE, &u8_error);
        
        // copy msg
        memcpy(&cmd_msg, cmd_msg_p, sizeof(display_tk_cmd_msg_t));
        
        // release resource to memory pool(uCOS memory heap)
        u8_error = OSMemPut(cmd_msg_mem, cmd_msg_p);
        
        if (u8_error != OS_ERR_NONE) {
            PrintWithBuf(buf, BUFSIZE, "Error: Display Task mem free %d\n", u8_error);
        }
        
        cmd_msg_p = (display_tk_cmd_msg_t *)0;
        
        if (u8_error) {
        }
        
        switch (cmd_msg.command) {
        case DISPLAY_TK_CLICK: // deprecated
            break;
        case DISPLAY_TK_UI_CB:
            switch (cmd_msg.ui_type) {
            case UI_TEXTBOX:
                
                ui_textbox_t *textbox;
                ui_textbox_info_t *textbox_info;
                
                // update text box context
                textbox = (ui_textbox_t *) cmd_msg.ui_obj;
                textbox_info = &textbox->text_info;
                memcpy(textbox_info->content, cmd_msg.buf, 16);
                
                // prepare passing argument to callback function
                ui_callback_arg.ui_obj = cmd_msg.ui_obj;
                ui_callback_arg.print_buf = buf;
                ui_callback_arg.len = BUFSIZE;
                
                textbox->draw_callback(&lcdCtrl, (void *)&ui_callback_arg);
                break;
            case UI_BUTTON:
                
                ui_button_t *button;
                
                button = (ui_button_t *) cmd_msg.ui_obj;
                
                // prepare passing argument to callback function
                ui_callback_arg.ui_obj = cmd_msg.ui_obj;
                ui_callback_arg.print_buf = buf;
                ui_callback_arg.len = BUFSIZE;
                
                button->draw_callback(&lcdCtrl, (void *)&ui_callback_arg);
                break;
            case UI_PROGRESSBAR:
                
                ui_progressbar_t *progressbar;
                
                progressbar = (ui_progressbar_t *) cmd_msg.ui_obj;
                
                // update curr and total progress value in progress bar
//                progressbar->curr = cmd_msg.u32_value0;
//                progressbar->total = cmd_msg.u32_value1;
                
                // prepare passing argument to callback function
                ui_callback_arg.ui_obj = cmd_msg.ui_obj;
                ui_callback_arg.print_buf = buf;
                ui_callback_arg.len = BUFSIZE;
                
                ui_callback_arg.u32_value0 = cmd_msg.u32_value0;
                ui_callback_arg.u32_value1 = cmd_msg.u32_value1;
                
                progressbar->draw_callback(&lcdCtrl, (void *)&ui_callback_arg);
                break;
            default:
                break;
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
    uint16_t u16_grp_flag;

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

    // finishing touch hardware perpherial initialization and post to mp3 task
    u16_grp_flag = OSFlagPost(init_event_group, 0x01, OS_FLAG_SET, &u8_error);
    
//    // mp3 and display task finish ui initialization
//    u16_grp_flag = OSFlagPend(init_event_group, 0x01, OS_FLAG_WAIT_CLR_ALL | OS_FLAG_CONSUME, 0, &u8_error);
//    
//    if (u8_error != OS_ERR_NONE) {
//        PrintWithBuf(buf, BUFSIZE, "Touch Task resume display task error: %d\n", u8_error);
//    }
    
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
                    u8_error = OSMboxPost(mp3_tk_cmd_mb, &ui_button_mp3_cmd_tbl[u32_i]);
                    
                    u8_error = textbox_update(&ui_textbox_tbl[BU_TEXTBOX_ID], button_text_tbl[u32_i], 16, buf, BUFSIZE); 
                                              
                    break;
                }
            }
        }
        
        if (touched && button_match) {
            OSTimeDly(100);
        } else {
            OSTimeDly(30);
        }
    }
}

/************************************************************************************

   Runs MP3 demo code

************************************************************************************/

static void progress_text_generate(uint32_t u32_num, char *dst, uint32_t u32_len)
{
    if (u32_len < 8) return;
    
    dst[0] = (u32_num/100)?(u32_num/100)+'0':' ';
    dst[1] = (u32_num/10)?(u32_num/10)+'0':' ';    
    dst[2] = u32_num%10+'0';
    dst[3] = '%';
    dst[4] = '\0';   
}

static void vol_text_generate(uint32_t u32_num, char *dst, uint32_t u32_len)
{
    if (u32_len < 16) return;
     
    dst[0] = 'V';
    dst[1] = 'O';
    dst[2] = 'L';
    dst[3] = ':';
    dst[4] = (u32_num/10)?(u32_num/10)+'0':' ';    
    dst[5] = u32_num%10+'0';
    dst[6] = '/';
    dst[7] = '1';
    dst[8] = '0';
    dst[9] = '\0';   
}

#define MP3_BUF_SIZE 2048

static uint8_t u8_mp3_buff[MP3_BUF_SIZE];

void Mp3DriverTask(void* pdata)
{
    PjdfErrCode pjdfErr;
    INT32U length;
    uint32_t nbytes;
    File dir, file;
    uint16_t u16_grp_flag;
    uint8_t u8_error;
    
    
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

    
    // event bits wait for 2 bits set
    
    // Send initialization data to the MP3 decoder and run a test
    PrintWithBuf(buf, BUFSIZE, "Starting MP3 Task\n");
    
    dir = SD.open("/", O_READ);
    
    PrintWithBuf(buf, BUFSIZE, "Open root directory: %s\n", dir.name());
    
    if (!dir) {
        PrintWithBuf(buf, BUFSIZE, "Error: could not open SD card file '%s'\n", "/");
    }

    static char mp3_files_table[100][16];
    static uint32_t u32_max_files = 0;
    
    while ((file = dir.openNextFile()) && u32_max_files < 100) {        
        memset(mp3_files_table[u32_max_files], 0, 16);
        strncpy(mp3_files_table[u32_max_files], file.name(), 15);
        
        char *p = NULL;
        p = strstr(mp3_files_table[u32_max_files], ".MP3");
        
        if (p) {
            ++u32_max_files;
        }
        file.close();
    }
    
    dir.close();
    
    for (uint32_t i = 0;i < u32_max_files; ++i) {
        PrintWithBuf(buf, BUFSIZE, "MP3 file in the table[%3d] '%s'\n", i, mp3_files_table[i]);
    }
    
    Mp3Init(hMp3);
    Mp3StreamInit(hMp3);
    
    // wait display and touch task ready
    u16_grp_flag = OSFlagPend(init_event_group, 0x03, OS_FLAG_WAIT_SET_ALL | OS_FLAG_CONSUME, 0, &u8_error);
    
    if (u8_error != OS_ERR_NONE) {
        PrintWithBuf(buf, BUFSIZE, "Error: Mp3 Task fails to pend on event group %d\n", u8_error);
    }
    
//    // display unblock first, and then unblock touch task after finishing ui initialization
//    u16_grp_flag = OSFlagPost(init_event_group, 0x02, OS_FLAG_SET, &u8_error);
//
//    if (u8_error != OS_ERR_NONE) {
//        PrintWithBuf(buf, BUFSIZE, "Error: Mp3 Task fails to post event group to lcd task %d\n", u8_error);
//    }
    
    static uint8_t vol_tbl[] = 
        { 0xFE, 0x50, 0x40, 0x3B, 0x38, 0x30, 0x2C, 0x26, 0x24, 0x23 };
    static char auto_label[][16] = { "AUTO\nOFF", "AUTO\nON" };
    static char progress_buf[16];
    static char vol_buf[16];
    static uint32_t u32_name_index = 0, u32_filesize = 0, isplay = 0;
    static uint32_t u32_playsize, u32_new_chunk_id = 0, u32_old_chunk_id = 0;
    static uint8_t mp3_vol = 5;
    static mp3_tk_cmd_t *mp3_tk_command_p = NULL;
    static mp3_tk_cmd_t mp3_tk_command = MP3_TK_NONE;
    static mp3_dr_cmd_t mp3_cmd_queue [3];
    static uint32_t cmd_in = 0, cmd_out = 0;
    static uint32_t isautoplay = 0;
    
#define CHUNCK_SIZE 100
    
    nbytes = MP3_BUF_SIZE;
    
    Mp3SetVol(hMp3, vol_tbl[mp3_vol], vol_tbl[mp3_vol]);
    
    DrawButtons2(buf, BUFSIZE);
    
    // initial song text box
    u8_error = textbox_update(&ui_textbox_tbl[SONG_TEXTBOX_ID], mp3_files_table[u32_name_index], 16, buf, BUFSIZE);
    OSTimeDly(30);
    
    // initial button text box
    u8_error = textbox_update(&ui_textbox_tbl[BU_TEXTBOX_ID], "", 16, buf, BUFSIZE);
    OSTimeDly(30);
    
    // initial vol text box
    vol_text_generate(mp3_vol, vol_buf, 16);
    u8_error = textbox_update(&ui_textbox_tbl[VOL_TEXTBOX_ID], vol_buf, 16, buf, BUFSIZE);
    OSTimeDly(30);
    
    // initial progress text box
    progress_text_generate(u32_old_chunk_id, progress_buf, 16);
    u8_error = textbox_update(&ui_textbox_tbl[PRO_BAR_TEXTBOX_ID], progress_buf, 16, buf, BUFSIZE);
    OSTimeDly(30);
    
    // initial auto flag text box
    u8_error = textbox_update(&ui_textbox_tbl[AUTO_TEXTBOX_ID], auto_label[isautoplay], 16, buf, BUFSIZE);
    OSTimeDly(30);
    
    // initial progress bar
    u8_error = progessbar_draw(&ui_progressbar_tbl[SONG_PROBAR_ID], u32_old_chunk_id, 100, buf, BUFSIZE);
    OSTimeDly(30);
    
    while (1)
    {
        if (isplay) {
            mp3_tk_command_p = (mp3_tk_cmd_t *)OSMboxPend(mp3_tk_cmd_mb, 30, &u8_error);
        } else {            
            mp3_tk_command_p = (mp3_tk_cmd_t *)OSMboxPend(mp3_tk_cmd_mb, 0, &u8_error);
        }
        
        mp3_tk_command = MP3_TK_NONE;
        
        // copy command from mailbox
        // even though no task will modify mailbox message, copy it to avoid sharing data
        if (mp3_tk_command_p) {
            mp3_tk_command = *mp3_tk_command_p;
        }
        
        switch (mp3_tk_command) {
        case MP3_TK_PLAY:
            if (!isplay) {
                if (!file) {
                    file = SD.open(mp3_files_table[u32_name_index], O_READ);
                    u32_filesize = file.size();
                    u32_playsize = 0;
                    u32_new_chunk_id = u32_old_chunk_id = 0;
                    
    
                    progress_text_generate(u32_old_chunk_id, progress_buf, 16);
                    u8_error = textbox_update(&ui_textbox_tbl[PRO_BAR_TEXTBOX_ID], progress_buf, 16, buf, BUFSIZE);
                    
                    u8_error = progessbar_draw(&ui_progressbar_tbl[SONG_PROBAR_ID], u32_old_chunk_id, 100, buf, BUFSIZE);

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
                
                u32_new_chunk_id = u32_old_chunk_id = 0;
                progress_text_generate(u32_old_chunk_id, progress_buf, 16);
                u8_error = textbox_update(&ui_textbox_tbl[PRO_BAR_TEXTBOX_ID], progress_buf, 16, buf, BUFSIZE);
                
                u8_error = progessbar_draw(&ui_progressbar_tbl[SONG_PROBAR_ID], u32_old_chunk_id, 100, buf, BUFSIZE);
                
                mp3_cmd_queue[cmd_in++] = MP3_DR_SOFT_RESET;
            }
            if (isplay) { 
                isplay = 0;
            }
            break;
        case MP3_TK_VOL_UP:
            if (mp3_vol < sizeof(vol_tbl)/sizeof(vol_tbl[0])) {
                ++mp3_vol;
                mp3_cmd_queue[cmd_in++] = MP3_DR_VOL_CHANGE;
                
                vol_text_generate(mp3_vol, vol_buf, 16);
                u8_error = textbox_update(&ui_textbox_tbl[VOL_TEXTBOX_ID], vol_buf, 16, buf, BUFSIZE); 
            }
            break;
        case MP3_TK_VOL_DOWN:
            if (mp3_vol > 0) {
                --mp3_vol;
                mp3_cmd_queue[cmd_in++] = MP3_DR_VOL_CHANGE;
                
                vol_text_generate(mp3_vol, vol_buf, 16);
                u8_error = textbox_update(&ui_textbox_tbl[VOL_TEXTBOX_ID], vol_buf, 16, buf, BUFSIZE); 
            }
            break;
        case MP3_TK_PREV:
            if (u32_name_index > 0) {
                --u32_name_index;
            } else {
                u32_name_index = (u32_max_files - 1);
            }
                
            if (file) {
                    
                file.close();                

                file = SD.open(mp3_files_table[u32_name_index], O_READ);
                u32_filesize = file.size();
                u32_playsize = 0;
                u32_new_chunk_id = u32_old_chunk_id = 0;
                
                if (!isautoplay) isplay = 0;
                
                // to clear rest data in vs1053
                mp3_cmd_queue[cmd_in++] = MP3_DR_SOFT_RESET;
            }
            
            u8_error = textbox_update(&ui_textbox_tbl[SONG_TEXTBOX_ID], mp3_files_table[u32_name_index], 16, buf, BUFSIZE);
                
            u32_new_chunk_id = u32_old_chunk_id = 0;
            progress_text_generate(u32_old_chunk_id, progress_buf, 16);
            u8_error = textbox_update(&ui_textbox_tbl[PRO_BAR_TEXTBOX_ID], progress_buf, 16, buf, BUFSIZE);
                
            u8_error = progessbar_draw(&ui_progressbar_tbl[SONG_PROBAR_ID], u32_old_chunk_id, 100, buf, BUFSIZE);
            break;
        case MP3_TK_NEXT:
            if (u32_name_index < (u32_max_files - 1)) {
                ++u32_name_index;
            } else {
                u32_name_index = 0;
            }
            
            if (file) {
                    
                file.close();                

                file = SD.open(mp3_files_table[u32_name_index], O_READ);
                u32_filesize = file.size();
                u32_playsize = 0;
                u32_new_chunk_id = u32_old_chunk_id = 0;
                
                if (!isautoplay) isplay = 0;
                
                // to clear rest data in vs1053
                mp3_cmd_queue[cmd_in++] = MP3_DR_SOFT_RESET;
            }
                
            u8_error = textbox_update(&ui_textbox_tbl[SONG_TEXTBOX_ID], mp3_files_table[u32_name_index], 16, buf, BUFSIZE);
                
            u32_new_chunk_id = u32_old_chunk_id = 0;
            progress_text_generate(u32_old_chunk_id, progress_buf, 16);
            u8_error = textbox_update(&ui_textbox_tbl[PRO_BAR_TEXTBOX_ID], progress_buf, 16, buf, BUFSIZE);
                
            u8_error = progessbar_draw(&ui_progressbar_tbl[SONG_PROBAR_ID], u32_old_chunk_id, 100, buf, BUFSIZE);                
            
            break;
        case MP3_TK_BKWARD:
            if (file) {
                
                uint32_t u32_new_seek, u32_tmpt_chunk;
                
                u32_tmpt_chunk = u32_new_chunk_id;
                
                if (u32_tmpt_chunk >= 10) {
                    u32_tmpt_chunk -= 10;
                } else {
                    u32_tmpt_chunk = 0;
                }
                    
                u32_new_seek = u32_tmpt_chunk * u32_filesize / 100;
                    
                if ( file.seek(u32_new_seek) ) {
                        
                    u32_old_chunk_id = u32_new_chunk_id;
                    u32_new_chunk_id = u32_tmpt_chunk;
                        
                    u32_playsize = u32_new_seek;

                    if (u32_new_chunk_id != u32_old_chunk_id) {
                        u32_old_chunk_id = u32_new_chunk_id;

                        progress_text_generate(u32_old_chunk_id, progress_buf, 16);
                        u8_error = textbox_update(&ui_textbox_tbl[PRO_BAR_TEXTBOX_ID], progress_buf, 16, buf, BUFSIZE);

                        u8_error = progessbar_draw(&ui_progressbar_tbl[SONG_PROBAR_ID], u32_old_chunk_id, 100, buf, BUFSIZE);
                    }
                        
                    // update progress bar
                                               
                    mp3_cmd_queue[cmd_in++] = MP3_DR_SOFT_RESET; 
                }
            }
            break;
        case MP3_TK_FRWARD:
            if (file) {
                
                uint32_t u32_new_seek, u32_tmpt_chunk;
                
                u32_tmpt_chunk = u32_new_chunk_id;
                
                if (u32_tmpt_chunk <= 90) {
                    u32_tmpt_chunk += 10;
                } else {
                    u32_tmpt_chunk = 100;
                }
                    
                u32_new_seek = u32_tmpt_chunk * u32_filesize / 100;
                    
                if ( file.seek(u32_new_seek) ) {
                        
                    u32_old_chunk_id = u32_new_chunk_id;
                    u32_new_chunk_id = u32_tmpt_chunk;
                        
                    u32_playsize = u32_new_seek;

                    if (u32_new_chunk_id != u32_old_chunk_id) {
                        u32_old_chunk_id = u32_new_chunk_id;

                        progress_text_generate(u32_old_chunk_id, progress_buf, 16);
                        u8_error = textbox_update(&ui_textbox_tbl[PRO_BAR_TEXTBOX_ID], progress_buf, 16, buf, BUFSIZE);

                        u8_error = progessbar_draw(&ui_progressbar_tbl[SONG_PROBAR_ID], u32_old_chunk_id, 100, buf, BUFSIZE);
                    }
                        
                    // update progress bar
                                               
                    mp3_cmd_queue[cmd_in++] = MP3_DR_SOFT_RESET; 
                }
            }
            break;
        case MP3_TK_AUTOPLAY:
            if (isautoplay == 0) {
                isautoplay = 1;
            } else {
                isautoplay = 0;
            }
            u8_error = textbox_update(&ui_textbox_tbl[AUTO_TEXTBOX_ID], auto_label[isautoplay], 16, buf, BUFSIZE);
            break;
        case MP3_TK_NONE:
            break;
        default:
            ;
        }

        if (isplay) {
            if (file.available()) {
                nbytes = MP3_BUF_SIZE;
                nbytes = file.read(u8_mp3_buff, nbytes);
                u32_playsize += nbytes;
                
                mp3_cmd_queue[cmd_in++] = MP3_DR_PLAY_CHUNK;
                
                u32_new_chunk_id = u32_playsize * CHUNCK_SIZE / u32_filesize; // divie zero trap??
                
                if (u32_new_chunk_id != u32_old_chunk_id) {
                    u32_old_chunk_id = u32_new_chunk_id;
                    
                    progress_text_generate(u32_old_chunk_id, progress_buf, 16);
                    u8_error = textbox_update(&ui_textbox_tbl[PRO_BAR_TEXTBOX_ID], progress_buf, 16, buf, BUFSIZE);
                    
                    u8_error = progessbar_draw(&ui_progressbar_tbl[SONG_PROBAR_ID], u32_old_chunk_id, 100, buf, BUFSIZE);
                }
            } else {
                u32_new_chunk_id = u32_old_chunk_id = 0;
                progress_text_generate(u32_old_chunk_id, progress_buf, 16);
                u8_error = textbox_update(&ui_textbox_tbl[PRO_BAR_TEXTBOX_ID], progress_buf, 16, buf, BUFSIZE);
                
                u8_error = progessbar_draw(&ui_progressbar_tbl[SONG_PROBAR_ID], u32_old_chunk_id, 100, buf, BUFSIZE);
                
                file.close();
                u32_playsize = 0;
                u32_filesize = 0;
                isplay = 0;
                mp3_cmd_queue[cmd_in++] = MP3_DR_SOFT_RESET;                
                
                if (isautoplay) {
                    
                    ++u32_name_index;
                    
                    if (u32_name_index >= u32_max_files) {
                        u32_name_index = 0;
                    }
                    
                    file = SD.open(mp3_files_table[u32_name_index], O_READ);
                    u32_filesize = file.size();
                    
                    u8_error = textbox_update(&ui_textbox_tbl[SONG_TEXTBOX_ID], mp3_files_table[u32_name_index], 16, buf, BUFSIZE);
                    
                    isplay = 1;
                }
            }
        }
        
        for (cmd_out; cmd_out < cmd_in; ++cmd_out) {
            switch (mp3_cmd_queue[cmd_out]) {
            case MP3_DR_PLAY_CHUNK:
                Mp3Stream(hMp3, u8_mp3_buff, nbytes);
                break;
            case MP3_DR_VOL_CHANGE:
                Mp3SetVol(hMp3, vol_tbl[mp3_vol], vol_tbl[mp3_vol]);
                break;
            case MP3_DR_SOFT_RESET:
                Mp3SoftReset(hMp3);
                break;
            case MP3_DR_NONE:
                break;
            default:
                ;
            }
        }
        cmd_out = cmd_in = 0;
        
        //OSTimeDly(30);
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