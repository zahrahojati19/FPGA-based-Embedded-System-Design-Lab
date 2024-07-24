/* Standard includes. */
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

/* Scheduler includes. */
#include "./FreeRTOS/include/FreeRTOS.h"
#include "./FreeRTOS/include/task.h"
#include "./FreeRTOS/include/queue.h"
#include "./FreeRTOS/include/timers.h"
#include "./FreeRTOS/include/semphr.h"
#include "./FreeRTOS/include/event_groups.h"

/*custom includes*/
#include "sys/alt_stdio.h"
#include "global.h"

/*-----------------------------------------------------------*/

#define mainQUEUE_SEND_TASK_PRIORITY        ( tskIDLE_PRIORITY + 1 )
#define mainQUEUE_RECEIVE_TASK_PRIORITY     ( tskIDLE_PRIORITY + 1 )
#define mainLED_BLINK_TASK_PRIORITY       	( tskIDLE_PRIORITY + 1 )
#define mainKEY_ACTION_TASK_PRIORITY		( tskIDLE_PRIORITY + 1 )
#define mainKEY_READ_TASK_PRIORITY			( tskIDLE_PRIORITY + 1 )
#define mainMOUSE_PACKET_TASK_PRIORITY		( tskIDLE_PRIORITY + 1 )
#define mainVGA_DISPLAY_TASK_PRIORITY		( tskIDLE_PRIORITY + 1 )
#define mainAUDIO_PLAY_TASK_PRIORITY		( tskIDLE_PRIORITY + 1 )

#define mainQUEUE_SEND_PERIOD_MS            pdMS_TO_TICKS( 4000 )
#define mainLED_BLINK_PERIOD		     	pdMS_TO_TICKS( 1000 )
#define mainKEY_READ_PERIOD		     		pdMS_TO_TICKS( 10   )
#define mainMOUSE_READ_PERIOD		        pdMS_TO_TICKS( 10   )
#define mainAUDIO_READ_PERIOD		        pdMS_TO_TICKS( 10   )
#define mainQUEUE_LENGTH                    ( 3 )
#define mouseQUEUE_LENGTH                   ( 24 )
#define playQUEUE_LENGTH                   ( 24 )

/*-----------------------------------------------------------*/
/* Idle task control block and stack */
static StaticTask_t Idle_TCB;
static StackType_t Idle_Stack[configMINIMAL_STACK_SIZE];

/*-----------------------------------------------------------*/

static void prvSetupHardware(void);
static void prvQueueReceiveTask(void *pvParameters);
static void prvQueueSendTask(void *pvParameters);
static void prvLedBlink(void *pvParameters);
static void prvKeyAcitonTask(void *pvParameters);
static void prvKeyReadTask(void *pvParameters);
static void prvMouse(void *pvParameters);
static void prvVGA(void *pvParameters);
static void play_audio(void *pvParameters);

/*-----------------------------------------------------------*/

static QueueHandle_t xQueue = NULL;
static QueueHandle_t Mouse_Queue = NULL;
static QueueHandle_t PlayQueue = NULL;

/*-----------------------------------------------------------*/

#define KEY1_EVENT 0x1
#define KEY2_EVENT 0x2
#define KEY3_EVENT 0x4
EventGroupHandle_t xCreatedEventGroupKeys;

/*-----------------------------------------------------------*/

void audio_ISR(void *, unsigned int);

/*-----------------------------------------------------------*/

struct alt_up_dev up_dev;
struct Mouse_packet mouse_packet;

int screen_x = 319, screen_y = 239;
int index_play;
volatile int buf_index_play, KEY_value;
volatile unsigned int play_buf[PLAY_SIZE];
volatile unsigned int note_buf[NOTE_SIZE];
int note_frequency[NOTE_NUM] = {261, 293, 329, 349, 392, 440, 494};
unsigned int notes[NOTE_NUM][NOTE_SIZE];

volatile int mouse_icon[16][8] = {
								  {0,-1,-1,-1,-1,-1,-1,-1},
								  {0,0,-1,-1,-1,-1,-1,-1},
								  {0,1,0,-1,-1,-1,-1,-1},
								  {0,1,1,0,-1,-1,-1,-1},
								  {0,1,1,1,0,-1,-1,-1},
								  {0,1,1,1,1,0,-1,-1},
								  {0,1,1,1,1,1,0,-1},
								  {0,1,1,1,1,0,0,0},
								  {0,1,1,1,0,-1,-1,-1},
								  {0,0,0,1,0,-1,-1,-1},
								  {0,-1,0,1,0,-1,-1,-1},
								  {-1,-1,-1,0,1,0,-1,-1},
								  {-1,-1,-1,0,1,0,-1,-1},
								  {-1,-1,-1,-1,0,1,0,-1},
								  {-1,-1,-1,-1,0,1,0,-1},
								  {-1,-1,-1,-1,-1,0,-1,-1}
								 };

/*-----------------------------------------------------------*/

void fix_overflow(int, int, int*, int*);
void draw_mouse(alt_up_pixel_buffer_dma_dev *, int, int);
// void erase_mouse(alt_up_pixel_buffer_dma_dev *, int, int, int, int, int, int, short, short);
void draw_note_bottons(alt_up_pixel_buffer_dma_dev *, alt_up_char_buffer_dev *);
int find_box(int, int);
int redraw_box(alt_up_pixel_buffer_dma_dev *, int, int, int);
void generate_notes();

/*----------------------------------------------------------*/
int main(void)
{
	prvSetupHardware();
	alt_irq_register (6, (void *) &up_dev, (void *) audio_ISR);
	generate_notes();

	xQueue = xQueueCreate(mainQUEUE_LENGTH, sizeof(uint32_t));
	Mouse_Queue = xQueueCreate(mouseQUEUE_LENGTH, sizeof(struct Mouse_packet));
	PlayQueue = xQueueCreate(playQUEUE_LENGTH, sizeof(int));

	xTaskCreate(prvQueueReceiveTask, "Rx", configMINIMAL_STACK_SIZE, NULL,
			mainQUEUE_RECEIVE_TASK_PRIORITY, NULL);

	xTaskCreate(prvQueueSendTask, "TX", configMINIMAL_STACK_SIZE, NULL,
			mainQUEUE_SEND_TASK_PRIORITY, NULL);

	xTaskCreate(prvMouse, "MOUSE", configMINIMAL_STACK_SIZE, NULL,
			mainMOUSE_PACKET_TASK_PRIORITY, NULL);

	xTaskCreate(prvVGA, "VGA", configMINIMAL_STACK_SIZE, NULL,
			mainVGA_DISPLAY_TASK_PRIORITY, NULL);

	xTaskCreate(play_audio, "AUDIO", configMINIMAL_STACK_SIZE, NULL,
			mainAUDIO_PLAY_TASK_PRIORITY, NULL);

	xCreatedEventGroupKeys = xEventGroupCreate();
	if (xCreatedEventGroupKeys == NULL) 
	{
		printf("unable to allocate memory to event group\n");
		return 0;
	}
	vTaskStartScheduler();

	for (;;)
		;
}
/*-----------------------------------------------------------*/

static void prvLedBlink(void *pvParameters) 
{
	TickType_t xNextWakeTime;
	xNextWakeTime = xTaskGetTickCount();

	unsigned int* red_led_base = (unsigned int*) RED_LEDS_BASE;
	*(red_led_base) = 0;

	for (;;) 
	{
		 vTaskDelayUntil( &xNextWakeTime, mainLED_BLINK_PERIOD);
		 *(red_led_base) = *(red_led_base) ^ 1;
	}
}
/*-----------------------------------------------------------*/

static void prvQueueSendTask(void *pvParameters) 
{
	TickType_t xNextWakeTime;
	uint32_t ulValueToSend = 0;

	/* Initialise xNextWakeTime - this only needs to be done once. */
	xNextWakeTime = xTaskGetTickCount();

	for (;;)
	{
		vTaskDelayUntil( &xNextWakeTime, mainQUEUE_SEND_PERIOD_MS);
//		printf(">>\"%x\" sent  @tick:%x\n", ulValueToSend, xNextWakeTime);
		xQueueSend( xQueue, &ulValueToSend, 0);
		ulValueToSend++;
	}
}

static void prvQueueReceiveTask(void *pvParameters) 
{
	uint32_t ulReceivedValue;

	for (;;) 
	{
		xQueueReceive(xQueue, &ulReceivedValue, portMAX_DELAY);
//		printf("<<\"%x\" received\n", ulReceivedValue);
	}
}

/*-----------------------------------------------------------*/
static void prvKeyAcitonTask(void *pvParameters)
{
	EventBits_t event_bits;
	for (;;) 
	{
		event_bits = xEventGroupWaitBits(xCreatedEventGroupKeys, 6, pdTRUE,
					 pdFALSE, portMAX_DELAY);
		xEventGroupClearBits(xCreatedEventGroupKeys, event_bits);
		switch (event_bits) 
		{
		case KEY1_EVENT:
			printf("key 1 was pressed\n");
			break;
		case KEY2_EVENT:
			printf("key 2 was pressed\n");
			break;
		case KEY3_EVENT:
			printf("key 3 was pressed\n");
			break;
		}
	}
}

static void prvKeyReadTask(void *pvParameters) 
{
	TickType_t xNextWakeTime;
	xNextWakeTime = xTaskGetTickCount();
	unsigned int key_val = 0;
	unsigned int prev_key_val = 0;
	unsigned int* key_base = (unsigned int*) PUSHBUTTONS_BASE;
	for (;;) 
	{
		vTaskDelayUntil( &xNextWakeTime, mainKEY_READ_PERIOD);
		key_val = *(key_base);
		// printf("key val: %x\n", key_val);
		if (key_val != prev_key_val && key_val != 0) 
		{
			switch (key_val) 
			{
			case 2:
				xEventGroupSetBits(xCreatedEventGroupKeys, KEY1_EVENT);
				break;
			case 4:
				xEventGroupSetBits(xCreatedEventGroupKeys, KEY2_EVENT);
				break;
			case 8:
				xEventGroupSetBits(xCreatedEventGroupKeys, KEY3_EVENT);
				break;
			}
		}
		prev_key_val = key_val;
	}
}
/*---------------------------------------------------------------*/
static void prvSetupHardware(void)
{
	alt_up_ps2_dev *PS2_dev;
	alt_up_char_buffer_dev *char_buffer_dev;
	alt_up_pixel_buffer_dma_dev *pixel_buffer_dev;
	alt_up_audio_dev *audio_dev;

	PS2_dev = alt_up_ps2_open_dev ("/dev/PS2_Port");
	if ( PS2_dev == NULL)
	{
		alt_printf ("Error: could not open PS2 device\n");
	}
	else
	{
		alt_printf ("Opened PS2 device\n");
		up_dev.PS2_dev = PS2_dev;
	}
	(void) alt_up_ps2_write_data_byte (PS2_dev, 0xFF);
	alt_up_ps2_enable_read_interrupt (PS2_dev);

	pixel_buffer_dev = alt_up_pixel_buffer_dma_open_dev ("/dev/VGA_Pixel_Buffer");
	if ( pixel_buffer_dev == NULL)
		alt_printf ("Error: could not open pixel buffer device\n");
	else
	{
		alt_printf ("Opened pixel buffer device\n");
		up_dev.pixel_buffer_dev = pixel_buffer_dev;
	}

	char_buffer_dev = alt_up_char_buffer_open_dev ("/dev/VGA_Char_Buffer");
	if ( char_buffer_dev == NULL)
		alt_printf ("Error: could not open character buffer device\n");
	else
	{
		alt_printf ("Opened character buffer device\n");
		up_dev.char_buffer_dev = char_buffer_dev;
	}

	audio_dev = alt_up_audio_open_dev ("/dev/Audio");
	if ( audio_dev == NULL)
		alt_printf ("Error: could not open audio device\n");
	else
	{
		alt_printf ("Opened audio device\n");
		up_dev.audio_dev = audio_dev;
	}

}

void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer,
		StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize) 
{
	*ppxIdleTaskTCBBuffer = &Idle_TCB;
	*ppxIdleTaskStackBuffer = &Idle_Stack[0];
	*pulIdleTaskStackSize = (uint32_t) configMINIMAL_STACK_SIZE;
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char * pcTaskName)  
{
	printf("stack overflow occured\n");
}

static void prvMouse(void *pvParameters) {
	TickType_t xNextWakeTime;
	xNextWakeTime = xTaskGetTickCount();

	unsigned char byte1, byte2, byte3;
	unsigned char PS2_data;
	int flag_ready = 0, packet_ready = 0;
	/* check for PS/2 data--display on HEX displays */
	while(1)
	{
		vTaskDelayUntil( &xNextWakeTime, mainMOUSE_READ_PERIOD);
		while(alt_up_ps2_read_data_byte (up_dev.PS2_dev, &PS2_data) == 0)
		{
			/* allows save the last three bytes of data */
			byte1 = byte2;
			byte2 = byte3;
			byte3 = PS2_data;
			flag_ready++;

			if ( (byte2 == (unsigned char) 0xAA) && (byte3 == (unsigned char) 0x00) )
			{
				// mouse inserted; initialize sending of data
				(void) alt_up_ps2_write_data_byte (up_dev.PS2_dev, (unsigned char) 0xF4);
				flag_ready = 1;
			}

			if(flag_ready == 2)
			{
				mouse_packet.left_click = byte1 & 0x1;
				mouse_packet.delta_x = (byte1 & 0x10) ? (-256 + (int)byte2) : (int)byte2;
				mouse_packet.delta_y = (byte1 & 0x20) ? (-256 + (int)byte3) : (int)byte3;
				xQueueSend(Mouse_Queue, &mouse_packet, 0);
				packet_ready = 1;
			}
			else if(flag_ready == 4)
			{
				flag_ready = 1;
			}
		}
	}
	return;
}

static void prvVGA(void *pvParameters) {
	struct Mouse_packet current_packet;
	short background_color = 0x0000;
	int note_flag = 0, box = 0;
	int center_x = 0, center_y = 0;
	alt_up_pixel_buffer_dma_draw_box (up_dev.pixel_buffer_dev, 0, 0, screen_x, 
									  screen_y, background_color, 0);
	draw_note_bottons(up_dev.pixel_buffer_dev, up_dev.char_buffer_dev);
	while (1)
	{
		xQueueReceive(Mouse_Queue, &current_packet, portMAX_DELAY);
		if (current_packet.left_click == 1 && box < 8 && box > 0)
		{
			KEY_value = 0x4;
			note_flag = box - 1;
			xQueueSend(PlayQueue, &note_flag, 0);
		}
		else if (current_packet.left_click == 1 && box == 8)
		{
			int i;
			index_play = 0;
			for(i = 0; i < PLAY_SIZE; i++)
				play_buf[i] = 0;

		}
		else if (current_packet.left_click == 1 && box == 9)
		{
			KEY_value = 0x6;
		}
		box = redraw_box(up_dev.pixel_buffer_dev, center_x, center_y, box);
		center_x += current_packet.delta_x;
		center_y -= current_packet.delta_y;
		draw_mouse(up_dev.pixel_buffer_dev, center_x, center_y);
		fix_overflow(screen_x, screen_y, &center_x, &center_y);
	}
}

void draw_note_bottons(alt_up_pixel_buffer_dma_dev *pixel_buffer_dev, alt_up_char_buffer_dev * char_buffer_dev)
{
	short box_color = 0xDAA5;
	int current_position = 35;
	int width = 30; int offset = 5;

	char texts[7][10] = {"DO\0", "RE\0", "MI\0", "FA\0", "SOL\0", "LA\0", "SI\0"};
	char rstart_text[20] = "restart\0";
	char play_text[20] = "play\0";

	alt_up_pixel_buffer_dma_draw_box (pixel_buffer_dev, current_position, 30, current_position + width, 60, box_color, 0);
	alt_up_char_buffer_string (char_buffer_dev, texts[i], (current_position + 12)/4, 11);

	alt_up_pixel_buffer_dma_draw_box (pixel_buffer_dev, current_position +(offset + width) , 30, current_position + width, 60, box_color, 0);
	alt_up_char_buffer_string (char_buffer_dev, texts[i], (current_position + 12)/4, 11);

	alt_up_pixel_buffer_dma_draw_box (pixel_buffer_dev, current_position +2*(offset + width), 30, current_position +2*(offset + width) + width, 60, box_color, 0);
	alt_up_char_buffer_string (char_buffer_dev, texts[i], (current_position + 12)/4, 11);

	alt_up_pixel_buffer_dma_draw_box (pixel_buffer_dev, current_position +3*(offset + width), 30, current_position +3*(offset + width) + width, 60, box_color, 0);
	alt_up_char_buffer_string (char_buffer_dev, texts[i], (current_position + 12)/4, 11);

	alt_up_pixel_buffer_dma_draw_box (pixel_buffer_dev, current_position +4*(offset + width), 30, current_position +4*(offset + width) + width, 60, box_color, 0);
	alt_up_char_buffer_string (char_buffer_dev, texts[i], (current_position + 12)/4, 11);

	alt_up_pixel_buffer_dma_draw_box (pixel_buffer_dev, current_position +5*(offset + width), 30, current_position +5*(offset + width) + width, 60, box_color, 0);
	alt_up_char_buffer_string (char_buffer_dev, texts[i], (current_position + 12)/4, 11);

	alt_up_pixel_buffer_dma_draw_box (pixel_buffer_dev, current_position +6*(offset + width), 30, current_position +6*(offset + width) + width, 60, box_color, 0);
	alt_up_char_buffer_string (char_buffer_dev, texts[i], (current_position + 12)/4, 11);

	width = 90;
	alt_up_pixel_buffer_dma_draw_box (pixel_buffer_dev, 50, 100, 140, 130, box_color, 0);
	alt_up_char_buffer_string (char_buffer_dev, rstart_text, (50 + 30)/4, 114/4);

	alt_up_pixel_buffer_dma_draw_box (pixel_buffer_dev, 170, 100, 260, 130, box_color, 0);
	alt_up_char_buffer_string (char_buffer_dev, play_text, (170 + 35)/4, 114/4);
}

void draw_mouse(alt_up_pixel_buffer_dma_dev *pixel_buffer_dev, int center_x, int center_y)
{
	short color;
	int i, j;
	for(i = 0; i < 16; i ++)
	{
		for(j = 0; j < 8; j++)
		{
			if(mouse_icon[i][j] != -1)
			{
				if(mouse_icon[i][j] == 0)
					color = 0x0000;
				else if(mouse_icon[i][j] == 1)
					color = 0xFFFF;
				alt_up_pixel_buffer_dma_draw_box (pixel_buffer_dev, center_x + j, center_y + i, center_x + j, center_y + i, color, 0);
			}
		}
	}
}

// void erase_mouse(alt_up_pixel_buffer_dma_dev *pixel_buffer_dev, int center_x, int center_y, int blue1_x1,
// 				  int blue1_y, int blue1_x2, int blue2_y, short background_color, short color)
// {
// 	alt_up_pixel_buffer_dma_draw_box (pixel_buffer_dev, center_x, center_y, center_x + 7, center_y + 15, background_color, 0);
// }

static void play_audio(void *pvParameters)
{
	TickType_t xNextWakeTime;
	xNextWakeTime = xTaskGetTickCount();
	int i;
	index_play = 0;
	int note_num;
	while(1)
	{
		if(KEY_value == 0x4) {
			xQueueReceive(PlayQueue, &note_num, portMAX_DELAY);
			if(note_num != -1) {
				if(index_play < 10) {
					for(i = 0; i < NOTE_SIZE; i++) {
						note_buf[i] = notes[note_num][i];
						play_buf[i + index_play * NOTE_SIZE] = notes[note_num][i];
					}
					index_play += 1;
				}
			}
			KEY_value = 0x2;
			// reset counter to start playbackُ
			buf_index_play = 0;
			// clear audio FIFOs
			alt_up_audio_reset_audio_core (up_dev.audio_dev);
			// enable audio-out interrupts
			alt_up_audio_enable_write_interrupt (up_dev.audio_dev);
		}
		else if(KEY_value == 0x6) {
			KEY_value = 0x8;
			// reset counter to start playback
			buf_index_play = 0;
			// clear audio FIFOs
			alt_up_audio_reset_audio_core (up_dev.audio_dev);
			// enable audio-out interrupts
			alt_up_audio_enable_write_interrupt (up_dev.audio_dev);
		}
	}
}

int redraw_box(alt_up_pixel_buffer_dma_dev *pixel_buffer_dev, int center_x, int center_y, int old_box_num)
{
	short replaced_color = 0x166;
	short orginal_color = 0xDAA5;
	short background_color = 0x0000;
	int width = 30;
	int offset = 5;
	int box_num;
	box_num = find_box(center_x, center_y);

	alt_up_pixel_buffer_dma_draw_box (pixel_buffer_dev, center_x, center_y, center_x + 7, center_y + 15, background_color, 0);
	if(box_num != 0 && box_num != 9 && box_num != 8)
	{
		alt_up_pixel_buffer_dma_draw_box (pixel_buffer_dev,
			35+(box_num-1)*(offset+width), 30, 65+(box_num-1)*(offset+width), 60, replaced_color, 0);
	}
	else if(box_num == 9 || box_num == 8)
	{
		alt_up_pixel_buffer_dma_draw_box (pixel_buffer_dev,
			50+(box_num-8)*(120), 100, 140+(box_num-8)*(120), 130, replaced_color, 0);
	}
	else if(old_box_num == 9 || old_box_num == 8)
	{
		alt_up_pixel_buffer_dma_draw_box (pixel_buffer_dev,
			50+(old_box_num-8)*(120), 100, 140+(old_box_num-8)*(120), 130, orginal_color, 0);
	}
	else if(old_box_num == 1 || old_box_num == 2 || old_box_num == 3 || old_box_num == 4
			|| old_box_num == 5 || old_box_num == 6 || old_box_num == 7)
	{
		alt_up_pixel_buffer_dma_draw_box (pixel_buffer_dev,
			35+(old_box_num-1)*(offset+width), 30, 65+(old_box_num-1)*(offset+width), 60, orginal_color, 0);
	}
	return box_num;
}

int find_box(int center_x, int center_y)
{
	if((center_x >= 35 && center_x + 7 <= 65) && (center_y >= 30 && center_y + 15 <= 60))
	    return 1;
	else if((center_x >= 70 && center_x + 7 <= 100) && (center_y >= 30 && center_y + 15 <= 60))
		return 2;
	else if((center_x >= 105 && center_x + 7 <= 135) && (center_y >= 30 && center_y + 15 <= 60))
		return 3;
	else if((center_x >= 140 && center_x + 7 <= 170) && (center_y >= 30 && center_y + 15 <= 60))
		return 4;
	else if((center_x >= 175 && center_x + 7 <= 205) && (center_y >= 30 && center_y + 15 <= 60))
		return 5;
	else if((center_x >= 210 && center_x + 7 <= 240) && (center_y >= 30 && center_y + 15 <= 60))
		return 6;
	else if((center_x >= 245 && center_x + 7 <= 275) && (center_y >= 30 && center_y + 15 <= 60))
		return 7;
	else if((center_x >= 50 && center_x + 7 <= 140) && (center_y >= 100 && center_y + 15 <= 130))
		return 8;
	else if((center_x >= 170 && center_x + 7 <= 260) && (center_y >= 100 && center_y + 15 <= 130))
		return 9;
	else
		return 0;
}

void generate_notes()
{
	int i, j;
	for(i = 0; i < NOTE_NUM; i++)
		for(j = 0; j < NOTE_SIZE; j++)
			notes[i][j] = ((int)(sin(2 * PI * note_
		frequency[i] * (j / 48000.0)) * 8388608.0) + 8388608.0) * 256;
}

void fix_overflow(int screen_x, int screen_y, int* center_x, int* center_y)
{
	if(*center_x > screen_x - 5)
		*center_x = screen_x - 5;
	else if (*center_x < 0)
		*center_x = 0;
	if(*center_y > screen_y - 5)
		*center_y = screen_y - 5;
	else if (*center_y < 0)
		*center_y = 0;
}