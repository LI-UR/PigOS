/*keyboard_mouse*/

#include "bootpack.h"

/*键盘缓冲区*/
struct FIFO *keyfifo;
int keydata;
/*鼠标缓冲区*/
struct FIFO *mousefifo;
int mousedata;

//keyboard
void wait_KBC_sendready(){
	for(;;){
		if((io_in8(PORT_KEYSTA) & KEYSTA_SEND_NOTREADY) == 0){
			break;
		}
	}
	return;
}

void init_keyboard(struct FIFO *fifo, int data){
	keyfifo = fifo;
	keydata = data;
	//键盘控制器初始化
	wait_KBC_sendready();
	io_out8(PORT_KEYCMD, KEYCMD_WRITE_MODE); //set mode
	wait_KBC_sendready();
	io_out8(PORT_KEYDAT, KBC_MODE); //use mouse mode
	
	return;
}
 /*键盘中断处理程序(0x21号)*/
void inthandler21(int *esp){
	int data;
	io_out8(PIC0_OCW, 0x61); //通知PIC0 IRQ-01已经处理完毕
	data = io_in8(PORT_KEYDAT);	//从键盘读入信息
	fifo_put(keyfifo, data + keydata);
	return;
}
 
 //mouse
 void enable_mouse(struct FIFO *fifo, int data, struct MOUSE_DEC *mdec){
	mousefifo = fifo;
	mousedata = data;
	//鼠标有效
	wait_KBC_sendready();
	io_out8(PORT_KEYCMD, KEYCMD_SENDTO_MOUSE);
	wait_KBC_sendready();
	io_out8(PORT_KEYDAT, MOUSECMD_ENABLE);
	//顺利的话，ACK(0xfa)会被发送
	mdec->mouse_phase = 0; 
	return;
}

int mouse_decode(struct MOUSE_DEC *mdec, unsigned char data){
	if(mdec->mouse_phase == 0){
		if(data == 0xfa)//等待鼠标的0xfa状态
			mdec->mouse_phase = 1;
		return 0;
	}
	if(mdec->mouse_phase == 1){
		mdec->mouse_buf[0] = data;
		mdec->mouse_phase = 2;
		return 0;
	}
	if(mdec->mouse_phase == 2){
		mdec->mouse_buf[1] = data;
		mdec->mouse_phase = 3;
		return 0;
	}
	if(mdec->mouse_phase == 3){
		mdec->mouse_buf[2] = data;
		mdec->mouse_phase = 1;
		mdec->btn = mdec->mouse_buf[0] & 0x07;
		mdec->x = (char)mdec->mouse_buf[1]; //转化为带符号类型
		mdec->y = (char)mdec->mouse_buf[2];
		mdec->y = - mdec->y;
		return 1;
	}
	return -1;
}

/*鼠标中断处理程序(0x2c号)*/
void inthandler2c(int *esp){
	int data;
	io_out8(PIC1_OCW, 0x64); //通知PIC1 IRQ-01已经处理完毕
	io_out8(PIC0_OCW, 0x62); //通知PIC0 IRQ-02已经处理完毕
	data = io_in8(PORT_KEYDAT);
	fifo_put(mousefifo, data + mousedata);
	return;
}