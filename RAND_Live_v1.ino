#include <SD.h>
#include <SPI.h>
#include "TeensyThreads.h"
#include <Entropy.h>
int inputNum;
int dataPos;
int devDelay;

File dataFile;
File devFile;

const int chipSelect = BUILTIN_SDCARD;

int pinMatrix[5][4] = {
  {17, 0, 28, 39},
  {20, 1, 29, 38},
  {21, 2, 30, 37},
  {22, 3, 31, 36},
  {23, 4, 32, 34}
};

void setup() {
  Entropy.Initialize();

  for (int a = 0; a < 5; a++) {
    for (int b = 0; b < 4; b++) {
      pinMode(pinMatrix[a][b], OUTPUT);
    }
  }

  pinMode(33, INPUT_PULLDOWN);
  pinMode(35, INPUT_PULLDOWN);
  delay(100);
  Serial.begin(115200);

  Serial.print("Initializing SD card...");
  if (!SD.begin(chipSelect)) {
    Serial.println("Initialization failed!");
    return;
  }
  Serial.println("Initialization done.");

  int sensorValue1 = digitalRead(35);
  int sensorValue2 = digitalRead(33);
  Serial.println(sensorValue1);
  Serial.println(sensorValue2);

  dataFile = SD.open("digitsR2.txt", FILE_READ);
  Serial.println(dataFile.size());
  Serial.println();
  devFile = SD.open("devR2.txt", FILE_READ);
  Serial.println(devFile.size());
  int id1, id2, id3, id4, id5; //thread ID numbers

  if (sensorValue1 == 0 && sensorValue2 == 0) {
    id2 = threads.addThread(randompage, Entropy.random(0, 5));
  } else if (sensorValue1 == 1 && sensorValue2 == 0) {
    id1 = threads.addThread(readbook, 0);
    id2 = threads.addThread(readbook, 1);
    id3 = threads.addThread(readbook, 2);
    id4 = threads.addThread(readbook, 3);
    id5 = threads.addThread(readbook, 4);
  }
  else if (sensorValue1 == 0 && sensorValue2 == 1) {
    id1 = threads.addThread(randpage, Entropy.random(0, 5));
  }
}

void readbook(int lCount) {
  //reading the book sequentially, one random digit at a time
  //concurrently in lines of five
  //sequential binary output
  volatile int lineCount = lCount + (lCount * 50);
  int columnCount = 0;
  char inputChar;
  while (dataFile.available()) {
    while (inputChar != '\r' && lineCount < 1020000) {
      int data = (lineCount + columnCount);
      dataFile.seek(data);
      inputChar = dataFile.read();
      //convert and read integer as binary
      uint8_t bitsCount = sizeof( inputChar ) * 4;
      char nibble[ bitsCount + 1 ];
      uint8_t i = 0;
      while ( bitsCount-- )
        nibble[ i++ ] = bitRead( inputChar, bitsCount ) + '0';
      nibble[ i ] = '\0';
      String fourBits = nibble;

      //read the binary string and seperate across relays
      for (int h = 3; h > -1; h--) {
        String binaryDigit = String(fourBits.charAt(h));
        if (binaryDigit == 1) {
          digitalWrite(pinMatrix[lCount][h], HIGH);
        }
        else digitalWrite(pinMatrix[lCount][h], LOW);
        threads.delay(104); //suggested for delays within a thread
        digitalWrite(pinMatrix[lCount][h], LOW);
      }

      Serial.print(columnCount);
      Serial.print(":");
      Serial.print(lineCount);
      Serial.print(": ");
      Serial.print(inputChar);
      Serial.println();
      columnCount = columnCount + 1;
      if (inputChar == '\r') {
        columnCount = 0;
        lineCount = lineCount + 255;
      }
      inputChar = ' ';
    }
  }
}

void randompage(int lCount) {
  //selecting a random number and using a fixed tempo
  //serial binary output
  volatile int lineCount = lCount;
  int columnCount = Entropy.random(50);
  char inputChar;
  while (dataFile.available()) {
    int data = (lineCount + columnCount);
    dataFile.seek(data);
    inputChar = dataFile.read();

    uint8_t bitsCount = sizeof( inputChar ) * 4;
    char nibble[ bitsCount + 1 ];
    uint8_t i = 0;
    while ( bitsCount-- )
      nibble[ i++ ] = bitRead( inputChar, bitsCount ) + '0';
    nibble[ i ] = '\0';
    String fourBits = nibble;

    for (int h = 3; h > -1; h--) {
      String binaryDigit = String(fourBits.charAt(h));
      if (binaryDigit == 1) {
        digitalWrite(pinMatrix[lCount][h], HIGH);
      }
      else digitalWrite(pinMatrix[lCount][h], LOW);
      delay(104);
      digitalWrite(pinMatrix[lCount][h], LOW);
    }

    Serial.print(lCount);
    Serial.print(":");
    Serial.print(columnCount);
    Serial.print(":");
    Serial.print(lineCount);
    Serial.print(": ");
    Serial.write(inputChar);
    Serial.println();
    //data count will be >1M due to each lines EOL char
    //therefore 1,020,000
    lCount = Entropy.random(0, 5);
    lineCount = ((lCount + (lCount * 50)) + ((255 * Entropy.random(4001))));
    columnCount = Entropy.random(50);
    inputChar = ' ';
  }
}

void randpage(int lCount) {
  //selecting a random number based on the book's instructions
  //serial binary output
  char inputChar;
  while (dataFile.available()) {
    Serial.print("Start: ");
    Serial.println(millis());
    randDataStart(lCount);
    randDevStart(lCount);
    Serial.print("Dev Delay: ");
    Serial.println(devDelay);
    Serial.print(dataPos);
    Serial.println();
    dataFile.seek(dataPos);
    inputChar = dataFile.read();
    Serial.print("Input Char: ");
    Serial.println(inputChar);
    uint8_t bitsCount = sizeof( inputChar ) * 4;
    char nibble[ bitsCount + 1 ];
    uint8_t i = 0;
    while ( bitsCount-- )
      nibble[ i++ ] = bitRead( inputChar, bitsCount ) + '0';
    nibble[ i ] = '\0';
    String fourBits = nibble;
    Serial.print("Stop: ");
    Serial.println(millis());
    for (int h = 3; h > -1; h--) {
      String binaryDigit = String(fourBits.charAt(h));
      if (binaryDigit == 1) {
        digitalWrite(pinMatrix[lCount][h], HIGH);
      }
      else digitalWrite(pinMatrix[lCount][h], LOW);
      delay(devDelay);
      digitalWrite(pinMatrix[lCount][h], LOW);
    }
    //data count will be >1M due to each lines EOL char
    //therefore 1,020,000
    lCount = Entropy.random(0, 5);
    inputChar = ' ';
  }
}

int randDataStart(int lCount) {
  //selecting a random number based on the book's instructions
  //serial binary output
  char rowRead;
  char colRead;

  String rowNumber;
  int colNumber = 0;
  volatile int dataRowStart = ((lCount + (lCount * 50)) + ((255 * Entropy.random(4001))));
  int dataColStart = 5 * (Entropy.random(10));
  int data = (dataRowStart + dataColStart);
  for (int x = 0; x < 5; x++) {
    dataFile.seek(data + x);
    rowRead = dataFile.read();
    rowNumber += rowRead;
  }
  for (int y = 0; y < 2; y++) {
    dataFile.seek(data + 5);
    colRead = dataFile.read();
    colNumber += colRead;
  }
  colNumber = colNumber % 50;

  int z = rowNumber.charAt(0) % 2;
  char modNumber;
  if (z == 0) {
    modNumber = '0';
  } else modNumber = '1';
  rowNumber.setCharAt(0, modNumber);
  Serial.print(rowNumber);
  Serial.print(", ");
  Serial.println(colNumber);
  dataPos = (rowNumber.toInt() + colNumber);
  Serial.println(dataPos);
  Serial.println();
  return dataPos;
}

int randDevStart(int lCount) {
  //selecting a random number based on the book's instructions
  //serial binary output
  String tblNumber;
  char readNbr;
  int devColumnNbr;
  String devLineNbr;
  // int devDelay;
  volatile int dataStart = ((lCount + (lCount * 50)) + ((255 * Entropy.random(4001))));
  for (int x = 0; x < 5; x++) {
    dataFile.seek(dataStart + x);
    readNbr = dataFile.read();
    tblNumber += readNbr;
  }
  devColumnNbr = tblNumber.charAt(4);
  devLineNbr = tblNumber.remove(4);

  volatile int devStart = (5 * devLineNbr.toInt() + devColumnNbr);
  devFile.seek(devStart);
  devDelay = devFile.parseInt();
  Serial.print("Dev delay: ");
  Serial.println(devDelay);
  return devDelay;
}

void loop() {
  // put your main code here, to run repeatedly:

}
