#include "keyboard.h"
#include "../lib/string.h"
///////////////////////////////////////
bool KeyBoardShift=0;//判断当时是否摁下shift
bool KeyBoardCapsLock=0;//判断大写是否锁定
bool KeyBoardCtrl=0;//判断ctrl是否按下
uint16_t KeyBoardQueue[KeyBoardQueueMaxCnt]={-1};//键盘键入值队列
int KeyBoardQueueIdx=0;
struct ioQueue KeyboardQueue;
//键盘输入码到MMZZ操作系统的字符码转换表
const enum KEYBOARDCODE ScanCodeConvertTable[][2]={
    {-1,-1},//0x0
    {CODE_ESC_DOWN,CODE_ESC_DOWN},//0x1
    {CODE_1_DOWN,CODE_BANG_DOWN},//0x2
    {CODE_2_DOWN,CODE_AT_DOWN},//0x3
    {CODE_3_DOWN,CODE_HASH_DOWN},//0x4
    {CODE_4_DOWN,CODE_DOLLAR_DOWN},//0x5
    {CODE_5_DOWN,CODE_MOD_DOWN},//0x6
    {CODE_6_DOWN,CODE_CARET_DOWN},//0x7
    {CODE_7_DOWN,CODE_AND_DOWN},//0x8
    {CODE_8_DOWN,CODE_STAR_DOWN},//0x9
    {CODE_9_DOWN,CODE_LEFT_PARENTHESIS_DOWN},//0xa
    {CODE_0_DOWN,CODE_RIGHT_PARENTHESIS_DOWN},//0xb
    {CODE_SUB_DOWN,CODE_UNDERSCORE_DOWN},//0xc
    {CODE_EQU_DOWN,CODE_ADD_DOWN},//0xd
    {CODE_BACKSPACE_DOWN,CODE_BACKSPACE_DOWN},//0xe
    {CODE_TAB_DOWN,CODE_TAB_DOWN},//0xf
    {CODE_Q_DOWN,CODE_Q_DOWN},//0x10
    {CODE_W_DOWN,CODE_W_DOWN},//0x11
    {CODE_E_DOWN,CODE_E_DOWN},//0x12
    {CODE_R_DOWN,CODE_R_DOWN},//0x13
    {CODE_T_DOWN,CODE_T_DOWN},//0x14
    {CODE_Y_DOWN,CODE_Y_DOWN},//0x15
    {CODE_U_DOWN,CODE_U_DOWN},//0x16
    {CODE_I_DOWN,CODE_I_DOWN},//0x17
    {CODE_O_DOWN,CODE_O_DOWN},//0x18
    {CODE_P_DOWN,CODE_P_DOWN},//0x19
    {CODE_LEFT_SQUARE_BRACKET_DOWN,CODE_LEFT_CURLY_BRACE_DOWN},//0x1a
    {CODE_RIGHT_SQUARE_BRACKET_DOWN,CODE_RIGHT_CURLY_BRACE_DOWN},//0x1b
    {CODE_ENTER_DOWN,CODE_ENTER_DOWN},//0x1c
    {CODE_CTRL_DOWN,CODE_CTRL_DOWN},//0x1d
    {CODE_A_DOWN,CODE_A_DOWN},//0x1e
    {CODE_S_DOWN,CODE_S_DOWN},//0x1f
    {CODE_D_DOWN,CODE_D_DOWN},//0x20
    {CODE_F_DOWN,CODE_F_DOWN},//0x21
    {CODE_G_DOWN,CODE_G_DOWN},//0x22
    {CODE_H_DOWN,CODE_H_DOWN},//0x23
    {CODE_J_DOWN,CODE_J_DOWN},//0x24
    {CODE_K_DOWN,CODE_K_DOWN},//0x25
    {CODE_L_DOWN,CODE_L_DOWN},//0x26
    {CODE_SEMICOLON_DOWN,CODE_COLON_DOWN},//0x27
    {CODE_SINGLE_QUOTATION_DOWN,CODE_DOUBLE_QUOTATION_DOWN},//0x28
    {CODE_BACKTICK_DOWN,CODE_TILDE_DOWN},//0x29
    {CODE_SHIFT_DOWN,CODE_SHIFT_DOWN},//0x2a
    {CODE_BACKSLASH_DOWN,CODE_OR_DOWN},//0x2b
    {CODE_Z_DOWN,CODE_Z_DOWN},//0x2c
    {CODE_X_DOWN,CODE_X_DOWN},//0x2d
    {CODE_C_DOWN,CODE_C_DOWN},//0x2e
    {CODE_V_DOWN,CODE_V_DOWN},//0x2f
    {CODE_B_DOWN,CODE_B_DOWN},//0x30
    {CODE_N_DOWN,CODE_N_DOWN},//0x31
    {CODE_M_DOWN,CODE_M_DOWN},//0x32
    {CODE_COMMA_DOWN,CODE_LESS_DOWN},//0x33
    {CODE_DOT_DOWN,CODE_GREAT_DOWN},//0x34
    {CODE_FORWARD_SLASH_DOWN,CODE_QUESTION_DOWN},//0x35
    {CODE_SHIFT_DOWN,CODE_SHIFT_DOWN},//0x36
    {-1,-1},//0x37
    {-1,-1},//0x38
    {CODE_SPACE_DOWN,CODE_SPACE_DOWN},//0x39
    {CODE_CAPS_LOCK_DOWN,CODE_CAPS_LOCK_DOWN},//0x3a
    {-1,-1},//0x3b
    {-1,-1},//0x3c
    {-1,-1},//0x3d
    {-1,-1},//0x3e
    {-1,-1},//0x3f
    {-1,-1},//0x40
    {-1,-1},//0x41
    {-1,-1},//0x42
    {-1,-1},//0x43
    {-1,-1},//0x44
    {-1,-1},//0x45
    {-1,-1},//0x46
    {-1,-1},//0x47
    {-1,-1},//0x48
    {-1,-1},//0x49
    {-1,-1},//0x4a
    {-1,-1},//0x4b
    {-1,-1},//0x4c
    {-1,-1},//0x4d
    {-1,-1},//0x4e
    {-1,-1},//0x4f
    {-1,-1},//0x50
    {-1,-1},//0x51
    {-1,-1},//0x52
    {-1,-1},//0x53
    {-1,-1},//0x54
    {-1,-1},//0x55
    {-1,-1},//0x56
    {-1,-1},//0x57
    {-1,-1},//0x58
    {-1,-1},//0x59
    {-1,-1},//0x5a
    {-1,-1},//0x5b
    {-1,-1},//0x5c
    {-1,-1},//0x5d
    {-1,-1},//0x5e
    {-1,-1},//0x5f
    {-1,-1},//0x60
    {-1,-1},//0x61
    {-1,-1},//0x62
    {-1,-1},//0x63
    {-1,-1},//0x64
    {-1,-1},//0x65
    {-1,-1},//0x66
    {-1,-1},//0x67
    {-1,-1},//0x68
    {-1,-1},//0x69
    {-1,-1},//0x6a
    {-1,-1},//0x6b
    {-1,-1},//0x6c
    {-1,-1},//0x6d
    {-1,-1},//0x6e
    {-1,-1},//0x6f
    {-1,-1},//0x70
    {-1,-1},//0x71
    {-1,-1},//0x72
    {-1,-1},//0x73
    {-1,-1},//0x74
    {-1,-1},//0x75
    {-1,-1},//0x76
    {-1,-1},//0x77
    {-1,-1},//0x78
    {-1,-1},//0x79
    {-1,-1},//0x7a
    {-1,-1},//0x7b
    {-1,-1},//0x7c
    {-1,-1},//0x7d
    {-1,-1},//0x7e
    {-1,-1},//0x7f
    {-1,-1},//0x80
    {CODE_ESC_UP,CODE_ESC_UP},//0x81
    {CODE_1_UP,CODE_BANG_UP},//0x82
    {CODE_2_UP,CODE_AT_UP},//0x83
    {CODE_3_UP,CODE_HASH_UP},//0x84
    {CODE_4_UP,CODE_DOLLAR_UP},//0x85
    {CODE_5_UP,CODE_MOD_UP},//0x86
    {CODE_6_UP,CODE_CARET_UP},//0x87
    {CODE_7_UP,CODE_AND_UP},//0x88
    {CODE_8_UP,CODE_STAR_UP},//0x89
    {CODE_9_UP,CODE_LEFT_PARENTHESIS_UP},//0x8a
    {CODE_0_UP,CODE_RIGHT_PARENTHESIS_UP},//0x8b
    {CODE_SUB_UP,CODE_UNDERSCORE_UP},//0x8c
    {CODE_EQU_UP,CODE_ADD_UP},//0x8d
    {CODE_BACKSPACE_UP,CODE_BACKSPACE_UP},//0x8e
    {CODE_TAB_UP,CODE_TAB_UP},//0x8f
    {CODE_Q_UP,CODE_Q_UP},//0x90
    {CODE_W_UP,CODE_W_UP},//0x91
    {CODE_E_UP,CODE_E_UP},//0x92
    {CODE_R_UP,CODE_R_UP},//0x93
    {CODE_T_UP,CODE_T_UP},//0x94
    {CODE_Y_UP,CODE_Y_UP},//0x95
    {CODE_U_UP,CODE_U_UP},//0x96
    {CODE_I_UP,CODE_I_UP},//0x97
    {CODE_O_UP,CODE_O_UP},//0x98
    {CODE_P_UP,CODE_P_UP},//0x99
    {CODE_LEFT_SQUARE_BRACKET_UP,CODE_LEFT_CURLY_BRACE_UP},//0x9a
    {CODE_RIGHT_SQUARE_BRACKET_UP,CODE_RIGHT_CURLY_BRACE_UP},//0x9b
    {CODE_ENTER_UP,CODE_ENTER_UP},//0x9c
    {CODE_CTRL_UP,CODE_CTRL_UP},//0x9d
    {CODE_A_UP,CODE_A_UP},//0x9e
    {CODE_S_UP,CODE_S_UP},//0x9f
    {CODE_D_UP,CODE_D_UP},//0xa0
    {CODE_F_UP,CODE_F_UP},//0xa1
    {CODE_G_UP,CODE_G_UP},//0xa2
    {CODE_H_UP,CODE_H_UP},//0xa3
    {CODE_J_UP,CODE_J_UP},//0xa4
    {CODE_K_UP,CODE_K_UP},//0xa5
    {CODE_L_UP,CODE_L_UP},//0xa6
    {CODE_SEMICOLON_UP,CODE_COLON_UP},//0xa7
    {CODE_SINGLE_QUOTATION_UP,CODE_DOUBLE_QUOTATION_UP},//0xa8
    {CODE_BACKTICK_UP,CODE_TILDE_UP},//0xa9
    {CODE_SHIFT_UP,CODE_SHIFT_UP},//0xaa
    {CODE_BACKSLASH_UP,CODE_OR_UP},//0xab
    {CODE_Z_UP,CODE_Z_UP},//0xac
    {CODE_X_UP,CODE_X_UP},//0xad
    {CODE_C_UP,CODE_C_UP},//0xae
    {CODE_V_UP,CODE_V_UP},//0xaf
    {CODE_B_UP,CODE_B_UP},//0xb0
    {CODE_N_UP,CODE_N_UP},//0xb1
    {CODE_M_UP,CODE_M_UP},//0xb2
    {CODE_COMMA_UP,CODE_LESS_UP},//0xb3
    {CODE_DOT_UP,CODE_GREAT_UP},//0xb4
    {CODE_FORWARD_SLASH_UP,CODE_QUESTION_UP},//0xb5
    {CODE_SHIFT_UP,CODE_SHIFT_UP},//0xb6
    {-1,-1},//0xb7
    {-1,-1},//0xb8
    {CODE_SPACE_UP,CODE_SPACE_UP},//0xb9
    {CODE_CAPS_LOCK_UP,CODE_CAPS_LOCK_UP},//0xba
    {-1,-1},//0xbb
    {-1,-1},//0xbc
    {-1,-1},//0xbd
    {-1,-1},//0xbe
    {-1,-1},//0xbf
    {-1,-1},//0xc0
    {-1,-1},//0xc1
    {-1,-1},//0xc2
    {-1,-1},//0xc3
    {-1,-1},//0xc4
    {-1,-1},//0xc5
    {-1,-1},//0xc6
    {-1,-1},//0xc7
    {-1,-1},//0xc8
    {-1,-1},//0xc9
    {-1,-1},//0xca
    {-1,-1},//0xcb
    {-1,-1},//0xcc
    {-1,-1},//0xcd
    {-1,-1},//0xce
    {-1,-1},//0xcf
    {-1,-1},//0xd0
    {-1,-1},//0xd1
    {-1,-1},//0xd2
    {-1,-1},//0xd3
    {-1,-1},//0xd4
    {-1,-1},//0xd5
    {-1,-1},//0xd6
    {-1,-1},//0xd7
    {-1,-1},//0xd8
    {-1,-1},//0xd9
    {-1,-1},//0xda
    {-1,-1},//0xdb
    {-1,-1},//0xdc
    {-1,-1},//0xdd
    {-1,-1},//0xde
    {-1,-1},//0xdf
    {-1,-1},//0xe0
    {-1,-1},//0xe1
    {-1,-1},//0xe2
    {-1,-1},//0xe3
    {-1,-1},//0xe4
    {-1,-1},//0xe5
    {-1,-1},//0xe6
    {-1,-1},//0xe7
    {-1,-1},//0xe8
    {-1,-1},//0xe9
    {-1,-1},//0xea
    {-1,-1},//0xeb
    {-1,-1},//0xec
    {-1,-1},//0xed
    {-1,-1},//0xee
    {-1,-1},//0xef
    {-1,-1},//0xf0
    {-1,-1},//0xf1
    {-1,-1},//0xf2
    {-1,-1},//0xf3
    {-1,-1},//0xf4
    {-1,-1},//0xf5
    {-1,-1},//0xf6
    {-1,-1},//0xf7
    {-1,-1},//0xf8
    {-1,-1},//0xf9
    {-1,-1},//0xfa
    {-1,-1},//0xfb
    {-1,-1},//0xfc
    {-1,-1},//0xfd
    {-1,-1},//0xfe
    {-1,-1},//0xff
};
///////////////////////////////////////
void KeyBoardInit(){
    put_str("keyboard init start!\n");
    memset(KeyBoardQueue,-1,sizeof(KeyBoardQueue));//
    ioQueueInit(&KeyboardQueue);//
    put_str("keyboard init done!\n");
}
void keyboard_interrupt(uint8_t code)
{
    static bool flag=0;
    uint16_t decode=-1;
    uint8_t scancode=inB(KBD_PORT);
    if(scancode!=0xe0){//如果不是扩展字符的开头
        if(flag){
            flag=0;
            //处理扩展字
            if(scancode==0x46)decode=CODE_L_DOWN;
            else if(scancode==0xc6)decode=CODE_L_UP;
            else if(scancode==0x4d)decode=CODE_R_DOWN;
            else if(scancode==0xcd)decode=CODE_R_UP;
            else if(scancode==0x48)decode=CODE_UP_DOWN;
            else if(scancode==0xc8)decode=CODE_UP_UP;
            else if(scancode==0x50)decode=CODE_DOWN_DOWN;
            else if(scancode==0xd0)decode=CODE_DOWN_UP;
        }else{
            //直接从表中找
            decode=ScanCodeConvertTable[scancode][KeyBoardShift];
        }
    }else{
        flag=1;
    }
    if(decode==(uint16_t)-1)return;//如果键入码为-1不处理
    //对shift进行处理
    if(decode==CODE_SHIFT_DOWN)KeyBoardShift=1;
    else if(decode==CODE_SHIFT_UP)KeyBoardShift=0;
    //对CapsLock处理
    if(decode==CODE_CAPS_LOCK_DOWN)KeyBoardCapsLock^=1;
    //对ctrl进行处理
    if(decode==CODE_CTRL_DOWN)KeyBoardCtrl=1;
    else if(decode==CODE_CTRL_UP)KeyBoardCtrl=0;
    //如果shift摁下,那就转大写
    if(KeyBoardShift||KeyBoardCapsLock){
        if(CODE_A_DOWN<=decode&&decode<=CODE_Z_DOWN)decode|=32768;//最高位做一个标志
    }
    //存入队列
    KeyBoardQueue[KeyBoardQueueIdx]=decode;
    KeyBoardQueueIdx=(KeyBoardQueueIdx+1)%KeyBoardQueueMaxCnt;//采用循环队列的方式
    //存入io队列
    {
        uint16_t code=KeyBoardFunction(KeyBoardCtrl?CODE_CTRL_DOWN:-1,decode);
        if(!ioQueueFull(&KeyboardQueue)&&code!=(uint16_t)-1){
            //依次放入第8位和高8位
            ioQueuePut(&KeyboardQueue,(uint32_t)code);
        }
    }
    //
}

uint8_t KeyBoardConvertTo(uint16_t code)
{
    //只有摁下才返回，抬起返回-1
    //处理功能按键
    if(IsFunctionKey(code)){
        return code;//因为功能按键对应的键码值都大于128,所以这里就算直接返回也不会与可见ascii码混肴的
    }
    //处理回车
    if(code==CODE_ENTER_DOWN){
        return '\n';
    }
    //处理退格
    if(code==CODE_BACKSPACE_DOWN){
        return '\b';
    }
    //处理字母
    uint16_t codeE=code&32767;
    if(CODE_A_DOWN<=codeE&&codeE<=CODE_Z_DOWN){
        if(code&32768)return codeE-CODE_A_DOWN+'A';
        return code-CODE_A_DOWN+'a';
    }
    //处理数字
    if(CODE_0_DOWN<=code&&code<=CODE_9_DOWN)
        return code-CODE_0_DOWN+'0';
    //处理其他字符
    switch (code)
    {
    case CODE_SPACE_DOWN:return ' ';
    case CODE_TILDE_DOWN:return '~';//~
    case CODE_BACKTICK_DOWN:return '`';//`
    case CODE_BANG_DOWN:return '!';//!
    case CODE_AT_DOWN:return '@';//@
    case CODE_HASH_DOWN:return '#';//#
    case CODE_DOLLAR_DOWN:return '$';//$
    case CODE_MOD_DOWN:return '%';//%
    case CODE_CARET_DOWN:return '^';//^
    case CODE_AND_DOWN:return '&';//&
    case CODE_STAR_DOWN:return '*';//*
    case CODE_LEFT_PARENTHESIS_DOWN:return '(';//(
    case CODE_RIGHT_PARENTHESIS_DOWN:return ')';//)
    case CODE_UNDERSCORE_DOWN:return '_';//_
    case CODE_SUB_DOWN:return '-';//-
    case CODE_ADD_DOWN:return '+';//+
    case CODE_EQU_DOWN:return '=';//=
    case CODE_LEFT_CURLY_BRACE_DOWN:return '{';//{
    case CODE_LEFT_SQUARE_BRACKET_DOWN:return '[';//[
    case CODE_RIGHT_CURLY_BRACE_DOWN:return '}';//}
    case CODE_RIGHT_SQUARE_BRACKET_DOWN:return ']';//]
    case CODE_OR_DOWN:return '|';//|
    case CODE_BACKSLASH_DOWN:return '\\';/* \ */
    case CODE_COLON_DOWN:return ':';//:
    case CODE_SEMICOLON_DOWN:return ';';//;
    case CODE_DOUBLE_QUOTATION_DOWN:return '\"';//"
    case CODE_SINGLE_QUOTATION_DOWN:return '\'';//'
    case CODE_COMMA_DOWN:return ',';//,
    case CODE_LESS_DOWN:return '<';//<
    case CODE_DOT_DOWN:return '.';//.
    case CODE_GREAT_DOWN:return '>';//>
    case CODE_QUESTION_DOWN:return '?';//?
    case CODE_FORWARD_SLASH_DOWN:return '/';// /
    default:
        return -1;
    }
    return -1;
}

uint16_t KeyBoardFunction(uint16_t code0, uint16_t code1)
{
    if(code0==(uint16_t)-1)return code1;
    if(code0==CODE_CTRL_DOWN){
        if(code1==CODE_Z_DOWN)return CODE_CTRL_Z;
        if(code1==CODE_C_DOWN)return CODE_CTRL_C;
        if(code1==CODE_L_DOWN)return CODE_CTRL_L;
        if(code1==CODE_U_DOWN)return CODE_CTRL_U;
    }
    //否则直接返回code1
    return code1;
}

uint16_t KeyBoardGet()
{
    uint32_t key=ioQueueGet(&KeyboardQueue);
    return (uint16_t)key;
}

void KeyBoardRead(void *buf, uint32_t cnt)
{
    char*str=(char*)buf;
    while(cnt){
        uint32_t key=ioQueueGet(&KeyboardQueue);
        uint8_t ch=KeyBoardConvertTo(key);
        //如果读取成功
        if(ch!=(uint8_t)(-1)){
            --cnt;
            *str=ch;
            ++str;
        }
    }
}

bool IsFunctionKey(uint16_t key)
{
    return key>CODE_CTRL_BEGIN&&key<CODE_CTRL_END;
}
