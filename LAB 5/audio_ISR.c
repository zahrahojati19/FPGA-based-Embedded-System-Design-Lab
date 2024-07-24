#include "global.h"

/* globals used for audio record/playback */
extern volatile int buf_index_play;
extern volatile unsigned int note_buf[NOTE_SIZE];					// audio buffer
extern volatile unsigned int play_buf[PLAY_SIZE];
extern volatile int KEY_value;

/***************************************************************************************
 * Audio - Interrupt Service Routine
 *
 * This interrupt service routine records or plays back audio, depending on which type
 * interrupt (read or write) is pending in the audio device.
****************************************************************************************/

void audio_ISR(struct alt_up_dev *up_dev, unsigned int id)
{
//	printf("Alo\n");
	int num_written;
//	unsigned int* red_led_base = (unsigned int*) RED_LEDS_BASE;
	if (alt_up_audio_write_interrupt_pending(up_dev->audio_dev))	// check for write interrupt
	{
//		printf("Hey\n");
		alt_up_parallel_port_write_data (up_dev->green_LEDs_dev, 0x40); // set LEDG[6] on

		// output data until the buffer is empty
		if (buf_index_play < NOTE_SIZE && KEY_value == 0x2)
		{
//			printf("Hey2\n");
//			alt_up_parallel_port_write_data (up_dev->green_LEDs_dev, 0x40); // set LEDG[6] on
			*((int *) GREEN_LEDS_BASE) = 1;

			num_written = alt_up_audio_play_r (up_dev->audio_dev, &(note_buf[buf_index_play]),
			 	NOTE_SIZE - buf_index_play);
			/* assume that we can write the same # words to the left and right */
			(void) alt_up_audio_play_l (up_dev->audio_dev, &(note_buf[buf_index_play]),
				num_written);
			buf_index_play += num_written;
//			printf("buf_index_play: %d\n", buf_index_play);
			if (buf_index_play == NOTE_SIZE)
			{
				// done playback
//				printf("done2\n");
//				alt_up_parallel_port_write_data (up_dev->green_LEDs_dev, 0); // turn off LEDG[6]
				*((int *) GREEN_LEDS_BASE) = 0;
				alt_up_audio_disable_write_interrupt(up_dev->audio_dev);
                KEY_value = 0x0;
			}
		}
		else if (buf_index_play < PLAY_SIZE && KEY_value == 0x8)
		{
			*((int *) GREEN_LEDS_BASE) = 1;
			num_written = alt_up_audio_play_r (up_dev->audio_dev, &(play_buf[buf_index_play]),
			 	PLAY_SIZE - buf_index_play);
			(void) alt_up_audio_play_l (up_dev->audio_dev, &(play_buf[buf_index_play]),
				num_written);
			buf_index_play += num_written;
			if (buf_index_play == PLAY_SIZE)
			{
				*((int *) GREEN_LEDS_BASE) = 0;
				alt_up_audio_disable_write_interrupt(up_dev->audio_dev);
                KEY_value = 0x0;
			}
		}
	}
	return;
}
