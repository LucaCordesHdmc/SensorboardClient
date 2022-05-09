
#include <Arduino.h>
/*This struct is being used to do the simplified trakcing algorithmus
*/
struct PosXYandSum // this struct will be used to get the new data in a nice format and to display its contents quickly if needed with .display()
{
    float posx;
    float posy;
    float sum;
    bool used = false;
    unsigned long timelastdetected;
    enum simplepos // see void vl5helper::simplifiedtracking(PosXYandSum *passedon) for the correct usage
    {
        undefined,
        lefttop,
        leftbottom,
        righttop,
        rightbottom,
        centertop,
        centerbottom
    };

    simplepos pastsimplepos[2];               // last frame and now
    // not using the this pointer so we can choose what we really want to display with this function. Just pass it on
    void displaySimplepos(simplepos passedon) 
    {

        switch (passedon)
        {
        case 0:
            Serial.print("undefined");
            break;
        case 1:
            Serial.print("lefttop");
            break;
        case 2:
            Serial.print("leftbottom");
            break;
        case 3:
            Serial.print("righttop");
            break;
        case 4:
            Serial.print("rightbottom");
            break;
        case 5:
            Serial.print("centertop");
            break;
        case 6:
            Serial.print("centerbottom");
            break;
        }
    }
    // this just copys the pos data from one object of PosXYandSum to another. If we would assign it there would be data loss of the remaining attributes in PosXYandSum
    void copypos(PosXYandSum t)
    { 
        this->posx = t.posx;
        this->posy = t.posy;
        this->sum = t.sum;//and the sum data
        this->used = t.used;//and the used data
    }
    //displays the position of this PosXYandSum only if it is indeed in use
    void displayP()
    {
        if (this->used)
        {
            Serial.print("x:");
            Serial.println(this->posx);
            Serial.print("y:");
            Serial.println(this->posy);
        }
    };
};

// the struct person should be used to store the old data (pos x and y and sum if needed)of a person in an array life float posx[].
//this struct can be used to process the data in an analytical way unlike PosXYandSum
struct person 
{
    float posx[5];
    float posy[5];
    float sum[5];
    bool exeting;
    float delta;
    int changedindex;
    bool isused = false;
    //essentially writes into the arrays and shifts them
    void writetoPerson(PosXYandSum passedon)
    {
        this->posx[1] = this->posx[0];
        this->posx[0] = passedon.posx;

        this->posy[1] = this->posy[0];
        this->posy[0] = passedon.posy;

        this->sum[1] = this->sum[0];
        this->sum[0] = passedon.sum;
        this->isused = true;
    }
    //.display() only shows the last 2 frames wich might be enough to get the idea
    void display()
    { 
        Serial.println("now");
        Serial.print("x:");
        Serial.print(this->posx[0]);
        Serial.print("y:");
        Serial.print(this->posy[0]);
        Serial.print("the sum:");
        Serial.print(this->sum[0]);
        Serial.println("a frame in the past");
        Serial.print("x:");
        Serial.print(this->posx[1]);
        Serial.print("y:");
        Serial.print(this->posy[1]);
        Serial.print("the sum:");
        Serial.print(this->sum[1]);
    };
};

class vl5helper
{
public:
    void lowestdeltasum();
    person persons[17];
    PosXYandSum temp[10]; // temporary array of PosXYandSum we can later use in our calculations
    void resetcurrentpersons(int per);

    vl5helper();                                   // constructor, what are useful parameters to give, resolution?
    int flagobject[65];                            // 1=person there, 0 nobody there (depending  on threshold) we could have float values for more accurate results
    int prevpeople[5];                             // prevpeople[0] is the current frames people [1] is the frame before
    int *updateOccupancy(int *ptr, int threshold); // give the first pointer of the data array of the vl5 sensor and the function returns the pointer to the first element in the matrix with the occupancy with 0 and 1s
    // you have to give the threshold if it is below 0 or 0 it get defaulted to 500
    uint16_t Aussteiger;
    uint16_t Einsteiger;
    void displaydata(int p); // displays the data of 0 to p-1 people, If 0 is passed, then it will show the data of the current people in the frame       if negative values are passed onto the functin nothing will be shown

  
    void trackpeolpe();
    int simplifiedtracking();
    // buffer reader just to make sure that there arne't any values stuck that need to be edited out to have a cear and correct representation
    void readandshiftbuffer(int *ptr);
    // should return -1 if there is nothing stuck, but it will return the index of the data that is stuck and needs resetting (softwarewire not sure how to reset it hardwarewise, but it will return the right value eventaully.)
    int isDataStuck(void);
     // resets the values that sit at that index of each buffer to 2m and does nothing more, also prints that a value got reset at the specific index
    void correctStuckBuffer(int index);
    //run is the umbrella function that can be called and calls the needed other functions
    int run(int *ptr, int threshold);
 // returns no of peolep. parameter to give it a flagpeoplematrix if we give  o the global flagpeoplematrix will be used
    PosXYandSum groupflagged(void);


    //EEPROM stuff:

    uint16_t getEinsteigerFromEEPROM(void);
    uint16_t getAussteigerFromEEPROM(void);
    void resetEEPROM(void);
    void writetoeeprom(uint16_t einsteiger, uint16_t ausseiger);

private:

/* we give a matrix with the 1s and 0s representing people we return the matrix but here is just one person with 2 and the other people dont get higlighted at all and have 0
  @param *ptrflag That is the pointer to the flagobjects first element
  @param pas Positoin of the pointer 
  */
    void checksurroundingrecursive(int *ptrflag, int pos); 
    //this processes the data that got created in checksurroundingrecursive
    PosXYandSum returnpersonofchecksurroundings(void);
    // not stable yet:

    void shiftdataintime(void);
    void shiftdatainpeople(int d);

 /*Sortnig
 @param arr[] Array of the floats that shall be sorted
 @param n size of arr[] if all elements should be sorted n=size(arr[])
 */
    void bubbleSort(float arr[], int n);

    void swapdata(int timeframe, int personsn, int personsm);
    //retuns the up position of pos @param pos passed on position 
    int retuppos(int pos);
    //retuns the left position of pos @param pos passed on position
    int retleftpos(int pos);
    //retuns the down position of pos @param pos passed on position
    int retdownpos(int pos);
    //retuns the right position of pos @param pos passed on position
    int retrightpos(int pos);
    //copies the data from one person to the next
    void copydata(int copiedto, int copiedfrom, int ptocopieto, int ptocopiefrom);

    int imageWidth;
    int *buffer0 = new int[64];//this is being used to detect stuck values and we are corretcting those with functions  (readandshiftbuffer isDataStuck correctStuckBuffer)
    int *buffer1 = new int[64];
    int *buffer2 = new int[64];

    int checksurroundings[64];
};
