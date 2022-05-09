#include "vl5helper.h"
#include <printmatrix.h>
#include <Arduino.h>
#include <EEPROM.h>
//#include "printmatrix.h" //for testing purpose

vl5helper::vl5helper()
{
    Serial.print("calling constructor");
    // resulution is 64 by default

    imageWidth = 8;
    Serial.print("initialzing everything");
    /*
    for (int i = 0; i < 5; i++)
    { // starting with every person in retet state
        resetcurrentpersons(i);
        copydata(1, 0, i, i);
        copydata(2, 0, i, i);
        Serial.print("eveything reseted");
        /////positionpeople[i]=0;
        // sumpeople[0][i]=0;
    }
    */
   Einsteiger=0;
   Aussteiger=0;
    for (int i = 0; i < 65; i++)
    {
        checksurroundings[i] = 0; // starting with empty chucksurroundings matrix
        *(buffer0 + i) = 2000;    // filling the fuffres with 2m each
        *(buffer1 + i) = 2000;
        *(buffer2 + i) = 2000;
        flagobject[i] = 0; // starting with empty flagobject matrix
    }
}
#define EEPROM_SIZE 40
// only 2 bytes of 512 neede
//writes to eeprom
//@param einsteiger writes into thee first byte and so on
void vl5helper::writetoeeprom(uint16_t einsteiger, uint16_t aussteiger)
{                                  // 258 = 1 0000 0010      we have to split the  uint16_t into 2 bytes
    if (EEPROM.begin(EEPROM_SIZE)) // DEGUBBING EEPROM.begin(EEPROM_SIZE) will fail at some point bc [E][esp32-hal-i2c.c:1080] i2cAddQueue(): malloc failure followed by [E][EEPROM.cpp:73] begin(): Unable to open NVS namespace: 257
    {
        int address = 0;
        // einsteiger
        uint8_t einsteiger81 = einsteiger;
        uint8_t einsteiger82 = einsteiger >> 8;

        EEPROM.write(address, einsteiger81); // EEPROM.put(address, boardId);

        address += sizeof(einsteiger81); // update address value

        EEPROM.write(address, einsteiger82); // EEPROM.put(address, boardId);
        address += sizeof(einsteiger82);     // update address value
                                             // aussteiger
        uint8_t aussteiger81 = aussteiger;
        uint8_t aussteiger82 = aussteiger >> 8;

        EEPROM.write(address, aussteiger81); // EEPROM.put(address, boardId);

        address += sizeof(aussteiger81); // update address value

        EEPROM.write(address, aussteiger82); // EEPROM.put(address, boardId);
        address += sizeof(aussteiger82);     // update address value

        EEPROM.commit();
    }
    else
    {
        ESP.restart(); // esp32 eeprom error might be the unable to open nvs namespace 257
    }
}

//reads from eeprom
//@returns aussteiger
uint16_t vl5helper::getAussteigerFromEEPROM()
{

    EEPROM.begin(EEPROM_SIZE);
    int address = 2;

    uint8_t aussteiger81;
    uint8_t aussteiger82;
    aussteiger81 = EEPROM.read(address); // EEPROM.get(address,readId);                                    // update address value
    address += sizeof(aussteiger81);
    aussteiger82 = EEPROM.read(address);
    address += sizeof(aussteiger82);
    uint16_t aussteiger = aussteiger81 + ((aussteiger82) << 8);
    return aussteiger;
}

//reads from eeprom
//@returns einsteiger
uint16_t vl5helper::getEinsteigerFromEEPROM() // achtung Einsteiger weredn oft öfter gezählt als die Aussteiger
{

    EEPROM.begin(EEPROM_SIZE);
    int address = 0;

    uint8_t Einsteiger81;
    uint8_t Einsteiger82;
    Einsteiger81 = EEPROM.read(address); // EEPROM.get(address,readId);                                    // update address value
    address += sizeof(Einsteiger81);
    Einsteiger82 = EEPROM.read(address);
    address += sizeof(Einsteiger82);
    uint16_t Einsteiger = Einsteiger81 + ((Einsteiger82) << 8);
    return Einsteiger;
}

//normal run
int vl5helper::run(int *ptr, int threshold) // pass <1 as threshold to use defautl value of 1500
{

    // in this funcino we call eveything we neat in an ordery neat fashion and it should be somewhat understandable for peolpe to read through
    int *arraybool;                                  // just a temp variable to hold in the 0 and 1 matrix but we could just call it directly where it's neede like groupflagged(updateOccupancy(&ptr[0],1000));
    arraybool = updateOccupancy(&ptr[0], threshold); // our ptr is the pointer with the mm values of the matrix we give it to updateOccupancy and a threshold we can choose.
    PosXYandSum copy[10];

    int NoOfPeople = 0;
   // Serial.println("the updataed occupancy matrix is:");
    printmatrix(arraybool, 8); // showing the current flagobject matrix we're working with
    do
    {

        copy[NoOfPeople] = groupflagged();          // groupflagged returns a preson at a time. The first preson is the first that got found and has therefor the lowest index. the last hast the least lowest
        temp[NoOfPeople].copypos(copy[NoOfPeople]); // we have to only copy the needed data or eles our pastsimplepos[1] gets empty
        persons[NoOfPeople].writetoPerson(temp[NoOfPeople]);
        if (temp[NoOfPeople].used == true)
        {
            temp[NoOfPeople].displayP();
        }
        NoOfPeople++; // increment i by one

    } while (temp[NoOfPeople - 1].used); // the last posXYandSum that groupflagged retuns is going to be empty || we can't just retun 0 and it expects the type PosXYandSum
                                         // now we have the current data in our temp array with useful data form temp[0] to temp[i-1]
                                         // i-1 is how many people are in the frame right now
                                         // when i is 1 than there is nobody visale in the frame
    for (int a = 0; a < NoOfPeople - 1; a++)
    {
        if (temp[a].sum < 2)
        { // this is a person with only one value we sould fiter them out
            int shift = 0;
            Serial.println("FILTERING out the following single person");
            temp[a].displayP();

            while (shift + a + 1 != NoOfPeople - 1)
            { // last person is NoOfpeople-2
                temp[a + shift] = temp[a + shift + 1];
                shift++;
            }
            temp[NoOfPeople - 2].used = false; // last person gets erased
            NoOfPeople--;                      // now we filtered out the person with only sum<2 and thus we have to decrease NoOfPeople by one
            a--;                               // could be that there are just two data points that have go get filtered out so we have to check the a. again.
            // we can iterate this until all sum=1 people are gone.
            // sum being a float could be changed in the future.
        }
    }
    if (NoOfPeople - 1 != 0)
    {
        Serial.print("we detected ");
        Serial.print(NoOfPeople - 1);
        Serial.print(" people ");
        Serial.print(" here is their data again: ");

        for (int a = 0; a < NoOfPeople - 1; a++)
        {
            if (temp[a].used)
            {
                Serial.print("\n person ");
                Serial.println(a);
                temp[a].displayP();
            }
        }
    }

    prevpeople[1] = prevpeople[0];
    prevpeople[0] = NoOfPeople - 1;

    return simplifiedtracking(); // returns the delta people in the bus
}
//returns -1 if nothing is stuck
//@return the index of pixel where the data got stuck
int vl5helper::isDataStuck() // this function only firlerst out the first value that is stuck wich is enough
{                            // so it does not filter out several stuck value  at a time
    for (int i = 0; i < 64; i++)
    {
        if ((*(buffer0 + i)) == (*(buffer1 + i)) == (*(buffer2 + i)))
        {
            if (*(buffer0 + i) < 1000)
            {
                return i; // a small value seems to be  stuck and has to be filterde out. we will return the index of that small stuck value
            }
        }
    }
    return -1; // nothing seems to be stuck
}

void vl5helper::correctStuckBuffer(int index)
{
    if (index != -1)
    {
        *(buffer0 + index) = 2000;
        *(buffer1 + index) = 2000;
        *(buffer2 + index) = 2000;
        Serial.println("buffer corrected at index ");
        Serial.println(index);
    }
}
//shifts the buffer to the next frame
void vl5helper::readandshiftbuffer(int *ptr)
{
    buffer2 = buffer1;
    buffer1 = buffer0;
    buffer0 = ptr;
}
//we wirte the 1 and 0 flags into flagobject we could have float values for more accurate results
//@param *ptr the 8by8 matrix of mm distance values
//@param threshold if mm passed on distance smaller  threshold 1 elese it  gets 0 
int *vl5helper::updateOccupancy(int *ptr, int threshold) // pass <1 as threshold to use defautl value of 1500
{                                                        // this function get the first parameter of the data and updates occupancy
    /*** Declare local variables *************************************************/
    // int sensorFlag = 0; // Flag which show if area is occupied with an object
    int distance = 0; // Temporarly variable to read data of distValues
    static int returnmat[65];
    /*** Calculate reference value in order to compare it with measured value ****/
    if (threshold < 1)
    {
        threshold = 1800; // mountingHigh - threshold;
    }
    readandshiftbuffer(ptr);           // with this function we have a buffer of 3 frames of the real distance data wich we can compore for stuck values (next line)
    correctStuckBuffer(isDataStuck()); // this is here to filter stuck data, data happens from time to time and pads get stuck at low values but they would recover by themselves after a second but this way it only takes 3 frames (300ms)

    /*** Read data of stored variable "distValues" and derive if flag set ********/
    for (int y = 0; y <= imageWidth * (imageWidth - 1); y += imageWidth)
    {
        for (int x = imageWidth - 1; x >= 0; x--)
        {
            distance = (*(ptr + x + y));
            if (distance <= threshold && !(distance == 0)) // sometimes data gets stuck at 0
            {                                              // there has to be a filter to filter stuck reading errors in the future

                returnmat[x + y] = 1;
                flagobject[x + y] = 1;
            }
            else
            {

                returnmat[x + y] = 0;
                flagobject[x + y] = 0;
            }
        }
    }
    return &returnmat[0];
    // groupflagged has to be called next to continue
}

//this groups  the 1 that are connected to another (not diagonally  together) together with the help of checksurroundingsrecursiv(flagobject,pos);  
PosXYandSum vl5helper::groupflagged(void) // the pointer is the flagobject pointer
{

    // printmatrix(&checksurroundings[0],8);

    for (int y = 0; y <= imageWidth * (imageWidth - 1); y += imageWidth)
    {
        for (int x = imageWidth - 1; x >= 0; x--)
        {

            if (flagobject[x + y] == 1)
            {
                // int flagoblect == 1;//go into recursive function now and exit it after there are no more undetected other 1 connected

                // Serial.print("starting the recursive function");
                checksurroundingrecursive(&flagobject[0], x + y);
                PosXYandSum temp;
                // Serial.print("flagobject");
                // printmatrix(flagobject, 8);
                // Serial.print("checkedsurroundings");
                // printmatrix(checksurroundings, 8);
                temp = returnpersonofchecksurroundings();
                return temp;
            }
        }
    }
    PosXYandSum empty;
    empty.used = false;
    empty.pastsimplepos[0] = PosXYandSum::simplepos::undefined;//we return somthing with in undefined simplepos
    return empty;
}

// changes the connected 1 to 2s and exits after being done!
//@param *prtflag this points to the 1s and 0s so the flagged matrix
void vl5helper::checksurroundingrecursive(int *ptrflag, int pos) // if int *ptr gets passed 0 we use the default flagobject matrix
{                                                                // scan surroundings first and mark them with not checked
                                                                 // return checksurroundigs
    //  2:checked all surroundings. 1 there is a one but surroundings not checked. 0nothing present
    /*0 0 1 0 1 0
      0 0 1 1 1 0
      0 0 0 1 0 0
      0 0 0 0 0 0
      1 1 0 0 0 0
      the algorith should understand that this is a group and remove after groupings
      in the end there should be
      0 0 2 0 2 0
      0 0 2 2 2 0
      0 0 0 2 0 0
      0 0 0 0 0 0
      0 0 0 0 0 0
      left.
      we can build the average of the location of the connected parts.
      In the future there could be other values than 1 for differing threshold than 1.
      We could use the sum of those to do a approximation of the irl size of the object
*/

    checksurroundings[pos] = 2;

    if (flagobject[retuppos(pos)] && checksurroundings[retuppos(pos)] == 0)
    { // it is occupied and hasnt been checked
        checksurroundings[retuppos(pos)] = 1;
    }
    if (flagobject[retdownpos(pos)] && checksurroundings[retdownpos(pos)] == 0)
    {
        checksurroundings[retdownpos(pos)] = 1;
    }
    if (flagobject[retrightpos(pos)] && checksurroundings[retrightpos(pos)] == 0)
    {
        checksurroundings[retrightpos(pos)] = 1;
    }
    if (flagobject[retleftpos(pos)] && checksurroundings[retleftpos(pos)] == 0)
    {
        checksurroundings[retleftpos(pos)] = 1;
    }
    // this means pos is now status 2 checked
    // printmatrix(&checksurroundings[0],8);

    // now we have to go to a 1 of checksurroundings//thats enough for now// that is a 1 in of flagobject but its surroundings havent been checked for other 1s
    for (int i = 0; i < 64; i++)
    {
        if (checksurroundings[i] == 1)
        {
            checksurroundingrecursive(&flagobject[0], i); // if there is no 1 left that means we can break the recursive function and it doesnt even get called
            /*
             * this has to be done like this bc it is not always next to it
             * 2 0 1
             * 2 2 2
             *   1
             */
        } // if therer is no 1 in checksurroundings array this means it is done and it will not be recuasive
    }
}

// the next following four functins just return the correct position according to the diagram
// matrix
// x is flipped see  for (int x = imageWidth - 1 ; x >= 0 ; x--)

//
// 0...7
// 8...15
//..
//..
// 55...63
// the following functions will retrn 64 if it the squre would be otherwise out of bounds
// flagobject[64] will be 0 and should not matter

//@return int of the upper coordinate
int vl5helper::retuppos(int pos)
{
    if (pos > 55)
    {              // no upper coordinate
        return 64; // will point to an empty square
    }
    else
    {
        return pos + 8;
    }
}
int vl5helper::retdownpos(int pos)
{
    if (pos < 8)
    {              // no upper coordinate
        return 64; // will point to an empty square
    }
    else
    {
        return pos - 8;
    }
}
int vl5helper::retrightpos(int pos)
{
    if ((pos + 1) % 8 == 0) // 7 15 and so on
    {
        return 64; // will point to an empty square
    }
    else
    {
        return pos + 1;
    }
}
int vl5helper::retleftpos(int pos)
{
    if (pos % 8 == 0)
    {
        return 64; // will point to an empty square
    }
    else
    {
        return pos - 1;
    }
}
//processes the stuff that is in the checksurroundings array and we to get a 
//@return PosXYandSum that we get of the data in checkedsurroundis
PosXYandSum vl5helper::returnpersonofchecksurroundings(void)
{ // recursive function is done now write is to f[people] or return it as a posXYandSum
    // ptr has to be a matix of 0 and 2 , 2 there where the person got detected

    int tempx = 0;
    int tempy = 0;

    // will be using globalcouter for averages   it is the same as sumofpeople now bc we always add one, advisid to be changed in the future
    float tsum = 0;
    int numberOfflags = 0;
    for (int i = 0; i < 64; i++) // x and y averages
    {
        if (checksurroundings[i] == 2)
        {
            numberOfflags++; // nubers flags in one person
            checksurroundings[i] = 0;
            flagobject[i] = 0;

            tsum = tsum + 1; // here it is just +1 but this can be anything according to your preference

            tempx = tempx + i % 8;
            tempy = tempy + (int)i / 8; // whole real number
        }
    }

    float tempxf = (float)tempx / numberOfflags;
    float tempyf = (float)tempy / numberOfflags;

    PosXYandSum temp;
    temp.posx = tempxf;
    temp.sum = tsum;
    temp.posy = tempyf;
    temp.used = true;
    return temp;
}
//struct will be is/will be deprecated and we use the simplified way  with PosXYandSum?
//this is the complicated way and it is/will be deprecated?
//WIP
void vl5helper::trackpeolpe()//filtering out what might by bad data //testing needed
{
    for (int i = 0; i < prevpeople[0]; i++)
    {
        if ((persons[i].posy[0] - persons[i].posy[1]) * (persons[i].posy[0] - persons[i].posy[1]) > 4 || (persons[i].posx[0] - persons[i].posx[1]) * (persons[i].posx[0] - persons[i].posx[1]) > 4)
        { // that means that the y or x position changed by more than 2 ((change)^2>4) and is most likely flawed and the index changed//that would be 7.14 m/s walking but humans might just walk with a speed of 1.5m/s
            Serial.print("Index changed and we Discard the Data!!!!!!!!!!!");
            temp[i].pastsimplepos[1] = PosXYandSum::simplepos::undefined;
        }
    }
}
#define middleY 3.5                 // (8-1)/2 so 0-3.5 || 3.5-7 gets devided into 2 sections
#define edgeX 2.33                  // (8-1)/3 so 0-2.33 || 2.33-5.66 || 5.66-7  gets devided into 3 sections
int vl5helper::simplifiedtracking() // the current passed on data is asseccable liken this:(passedon+i)->used . Note: i is a value between 0 to current number of peolpe aka prevpeople[0]
{
    // this is not an analytical method
    //  writing data corretly:
//filtering out data with trackpeople
trackpeolpe();

    for (int i = 0; i < prevpeople[0]; i++)
    {

        if (temp[i].posy < middleY && temp[i].posx < edgeX)
        {
            // lefttop
            temp[i].pastsimplepos[0] = PosXYandSum::simplepos::lefttop;
        }
        if (temp[i].posy > middleY && temp[i].posx < edgeX)
        {
            // leftbottom
            temp[i].pastsimplepos[0] = PosXYandSum::simplepos::leftbottom;
        }
        if (temp[i].posy < middleY && temp[i].posx > 2 * edgeX)
        {
            // righttop
            temp[i].pastsimplepos[0] = PosXYandSum::simplepos::righttop;
        }
        if (temp[i].posy > middleY && temp[i].posx > 2 * edgeX)
        {
            // rightbottom
            temp[i].pastsimplepos[0] = PosXYandSum::simplepos::rightbottom;
        }
        if (temp[i].posy < middleY && temp[i].posx > edgeX && temp[i].posx < 2 * edgeX)
        {
            // centertop
            temp[i].pastsimplepos[0] = PosXYandSum::simplepos::centertop;
        }
        if (temp[i].posy > middleY && temp[i].posx > edgeX && temp[i].posx < 2 * edgeX)
        {
            // centerbottom
            temp[i].pastsimplepos[0] = PosXYandSum::simplepos::centerbottom;
        }
        Serial.print(" simplepos[0] ");
        temp[i].displaySimplepos((temp[i].pastsimplepos[0])); // show the current simplepos
        Serial.print(" simplepos[1] ");
        temp[i].displaySimplepos((temp[i].pastsimplepos[1])); // show the last simplepos
        Serial.println(" ");
        // Serial.println("");
        // Serial.print("last edit of it");
        // Serial.print(temp[i].timelastdetected);
        if (temp[i].timelastdetected + 1000 < millis()&&temp[i].timelastdetected!=0)
        { // if the last write occured more than a second prior we undefine pastsimplepos[1] so we won't acciddentally get a wrong pass through
        //there had to be a last write and timelastdetected has to be defined 
            temp[i].pastsimplepos[1] = PosXYandSum::simplepos::undefined;
            Serial.printf("resetted temp[%d].pastsimplepas[1] after 1s of not being in use", i);
            Serial.printf("\t time is now %u and  the last time edited is %u \n",millis(),temp[i].timelastdetected);
        }
        temp[i].timelastdetected = millis();
    }
    // now to the important code that says when people get out or not
    int forreturn = 0;
    // easier to read:
    for (int i = 0; i < prevpeople[0]; i++)
    {
        // bottom to top
        if (temp[i].pastsimplepos[0] == PosXYandSum::simplepos::lefttop && temp[i].pastsimplepos[1] == PosXYandSum::simplepos::leftbottom)
        {
            Einsteiger = Einsteiger + 1;
            Serial.println("went out left top ");
            forreturn = 1;
        }
        if (temp[i].pastsimplepos[0] == PosXYandSum::simplepos::righttop && temp[i].pastsimplepos[1] == PosXYandSum::simplepos::rightbottom)
        {
            Einsteiger = Einsteiger + 1;
            Serial.println("went out right top ");
            forreturn = 1;
        }
        if (temp[i].pastsimplepos[0] == PosXYandSum::simplepos::centertop && temp[i].pastsimplepos[1] == PosXYandSum::simplepos::centerbottom)
        {
            Einsteiger = Einsteiger + 1;
            Serial.println("went out center top ");
            forreturn = 1;
        }

        // top to bottom
        if (temp[i].pastsimplepos[1] == PosXYandSum::simplepos::lefttop && temp[i].pastsimplepos[0] == PosXYandSum::simplepos::leftbottom)
        {

            Aussteiger = Aussteiger + 1;
            Serial.println("went out left bottom ");
            forreturn = -1;
        }
        if (temp[i].pastsimplepos[1] == PosXYandSum::simplepos::righttop && temp[i].pastsimplepos[0] == PosXYandSum::simplepos::rightbottom)
        {
            Aussteiger = Aussteiger + 1;
            Serial.println("went out right bottom ");
            forreturn = -1;
        }
        if (temp[i].pastsimplepos[1] == PosXYandSum::simplepos::centertop && temp[i].pastsimplepos[0] == PosXYandSum::simplepos::centerbottom)
        {
            Aussteiger = Aussteiger + 1;
            Serial.println("went out center bottom ");
            forreturn = -1;
        }
    }

    for (int i = 0; i < prevpeople[0]; i++)
    {

        temp[i].pastsimplepos[1] = temp[i].pastsimplepos[0]; // pass on the last simeplepos in time
        Serial.print("we passed on the current simplepos to pastsimplepos[1] it is now:");
        temp[i].displaySimplepos(temp[i].pastsimplepos[1]);
    }
    if(forreturn!=0){
        Serial.printf("returning %d",forreturn);
    }
    return forreturn;
}
//when using eeprom this can becalled to really reset the data of einsteiger and ausseiger in the eeprom
// ressetting won't clear the data in the eeprom 
void vl5helper::resetEEPROM() // our einsteiger und Ausseiger will be resetted. If this is not called we will use the value that is in the  eeprom
{
    writetoeeprom(0, 0);
}
//struct will be is/will be deprecated and we use the simplified way  with PosXYandSum?
void vl5helper::lowestdeltasum()
{
    float sumofsums;
    for (int i = 0; i < prevpeople[0]; i++)
    {
        sumofsums = +persons[i].delta;
    }
}

// A function to implement bubblesort
void vl5helper::bubbleSort(float arr[], int n) // pass on ann array and its the size until it should be sorted n
{
    int i, j;
    float tempf;
    for (i = 0; i < n - 1; i++)

        // Last i elements are already in place
        for (j = 0; j < n - i - 1; j++)
            if (arr[j] > arr[j + 1])
            {
                tempf = arr[j];
                arr[j] = arr[j + 1];
                arr[j + 1] = tempf;
            }
}

//struct will be is/will be deprecated and we use the simplified way  with PosXYandSum?
//this is here to show the data in the presons
void vl5helper::displaydata(int p)
{ // this only shows the current and last x ,y and sum of the current people
    if (p == 0)
    {

        if (prevpeople[0] > 1)
        {
            Serial.print("The predictions made are just an estimate and the index isn't taken into consideration\n this means the precition is likely flawed\n");
            Serial.println("\n");

            Serial.print("The data of our ");
            Serial.print(prevpeople[0]);
            Serial.print(" current people with 0 indexing is: \n");
            for (int i = 0; i < prevpeople[0]; i++)
            {
                Serial.print("person:");
                Serial.println(i);
                Serial.print("the sum ");
                Serial.print(persons[i].sum[0]);
                Serial.print("the sum[1] ");
                Serial.println(persons[i].sum[1]);
                Serial.print("the xpos[0]:");
                Serial.print(persons[i].posx[0]);
                Serial.print(" the xpos[1]:");
                Serial.print(persons[i].posx[1]);
                Serial.print(" the next xpos might be: ");
                float xpos = 2 * persons[i].posx[0] - persons[i].posx[1];
                Serial.print(xpos);

                Serial.println(" this is wrong if it wasn't threr perviously resettet ");

                Serial.print("the ypos[0]:");
                Serial.print(persons[i].posy[0]);
                Serial.print(" the ypos[1]:");
                Serial.print(persons[i].posy[1]);
                Serial.print(" the next ypos might be: ");
                float ypos = 2 * persons[i].posy[0] - persons[i].posy[1];
                Serial.print(ypos);

                Serial.println(" this is wrong if it wasn't threr perviously resettet ");
            }
        }
    }
    else
    {
        // showing all the data
        for (int i = 0; i < p; i++)
        {
            Serial.print("person:");
            Serial.println(i);
            Serial.print("the sum ");
            Serial.print(persons[i].sum[0]);
            Serial.print("the sum[1] ");
            Serial.println(persons[i].sum[1]);
            Serial.print("the xpos[0]:");
            Serial.print(persons[i].posx[0]);
            Serial.print(" the xpos[1]:");
            Serial.print(persons[i].posx[1]);
            Serial.print(" the xpos[2]:");
            Serial.print(persons[i].posx[2]);
            Serial.print(" the next xpos might be: ");
            float xpos = 2 * persons[i].posx[0] - persons[i].posx[1];
            Serial.print(xpos);

            Serial.println(" this is wrong if it wasn't threr perviously resettet ");

            Serial.print("the ypos[0]:");
            Serial.print(persons[i].posy[0]);
            Serial.print(" the ypos[1]:");
            Serial.print(persons[i].posy[1]);
            Serial.print(" the ypos[2]:");
            Serial.print(persons[i].posy[2]);
            Serial.print(" the next ypos might be: ");
            float ypos = 2 * persons[i].posy[0] - persons[i].posy[1];
            Serial.print(ypos);

            Serial.println(" this is wrong if it wasn't threr perviously resettet ");
        }
    }
}
//struct will be is/will be deprecated and we use the simplified way  with PosXYandSum?
//reset the person with index per
void vl5helper::resetcurrentpersons(int per)
{ // this resets persons[i].posx[0] .sum[0] and .posy[0] to -1
    persons[per].changedindex = false;
    persons[per].isused = false;
    persons[per].posx[0] = 100;
    persons[per].posy[0] = 100;
    persons[per].sum[0] = 0;
    // persons[per].isused=false;
}
//struct will be is/will be deprecated and we use the simplified way  with PosXYandSum?
void vl5helper::swapdata(int timeframe, int personsn, int personsm)
{ // this is not useful you can just do persons[i]=persons[k];
    float tempsum, tempx, tempy;
    tempsum = persons[personsn].sum[timeframe];
    tempx = persons[personsn].posy[timeframe];
    tempy = persons[personsn].posx[timeframe];
    persons[personsn].sum[timeframe] = persons[personsm].sum[timeframe];
    persons[personsn].posy[timeframe] = persons[personsm].posy[timeframe];
    persons[personsn].posx[timeframe] = persons[personsm].posx[timeframe];
    persons[personsm].sum[timeframe] = tempsum;
    persons[personsm].posy[timeframe] = tempy;
    persons[personsm].posx[timeframe] = tempx;
}
//struct will be is/will be deprecated and we use the simplified way  with PosXYandSum?
void vl5helper::copydata(int framecopiedto, int framecopiedfrom, int ptocopieto, int ptocopiefrom)
{
    // we can also do person[i]=person[k] but that would be for all timeframes wich is not always needed
    persons[ptocopieto].posy[framecopiedto] = persons[ptocopiefrom].posy[framecopiedfrom];
    persons[ptocopieto].posx[framecopiedto] = persons[ptocopiefrom].posx[framecopiedfrom];
    persons[ptocopieto].sum[framecopiedto] = persons[ptocopiefrom].sum[framecopiedfrom];
}
//struct will be is/will be deprecated and we use the simplified way  with PosXYandSum?
void vl5helper::shiftdataintime()
{
    for (int people = 0; people < 5; people++)
    {
        copydata(3, 2, people, people);
        copydata(2, 1, people, people);
        copydata(1, 0, people, people);
    }
}
//struct will be is/will be deprecated and we use the simplified way  with PosXYandSum?
void vl5helper::shiftdatainpeople(int d)
{
    if (d == 1)
    {
        for (int i = 1; i < 3; i++)
        {
            for (int j = 0; j < 4; j++)
            {
                copydata(i, i, j, j + 1);
            }
        }
    }
    if (d == -1)
    {
        for (int i = 1; i < 3; i++)
        {
            for (int j = 4; j < 0; j--)
            {
                copydata(i, i, j, j - 1);
            }
        }
    }
}
