/*
 * MyKomfortBlinker_v1.04.cpp
 *
 * Copyright 2017 Alexey Komarov rdalexey@yandex.ru
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
 
 


#define F_CPU 4800000UL				// тактовая частота микроконтроллера 4,8MHz. FUSE - lfuse=59 hfuse=FA (CKDIV8=0)
//#define F_CPU 600000UL			// тактовая частота микроконтроллера 0,6MHz. FUSE - lfuse=49 hfuse=FA (CKDIV8=1)
//#define F_CPU 1200000UL			// тактовая частота микроконтроллера 1,2MHz. FUSE - lfuse=4A hfuse=FA  (CKDIV8=1)

#include <avr/io.h>					// Подключение устройств ввода/вывода в зависимости от типа МК AVR
#include <util/delay.h>				// Удобные функции циклов задержки. void _delay_us (double __us) и void _delay_ms (double __ms)
#include <avr/eeprom.h>				// Библиотека для работы с EEPROM
//#include <avr/interrupt.h>		// Библиотека прерываний
#include <stdint.h>					// Библиотека объявляет несколько целочисленных типов и макросов. Например:
									// uint8_t	u		- unsigned	- беззнаковое
									//			int		- integer	- целое
									//			8					- колличество бит
									//			_t					- обозначение того, что эта аббревиатура - тип, а не функция и не процедура

#define Out_Links		PORTB3		// Переменная для режима левого поворотника, которая соответствует физическому порту PORTB3
#define Out_Rechts		PORTB4		// Переменная для режима правого поворотника, которая соответствует физическому порту PORTB4
#define Conf_Taste		PORTB5		// Переменная для режима входа в режим конфигурирования параметров Komfortblinker, которая соответствует 
									// физическому порту PORTB5

uint8_t EEMEM		  ee_Blinks;	// Переменная устанавливает количество повторений морганий поворотника. Значение по умолчанию 0х04
uint8_t EEMEM   ee_Danke_Blinks;	// Переменная устанавливает количество повторений морганий аварийки в режиме "Спасибо". Значение по умолчанию 0х02
uint8_t EEMEM   ee_Pause_Blinks;	// Переменная устанавливает паузу между режимами работы поворотника. Значение по умолчанию 0х10 
									// (10*50=500us или 0,5 секунды)
uint8_t EEMEM	  ee_Pause_Keys;	// Переменная устанавливает паузу между опросами состояния кнопок. Значение по умолчанию 0х10 
									// (10*10=100ms или 0,1 секунды)
uint8_t EEMEM ee_init_conf_flag;	// флаг, 0 - начальная конфигурация, значений в EEPROM нет и при включении питания значения EEPROM будут
									// перезаписаны значениями по умолчанию. Положительное число означает колличество параметров хранимых 
									// в памяти EEPROM.
uint8_t ee_flag			 = 0x00;	// флаг, установка флага в "1" показывает что мы находимся в режиме конфигирации.
uint8_t blink_flag		 = 0x00;	// флаг, установка флага в не нулевое значение показывает активность на входах. unsigned значит беззнаковый.
									// В двоичном представлении у нас старший бит отводится под знак, а значит в один байт (char) влазит число +127/-128,
									// но если знак отбросить то влезет уже от 0 до 255. Обычно знак не нужен. Так что unsigned
									// blink_flag = 1 - был включен правый поворотник
									// blink_flag = 2 - был включен левый поворотник
									// blink_flag = 3 - был включен режим "Спасибо"
									// blink_flag = 4 - работает реле
									// blink_flag = 5 - реле работает в режиме конфигурации
									// blink_flag = 8 - оработка ожидания выключения реле для возврата в исходное состояние
uint8_t blink_counter	= 0x00;		// Счетчик, хранит значение оставшихся поворения моргания поворотником
uint8_t my_key_state	= 0x00;		// переменная для хранения состояния входов
uint8_t my_puls			= 0x00;		// переменная для хранения состояния количества повторений морганий (поворотник (Blinks 4),
									// для "Спасибо" (Danke_Blinks 2))
uint8_t not_state		= 0x00;		// переменная для хранения статуса принудительной отмены включенного поворотника
uint8_t ee_counter		= 0x00;		// переменная для временного хранения значений, в последствии сохраняемых в EEPROM


uint8_t Not_Blink_State (void)		// Проверка на принудительную отмену включенного поворотника, т.е. например работает левый поворотник
									// и мы коротким касанием правого поворотника отменяем действие левого поворотника
{
	uint8_t j = 0; // переменная j
	if ((blink_flag == 0x01) && ((my_key_state | 0b11111110) == 0b11111110)) j	= 1; 
	// включен поворотник вправо, коротким касанием поворотноком влево отменяем моргание, т.е. проверяем побитовым ИЛИ (OR) 2-й разряд на "0"
	if ((blink_flag == 0x02) && ((my_key_state | 0b11111101) == 0b11111101)) j	= 1; 
	// включен поворотник влево, коротким касанием поворотноком вправо отменяем моргание, т.е. проверяем побитовым ИЛИ (OR) 1-й разряд на "0"
	return j;
}

void Reset_Relay (void) // подпрограмма возврата к исходмону состоянию, когда все выключено и мы ждем нажатия поворотника или аварийки
{
	 PORTB			&=	~(1<<Out_Links);
	 // записываем "0" в порт Aus_Links (PORTB3), т.е. выключаем транзистор и размыкаем цепь принудительного удержания левого поворотника
	 // для записи  1 в порт: PORTB |= 1 << Aus_Links, т.е. замкнуть контакты левого поворотника через транзистор;
	 PORTB			&=	~(1<<Out_Rechts);	
	 // записываем "0" в порт Aus_Rechts (PORTB4), т.е. выключаем транзистор и размыкаем цепь принудительного удержания правого поворотника
	 blink_flag		=				0x00;	// обнуляем флаг
	 blink_counter	=				0x00;	// обнуляем счетчик
	 my_puls		=				0x00;	// обнуляем переменную для хранения состояния количества повторений морганий
	 if (ee_flag == 0x00)					// если находимся в режиме настроек, то пропускаем паузу (в случае нехватки памяти, проверку можно удалить)
	 {
		for (uint8_t i = 0; i < eeprom_read_byte(&ee_Pause_Blinks); i++)
		// выдерживаем паузу (4*10=40ms= 0,04s), тем самым избавляемся от непроизвольного включения левого поворотника, после того, как правый,
		// включенный, поворотник был автоматически отщелкнут возвратом руля из поворота на право. Так сказать, устраняем механический дефект
		{
			_delay_ms(10);				// ждем 10 миллисекунд
		}	//for
	 }		//if
}

uint8_t Get_Key_Data (void) // Подпрограмма опроса состояния входов с антидребезгом
{
	uint8_t k;							// переменная "k"
	do 
	{
		k = (PINB & 0b00100111);		// методом побитового И "&" определяем состояние только первых 3-х разрядов (0b00000 "1" "1" "1"),
		if (ee_flag == 0x00)
		{
			for (uint8_t t = 0; t < eeprom_read_byte(&ee_Pause_Keys); t++)
			// выдерживаем паузу (100*10=1000us= 1ms), тем самым избавляемся от непроизвольного включения левого поворотника, после того,
			// как правый, включенный, поворотник был автоматически отщелкнут возвратом руля из поворота на право.
			// Так сказать, устраняем механический дефект выключателя
			{
				_delay_us(100);			// ждем 100 микросекунд
			}	//for
		}		//if
		else
		{
			_delay_ms(2);				// ждем 2 миллисекунды
		}		//else
	} while (k != (PINB & 0b00100111)); 
	// сравниваем i (состояние портов до паузы) и текущее состояние портов. Если состояния разные, что в нашем случее верно,
	// переходим опять в начало цикла и вновь считываем значения портов, ждем, повторяем чтение и сравниваем. Если состояния одинаковые,
	// а значит мы предполагаем что считали устойчивое состояние портов, то по условию "while" это "fulse" мы завершаем подпрограмму и
	asm volatile("wdr");				// сброс сторожевого таймера WDT	
	return k;							// возвращаем в основную программу текущее состояние портов
}

uint8_t Get_Kurz_Lang_Druck (void)		// Подпрограмма опроса длительности нажатия кнопки. 2.
{
	uint8_t i = 0;
	do
	{
		my_key_state = Get_Key_Data();						// получаем состояние входов
	} while ((my_key_state & 0b00100000) == 0b00100000);	// если кнопка не нажата, то просто ждем нажатия для продолжения работы
	
	while (((my_key_state | 0b11011111) == 0b11011111) && (i < 0xC8)) 
	// (0xC8=200)200*2ms = !!! 3 секунды. Нажатие меньше 3 сек. принимаем за короткое нажатие. Непонятно, почему фактически получается 3с.
	{
		my_key_state = Get_Key_Data();	// получаем состояние входов
		i++;							// увеличиваем счетчик на 1 (означает 10 ms)
	}
	return i;
}

uint8_t Get_Kurz_Lang_Counter (void)	// Подпрограмма счета коротких нажатий и ожидания длинного нажатия кнопки. 1.
{
	uint8_t j = 0;
	blink_counter = Get_Kurz_Lang_Druck(); // получаем число от 0 до 0xC8 (200)
	while ((blink_counter < 0xC8) && (j < 0xFE)) 
	//считаем колличество коротких нажатий (blink_counter < 0xC8), максимально возможное колличество нажатий 254 (j < 0xFE).
	{
		j++;
		blink_counter = Get_Kurz_Lang_Druck();
	}
	return j;
}
	
void Blink_Relay (uint8_t pulsData)
{
	PORTB			|= (1<<Out_Links); 	// включаем удержание левого поворотника (записываем в порт логическую "1")
	PORTB			|= (1<<Out_Rechts);	// включаем удержание правого поворотника (записываем в порт логическую "1")
	blink_flag		= 0x05;				// устанавливаем флаг для обозбачения режима, в котором мы находимся
	do 
	{
		if (blink_flag == 0x08)
		// Проверяем, был ли установлен blink_flag в 0х08, т.е. это означает, что поворотник включен надолго или работает режим "Аварийка",
		// но мы уже моргнули нужное количество раз
		{
			if (my_key_state == 0b00000011)	
			// Проверяем, работает ли реле !!!В последствии убрать проверку поворотников, она здесь не нужна или нужна????
			{
				Reset_Relay();
			}
		}
		else
		{
			my_key_state = Get_Key_Data();	// опрашиваем состояния входов, нам важно состояние 6 и 3-го разрядов 
											// 0b00100000 - состояние по умолчанию
											//     |  |---> состояние реле		0 - выкл 1 - вкл
											//     |------> режим конфигурации	1 - выкл 0 - вкл
			if ((my_key_state | 0b11111011) == 0b11111011) // счетчик морганий, реле обесточено
			{
				if (blink_counter >= pulsData)
				{ // моргнули нужное количество раз
					PORTB		&= ~(1<<Out_Links);		// выключаем удержание левого поворотника (записываем в порт логический "0")
					PORTB		&= ~(1<<Out_Rechts);	// выключаем удержание правого поворотника (записываем в порт логический "0")
					blink_flag	= 0x08;					// оставляем флаг для дальнейшей проверки, что происходит с реле...
				}
				else // еще не доморгали нужное количество раз
				{
					blink_counter++;
					while ((my_key_state | 0b11111011) == 0b11111011) // ожидание включения реле
					{
						my_key_state = Get_Key_Data();
						// опрашиваем состояния входов, еще должны проверить, не была ли инициирована отмена включенного поворотника
					} // while
				} // else
			} // if
		} // else
	} while (blink_flag == 0x05);
}

int main(void)
// функция main - точка входа в главную прогрмму. void это тип данных которые мы передаем в функцию, в данном случае main
// также не может ничего принять извне, поэтом void - пустышка. Заглушка, применяется тогда когда не надо ничего передавать или возвращать.
{
/* Инициализация портов ввода-вавода. Инициализация порта B:
    Func5=In  Func4=Out  Func3=Out  Func2=In  Func1=In  Func0=In --- In - вход, Out - выход
    State5=T  State4=0   State3=0   State2=T  State1=P  State0=P  --- 0 - порт настроен как выход и притянут к земле, 
																	  Т - порт настроен как вход и находится в режиме Hi-Z,
																      P - порт настроен как вход и находится в режиме PullUp
    PINх (PINB)   - регистр чтения. Из него можно только читать. В регистре PINx содержится информация о реальном текущем логическом уровне 
	                на выводах порта, вне зависимости от настроек порта. Так что если хотим узнать что у нас на входе - читаем
					соответствующий бит регистра PINx
    DDRx (DDRB)   - регистр направления порта. Порт в конкретный момент времени может быть либо входом либо выходом
				    DDRBy=0 - вывод работает как ВХОД
				    DDRBy=1 - вывод работает на ВЫХОД
    PORTx (PORTB) - Режим управления состоянием вывода. Когда мы настраиваем вывод на вход, то от PORT зависит тип входа (Hi-Z или PullUp)
				    Когда ножка настроена на выход, то значение соответствующего бита в регистре PORTx определяет состояние вывода.
					Если PORTxy=1 то на выводе лог1, если PORTxy=0 то на выводе лог "0"
				    Когда ножка настроена на вход, то если PORTxy=0, то вывод в режиме Hi-Z. 
					Если PORTxy=1 то вывод в режиме PullUp с подтяжкой резистором в 100к до питания 
*/
 
 //DDRB &= ~(1<<PORTB2|1<<PORTB1|1<<PORTB0);	
 // конфигурируем порты PORTB2, PORTB1 и PORTB0 как ВХОДЫ. Не имеет смысла делать, т.к. они по умолчанию ВХОДЫ
 DDRB  |= 1<<Out_Rechts|1<<Out_Links;			// конфигурируем порты PORTB4 и PORTB3 как выходы
 //PORTB |= 1<<PORTB1|1<<PORTB0;				
 // записываем в порты PORTB1 и PORTB0 "1", т.е. подтагиваем их через внутреннее сопротивление микроконтроллера (100кОм) к +5V.
 // Хотя на них постоянно будет +4,7v
 PORTB |= 1<<Conf_Taste|1<<PORTB1|1<<PORTB0;	
 // Порт "RESET" (PORTB5) используем как вход, записываем в порт PORTB5 и порты PORTB1 и PORTB0 "1", т.е.
 // подтагиваем их через внутреннее сопротивление микроконтроллера (100кОм) к +5V при появлении на этом порту логического "0" - будем
 // переходить в режим измемения настроек по умолчанию
 
 //PORTB &= ~(1<<PORTB2);						
 // значение по умолчанию (ножка в режиме вход и в режиме Hi-Z), внешними цепями удерживаем на этом выводе логический "0"
 
 // Инициализация сторожевого таймера (перенесено из CodeVisionAVR, позже узнать как это работает и как корректно инициализиривать :) )
 // нужен для предотвращения зависания микроконтроллера и зацикливания программы
 // Watchdog Timer Prescaler: OSC/1024k
 // Watchdog Timer interrupt: Off
 //#pragma optsize-
 WDTCR=0x39; // 0b00111001
 WDTCR=0x29; // 0b00101001
 //#ifdef _OPTIMIZE_SIZE_
 //#pragma optsize+
 //#endif 
 
 // Global enable interrupts
 // sei(); // разрешаем прерывания
 
 if (eeprom_read_byte(&ee_init_conf_flag) != 0x04) // проверяем состояние конфигурации в  EEPROM
 {
	 eeprom_write_byte(&ee_Blinks,			0x04);
	 // устанавливаем количество повторений морганий поворотника. Значение по умолчанию 0х04
	 eeprom_write_byte(&ee_Danke_Blinks,	0x00);
	 // устанавливаем количество повторений морганий режима Danke. Значение по умолчанию (0х02) 0x00 выключено
	 eeprom_write_byte(&ee_Pause_Blinks,	0x04);
	 // устанавливаем паузу между режимами работы поворотника. Значение по умолчанию 0х04 (4*10=40ms или 0,06 секунды)
	 eeprom_write_byte(&ee_Pause_Keys,		0x0A);
	 // устанавливаем паузу между опросами состояния кнопок. Значение по умолчанию 0х0A (10*10=100us или 0,1 секунды)
	 eeprom_write_byte(&ee_init_conf_flag,	0x04);
	 // устанавливаем флаг 0х04, означает, что в память записано 4-e параметра и начальная конфигурация проведена
 } 
 
	while(1) // основной цикл программы
    {
		my_key_state = Get_Key_Data();	/* 0b00100011 - значение по умолчанию, левый и правый поворотники "висят" в воздухе и притянуты к +12V, 
														реле поворотов обесточено и притянуто к земле
											   ||||||-> вправо				1 - выкл 0 - вкл
											   |||||--> влево				1 - выкл 0 - вкл
											   ||||---> состояние реле		0 - выкл 1 - вкл
											   |||----> влево реле			0 - выкл 1 - вкл
											   ||-----> вправо реле			0 - выкл 1 - вкл
											   |------> режим конфигурации	1 - выкл 0 - вкл */
		//if((my_key_state | 0b11111111) != 0b11111111) // нажата ли кнопка настройки (PORTB5) - заглушка, когда нужно убрать подпрограму 
														// записи настроек в память
		if((my_key_state | 0b11011111) == 0b11011111) // нажата ли кнопка настройки (PORTB5)
		{
			ee_flag = 0x01;		// обозначаем, что мы в режиме конфигурации
			Reset_Relay();		// на всякий пожарный выключим удержание поворотников и обнулим переменные
			Blink_Relay(0x06);	// режим конфигурации, моргнем 7 (0,1,2,3,4,5,6) раз, мы в режиме установки колличества повторений
								// морганий поворотника (uint8_t Blinks = 0x04;) - 4-е по умолчанию.
						
			ee_counter = Get_Kurz_Lang_Counter();
			// если длинное нажатие на кнопку или колличество коротких нажатий превысило 254, то записываем колличество короткий нажатий
			// в память и переходим к настройке колличества морганий режима Danke.
			if (ee_counter > 0x00) 
			{
				eeprom_write_byte(&ee_Blinks, --ee_counter);
			// устанавливаем количество повторений морганий поворотника. Значение по умолчанию 0х04
			}// if
			
			Reset_Relay();		// на всякий пожарный выключим удержание поворотников и обнулим переменные
			Blink_Relay(0x01);	
			// моргнем 2-а (0,1) раза, и переходим в режим установки колличества повторений морганий режима Danke
			// (uint8_t Danke_Blinks = 0x02;) - 2-a по умолчанию.
			ee_counter = Get_Kurz_Lang_Counter();
			// если длинное нажатие на кнопку или колличество коротких нажатий превысило 254, то записываем колличество коротких
			// нажатий в память и переходим к настройке колличества морганий режима Danke.
			if (ee_counter > 0x00)
			{
				eeprom_write_byte(&ee_Danke_Blinks, --ee_counter);
			// устанавливаем количество повторений морганий режима Danke. Значение по умолчанию 0х02
			} // if
			
			Reset_Relay();		// на всякий пожарный выключим удержание поворотников и обнулим переменные
			Blink_Relay(0x02);	// моргнем 3-а (0,1,2) раза, переходим в режим установки задержки (хххххх = 10;) - 10-ть по умолчанию 
								// (10*50=500 миллисекунд)
			ee_counter = Get_Kurz_Lang_Counter();
			// если длинное нажатие на кнопку или колличество коротких нажатий превысило 254, то записываем колличество короткий нажатий
			// в память и переходим к настройке колличества морганий режима Danke.
			if (ee_counter > 0x00)
			{
				eeprom_write_byte(&ee_Pause_Blinks, ee_counter);
			// устанавливаем количество повторений цикла для паузы между протвоположными состояниыми поворотника. Значение по умолчанию 0х04
			} // if
			
			Reset_Relay();		// на всякий пожарный выключим удержание поворотников и обнулим переменные
			Blink_Relay(0x03);	// моргнем 4-е (0,1,2,3) раза, переходим в режим установки задержки (хххххх = 10;) - 10-ть по умолчанию
								// (10*100=1000 микросекунд или 1 миллисекунда)
			ee_counter = Get_Kurz_Lang_Counter();
			// если длинное нажатие на кнопку или колличество коротких нажатий превысило 254, то записываем колличество короткий нажатий
			// в память и переходим к настройке колличества морганий режима Danke.
			if (ee_counter > 0x00)
			{
				eeprom_write_byte(&ee_Pause_Keys, ee_counter);
			// устанавливаем количество повторений цикла для паузы антидребезга. Значение по умолчанию 0х0А (10)
			} // if

			Reset_Relay();
			Blink_Relay(0x06);	// моргнем 7-ть (0,1,2,3,4,5,6) раз, этим показываем, что вышли из програмы настройки пользовательских установок
			ee_flag = 0x00;  // сбрасываем флаг "режим конфигурации"
		};//if
				
		not_state = Not_Blink_State();	// Проверка на принудительную отмену включенного поворотника, т.е. например работает левый поворотник
										// и мы коротким касанием правого поворотника отменяем действие левого поворотника.
		if (not_state == 0x01) Reset_Relay();	// включен поворотник вправо, коротким касанием поворотноком влево отменяем моргание или
												// включен поворотник влево, коротким касанием поворотноком вправо отменяем моргание

		if ((blink_flag & 0x08) == 0x08)		// Проверяем, был ли установлен blink_flag в 0х08, т.е. это означает, что поворотник включен
												// надолго или работает режим "Аварийка". 
		{
			if ((my_key_state & 0b00000111) == 0b00000011)	// Проверяем, работает ли реле и выключены ли при этом поворотники. Реле выключено,
															// и поворотники выключены, то 
			{
				Reset_Relay();
			}		//if
		}			//if
		else
		{
			if ((blink_flag & 0b00000111) > 0x00)						// Что-то было включено
			{
				if ((my_key_state | 0b11111011) == 0b11111011)			// счетчик морганий, реле обесточено
				{
					if ((blink_flag & 0x03) == 0x03)					// проверяем состояние флага 
					{													// флаг был установлен в режим аварийки
						my_puls = eeprom_read_byte(&ee_Danke_Blinks);	// берем соответствующее количество морганий "Danke_Blinks"
					} 
					else // флаг в режиме поворотника
					{
						my_puls = eeprom_read_byte(&ee_Blinks);			// берем соответствующее количество морганий "Blinks"
					}					
					if (blink_counter >= my_puls)
					{ // моргнули нужное количество раз
						PORTB		&= ~(1<<Out_Links);		// выключаем удержание левого поворотника (записываем в порт логический "0")
						PORTB		&= ~(1<<Out_Rechts);	// выключаем удержание правого поворотника (записываем в порт логический "0")
						blink_flag	= 0x08;					// оставляем флаг для дальнейшей проверки, что происходит с реле...
					} 
					else // еще не доморгали нужное количество раз
					{
						blink_counter++;
						while ((my_key_state | 0b11111011) == 0b11111011 && not_state == 0x00)
						// ожидание включения реле и проверка на принудительную отмену включенного поворотника
						{
							my_key_state = Get_Key_Data();
							// опрашиваем состояния входов, по идее еще должны проверить, не была ли инициирована отмена включенного поворотника
							not_state = Not_Blink_State();	// Проверка на принудительную отмену включенного поворотника
							if (not_state == 0x01) Reset_Relay();
						}	// while
					}		// else
				}			// if
			}				// if
			else // ничего не включено
			{
				if (blink_flag == 0x00)
				// что-то было включено? - проверяем, что было включено и устанавливаем соответствующий флаг и состояние портов
				{
/*
					if ((my_key_state & 0b00000111) == 0b00000000)
					{
						PORTB			&= ~(1<<Out_Links);		// выключаем удержание левого поворотника (записываем в порт логический "0")
						PORTB			&= ~(1<<Out_Rechts);	// выключаем удержание правого поворотника (записываем в порт логический "0")
						blink_flag		= 0x08;
					}
					if ((my_key_state & 0b00000111) == 0b00000111)
					{
						PORTB			|= 1<<Out_Links; 	// включаем удержание левого поворотника (записываем в порт логическую "1")
						PORTB			|= 1<<Out_Rechts;	// включаем удержание правого поворотника (записываем в порт логическую "1")
						blink_flag		= 0x03;
						blink_counter	= 0x01;
					}
					if ((my_key_state & 0b00000111) == 0b00000101)
					{
						PORTB			|= 1<<Out_Links;	// включаем удержание левого поворотника (записываем в порт логическую "1")
						blink_flag		= 0x01;
						blink_counter	= 0x01;
					}
					if ((my_key_state & 0b00000111) == 0b00000110)
					{
						PORTB			|= 1<<Out_Rechts;	// включаем удержание правого поворотника (записываем в порт логическую "1")
						blink_flag		= 0x02;
						blink_counter	= 0x01;
					}
*/
/* Выше указанный блок требует на 4 байта больше памяти, чем ниже указанный с использованием switch */
					switch (my_key_state & 0b00000111) // проверка на включение
					{
						case 0b00000000: // выключаем удержание поворотников (и правого и левого)
							PORTB			&= ~(1<<Out_Links);		// выключаем удержание левого поворотника (записываем в порт логический "0")
							PORTB			&= ~(1<<Out_Rechts);	// выключаем удержание правого поворотника (записываем в порт логический "0")
							blink_flag		= 0x08;
						break;	
						case 0b00000111: // режим аварийки или "спасибо"
							if (eeprom_read_byte(&ee_Danke_Blinks) != 0x00)
							{
								PORTB			|= 1<<Out_Links; 	// включаем удержание левого поворотника (записываем в порт логическую "1")
								PORTB			|= 1<<Out_Rechts;	// включаем удержание правого поворотника (записываем в порт логическую "1")
								blink_flag		= 0x03;
								blink_counter	= 0x01;
							}
						break;
						/*
						case 0b00000001: // включен поворотник влево
							PORTB			|= 1<<Out_Links;	// включаем удержание левого поворотника (записываем в порт логическую "1")
							blink_flag	= 0x01;
							blink_counter	= 0x00;
						break;
						*/
						case 0b00000101: // включен поворотник влево и реле уже включилось
						if (eeprom_read_byte(&ee_Blinks) != 0x00)
							{
								PORTB			|= 1<<Out_Links;	// включаем удержание левого поворотника (записываем в порт логическую "1")
								blink_flag		= 0x01;
								blink_counter	= 0x01;
							}
						break;
						/*
						case 0b00000010: // включен поворотник вправо
							PORTB			|= 1<<Out_Rechts;	// включаем удержание правого поворотника (записываем в порт логическую "1")
							blink_flag	= 0x02;
							blink_counter	= 0x00;
						break;
						*/
						case 0b00000110: // включен поворотник вправо и реле уже включилось
							if (eeprom_read_byte(&ee_Blinks) != 0x00)
							{
								PORTB			|= 1<<Out_Rechts;	// включаем удержание правого поворотника (записываем в порт логическую "1")
								blink_flag		= 0x02;
								blink_counter	= 0x01;
							}
						break;
					}	// switch
/* */
				}		// if
			}			// else
		}				// else
    }					// while
return 0;	/* return - это возвращаемое значение, которое функция main отдаст при завершении, поскольку у нас int, то есть
			   мы должны вернуть число. Хотя это все равно не имеет смысла, т.к. на микроконтроллере из main нам
			   выходить разве что в никуда. Возвращаем нуль. А компилятор обычно умный и на этот случай код не генерит */
} // main
