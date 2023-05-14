
#include <Servo.h>    // Servo motor kütüphanesi projeye dahil edildi//
#include <AFMotor.h>  // Adafruit Motor Shield kütüphanesi projeye dahil edildi// 

#define LINE_BUFFER_LENGTH 1024 // Makro tanımlandı ve 1024 değeri atandı. 1024 karakter uzunluğunda bir satır tamponunu temsil ediyor.
                                //Bu tampon, seri iletişim arayüzünden alınan karakterleri depolamak veya işlemek için kullanılabilir.


char STEP = MICROSTEP ; // STEP MOTORLARIN ÇALIŞMA ŞEKLİ = MICROSTEP ;

 
const int penZUp =55;  // kalemin yukardaki açısı
const int penZDown = 27;  // kalemin aşağıdaki açısı

// Servo PWM  10.pin
const int penServoPin =10 ; // Servo motorun bağlı olduğu pin numarası.

const int stepsPerRevolution = 20; // Step motorlarının bir tam devir için kaç adım attığını belirtir


Servo penServo;  // Kalem kontrolu için servo motoru tanımlandı

// L293D H-köprüsü için bu Arduino pinlerini kullanarak X ve Y ekseni için adımları başlat
AF_Stepper myStepperY(stepsPerRevolution,1);            
AF_Stepper myStepperX(stepsPerRevolution,2);  

/* Genel değişkenler    */
struct point { 
  float x; 
  float y; 
  float z; 
};

// Geçerli konum
struct point actuatorPos;

//  Çizim ayarları
float StepInc = 1; // Adım miktarını belirleyen bir kayan nokta sayısıdır. Bu değer, çizgi çizme işlemi sırasında her adımda ne kadar hareket edileceğini belirler.

int StepDelay = 1; // Adım arası gecikmeyi belirleyen bir tamsayıdır. Bu değer, adım motorlarının her bir adımdan sonra bekleyeceği süreyi belirler.

int LineDelay =0;  // Satır arası gecikmeyi belirleyen bir tamsayıdır. Bu değer, iki farklı satırın çizimi arasında bekleyeceği süreyi belirler.

int penDelay = 50; //  Kalemin yukarıya veya aşağıya hareket etmesi için gerekli gecikmeyi belirleyen bir tamsayıdır.


float StepsPerMillimeterX = 108;  // x eksenindeki adım motorunun her bir milimetreye karşılık gelen adım sayısını belirtir.
float StepsPerMillimeterY = 108;  // y eksenindeki adım motorunun her bir milimetreye karşılık gelen adım sayısını belirtir.

// Çizim alanı ölçüleri buraya yazılmalı,
// Dvd mekanizmaları 35 mm ile 45mm arasında hareket alanı sağlamaktadır.
 
float Xmin = 0;
float Xmax = 35;
float Ymin = 0;
float Ymax = 35;
float Zmin = 0;
float Zmax = 1;

float Xpos = Xmin;
float Ypos = Ymin;
float Zpos = Zmax; 

boolean verbose = false;




void setup() {
  //  Setup
  
  Serial.begin( 9600 ); //seri haberleşme hızını 9600 bps (bit per saniye) olarak ayarlayarak seri haberleşmeyi başlattır.
  
  penServo.attach(penServoPin); // servo motorunu belirtilen pime bağlamak için kullanılır.
  penServo.write(penZDown);     // servo motoruna açı değerini göndererek kalem durumunu kontrol eder.
  delay(100);                   //100 milisaniye (0.1 saniye) gecikme yapar

  //motor hız ayarı 
  myStepperX.setSpeed(200);

  myStepperY.setSpeed(200);  

 
  //  Bildirimler
  Serial.print("X ekseni aralığı "); 
  Serial.print(Xmin); 
  Serial.print(" to "); 
  Serial.print(Xmax); 
  Serial.println(" mm."); 
  Serial.print("y ekseni aralığı "); 
  Serial.print(Ymin); 
  Serial.print(" to "); 
  Serial.print(Ymax); 
  Serial.println(" mm."); 
}

/**********************
 * void loop() - ana döngü
 ***********************/
void loop() 
{
  
  delay(100);
  char line[ LINE_BUFFER_LENGTH ]; // LINE_BUFFER_LENGTH değişkeni, line dizisinin maksimum uzunluğunu belirtir.
  char c;                          // c adında bir karakter değişkeni ve lineIndex adında bir tamsayı değişkeni tanımlanır. lineIndex, mevcut satırın dizideki indeksini tutar.
  int lineIndex;                   
  bool lineIsComment, lineSemiColon; 

  lineIndex = 0;
  lineSemiColon = false; 
  lineIsComment = false; //lineIndex değişkenini sıfırlar, lineSemiColon değişkenini false olarak ayarlar ve lineIsComment değişkenini false olarak ayarlar.

  while (1) {

    // Seri alım - Çoğunlukla Grbl'den, noktalı virgül desteği eklendi
    while ( Serial.available()>0 ) {
      c = Serial.read();
      if (( c == '\n') || (c == '\r') ) {             // Eksen sonuna ulaşıldı
        if ( lineIndex > 0 ) {                        // Hat tamamlandı. Sonra çalıştırın!
          line[ lineIndex ] = '\0';                   // Dize sonlandır
          if (verbose) { 
            Serial.print( "Received : "); 
            Serial.println( line ); 
          }
          processIncomingLine( line, lineIndex );
          lineIndex = 0;
        } 
        else { 
          // Satırı boşaltın veya yorum yapın. Blok atla.
        }
        lineIsComment = false;
        lineSemiColon = false;
        Serial.println("ok");    
      } 
      else {
        if ( (lineIsComment) || (lineSemiColon) ) {   // Tüm yorum karakterlerini atın
          if ( c == ')' )  lineIsComment = false;     // Yorumun sonu. Hattına devam et.
        } 
        else {
          if ( c <= ' ' ) {                           // Boşlukları  ve karakterleri kontrol edin
          } 
          else if ( c == '/' ) {                    // Blok silme desteklenmiyor. Karakteri yoksay.
          } 
          else if ( c == '(' ) {                    // Yorum işaretleme özelliğini etkinleştirin ve ')' veya EOL'ye kadar tüm karakterleri yok sayın.
            lineIsComment = true;
          } 
          else if ( c == ';' ) {
            lineSemiColon = true;
          } 
          else if ( lineIndex >= LINE_BUFFER_LENGTH-1 ) {
            Serial.println( "ERROR - lineBuffer overflow" );
            lineIsComment = false;
            lineSemiColon = false;
          } 
          else if ( c >= 'a' && c <= 'z' ) {        // küçük harf
            line[ lineIndex++ ] = c-'a'+'A';
          } 
          else {
            line[ lineIndex++ ] = c;
          }
        }
      }
    }
  }
}
//sürekli bir döngü içinde çalışır ve seri haberleşme üzerinden gelen verileri işler. Döngü, Serial.available() fonksiyonunu kullanarak seri bağlantı üzerindeki gelen veri olup olmadığını kontrol eder.

void processIncomingLine( char* line, int charNB ) {
  int currentIndex = 0;
  char buffer[ 64 ];                                 
  struct point newPos;

  newPos.x = 0.0;
  newPos.y = 0.0;

  

  while( currentIndex < charNB ) {
    switch ( line[ currentIndex++ ] ) {              // Varsa, komutu seçin
    case 'U':
      penUp(); 
      break;
    case 'D':
      penDown(); 
      break;
    case 'G':
      buffer[0] = line[ currentIndex++ ];          // /! \ Dirty - Yalnızca 2 basamaklı komutlarla çalışır
   
      buffer[1] = '\0';

      switch ( atoi( buffer ) ){                   // G kodlarını seçin
      case 0:                                   // G00 & G01 - Hareket veya hızlı hareket. Burada aynı
      case 1:
        // /!\ Dirty - Suppose that X is before Y
        char* indexX = strchr( line+currentIndex, 'X' );  //Dizgede X / Y pozisyonunu alın (varsa)
        char* indexY = strchr( line+currentIndex, 'Y' );
        if ( indexY <= 0 ) {
          newPos.x = atof( indexX + 1); 
          newPos.y = actuatorPos.y;
        } 
        else if ( indexX <= 0 ) {
          newPos.y = atof( indexY + 1);
          newPos.x = actuatorPos.x;
        } 
        else {
          newPos.y = atof( indexY + 1);
          indexY = '\0';
          newPos.x = atof( indexX + 1);
        }
        drawLine(newPos.x, newPos.y );
        //        Serial.println("ok");
        actuatorPos.x = newPos.x;
        actuatorPos.y = newPos.y;
        break;
      }
      break;
    case 'M':
      buffer[0] = line[ currentIndex++ ];        
      buffer[1] = line[ currentIndex++ ];
      buffer[2] = line[ currentIndex++ ];
      buffer[3] = '\0';
      switch ( atoi( buffer ) ){
      case 300:
        {
          char* indexS = strchr( line+currentIndex, 'S' );
          float Spos = atof( indexS + 1);
          //         Serial.println("ok");
          if (Spos == 30) { 
            penDown(); 
          }
          if (Spos == 50) { 
            penUp(); 
          }
          break;
        }
      case 114:                                // M114 - Pozisyon raporu
        Serial.print( "Absolute position : X = " );
        Serial.print( actuatorPos.x );
        Serial.print( "  -  Y = " );
        Serial.println( actuatorPos.y );
        break;
      default:
        Serial.print( "Command not recognized : M");
        Serial.println( buffer );
      }
    }
  }



}


/*********************************
 * (X0; y0) ile (x1; y1) arasında bir çizgi çizin.
  * int (x1; y1): Başlangıç koordinatları
  * int (x2; y2): Bitiş koordinatları
 **********************************/
void drawLine(float x1, float y1) {

  if (verbose)
  {
    Serial.print("fx1, fy1: ");
    Serial.print(x1);
    Serial.print(",");
    Serial.print(y1);
    Serial.println("");
  }  

  // Talimatları sınırlar içinde yerine getirin
  if (x1 >= Xmax) { 
    x1 = Xmax; 
  }
  if (x1 <= Xmin) { 
    x1 = Xmin; 
  }
  if (y1 >= Ymax) { 
    y1 = Ymax; 
  }
  if (y1 <= Ymin) { 
    y1 = Ymin; 
  }

  if (verbose)
  {
    Serial.print("Xpos, Ypos: ");
    Serial.print(Xpos);
    Serial.print(",");
    Serial.print(Ypos);
    Serial.println("");
  }

  if (verbose)
  {
    Serial.print("x1, y1: ");
    Serial.print(x1);
    Serial.print(",");
    Serial.print(y1);
    Serial.println("");
  }

  // Koordinatları adımlara dönüştürme
  x1 = (int)(x1*StepsPerMillimeterX);
  y1 = (int)(y1*StepsPerMillimeterY);
  float x0 = Xpos;
  float y0 = Ypos;

  //  Koordinatların değişimini bulma
  long dx = abs(x1-x0);
  long dy = abs(y1-y0);
  int sx = x0<x1 ? StepInc : -StepInc;
  int sy = y0<y1 ? StepInc : -StepInc;

  long i;
  long over = 0;

  if (dx > dy) {
    for (i=0; i<dx; ++i) {
      myStepperX.onestep(sx,STEP);
      over+=dy;
      if (over>=dx) {
        over-=dx;
        myStepperY.onestep(sy,STEP);
      }
    delay(StepDelay);
    }
  }
  else {
    for (i=0; i<dy; ++i) {
      myStepperY.onestep(sy,STEP);
      over+=dx;
      if (over>=dy) {
        over-=dy;
        myStepperX.onestep(sx,STEP);
      }
      delay(StepDelay);
    }    
  }

  if (verbose)
  {
    Serial.print("dx, dy:");
    Serial.print(dx);
    Serial.print(",");
    Serial.print(dy);
    Serial.println("");
  }

  if (verbose)
  {
    Serial.print("Going to (");
    Serial.print(x0);
    Serial.print(",");
    Serial.print(y0);
    Serial.println(")");
  }

  // Herhangi bir sonraki satır gönderilmeden önce gecikme
  delay(LineDelay);
 // Pozisyonları güncelle
  Xpos = x1;
  Ypos = y1;
}

//  Kalemi yükseltir
void penUp() { 
  penServo.write(penZUp); 
  delay(penDelay); 
  Zpos=Zmax; 
  digitalWrite(15, LOW);
    digitalWrite(16, HIGH);
  if (verbose) { 
    Serial.println("Pen up!"); 
    
  } 
}
//  Kalemi indirir
void penDown() { 
  penServo.write(penZDown); 
  delay(penDelay); 
  Zpos=Zmin; 
  digitalWrite(15, HIGH);
    digitalWrite(16, LOW);
  if (verbose) { 
    Serial.println("Pen down."); 
    
    
  } 
}
