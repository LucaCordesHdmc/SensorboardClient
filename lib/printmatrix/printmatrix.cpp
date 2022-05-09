#include <Arduino.h>
#include "printmatrix.h"
void printmatrix(int *ptr,int edge){
     Serial.print("Matrix:\n");

  for (int i = 0; i < edge * edge; i++) { //edge*edge= number of values in matrix
    if (i % edge == 0 && i != 0) {
      Serial.print("\n\r");
    }
    int tempref=*(ptr+i);
    Serial.print(tempref);
    Serial.print("  ");
    //Debugger::printf("%5d", *(ptr + i));

  }
  Serial.print("\n");
}