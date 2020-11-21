/*
 * SimpleTimer.c
 *
 * Created: 13.05.2019 18:33:14
 * Author : Mikhail
 */ 
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>

// ���������� ����������
#define SEGMENT_PORT PORTD
#define SEGMENT_DDR  DDRD
#define SEGMENT_A  PIND0
#define SEGMENT_B  PIND1
#define SEGMENT_C  PIND2
#define SEGMENT_D  PIND3
#define SEGMENT_E  PIND4
#define SEGMENT_F  PIND5
#define SEGMENT_G  PIND6
#define SEGMENT_PD PIND7

// ���������� ������������
#define DIGIT_PORT PORTC
#define DIGIT_DDR DDRC
#define DIGIT_0 PINC0
#define DIGIT_1 PINC1
#define DIGIT_2 PINC2
#define DIGIT_3 PINC3

// ���������� ����
#define RELAY_PORT PORTB
#define RELAY_DDR DDRB
#define RELAY PINB1

// ���������� ��������
#define BUZZER_PORT PORTB
#define BUZZER_DDR DDRB
#define BUZZER PINB0

// ���������� ��������
#define KEYBOARD_CONTROL_PORT PORTC
#define KEYBOARD_CONTROL_DDR DDRC
#define KEYBOARD_CONTROL_PIN PINC
#define KEY_START PINC4
#define KEY_STOP PINC5

#define KEYBOARD_SET_PORT PORTB
#define KEYBOARD_SET_DDR DDRB
#define KEYBOARD_SET_PIN PINB
#define KEY_PLUS PINB3
#define KEY_MINUS PINB4

#define MINUS 1
#define PLUS 2
#define START 4
#define STOP 8

// �������� ��� ���������� �������� ��������� (���������� ������)
#define KEY_PAUSE 1000

// ����������� ������ ������� ������ ��� ��������� ������ ���������� ��������
#define LONG_TIME_KEY_DURATION 300000

// ����������� ������ ������� ������ ��� ��������� ������ ���������� ��������
#define SUPER_LONG_TIME_KEY_DURATION 800000

// ����������� ������ ����� ���������� ������� �������� ����� ������� �� ������ ���������� ��������
#define FAST_TIME_SET_PAUSE 10000

// ����������� ������ ����� ���������� ������� �������� ����� ������� �� ������ ���������� ��������
#define SUPER_FAST_TIME_SET_PAUSE 2000


// ����������� ���������� ����� � �������� 59 ��� 59 ��� (59*60+59)
#define MAX_TIME 3599

// ������ ����������� 1 ��� ������������� ����������
#define COMPARE_B_HI 0x3D
#define COMPARE_B_LOW 0x09

// ������ ����������� 1 ��� ��������� ����������
#define COMPARE_A_HI 0x7A
#define COMPARE_A_LOW 0x12

// ������ ����������� 2 ��� ���������� ��� � ���� ms
#define COMPARE_BEEPER 0x08

// ����� eeprom ��� �������� �������������� ����� �������
uint32_t EEMEM lastTimeAddress;

// ������� ������������ ����� � ��������
unsigned long time=0;

// ������� ��������� �����������, ��� ��������� �����
unsigned char separatorState = 1;

// ���� ����, ��� ���� ��������� ��� ����������
unsigned char hideDigit = 0;

// ������� �������� ���������
unsigned char active_digit=0;


// ���������� ��� ������ �������
// ������� ������ �������, �������� ���������� ���������� ������� ��� ����� ����� ��������
unsigned int busser_work=0;
// ���������� ������ �������
unsigned char beep_count = 0;
// ����� ������ �������
unsigned int beep_time = 0;
// ����� ����� �������
unsigned int beep_pause_time = 0;
// ������� ����� �����
unsigned char current_beep_num = 0;
// ������� �������� ������ �������
unsigned char current_beep_iteration_num = 0;
// ������ �� �������, ������� ���� ������� ����� ��������� �������
void (*call_back)(void) = NULL;
// ���������� �������� ������� �������
unsigned char beep_iterations = 0;
// ����� ����� �������� �������
unsigned int beep_iterations_pause = 0;
// ���� ���� ��� ������ ����� ������� ����� ��������
unsigned char iteration_pause_flag = 0;
// ���� ���� ��� ���� ������ ��������� �� ����� ����� ������� ����� ��������
unsigned char hide_digit_on_iteration_pause = 0;

// ������� ��������� ����������, 
// ������ ������� - ������������ �����, 
// �������� �������� ������� - ������� ����� ��� ��������������� ����������
unsigned char segments[11] = {
	_BV(SEGMENT_A) | _BV(SEGMENT_B) | _BV(SEGMENT_C) | _BV(SEGMENT_D) | _BV(SEGMENT_E) | _BV(SEGMENT_F),					// 0
	_BV(SEGMENT_B) | _BV(SEGMENT_C),																						// 1
	_BV(SEGMENT_A) | _BV(SEGMENT_B) | _BV(SEGMENT_G) | _BV(SEGMENT_E) | _BV(SEGMENT_D),										// 2
	_BV(SEGMENT_A) | _BV(SEGMENT_B) | _BV(SEGMENT_G) | _BV(SEGMENT_C) | _BV(SEGMENT_D),										// 3
	_BV(SEGMENT_F) | _BV(SEGMENT_G) | _BV(SEGMENT_B) | _BV(SEGMENT_C),														// 4
	_BV(SEGMENT_A) | _BV(SEGMENT_F) | _BV(SEGMENT_G) | _BV(SEGMENT_C) | _BV(SEGMENT_D),										// 5
	_BV(SEGMENT_A) | _BV(SEGMENT_F) | _BV(SEGMENT_G) | _BV(SEGMENT_C) | _BV(SEGMENT_D) | _BV(SEGMENT_E),					// 6
	_BV(SEGMENT_A) | _BV(SEGMENT_B) | _BV(SEGMENT_C),																		// 7
	_BV(SEGMENT_A) | _BV(SEGMENT_B) | _BV(SEGMENT_C) | _BV(SEGMENT_D) | _BV(SEGMENT_E) | _BV(SEGMENT_F) | _BV(SEGMENT_G),	// 8
	_BV(SEGMENT_A) | _BV(SEGMENT_B) | _BV(SEGMENT_C) | _BV(SEGMENT_D) | _BV(SEGMENT_F) | _BV(SEGMENT_G),					// 9
	_BV(SEGMENT_A) | _BV(SEGMENT_D) | _BV(SEGMENT_E) | _BV(SEGMENT_F) | _BV(SEGMENT_G)										// E
	};

// ������ ��������� �������
// count - ���������� ��������
// beepTime - ����� �������� � ms
// pauseTime - ����� ����� ����� ���������
// callBack - ������ �� �������, ������� ���� ������� ����� ���� ��� ��������� ������
// iterations - ���������� �������� (�������) ������� 
// iterationPause - ����� ����� �������� �������
// hideDigitOnIterationPause - ���� ���� ��� ���� ��������� ��������� �� ����� ����� ����� ��������
void beep(unsigned char count, 
			unsigned int beepTime, 
			unsigned int pauseTime, 
			void (*callBack)(void), 
			unsigned char iterations, 
			unsigned int iterationPause, 
			unsigned char hideDigitOnIterationPause){
	// ������������� ��������� ������ ������� � ������� �� ������
	beep_count = count;
	beep_time = beepTime;
	beep_pause_time = pauseTime;
	current_beep_num = 1;
	current_beep_iteration_num = 1;
	call_back = callBack;
	beep_iterations = iterations;
	beep_iterations_pause = iterationPause;
	hide_digit_on_iteration_pause= hideDigitOnIterationPause;

	// ����� ������� � ��������� �� 1024, ��������� � ������������ ���������� ������� ���������� ~ 100��, ��� ������ ����� ������������ ~ 1ms
	TCCR2 |= _BV(CS22) | _BV(CS21) | _BV(CS20); 
	
	// �������� �������
	BUZZER_PORT |= _BV(BUZZER);
}

// ������ �������
void startTimer(){
	// ����� ������� � ��������� �� 256
	TCCR1B |= _BV(CS12); 
	
	// �������� ����
	RELAY_PORT |= _BV(RELAY);
	
	// ���������� ������� ��������� eeprom
	unsigned long inErrpromData;
	inErrpromData = eeprom_read_dword(&lastTimeAddress);
	
	// ���������� ������ ���� �������� ����������, ���� ��� ������ eeprom �� �������
	if (inErrpromData != time){
		eeprom_write_dword(&lastTimeAddress, time);
	}
}

// ��������� �������
void stopTimer(){
	// ��������� �������
	TCCR1B &= ~(_BV(CS12)); 

	// ��������� ����
	RELAY_PORT &= ~(_BV(RELAY));
	
	// �������� �����������
	separatorState = 1;	
}

// ���������� �������, ������� ������������� ������� ������
void executeCommand(unsigned char press_key){
	if (press_key & PLUS){
		if (time < MAX_TIME) time++;
	}else if(press_key & MINUS){
		if (time > 0) time--;
	}else if(press_key & START){
		startTimer();
	}else if(press_key & STOP){
		stopTimer();
	}			
}

// C���� ������� � �������� �� eeprom
void resetTimer(){
	time = eeprom_read_dword(&lastTimeAddress);
	if (time > MAX_TIME) time = 0;
}

// ������� �����
int main(void){
	// ���� ��������� � ���� �� �����
	SEGMENT_DDR = 0xFF;
	DIGIT_DDR |= _BV(DIGIT_0) | _BV(DIGIT_1) | _BV(DIGIT_2) | _BV(DIGIT_3);
	
	// ���� ���� �� �����
	RELAY_DDR |= _BV(RELAY);

	// ���� ������� �� �����
	BUZZER_DDR |= _BV(BUZZER);
	
	// ���� ������ �� ����
	KEYBOARD_CONTROL_DDR &= ~(_BV(KEY_START) | _BV(KEY_STOP));
	KEYBOARD_SET_DDR &= ~(_BV(KEY_PLUS) | _BV(KEY_MINUS));
	
	// ������ 0, ������������ ���������
	TCCR0 |= _BV(CS01) | _BV(CS00);	// ������ ������� � ��������� 64
	TIMSK |= _BV(TOIE0);			// ��������� ����������

	// ������ 1, ������ ������������ � ��������� ����������
	TCCR1B |= _BV(WGM12);			// ������������� ����� ��� (����� �� ����������)
	TIMSK |= _BV(OCIE1A);			//������������� ��� ���������� ���������� 1��� �������� �� ���������� � OCR1A(H � L)
	TIMSK |= _BV(OCIE1B);			//������������� ��� ���������� ���������� 1��� �������� �� ���������� � OCR1B(H � L)
	OCR1AH = COMPARE_A_HI; 			// ��������� ����� � ���������� A (������� ����)
	OCR1AL = COMPARE_A_LOW;			// ��������� ����� � ���������� A (������� ����)
	OCR1BH = COMPARE_B_HI;			// ��������� ����� � ���������� B (������� ����)
	OCR1BL = COMPARE_B_LOW;			// ��������� ����� � ���������� B (������� ����)

	// ������ 2, �������
	TCCR2 |= _BV(WGM21);			// ������������� ����� ��� (����� �� ����������)
	TIMSK |= _BV(OCIE2);			// ��������� ����������
	OCR2 = COMPARE_BEEPER;			// ��������� ����� � ����������, ���������� ������� ���������� ~100��
	
	// �������� ����������
	sei();

    // ���������� ��� ������ �� �������� ���������
    unsigned int key_pause=KEY_PAUSE+1;

	// ���������� ��������� ������	
	unsigned char parent_key_status = 0;

	// ���������� ��������� ������	
	unsigned long key_duration = 0;

	// ������� ��� ����� � ���������� ������ ��������� �������� �������	
	unsigned long fast_time_set_pause = 0;

	// ����� ������� �������
	resetTimer();

	// ���� ���������
    while (1){
		// ������ �� ��������, �� ��������� �� ������� ������ ������������ ���������� ������
		if (key_pause > KEY_PAUSE){

			// ���������� ������ ��� ��������� ������
			unsigned char press_key = 0;
			
			// ����� ������ � ���������� ��� ������ � ���������� press_key
			if ((KEYBOARD_SET_PIN & _BV(KEY_PLUS)) == 0x00){		
				press_key = PLUS;
			}else if ((KEYBOARD_SET_PIN & _BV(KEY_MINUS)) == 0x00){
				press_key = MINUS;
			}else if ((KEYBOARD_CONTROL_PIN & _BV(KEY_START)) == 0x00){
				press_key = START;
			}else if ((KEYBOARD_CONTROL_PIN & _BV(KEY_STOP)) == 0x00){
				press_key = STOP;
			}
			
			// ������ ���� ���� ������
			if (press_key){
				// ����� ��������� ������
				if (parent_key_status == 0){
					// ���������� ����� ��� ������ �� ��������
					key_pause = 0;
					
					// ����� ���� ��������� ����
					beep(1, 100, 0, NULL, 0, 0, 0);
					
					// ��������� �������
					executeCommand(press_key);
					
				}else{
					// �������� ���� �������� ��� ������� ������, ����������� �� ���� �� ���������	
					if (key_duration > SUPER_LONG_TIME_KEY_DURATION){
						// ����� ����� ������ ������ �������� ������ ���������� ��������
						if (fast_time_set_pause > SUPER_FAST_TIME_SET_PAUSE){
							fast_time_set_pause = 0;
							// ��������� �������
							if (press_key & PLUS || press_key & MINUS){
								executeCommand(press_key);
							}						
						}else{
							fast_time_set_pause ++;
						}
					}else if (key_duration > LONG_TIME_KEY_DURATION){
						// ������ ������ �����, �������� ������ ��������� ��������
						if (fast_time_set_pause > FAST_TIME_SET_PAUSE){
							fast_time_set_pause = 0;
							// ��������� �������
							if (press_key & PLUS || press_key & MINUS){
								executeCommand(press_key);
							}						
						}else{
							fast_time_set_pause ++;
						}
						key_duration++;
					}else{
						key_duration++;
					}
				}
				parent_key_status = 1;
			}else{
				// ����� ��������� ������
				if (parent_key_status){
					// ���������� ����� ��� ������ �� ��������
					key_pause = 0;
				}

				parent_key_status = 0;
				key_duration = 0;
			}
		}else{
			key_pause++;
		}
    }
}

// ������� ���������� ��� ���� �������. �������� ���� ������ ���� ms ��� �������� �������
ISR (TIMER2_COMP_vect){
	// ���������� ������� ����������� ����������� ������� � ����� ����� ��������
	busser_work++;
	
	if (BUZZER_PORT & _BV(BUZZER)){
		// ���� ����� ����������� �������� �������
		if (busser_work > beep_time){
			
			// ����� ������� �����������, ��������� ����
			BUZZER_PORT &= ~(_BV(BUZZER));
			
			// ���������� ������� �������
			busser_work = 0;
			
			// ��������� ���� �� ��� ��� ������
			if (current_beep_num < beep_count){
				// ��� ���� ���������, ������ �� �����
				current_beep_num++;
			}else{
				// �������� �����������, ��������� ���� �� ���������
				if (current_beep_iteration_num < beep_iterations){
					current_beep_num = 1;
					current_beep_iteration_num++;
					iteration_pause_flag = 1;
					
					// ����� ��������� ���� ������� ���� ��� ���� ������ ��������� � ������
					if (hide_digit_on_iteration_pause){
						hideDigit = 1;
					}
				}else{
					// ����������, ��������� ������, ������� ���������
					TCCR2 &= ~(_BV(CS22) | _BV(CS21) | _BV(CS20));

					// ����� ������� ����� �������				
					if (call_back != NULL){
						call_back();
					}
				}
			}
		}
	}else{
		// ���� ������ ����������� �������� �����
		if ((busser_work > beep_pause_time && iteration_pause_flag == 0) 
			|| (busser_work > beep_iterations_pause && iteration_pause_flag == 1)){
			
			// ����� ����� ��� ��� ����� ����� �������� � �������� ���������
			if (iteration_pause_flag){
				iteration_pause_flag = 0;
				hideDigit = 0;
			}
			
			// ����� ��������� �������� ����
			BUZZER_PORT |= _BV(BUZZER);
			
			// ���������� ������� ����������
			busser_work = 0;
		}
	}	
}

// ���������� ������� 1, ������������� ��������� ��� ���������� �������
ISR (TIMER1_COMPB_vect){
	// ����� �����������
	separatorState = 0;		
}

// ���������� ������� 1, ��������� ��������� ��� ���������� �������
ISR (TIMER1_COMPA_vect){
	// ������ �������, ��������� �����
	if (time > 0) time--;
	
	// �������� �����������
	separatorState = 1;		
	
	// �������� �� �� ��� ����� �����������
	if (time == 0){
		// ��������� �������
		stopTimer();
		
		// �������� ������ 3 ���� �� ��� ����
		beep(3, 100, 50, resetTimer, 3, 600, 1);
	}
}

// ���������� ������� 0, ������������ ���������
ISR (TIMER0_OVF_vect){
		
	// ��������� ��������� �������� ���������
	unsigned char next_digit = active_digit; 
	next_digit++;
	next_digit = next_digit % 4;

	// ��������� �������� time. �� ����� ���������� �������� �� 0 �� MAX_TIME
	if (time > MAX_TIME) time=MAX_TIME;

	// ��������� ������ � �������
	unsigned int min=0, sec=0;
	min=time / 60;
	sec=time % 60;

	// ������� ����� �����������
	unsigned char separatorSegment = 0;
	
	// ��������� �������� ���������� ����������
	unsigned int value = 0;
	if (next_digit == 0){
		value = min / 10;
	}else if(next_digit == 1){
		value = min % 10;
		// ������������� ������� ����� ���������� �� ��������� ���� �� �������
		if (separatorState) separatorSegment = _BV(SEGMENT_PD);
	}else if(next_digit == 2){
		value = sec / 10;
	}else{
		value = sec % 10;
	}

	// ��������� ������� ���������
	DIGIT_PORT &= ~(_BV(active_digit));
		
	// ������������� �������� � ��������� � ��������� ���������
	SEGMENT_PORT = separatorSegment | segments[value];
	
	// �������� ��������� ���� �� ������� ���� �������� ��� ����������
	if (hideDigit == 0){
		DIGIT_PORT |= _BV(next_digit);
	}
	
	// ���������� ����� �������� ���������
	active_digit = next_digit;
}