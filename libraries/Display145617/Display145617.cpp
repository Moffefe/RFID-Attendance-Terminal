#include "Arduino.h"
#include "Display145617.h"



const char custom[][8] PROGMEM = {                        // Custom character definitions
	{ 0x1F, 0x1F, 0x1F, 0x00, 0x00, 0x00, 0x00, 0x00 }, // char 1 
	{ 0x18, 0x1C, 0x1E, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F }, // char 2 
	{ 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x0F, 0x07, 0x03 }, // char 3 
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x1F, 0x1F, 0x1F }, // char 4 
	{ 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1E, 0x1C, 0x18 }, // char 5 
	{ 0x1F, 0x1F, 0x1F, 0x00, 0x00, 0x00, 0x1F, 0x1F }, // char 6 
	{ 0x1F, 0x00, 0x00, 0x00, 0x00, 0x1F, 0x1F, 0x1F }, // char 7 
	{ 0x03, 0x07, 0x0F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F }  // char 8 
};

// BIG FONT Character Set
// - Each character coded as 1-4 byte sets {top_row(0), top_row(1)... bot_row(0), bot_row(0)..."}
// - number of bytes terminated with 0x00; Character is made from [number_of_bytes/2] wide, 2 high
// - codes are: 0x01-0x08 => custom characters, 0x20 => space, 0xFF => black square

const char bigChars[][8] PROGMEM = {
	{ 0x20, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // Space
	{ 0xFF, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // !
	{ 0x05, 0x05, 0x20, 0x20, 0x00, 0x00, 0x00, 0x00 }, // "
	{ 0x04, 0xFF, 0x04, 0xFF, 0x04, 0x01, 0xFF, 0x01 }, // #
	{ 0x08, 0xFF, 0x06, 0x07, 0xFF, 0x05, 0x00, 0x00 }, // $
	{ 0x01, 0x20, 0x04, 0x01, 0x04, 0x01, 0x20, 0x04 }, // %
	{ 0x08, 0x06, 0x02, 0x20, 0x03, 0x07, 0x02, 0x04 }, // &
	{ 0x05, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // '
	{ 0x08, 0x01, 0x03, 0x04, 0x00, 0x00, 0x00, 0x00 }, // (
	{ 0x01, 0x02, 0x04, 0x05, 0x00, 0x00, 0x00, 0x00 }, // )
	{ 0x01, 0x04, 0x04, 0x01, 0x04, 0x01, 0x01, 0x04 }, // *
	{ 0x04, 0xFF, 0x04, 0x01, 0xFF, 0x01, 0x00, 0x00 }, // +
	{ 0x20, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // 
	{ 0x04, 0x04, 0x04, 0x20, 0x20, 0x20, 0x00, 0x00 }, // -
	{ 0x20, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // .
	{ 0x20, 0x20, 0x04, 0x01, 0x04, 0x01, 0x20, 0x20 }, // /
	{ 0x08, 0x01, 0x02, 0x03, 0x04, 0x05, 0x00, 0x00 }, // 0
	{ 0x01, 0x02, 0x20, 0x04, 0xFF, 0x04, 0x00, 0x00 }, // 1
	{ 0x06, 0x06, 0x02, 0xFF, 0x07, 0x07, 0x00, 0x00 }, // 2
	{ 0x01, 0x06, 0x02, 0x04, 0x07, 0x05, 0x00, 0x00 }, // 3
	{ 0x03, 0x04, 0xFF, 0x20, 0x20, 0xFF, 0x00, 0x00 }, // 4
	{ 0xFF, 0x06, 0x06, 0x07, 0x07, 0x05, 0x00, 0x00 }, // 5
	{ 0x08, 0x06, 0x06, 0x03, 0x07, 0x05, 0x00, 0x00 }, // 6
	{ 0x01, 0x01, 0x02, 0x20, 0x08, 0x20, 0x00, 0x00 }, // 7
	{ 0x08, 0x06, 0x02, 0x03, 0x07, 0x05, 0x00, 0x00 }, // 8
	{ 0x08, 0x06, 0x02, 0x07, 0x07, 0x05, 0x00, 0x00 }, // 9
	{ 0xA5, 0xA5, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // :
	{ 0x04, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // ;
	{ 0x20, 0x04, 0x01, 0x01, 0x01, 0x04, 0x00, 0x00 }, // <
	{ 0x04, 0x04, 0x04, 0x01, 0x01, 0x01, 0x00, 0x00 }, // =
	{ 0x01, 0x04, 0x20, 0x04, 0x01, 0x01, 0x00, 0x00 }, // >
	{ 0x01, 0x06, 0x02, 0x20, 0x07, 0x20, 0x00, 0x00 }, // ?
	{ 0x08, 0x06, 0x02, 0x03, 0x04, 0x04, 0x00, 0x00 }, // @
	{ 0x08, 0x06, 0x02, 0xFF, 0x20, 0xFF, 0x00, 0x00 }, // A
	{ 0xFF, 0x06, 0x05, 0xFF, 0x07, 0x02, 0x00, 0x00 }, // B
	{ 0x08, 0x01, 0x01, 0x03, 0x04, 0x04, 0x00, 0x00 }, // C
	{ 0xFF, 0x01, 0x02, 0xFF, 0x04, 0x05, 0x00, 0x00 }, // D
	{ 0xFF, 0x06, 0x06, 0xFF, 0x07, 0x07, 0x00, 0x00 }, // E
	{ 0xFF, 0x06, 0x06, 0xFF, 0x20, 0x20, 0x00, 0x00 }, // F
	{ 0x08, 0x01, 0x01, 0x03, 0x04, 0x02, 0x00, 0x00 }, // G
	{ 0xFF, 0x04, 0xFF, 0xFF, 0x20, 0xFF, 0x00, 0x00 }, // H
	{ 0x01, 0xFF, 0x01, 0x04, 0xFF, 0x04, 0x00, 0x00 }, // I
	{ 0x20, 0x20, 0xFF, 0x04, 0x04, 0x05, 0x00, 0x00 }, // J
	{ 0xFF, 0x04, 0x05, 0xFF, 0x20, 0x02, 0x00, 0x00 }, // K
	{ 0xFF, 0x20, 0x20, 0xFF, 0x04, 0x04, 0x00, 0x00 }, // L
	{ 0x08, 0x03, 0x05, 0x02, 0xFF, 0x20, 0x20, 0xFF }, // M
	{ 0xFF, 0x02, 0x20, 0xFF, 0xFF, 0x20, 0x03, 0xFF }, // N
	{ 0x08, 0x01, 0x02, 0x03, 0x04, 0x05, 0x00, 0x00 }, // 0
	{ 0x08, 0x06, 0x02, 0xFF, 0x20, 0x20, 0x00, 0x00 }, // P
	{ 0x08, 0x01, 0x02, 0x20, 0x03, 0x04, 0xFF, 0x04 }, // Q
	{ 0xFF, 0x06, 0x02, 0xFF, 0x20, 0x02, 0x00, 0x00 }, // R
	{ 0x08, 0x06, 0x06, 0x07, 0x07, 0x05, 0x00, 0x00 }, // S
	{ 0x01, 0xFF, 0x01, 0x20, 0xFF, 0x20, 0x00, 0x00 }, // T
	{ 0xFF, 0x20, 0xFF, 0x03, 0x04, 0x05, 0x00, 0x00 }, // U
	{ 0x03, 0x20, 0x20, 0x05, 0x20, 0x02, 0x08, 0x20 }, // V
	{ 0xFF, 0x20, 0x20, 0xFF, 0x03, 0x08, 0x02, 0x05 }, // W
	{ 0x03, 0x04, 0x05, 0x08, 0x20, 0x02, 0x00, 0x00 }, // X
	{ 0x03, 0x04, 0x05, 0x20, 0xFF, 0x20, 0x00, 0x00 }, // Y
	{ 0x01, 0x06, 0x05, 0x08, 0x07, 0x04, 0x00, 0x00 }, // Z
	{ 0xFF, 0x01, 0xFF, 0x04, 0x00, 0x00, 0x00, 0x00 }, // [
	{ 0x01, 0x04, 0x20, 0x20, 0x20, 0x20, 0x01, 0x04 }, // Backslash
	{ 0x01, 0xFF, 0x04, 0xFF, 0x00, 0x00, 0x00, 0x00 }, // ]
	{ 0x08, 0x02, 0x20, 0x20, 0x00, 0x00, 0x00, 0x00 }, // ^
	{ 0x20, 0x20, 0x20, 0x04, 0x04, 0x04, 0x00, 0x00 }  // _
};



Display145617::Display145617() :
	lcd(NULL),
	nb(0),
	bc(0)
{
	text_1 = String();
	text_2 = String();
	text_3 = String();
	text_4 = String();
	cleanLine = String();
	cleanLine += F("                    ");
}

void Display145617::Update()
{
	SwitchState();

	switch (currentState)
	{
		case State::blank:
			BlankProcedure();
			break;

		case State::text:
			TextProcedure();
			break;

		case State::centeredText:
			CenteredTextProcedure();
			break;

		case State::horizontalMenu:
			HorizontalMenuProcedure();
			break;

		case State::time:
			TimeProcedure();
			break;

		case State::bigTime:
			BigTimeProcedure();
			break;

		case State::bigTimeWithText:
			BigTimeWithTextProcedure();
			break;
	}
}

void Display145617::begin()
{
	lcd = new LiquidCrystal_I2C(0x3F, 20, 4);			//
	lcd->init();										// Inicjacja wyswietlacza LCD
	lcd->backlight();									//

	for (nb = 0; nb < 8; ++nb)							//
	{													//
		for (bc = 0; bc < 8; ++bc)						// Definiujemy w wyswietlaczu segmenty duzych znakow,
			bb[bc] = pgm_read_byte(&custom[nb][bc]);	// dzieki temu moga dzialac metody WriteBigChar() i WriteBigString()
														//
		lcd->createChar(nb + 1, bb);					//
	}													//
}

void Display145617::SetState(State newState, uint8_t initializationData)
{
	nextState = newState;
	nextStateInit = bit(sizeof(initializationData) * 8 - 1) | initializationData;
	updateManager.Attach(this);
}

void Display145617::SwitchState()
{
	if (nextStateInit)
	{
		currentState = nextState;
		updateInfo.data_1 = nextStateInit;

		nextStateInit = 0;
	}
}

// Pusty ekran
void Display145617::BlankProcedure()
{
	if (bitRead(updateInfo.data_1, sizeof(updateInfo.data_1) * 8 - 1))
	{
		lcd->clear();
		updateManager.Detach(this);

		bitClear(updateInfo.data_1, sizeof(updateInfo.data_1) * 8 - 1);
		return;
	}
}

// 4 linie tekstu
// 'text_1' - 'text_4' - tresc ktora ma zostac wyswietlona
void Display145617::TextProcedure()
{
	if (bitRead(updateInfo.data_1, sizeof(updateInfo.data_1) * 8 - 1))
	{
		if (!(text_1.length() > 20 || text_2.length() > 20 || text_3.length() > 20 || text_4.length() > 20))
		{
			lcd->clear();
			lcd->setCursor(0, 0);
			lcd->print(text_1.c_str());
			lcd->setCursor(0, 1);
			lcd->print(text_2.c_str());
			lcd->setCursor(0, 2);
			lcd->print(text_3.c_str());
			lcd->setCursor(0, 3);
			lcd->print(text_4.c_str());
		}

		updateManager.Detach(this);

		bitClear(updateInfo.data_1, sizeof(updateInfo.data_1) * 8 - 1);
		return;
	}
}

// Wycentrowany na srodku tekst. Dwie linie maks po 20 znakow
// 'text_1' - tekst ktory ma byc wyswietlony w 1 lini
// 'text_2' - tekst ktory ma byc wyswietlony w 2 lini
void Display145617::CenteredTextProcedure()
{
	if (bitRead(updateInfo.data_1, sizeof(updateInfo.data_1) * 8 - 1))
	{
		if (!(text_1.length() > 20 || text_2.length() > 20))
		{
			lcd->clear();

			uint8_t x = (20 - text_1.length()) / 2;
			lcd->setCursor(x, 1);
			lcd->print(text_1.c_str());
			x = (20 - text_2.length()) / 2;
			lcd->setCursor(x, 2);
			lcd->print(text_2.c_str());
		}

		updateManager.Detach(this);

		bitClear(updateInfo.data_1, sizeof(updateInfo.data_1) * 8 - 1);
		return;
	}
}

// Wycentrowany na srodku tekst. Dwie linie maks po 18 znakow, plus strzalki z obu stron
// 'text_1' - tekst ktory ma byc wyswietlony w 1 lini
// 'text_2' - tekst ktory ma byc wyswietlony w 2 lini
void Display145617::HorizontalMenuProcedure()
{
	if (bitRead(updateInfo.data_1, sizeof(updateInfo.data_1) * 8 - 1))
	{
		if (!(text_1.length() > 18 || text_2.length() > 18))
		{
			lcd->clear();

			uint8_t x = (20 - text_1.length()) / 2;
			lcd->setCursor(x, 1);
			lcd->print(text_1.c_str());
			x = (20 - text_2.length()) / 2;
			lcd->setCursor(x, 2);
			lcd->print(text_2.c_str());
		}

		bitWrite(updateInfo.data_1, 0, true);			// Ustaw strzalki jako widoczne
		updateInfo.time_1 = millis();

		bitClear(updateInfo.data_1, sizeof(updateInfo.data_1) * 8 - 1);
		return;
	}

	if ((long)(millis() - updateInfo.time_1) >= 0)
	{
		bool arrowsVisible = bitRead(updateInfo.data_1, 0);
		bitWrite(updateInfo.data_1, 0, !arrowsVisible);

		lcd->setCursor(0, 1);
		if (arrowsVisible)
			lcd->write(127);
		else
			lcd->write(' ');

		lcd->setCursor(19, 1);
		if (arrowsVisible)
			lcd->write(126);
		else
			lcd->write(' ');

		updateInfo.time_1 = millis() + 600;
	}
}

// Czas systemowy w prawym gornym rogu ekranu
void Display145617::TimeProcedure()
{
	if (bitRead(updateInfo.data_1, sizeof(updateInfo.data_1) * 8 - 1))
	{
		lcd->clear();

		bitWrite(updateInfo.data_1, 2, true);			// Ustaw separator godziny jako widoczny,
		updateInfo.data_1 = updateInfo.data_1 | 3;		// oraz ustaw jego licznik na 3
		updateInfo.time_1 = millis();

		bitClear(updateInfo.data_1, sizeof(updateInfo.data_1) * 8 - 1);
		return;
	}

	if ((long)(millis() - updateInfo.time_1) >= 0)
	{
		bool timeSeparatorVisible = bitRead(updateInfo.data_1, 2);
		uint8_t timeSeparatorCounter = updateInfo.data_1 & 3;

		--timeSeparatorCounter;
		if (!timeSeparatorCounter)
		{
			timeSeparatorVisible = !timeSeparatorVisible;
			timeSeparatorCounter = 3;
		}

		bitWrite(updateInfo.data_1, 2, timeSeparatorVisible);
		updateInfo.data_1 = updateInfo.data_1 & ~3;
		updateInfo.data_1 = updateInfo.data_1 | timeSeparatorCounter;

		String timeString;
		time_t t = now();
		uint8_t temp;

		temp = hour(t);
		if (temp < 10)
			timeString += '0';
		timeString += temp;

		timeString += timeSeparatorVisible ? ':' : ' ';

		temp = minute(t);
		if (temp < 10)
			timeString += '0';
		timeString += temp;

		timeString += timeSeparatorVisible ? ':' : ' ';

		temp = second(t);
		if (temp < 10)
			timeString += '0';
		timeString += temp;

		lcd->setCursor(12, 0);
		lcd->print(timeString.c_str());

		updateInfo.time_1 = millis() + 200;
	}
}

// Czas systemowy, dwa pierwsze wiersze wyswietlacza
void Display145617::BigTimeProcedure()
{
	if (bitRead(updateInfo.data_1, sizeof(updateInfo.data_1) * 8 - 1))
	{
		lcd->clear();

		bitWrite(updateInfo.data_1, 2, true);			// Ustaw separator godziny jako widoczny,
		updateInfo.data_1 = updateInfo.data_1 | 3;		// oraz ustaw jego licznik na 3
		updateInfo.time_1 = millis();

		bitClear(updateInfo.data_1, sizeof(updateInfo.data_1) * 8 - 1);
		return;
	}

	if ((long)(millis() - updateInfo.time_1) >= 0)
	{
		bool timeSeparatorVisible = bitRead(updateInfo.data_1, 2);
		uint8_t timeSeparatorCounter = updateInfo.data_1 & 3;
		
		--timeSeparatorCounter;
		if (!timeSeparatorCounter)
		{
			timeSeparatorVisible = !timeSeparatorVisible;
			timeSeparatorCounter = 3;
		}
		
		bitWrite(updateInfo.data_1, 2, timeSeparatorVisible);
		updateInfo.data_1 = updateInfo.data_1 & ~3;
		updateInfo.data_1 = updateInfo.data_1 | timeSeparatorCounter;

		String timeString;
		time_t t = now();
		uint8_t temp;

		temp = hour(t);
		if (temp < 10)
			timeString += '0';
		timeString += temp;

		timeString += timeSeparatorVisible ? ':' : ' ';

		temp = minute(t);
		if (temp < 10)
			timeString += '0';
		timeString += temp;

		timeString += timeSeparatorVisible ? ':' : ' ';

		temp = second(t);
		if (temp < 10)
			timeString += '0';
		timeString += temp;

		WriteBigString(timeString, 0, 0);
		updateInfo.time_1 = millis() + 200;
	}
}

// Tak jak "bigTime", plus wycentrowany napis pod spodem
// 'text_1' - tekst ktory ma byc wyswietlony w 3 lini (na samym dole)
void Display145617::BigTimeWithTextProcedure()
{
	if (bitRead(updateInfo.data_1, sizeof(updateInfo.data_1) * 8 - 1))
	{
		lcd->clear();

		if (!(text_1.length() > 20))
		{
			uint8_t x = (20 - text_1.length()) / 2;
			lcd->setCursor(x, 3);
			lcd->print(text_1.c_str());
		}

		bitWrite(updateInfo.data_1, 2, true);			// Ustaw separator godziny jako widoczny,
		updateInfo.data_1 = updateInfo.data_1 | 3;		// oraz ustaw jego licznik na 3
		updateInfo.time_1 = millis();

		bitClear(updateInfo.data_1, sizeof(updateInfo.data_1) * 8 - 1);
		return;
	}

	if ((long)(millis() - updateInfo.time_1) >= 0)
	{
		bool timeSeparatorVisible = bitRead(updateInfo.data_1, 2);
		uint8_t timeSeparatorCounter = updateInfo.data_1 & 3;

		--timeSeparatorCounter;
		if (!timeSeparatorCounter)
		{
			timeSeparatorVisible = !timeSeparatorVisible;
			timeSeparatorCounter = 3;
		}

		bitWrite(updateInfo.data_1, 2, timeSeparatorVisible);
		updateInfo.data_1 = updateInfo.data_1 & ~3;
		updateInfo.data_1 = updateInfo.data_1 | timeSeparatorCounter;

		String timeString;
		time_t t = now();
		uint8_t temp;

		temp = hour(t);
		if (temp < 10)
			timeString += '0';
		timeString += temp;

		timeString += timeSeparatorVisible ? ':' : ' ';

		temp = minute(t);
		if (temp < 10)
			timeString += '0';
		timeString += temp;

		timeString += timeSeparatorVisible ? ':' : ' ';

		temp = second(t);
		if (temp < 10)
			timeString += '0';
		timeString += temp;

		WriteBigString(timeString, 0, 0);
		updateInfo.time_1 = millis() + 200;
	}
}

int Display145617::WriteBigChar(char ch, byte x, byte y)
{
	if (ch < ' ' || ch > '_') return 0;               // If outside table range, do nothing
	nb = 0;                                             // character byte counter 
	for (bc = 0; bc<8; bc++) {
		bb[bc] = pgm_read_byte(&bigChars[ch - ' '][bc]);  // read 8 bytes from PROGMEM
		if (bb[bc] != 0) nb++;
	}

	bc = 0;
	for (row = y; row < y + 2; row++) {
		for (col = x; col < x + nb / 2; col++) {
			lcd->setCursor(col, row);                      // move to position
			lcd->write(bb[bc++]);                          // write byte and increment to next
		}
		//    lcd.setCursor(col, row);
		//    lcd.write(' ');                                 // Write ' ' between letters
	}
	return nb / 2 - 1;                                      // returns number of columns used by char
}

void Display145617::WriteBigString(String& str, byte x, byte y)
{
	for (uint8_t i = 0; i < str.length(); ++i)
		x += WriteBigChar(str[i], x, y) + 1;
}