/*
 * SimpleTimer.c
 *
 * Created: 13.05.2019 18:33:14
 * Author : Mikhail
 */ 
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>

// Управление сегментами
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

// Управление индикаторами
#define DIGIT_PORT PORTC
#define DIGIT_DDR DDRC
#define DIGIT_0 PINC0
#define DIGIT_1 PINC1
#define DIGIT_2 PINC2
#define DIGIT_3 PINC3

// Управление реле
#define RELAY_PORT PORTB
#define RELAY_DDR DDRB
#define RELAY PINB1

// Управление пищалкой
#define BUZZER_PORT PORTB
#define BUZZER_DDR DDRB
#define BUZZER PINB0

// Управление кнопками
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

// Задержка для исключения дребезга контактов (количество циклов)
#define KEY_PAUSE 1000

// Количеситво циклов нажатой кнопки для включения первой ускоренной скорости
#define LONG_TIME_KEY_DURATION 300000

// Количеситво циклов нажатой кнопки для включения второй ускоренной скорости
#define SUPER_LONG_TIME_KEY_DURATION 800000

// Количеситво циклов через прошествие которых меняется время таймера на первой ускоренной скорости
#define FAST_TIME_SET_PAUSE 10000

// Количеситво циклов через прошествие которых меняется время таймера на второй ускоренной скорости
#define SUPER_FAST_TIME_SET_PAUSE 2000


// Максимально допустимое время в секундах 59 мин 59 сек (59*60+59)
#define MAX_TIME 3599

// Данные компаратора 1 для полусекундных интервалов
#define COMPARE_B_HI 0x3D
#define COMPARE_B_LOW 0x09

// Данные компаратора 1 для секундных интервалов
#define COMPARE_A_HI 0x7A
#define COMPARE_A_LOW 0x12

// Данные компаратора 2 для прерываний раз в одну ms
#define COMPARE_BEEPER 0x08

// Адрес eeprom где хранится первоначальное время таймера
uint32_t EEMEM lastTimeAddress;

// Текущее отображаемое время в секундах
unsigned long time=0;

// Текущее состояние разделителя, при включении горит
unsigned char separatorState = 1;

// Флаг того, что надо выключить все индикаторы
unsigned char hideDigit = 0;

// Текущий активный индикатор
unsigned char active_digit=0;


// Переменные для работы пищалки
// Счетчик работы пищалки, отмеряет количество милисекунд пищания или паузы между пищанием
unsigned int busser_work=0;
// Количество циклов пищания
unsigned char beep_count = 0;
// Время работы пищалки
unsigned int beep_time = 0;
// Время паузы пищания
unsigned int beep_pause_time = 0;
// Текущий номер писка
unsigned char current_beep_num = 0;
// Текущая итерация пекета пищания
unsigned char current_beep_iteration_num = 0;
// Ссылка на функцию, которую надо вызвать после окончания пищания
void (*call_back)(void) = NULL;
// Количество итераций пакетов пищания
unsigned char beep_iterations = 0;
// Пауза между пакетами пищания
unsigned int beep_iterations_pause = 0;
// Флаг того что сейчас пауза пищания между пакетами
unsigned char iteration_pause_flag = 0;
// Флаг того что надо гасить индикатор во время паузы пищания между пакетами
unsigned char hide_digit_on_iteration_pause = 0;

// Матрица сегментов индикатора, 
// индекс массива - отображаемое число, 
// значение элемента массива - битовая маска для семисегментного индикатора
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

// Запуск звукового сигнала
// count - количество сигналов
// beepTime - время звучания в ms
// pauseTime - время паузы между сигналами
// callBack - Ссылка на функцию, которую надо вызвать после того как закончили пищать
// iterations - количество итераций (пакетов) пищания 
// iterationPause - Пауза между пакетами пищания
// hideDigitOnIterationPause - Флаг того что надо выключить индикатор во время паузы между пакетами
void beep(unsigned char count, 
			unsigned int beepTime, 
			unsigned int pauseTime, 
			void (*callBack)(void), 
			unsigned char iterations, 
			unsigned int iterationPause, 
			unsigned char hideDigitOnIterationPause){
	// Устанавливаем параметры работы пищалки и текущий ее статус
	beep_count = count;
	beep_time = beepTime;
	beep_pause_time = pauseTime;
	current_beep_num = 1;
	current_beep_iteration_num = 1;
	call_back = callBack;
	beep_iterations = iterations;
	beep_iterations_pause = iterationPause;
	hide_digit_on_iteration_pause= hideDigitOnIterationPause;

	// Старт таймера с делителем на 1024, совместно с компаратором определяет частоту прерываний ~ 100гц, или период между прерываниями ~ 1ms
	TCCR2 |= _BV(CS22) | _BV(CS21) | _BV(CS20); 
	
	// Включаем пищалку
	BUZZER_PORT |= _BV(BUZZER);
}

// Запуск таймера
void startTimer(){
	// Старт таймера с делителем на 256
	TCCR1B |= _BV(CS12); 
	
	// Включаем реле
	RELAY_PORT |= _BV(RELAY);
	
	// Зачитываем текущее состояние eeprom
	unsigned long inErrpromData;
	inErrpromData = eeprom_read_dword(&lastTimeAddress);
	
	// Записываем только если значение изменилось, чтоб зря ресурс eeprom не тратить
	if (inErrpromData != time){
		eeprom_write_dword(&lastTimeAddress, time);
	}
}

// Остановка таймера
void stopTimer(){
	// Остановка таймера
	TCCR1B &= ~(_BV(CS12)); 

	// Выключаем реле
	RELAY_PORT &= ~(_BV(RELAY));
	
	// зажигаем разделитель
	separatorState = 1;	
}

// Выполнение команды, которая соответствует нажатой кнопке
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

// Cборо таймера к значению из eeprom
void resetTimer(){
	time = eeprom_read_dword(&lastTimeAddress);
	if (time > MAX_TIME) time = 0;
}

// входная точка
int main(void){
	// Порт сегментов и цифр на выход
	SEGMENT_DDR = 0xFF;
	DIGIT_DDR |= _BV(DIGIT_0) | _BV(DIGIT_1) | _BV(DIGIT_2) | _BV(DIGIT_3);
	
	// Порт реле на выход
	RELAY_DDR |= _BV(RELAY);

	// Порт пищалки на выход
	BUZZER_DDR |= _BV(BUZZER);
	
	// Порт кнопок на вход
	KEYBOARD_CONTROL_DDR &= ~(_BV(KEY_START) | _BV(KEY_STOP));
	KEYBOARD_SET_DDR &= ~(_BV(KEY_PLUS) | _BV(KEY_MINUS));
	
	// Таймер 0, динамическая индикация
	TCCR0 |= _BV(CS01) | _BV(CS00);	// Запуск таймера с делителем 64
	TIMSK |= _BV(TOIE0);			// Включение прерывания

	// Таймер 1, Отсчет полсекундных и секундных интервалов
	TCCR1B |= _BV(WGM12);			// устанавливаем режим СТС (сброс по совпадению)
	TIMSK |= _BV(OCIE1A);			//устанавливаем бит разрешения прерывания 1ого счетчика по совпадению с OCR1A(H и L)
	TIMSK |= _BV(OCIE1B);			//устанавливаем бит разрешения прерывания 1ого счетчика по совпадению с OCR1B(H и L)
	OCR1AH = COMPARE_A_HI; 			// Установка числа в компаратор A (старший байт)
	OCR1AL = COMPARE_A_LOW;			// Установка числа в компаратор A (младший байт)
	OCR1BH = COMPARE_B_HI;			// Установка числа в компаратор B (старший байт)
	OCR1BL = COMPARE_B_LOW;			// Установка числа в компаратор B (младший байт)

	// Таймер 2, Пищалка
	TCCR2 |= _BV(WGM21);			// устанавливаем режим СТС (сброс по совпадению)
	TIMSK |= _BV(OCIE2);			// Включение прерывания
	OCR2 = COMPARE_BEEPER;			// Установка числа в компаратор, определяет частоту прерываний ~100Гц
	
	// Включаем прирывания
	sei();

    // Переменная для защиты от дребезга контактов
    unsigned int key_pause=KEY_PAUSE+1;

	// Предыдущее состояние кнопки	
	unsigned char parent_key_status = 0;

	// Предыдущее состояние кнопки	
	unsigned long key_duration = 0;

	// счетчик для паузы в ускоренном режиме установки значения таймера	
	unsigned long fast_time_set_pause = 0;

	// Сброс времени таймера
	resetTimer();

	// Цикл программы
    while (1){
		// Защита от дребезга, не реагируем на нажатие кнопок определенное количество циклов
		if (key_pause > KEY_PAUSE){

			// Переменная хранит что конкретно нажато
			unsigned char press_key = 0;
			
			// Опрос кнопок и запоминаем что нажато в переменной press_key
			if ((KEYBOARD_SET_PIN & _BV(KEY_PLUS)) == 0x00){		
				press_key = PLUS;
			}else if ((KEYBOARD_SET_PIN & _BV(KEY_MINUS)) == 0x00){
				press_key = MINUS;
			}else if ((KEYBOARD_CONTROL_PIN & _BV(KEY_START)) == 0x00){
				press_key = START;
			}else if ((KEYBOARD_CONTROL_PIN & _BV(KEY_STOP)) == 0x00){
				press_key = STOP;
			}
			
			// Нажата хоть одна кнопка
			if (press_key){
				// смена состояния кнопки
				if (parent_key_status == 0){
					// Сбрасываем паузу для защиты от дребезга
					key_pause = 0;
					
					// Пищим один корроткий писк
					beep(1, 100, 0, NULL, 0, 0, 0);
					
					// Выполняем команду
					executeCommand(press_key);
					
				}else{
					// Повторно сюда попадаем при нажатой кнопке, высчитываем не надо ли ускорится	
					if (key_duration > SUPER_LONG_TIME_KEY_DURATION){
						// Очень долго нажата кнопка включаем вторую ускоренную скорость
						if (fast_time_set_pause > SUPER_FAST_TIME_SET_PAUSE){
							fast_time_set_pause = 0;
							// Выполняем команду
							if (press_key & PLUS || press_key & MINUS){
								executeCommand(press_key);
							}						
						}else{
							fast_time_set_pause ++;
						}
					}else if (key_duration > LONG_TIME_KEY_DURATION){
						// Кнопка нажата долго, включаем первую ускоенную скорость
						if (fast_time_set_pause > FAST_TIME_SET_PAUSE){
							fast_time_set_pause = 0;
							// Выполняем команду
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
				// смена состояния кнопки
				if (parent_key_status){
					// Сбрасываем паузу для защиты от дребезга
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

// Счетчик милисекунд для нужд пищалки. Попадаем сюда каждую одну ms при активной пищалке
ISR (TIMER2_COMP_vect){
	// Переменная которая отсчитывает милисекунды пищания и паузы между пищанием
	busser_work++;
	
	if (BUZZER_PORT & _BV(BUZZER)){
		// Если пищим отсчитываем интервал пищания
		if (busser_work > beep_time){
			
			// Время пищание закончилось, Выключаем звук
			BUZZER_PORT &= ~(_BV(BUZZER));
			
			// Сбрасываем счетчик милисек
			busser_work = 0;
			
			// Проверяем надо ли еще раз пищать
			if (current_beep_num < beep_count){
				// Еще надо пропищать, уходим на паузу
				current_beep_num++;
			}else{
				// итерация закончилась, проверяем надо ли повторить
				if (current_beep_iteration_num < beep_iterations){
					current_beep_num = 1;
					current_beep_iteration_num++;
					iteration_pause_flag = 1;
					
					// Гасим индикатор если взведен флаг что надо гасить индикатор в паузах
					if (hide_digit_on_iteration_pause){
						hideDigit = 1;
					}
				}else{
					// Отпищались, Отключаем таймер, пищалка замолкает
					TCCR2 &= ~(_BV(CS22) | _BV(CS21) | _BV(CS20));

					// Вызов функции после пищания				
					if (call_back != NULL){
						call_back();
					}
				}
			}
		}
	}else{
		// Если молчим отсчитываем интервал паузы
		if ((busser_work > beep_pause_time && iteration_pause_flag == 0) 
			|| (busser_work > beep_iterations_pause && iteration_pause_flag == 1)){
			
			// Сброс флага что это пауза между пакетами и зажигаем индикатор
			if (iteration_pause_flag){
				iteration_pause_flag = 0;
				hideDigit = 0;
			}
			
			// Пауза закончена включаем звук
			BUZZER_PORT |= _BV(BUZZER);
			
			// Сбрасываем счетчик милисекунд
			busser_work = 0;
		}
	}	
}

// Прерывание таймера 1, полусекундные интервалы при работающем таймере
ISR (TIMER1_COMPB_vect){
	// Гасим разделитель
	separatorState = 0;		
}

// Прерывание таймера 1, секундные интервалы при работающем таймере
ISR (TIMER1_COMPA_vect){
	// Прошла секунда, уменьшаем время
	if (time > 0) time--;
	
	// Зажигаем разделитель
	separatorState = 1;		
	
	// Проверка на то что время закончилось
	if (time == 0){
		// остановка таймера
		stopTimer();
		
		// Звуковой сигнал 3 раза по три раза
		beep(3, 100, 50, resetTimer, 3, 600, 1);
	}
}

// Прерывание таймера 0, динамическая индикация
ISR (TIMER0_OVF_vect){
		
	// Вычисляем следующий активный индикатор
	unsigned char next_digit = active_digit; 
	next_digit++;
	next_digit = next_digit % 4;

	// Проверяем значение time. Мы умеем отображать значения от 0 до MAX_TIME
	if (time > MAX_TIME) time=MAX_TIME;

	// Вычисляем минуты и секунды
	unsigned int min=0, sec=0;
	min=time / 60;
	sec=time % 60;

	// Битовая маска разделителя
	unsigned char separatorSegment = 0;
	
	// Вычисляем значение следующего индикатора
	unsigned int value = 0;
	if (next_digit == 0){
		value = min / 10;
	}else if(next_digit == 1){
		value = min % 10;
		// Устанавливаем битовую маску сепаратора на включение если он активен
		if (separatorState) separatorSegment = _BV(SEGMENT_PD);
	}else if(next_digit == 2){
		value = sec / 10;
	}else{
		value = sec % 10;
	}

	// Выключаем текущий индикатор
	DIGIT_PORT &= ~(_BV(active_digit));
		
	// Устанавливаем значение и сепаратор в следующий индикатор
	SEGMENT_PORT = separatorSegment | segments[value];
	
	// Включаем индикатор если не взведен флаг погасить все индикаторы
	if (hideDigit == 0){
		DIGIT_PORT |= _BV(next_digit);
	}
	
	// запоминаем новый активный индикатор
	active_digit = next_digit;
}