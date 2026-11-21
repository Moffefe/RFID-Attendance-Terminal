#include <Display145617.h>



Display145617 display;



void setup()
{
  setTime(0);
  display.begin();
  
  //display.SetState(Display145617::State::blank);

  //display.text_1 = "arka gdynia";
  //display.text_2 = "kurwa swinia";
  //display.SetState(Display145617::State::centeredText);

  //display.text_1 = "arka gdynia";
  //display.text_2 = "kurwa swinia";
  //display.SetState(Display145617::State::horizontalMenu);

  //display.SetState(Display145617::State::time);

  //display.SetState(Display145617::State::bigTime);

  display.text_1 = "arka gdynia";
  display.SetState(Display145617::State::bigTimeWithText);
}

void loop()
{
  updateManager.UpdateAll();
}
