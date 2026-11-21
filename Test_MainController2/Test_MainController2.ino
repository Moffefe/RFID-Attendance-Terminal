#include <MainController2.h>



MainController2* mainController;



void setup()
{
  mainController = new MainController2();
}

void loop()
{
  updateManager.UpdateAll();
}
