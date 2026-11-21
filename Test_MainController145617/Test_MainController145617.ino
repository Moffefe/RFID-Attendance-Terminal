#include <MainController145617.h>



MainController145617* mainController;



void setup()
{
  Serial1.begin(115200);                         // Do debugowania
  mainController = new MainController145617();
}

void loop()
{
  updateManager.UpdateAll();
}
