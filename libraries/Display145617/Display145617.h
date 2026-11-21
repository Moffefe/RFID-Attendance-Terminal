/*
Display145617 - klasa tworzaca interfejs na wyswietlaczu LCD
				Wyswietlanie duzych liter skopiowano z - http://woodsgood.ca/projects/2015/02/17/big-font-lcd-characters/
*/
#ifndef Display145617_h
#define Display145617_h

#include <stdint.h>
#include <UpdateManager.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <TimeLib.h>


class Display145617 : public UpdateManagerUser
{
public:
	enum State
	{
		blank,					// Pusty ekran
		text,						// 4 linie tekstu
		centeredText,					// Wycentrowany na srodku tekst. Dwie linie maks po 20 znakow
		horizontalMenu,						// Wycentrowany na srodku tekst. Dwie linie maks po 18 znakow, plus strzalki z obu stron
		time,									// Czas systemowy w prawym gornym rogu ekranu
		bigTime,									// Czas systemowy, dwa pierwsze wiersze wyswietlacza
		bigTimeWithText									// Tak jak "bigTime", plus wycentrowany napis pod spodem
	};



	String text_1;
	String text_2;
	String text_3;
	String text_4;



	Display145617();
	void Update();
	void begin();
	void SetState(State, uint8_t = 0);

private:
	struct UpdateInfo
	{
		unsigned long time_1;
		uint8_t data_1;
	};



	LiquidCrystal_I2C* lcd;
	byte col, row, nb, bc;
	byte bb[8];
	String cleanLine;
	State currentState;
	UpdateInfo updateInfo;
	State nextState;
	uint8_t nextStateInit;



	void SwitchState();
	void BlankProcedure();
	void TextProcedure();
	void CenteredTextProcedure();
	void HorizontalMenuProcedure();
	void TimeProcedure();
	void BigTimeProcedure();
	void BigTimeWithTextProcedure();
	int WriteBigChar(char, byte, byte);
	void WriteBigString(String&, byte, byte);
};

#endif