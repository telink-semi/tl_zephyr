#ifndef __APP_KB_MATRIX_H__
#define __APP_KB_MATRIX_H__

/* Enable C linkage for C++ Compilers: */
#if defined(__cplusplus)
extern "C" {
#endif


//keyboard vaule


#define T_FN                            0xff
#define KB_WIN_LOCK                     0x01

#define KB_A                            0x04
#define KB_B                            0x05
#define KB_C                            0x06//6
#define KB_D                            0x07//7
#define KB_E                            0x08//8
#define KB_F                            0x09//9
#define KB_G                            0x0a//10
#define KB_H                            0x0b//11
#define KB_I                            0x0c//12
#define KB_J                            0x0d//13
#define KB_K                            0x0e//14
#define KB_L                            0x0f//15
#define KB_M                            0x10//16
#define KB_N                            0x11//17
#define KB_O                            0x12//18
#define KB_P                            0x13//19
#define KB_Q                            0x14//20
#define KB_R                            0x15//21
#define KB_S                            0x16//22
#define KB_T                            0x17//23
#define KB_U                            0x18//24
#define KB_V                            0x19//25
#define KB_W                            0x1a//26
#define KB_X                            0x1b//27
#define KB_Y                            0x1c//28
#define KB_Z                            0x1d//29
#define KB_1                            0x1e//30
#define KB_2                            0x1f//31
#define KB_3                            0x20//32
#define KB_4                            0x21//33
#define KB_5                            0x22//34
#define KB_6                            0x23//35
#define KB_7                            0x24//36
#define KB_8                            0x25//37
#define KB_9                            0x26//38
#define KB_0                            0x27//39
#define KB_Enter                        0x28//40
#define KB_Esc                          0x29//41
#define KB_Back                         0x2a//42
#define KB_Tab                          0x2b//43
#define KB_Space                        0x2c//44
#define KB_jianhao                      0x2d//45//-_jianhao
#define KB_denghao                      0x2e//46//+= denghao
#define KB_Lguohao                      0x2f//47//{[
#define KB_Rguohao                      0x30//48//}]
#define KB_xiegang                      0x31//49//|\ xiegang
#define KB_K42                          0x32//50
#define KB_fenhao                       0x33//51//:; fen hao
#define KB_yinhao                       0x34//52//" ' yin hao
#define KB_dunhao                       0x35//53//~`dun hao
#define KB_douhao                       0x36//54 //<,
#define KB_juhao                        0x37//55    //>.
#define KB_wenhao                       0x38//56//?/
#define KB_Caps                         0x39//57
#define KB_F1                           0x3a//58
#define KB_F2                           0x3b//59
#define KB_F3                           0x3c//60
#define KB_F4                           0x3d//61
#define KB_F5                           0x3e//62
#define KB_F6                           0x3f//63
#define KB_F7                           0x40//64
#define KB_F8                           0x41//65
#define KB_F9                           0x42//66
#define KB_F10                          0x43//67
#define KB_F11                          0x44//68
#define KB_F12                          0x45//69

#define KB_PrtSc                        0x46//70
#define KB_Scroll                       0x47//71
#define KB_Pause                        0x48//72//Pause_Break
#define KB_Insert                       0x49//73
#define KB_Home                         0x4a//74
#define KB_PgUp                         0x4b//75//page up
#define KB_Delete                       0x4c//76
#define KB_End                          0x4d//77
#define KB_PgDown                       0x4e//78//page down
#define KB_Right                        0x4f//79
#define KB_Left                         0x50//80
#define KB_Down                         0x51//81
#define KB_Up                           0x52//82
#define KB_Num                          0x53//83
#define KP_chuhao                       0x54//84
#define KP_chenghao                     0x55//85
#define KP_jianhao                      0x56//86
#define KP_jiahao                       0x57//87
#define KP_enter                        0x58//88
#define KP_1                            0x59//89
#define KP_2                            0x5a//90
#define KP_3                            0x5b//91
#define KP_4                            0x5c//92
#define KP_5                            0x5d//93
#define KP_6                            0x5e//94
#define KP_7                            0x5f//95
#define KP_8                            0x60//96
#define KP_9                            0x61//97
#define KP_0                            0x62//98
#define KP_Del                          0x63//99
#define KB_K45                          0x64//100
#define KB_APP                          0x65//101
#define KB_Power                        0x66//102
#define KP_Equal                        0x67//103
#define KB_F13                          0x68//104
#define KB_F14                          0x69//105
#define KB_F15                          0x6a//106
#define KB_F16                          0x6b//107
#define KB_F17                          0x6c//108
#define KB_F18                          0x6d//109

#define KB_F19                          0x6e//110
#define KB_F20                          0x6f//111
#define KB_F21                          0x70//112
#define KB_F22                          0x71//113
#define KB_F23                          0x72//114
#define KB_F24                          0x73//115


#define KB_K107                         0x85//133
#define KB_K56                          0x87//135
#define KB_K133                         0x88//136
#define KB_K14                          0x89//137
#define KB_K132                         0x8a//138
#define KB_K131                         0x8b//139
#define KB_K150                         0x90//144
#define KB_K151                         0x91//145

#define KB_LCtrl                        0xE0
#define KB_LShift                       0xE1
#define KB_LAlt                         0xE2
#define KB_LWin                         0xE3
#define KB_RCtrl                        0xE4
#define KB_RShift                       0xE5
#define KB_RAlt                         0xE6
#define KB_RWin                         0xE7
    
        
#define S_SLEEP                         0xa0
#define S_POWER                         0xa1
#define S_WAKEUP                        0xa2
    
#define C_INDEX_START                   0xa3
#define C_wSearch                       0xa3//www search
#define C_wHome                         0xa4
#define C_wBack                         0xa5
#define C_wForward                      0xa6
#define C_wStop                         0xa7
#define C_wRefresh                      0xa8
#define C_wFavorite                     0xa9
#define C_Media                         0xaa
#define C_email                         0xab
#define C_calculator                    0xac
#define C_computer                      0xad
#define C_nextTrace                     0xae
#define C_prevTrace                     0xaf
#define C_stop                          0xb0
#define C_Play                          0xb1
#define C_mute                          0xb2
#define C_volUP                         0xb3
#define C_volDown                       0xb4
#define C_TELINK                        0xb5
#define C_zoomIn                        0xb6
#define C_zoomOUT                       0xb7
#define C_panLeft                       0xb8
#define C_panRight                      0xb9
#define C_brightUp                      0xba
#define C_brightDn                      0xbb
#define C_RJECT                         0Xbc
#define C_POWER                         0xBD
#define C_terminalLock                  0xBE

#define C_INDEX_END                     0xBe

#define MS_CPI                          0xc0
#define MS_LEFT                         0xc1
#define MS_RIGHT                        0xc2
#define MS_MIDDLE                       0xc3
#define MS_K4                           0xc4
#define MS_K5                           0xc5
#define MS_WUP                          0xc6//wheel up
#define MS_WDOWN                        0xc7// wheel dowm

#if 0
#define T_INDEX_START                   0xc8

#define T_Power                         0xc8
#define T_bind                          0xc9
#define T_Keyobard                      0xca
#define T_lock                          0xcb
#define T_brightUP                      0xcc
#define T_brightDN                      0xcd
#define T_languageC                     0xce
#define T_copy                          0xcf
#define T_paste                         0xd0
#define T_shear                         0xd1
#define T_Phone                         0xd2
#define T_PrtSc                         0xd3
#define T_BackLight                     0XD4
#define T_TaskManager                   0xd5
#define KB_ALT_NBS                      0xd6
#define KB_SHIFT_NBS                    0xd7
#define T_INDEX_END                     0xd7

#define J_start                         0xdd
#define J_back                          0xde
#define J_select                        0xdf
#define J_A                             0xf0
#define J_B                             0xf1
#define J_X                             0xf2
#define J_Y                             0xf3
#define J_HAT_LEFT                      0xf4
#define J_HAT_RIGH                      0xf5
#define J_HAT_UP                        0xf6
#define J_HAT_RIGHT                     0xf7
#define J_L1                            0xf8
#define J_R1                            0xf9
#define J_L2                            0xfa
#define J_R2                            0xfb
#define J_L3                            0xfc
#define J_R3                            0xfd
#define J_HOME                          0xfe
#endif
///////////////////////////////////////////////////////////////////////////////////////////




#define ALL_KEY_BUF_SIZE   16

#define MAX_BTN_CNT           144//8*16  TOTAL_ROW*TOTAL_COL

typedef struct{

    unsigned char nk[8];//normal key 
        
    unsigned char tk_bits[ALL_KEY_BUF_SIZE];  //total key
    unsigned char nk_bits[ALL_KEY_BUF_SIZE];  //all_key
    unsigned char ak_bits[ALL_KEY_BUF_SIZE];  //all_key

    unsigned char press_cnt;
    unsigned char cnt;
    unsigned char ck; // consume_key
    unsigned char sk;  //system_key
        
    unsigned int special_key_press_f;
    
    unsigned char keycode[MAX_BTN_CNT]; 

    
    
    
}app_kb_data_t;
 extern app_kb_data_t app_key_buf;
extern unsigned char    fn_flag;

unsigned char key_scan(void);

void digit_keyscan_init(void);
void digit_keyscan_handle(void);

/* Disable C linkage for C++ Compilers: */
#if defined(__cplusplus)
}
#endif

#endif 

