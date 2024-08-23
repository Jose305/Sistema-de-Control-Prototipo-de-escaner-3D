/*
   Prototipo de escáner 3D
   
   Descripción:
   Este programa controla un prototipo de escáner 3D, gestionando el movimiento de un motor paso a paso,
   la captura de imágenes con una cámara, y la interacción con el usuario mediante una pantalla LCD y un potenciómetro.
 
   Funciones Principales:
    - Control del Motor Paso a Paso: Gira la base del escáner a intervalos precisos para capturar imágenes desde diferentes ángulos.
    - Captura de Imágenes: Envía señales a la cámara para tomar fotografías en diferentes ángulos.
    - Interacción con el Usuario: Muestra información en la pantalla LCD y permite ajustar el brillo con el potenciómetro.

  Librerías Utilizadas:
    - LiquidCrystal: Para controlar la pantalla LCD.
    - Stepper: Para controlar el motor paso a paso.
    - Wire: Para la comunicación I2C.
    - Servo: Para el control de servomotores.

   Variables Globales:
    - MenuNr: Número del menú actual.
    - SwMenu: Submenú seleccionado dentro del menú actual.
    - rolePerMinute: Velocidad del motor en revoluciones por minuto (RPM).
    - TurnNr: Número de turnos que el motor debe realizar.
    - CurrentTurn: Contador de los turnos actuales completados.
    - Steps: Número de pasos del motor.
    - FlagX: Variables de bandera utilizadas para controlar el estado de entrada del joystick.
    - FastChng: Variable para controlar el modo de cambio rápido.
    - SetTime: Tiempo de referencia para el modo de cambio rápido.
    - LongInt, ShortInt: Intervalos de tiempo utilizados en el modo de cambio rápido.
    - BtnCancelFlag, CinCancelFlag: Banderas para cancelar operaciones.
    - PhotoNr: Número de fotos (puede estar relacionado con alguna funcionalidad adicional).
 
   Nota:
    - Asegurarse de que las conexiones del hardware (LCD, joystick, motor paso a paso) estén correctas
    y que las librerías necesarias estén instaladas en el entorno de desarrollo.

*/

// Librerías utilizadas
#include <LiquidCrystal.h>  // Incluye la librería para el LCD estándar (no I2C)
#include <Wire.h>  // Incluye la librería para el I2C del LCD. SCL se conecta al pin A5, SDA se conecta al pin A4. Se necesita un jumper en el pin de retroiluminación LED en la placa I2C.
#include <Stepper.h>  // Incluye la librería para controlar motores paso a paso.
#include <Servo.h>  // Incluye la librería para controlar servos.

LiquidCrystal lcd(1, 2, 4, 5, 6, 7);  // Inicializa el LCD 1602 usando los pines especificados.

const int SW_pin = 8; // Pin digital conectado a la salida del interruptor.
const int X_pin = A0; // Pin analógico conectado a la salida X del joystick.
const int Y_pin = A1; // Pin analógico conectado a la salida Y del joystick.

int MenuNr = 0;   // Número del menú.
int PhotoNr = 2;  // Cantidad de fotos que se deben tomar.
bool Flag1 = 0;   // Bandera activa durante un ciclo de programa para evitar incrementar/decrementar continuamente el número del menú.
bool Flag2 = 0;   // Bandera activa durante un ciclo de programa para evitar incrementar/decrementar continuamente el número de fotos.
bool Flag3 = 0;   // Bandera activa durante un ciclo de programa para evitar incrementar/decrementar continuamente las RPM.
bool Flag4 = 0;   // Bandera activa durante un ciclo de programa para evitar incrementar/decrementar continuamente el número de giros.
bool Flag5 = 0;   // Bandera activa durante un ciclo de programa para evitar incrementar/decrementar continuamente las RPM.
bool Flag6 = 0;   // Bandera activa durante un ciclo de programa para limpiar el LCD.
int SwMenu = 0;   // Menú del interruptor (submenús en los menús principales).
bool BtnFlag = 0; // Bandera activa durante un ciclo de programa para evitar incrementar continuamente SwMenu cuando se presiona el botón.

// Variables añadidas para modos de cambio rápido y cancelación
int FastChng = 0;  // Indica el modo de cambio rápido. 0 = apagado, 1 = modo de demora, 2 = modo de cambio rápido.
const unsigned long FastDelay = 1000;  // Tiempo de demora para el modo rápido (antes de que los valores cambien rápido).
const unsigned long ShortInt = 100;  // Intervalo corto de cambio rápido.
const unsigned long LongInt = 300;  // Intervalo largo de cambio rápido.
const unsigned long BtnDelay = 2000;  // Retardo para la cancelación de operaciones al presionar el botón. Nota: este es un retardo aproximado, ya que el motor paso a paso suspende toda la ejecución del programa hasta que finaliza su movimiento.
unsigned long SetTime = 0; // Valor de tiempo para modos de cambio rápido y cancelación con botón. Usado para calcular intervalos de tiempo.
bool BtnCancelFlag = 0; // Bandera usada para detectar cuando se presiona el botón para cancelar operaciones.
bool MaxSwMenu = 0;  // Bandera usada para detectar cuando se alcanza el máximo SwMenu.
bool CinCancelFlag = 0;  // Bandera usada para activar la cancelación cinematográfica. 1 = cancelar operación cinematográfica.
int StepPoll = 480;  // Número de pasos del motor para sondear la cancelación cinematográfica (a 15 rpm).
int Cntr = 0;  // Contador de pasos para la cancelación del motor cinematográfico.
// Fin de las variables añadidas.

const int stepsPerRevolution = 2000;  // Cambia esto para ajustar el número de pasos por revolución.
int FullRev = 14336;  // 1 revolución completa del engranaje grande -> La relación entre el engranaje pequeño y el grande es de 7:1.
int rolePerMinute = 15;  // Rango ajustable del motor paso a paso 28BYJ-48 es de 0~17 rpm.
int PhotoTaken = 0;  // Cantidad de fotos tomadas.
int StepPerPhoto;  // Cantidad de pasos por foto (calculado -> ver MenuNr 0/SwMenu 2).
int TurnNr = 1;  // Cantidad de giros.
int CurrentTurn = 0;  // Almacena el número de giro actual.
int Steps = 0;  // Cantidad de pasos individuales que el motor paso a paso tiene que girar.

Stepper myStepper(stepsPerRevolution, 9, 11, 10, 12);  // Inicializa el motor paso a paso usando los pines especificados.

Servo Servo1;  // Define Servo1 como un servo.

void setup() {
  lcd.begin(16, 2);                   // Configura el LCD con 16 columnas y 2 filas para la conexión estándar (no I2C).
  pinMode(SW_pin, INPUT);             // Configura el pin del interruptor como entrada.
  digitalWrite(SW_pin, HIGH);         // Establece el pin del interruptor en HIGH (estado alto).

  pinMode(19, OUTPUT);                // Configura el pin 19 como salida.
  digitalWrite(19, HIGH);             // Establece el pin 19 en HIGH (estado alto).
  
  myStepper.setSpeed(rolePerMinute);  // Configura las RPM del motor paso a paso.

  Servo1.attach(3);                   // Conecta el servo al pin 3.
  Servo1.write(90);                   // Mueve el servo a la posición media (90 grados).

  lcd.setCursor(0, 0);                // Configura el cursor del LCD en la posición 0,0 (inicio).
  lcd.print(" ESPE STO. DGO. ");      // Muestra el texto "ESPE STO. DGO." en la primera línea del LCD.
  lcd.setCursor(1, 1);                // Configura el cursor del LCD en la posición 1,1 (segunda línea).
  lcd.print("  ESCANNER 3D   ");      // Muestra el texto "ESCANNER 3D" en la segunda línea del LCD.
  delay(3000);                        // Espera 3 segundos.
  lcd.clear();                        // Limpia la pantalla del LCD.
  lcd.setCursor(0, 0);                // Configura el cursor del LCD en la posición 0,0 (inicio).
  lcd.print("Elaborado por");         // Muestra el texto "Elaborado por" en la primera línea del LCD.
  lcd.setCursor(0, 1);                // Configura el cursor del LCD en la posición 0,1 (segunda línea).
  lcd.print("Jose Ruiz");             // Muestra el texto "Jose Ruiz" en la segunda línea del LCD.
  delay(2000);                        // Espera 2 segundos.
  lcd.clear();                        // Limpia la pantalla del LCD. Finaliza la pantalla de inicio.
}


void loop() {

  int XValue = analogRead(X_pin);     // Lee el valor analógico del eje X del joystick.
  int YValue = analogRead(Y_pin);     // Lee el valor analógico del eje Y del joystick.
  int SwValue = digitalRead(SW_pin);  // Lee el valor digital del botón del joystick.

  if (MenuNr < 0){  // Si el número del menú es menor que 0, establecer el número del menú a 0. Esto define el número mínimo de menús.
    MenuNr = 0;
  }
  else if (MenuNr > 2){  // Si el número del menú es mayor que 2, establecer el número del menú a 2. Esto define el número máximo de menús.
    MenuNr = 2;
  }

  if (XValue < 400 && Flag1 == 0 && SwMenu == 0){  // Si el joystick se empuja hacia la derecha, la bandera 1 es 0 y SwMenu es 0, entonces se añade 1 al número del menú (el propósito de la bandera está en los comentarios de las banderas arriba).
    MenuNr = MenuNr + 1; 
    Flag1 = 1;
    lcd.clear();
  }

  if (XValue > 600 && Flag1 == 0 && SwMenu == 0){  // Si el joystick se empuja hacia la izquierda, la bandera 1 es 0 y SwMenu es 0, entonces se resta 1 del número del menú (el propósito de la bandera está en los comentarios de las banderas arriba).
    MenuNr = MenuNr - 1;
    Flag1 = 1;
    lcd.clear();
  }

  if (XValue > 399 && XValue < 599 && Flag1 == 1){  // Si el joystick está en posición neutral, se restablece la bandera 1 a 0 (el propósito de la bandera está en los comentarios de las banderas arriba).
    Flag1 = 0;
  }

  if (SwValue == 0 && BtnFlag == 0 && MaxSwMenu == 0){  // Si se presiona el botón, la bandera es 0 y MaxSwMenu es 0, entonces se añade 1 a SwMenu.
    SwMenu = SwMenu + 1;
    BtnFlag = 1;
    BtnCancelFlag = 0;
    lcd.clear();
  }

  if (SwValue == 1 && BtnFlag == 1){  // Si no se presiona el botón y la bandera es 1, se restablece la bandera (el propósito de la bandera está en los comentarios de las banderas arriba).
    BtnFlag = 0;
  }

  

 //***********************************************Menu0 - Fotogrametria ***********************************************//


  if (MenuNr == 0){  // Si el número del menú es 0, se ejecuta el código del Menú 0.

    if (SwMenu == 0){ // Si el submenú (SwMenu) es 0, se muestra el texto "Fotogrametria" en la pantalla LCD.
      lcd.setCursor(0, 0);  // Configura el cursor en la posición 0,0 de la pantalla LCD.
      lcd.print("Fotogrametria");  // Muestra el texto "Fotogrametria".
    }
  
    if (SwMenu == 1){ // Si el submenú (SwMenu) es 1, se muestra el número de fotos y permite ajustar la cantidad de fotos.
      lcd.setCursor(0, 0);  // Configura el cursor en la posición 0,0 de la pantalla LCD.
      lcd.print("Nr. de fotos ");  // Muestra el texto "Nr. de fotos".
      lcd.setCursor(7, 1);  // Configura el cursor en la posición 7,1 (segunda línea).
      lcd.print(PhotoNr);  // Muestra el número de fotos (PhotoNr).

      if (FastChng == 0) {  // Si no se está en modo de cambio rápido, actualiza el tiempo de referencia.
        SetTime = millis();  // Establece SetTime al tiempo actual en milisegundos.
      }
      
      if (YValue < 400 && Flag2 == 0){ // Si el joystick se mueve hacia arriba (YValue < 400), la bandera 2 es 0 y SwMenu es 1, entonces se añade 2 al número de fotos.
        PhotoNr = PhotoNr + 2;  // Incrementa PhotoNr en 2.
        Flag2 = 1;  // Establece Flag2 a 1 para evitar cambios repetitivos.
        FastChng = 1;  // Activa el modo de cambio rápido.
        lcd.clear();  // Limpia la pantalla LCD.
      }
      
      if (YValue > 600 && Flag2 == 0){ // Si el joystick se mueve hacia abajo (YValue > 600), la bandera 2 es 0 y SwMenu es 1, entonces se resta 2 del número de fotos.
        PhotoNr = PhotoNr - 2;  // Decrementa PhotoNr en 2.
        Flag2 = 1;  // Establece Flag2 a 1 para evitar cambios repetitivos.
        FastChng = 1;  // Activa el modo de cambio rápido.
        lcd.clear();  // Limpia la pantalla LCD.
      }
      
      if (YValue > 399 && YValue < 599 && Flag2 == 1){  // Si el joystick está en la posición neutral (YValue entre 400 y 600) y Flag2 es 1, restablece Flag2 y desactiva el modo de cambio rápido.
        Flag2 = 0;  // Restablece Flag2.
        FastChng = 0;  // Desactiva el modo de cambio rápido.
      }

      if (YValue < 400 && FastChng == 1) {  // Si el joystick está hacia arriba y el modo de cambio rápido está activado, y se mantiene por más tiempo que el tiempo de demora rápido, entra en el modo de cambio rápido.
        if ((millis() - SetTime) > FastDelay) {  // Si ha pasado más tiempo que FastDelay desde SetTime:
          FastChng = 2;  // Establece FastChng a 2 para indicar el modo de cambio rápido.
          SetTime = millis();  // Actualiza SetTime al tiempo actual.
        }
      }

      if (YValue > 600 && FastChng == 1) {  // Si el joystick está hacia abajo y el modo de cambio rápido está activado, y se mantiene por más tiempo que el tiempo de demora rápido, entra en el modo de cambio rápido.
        if ((millis() - SetTime) > FastDelay) {  // Si ha pasado más tiempo que FastDelay desde SetTime:
          FastChng = 2;  // Establece FastChng a 2 para indicar el modo de cambio rápido.
          SetTime = millis();  // Actualiza SetTime al tiempo actual.
        }
      }

      if (YValue < 400 && FastChng == 2) {  // Si estamos en modo de cambio rápido y el joystick está hacia arriba:
        if ((millis() - SetTime) > (LongInt - (400 - YValue) * (LongInt - ShortInt) / 400)) {  // Si ha pasado suficiente tiempo basado en la deflexión del joystick:
          PhotoNr = PhotoNr + 2;  // Incrementa PhotoNr en 2.
          SetTime = millis();  // Actualiza SetTime al tiempo actual.
          lcd.clear();  // Limpia la pantalla LCD.
        }
      }

      if (YValue > 600 && FastChng == 2) {  // Si estamos en modo de cambio rápido y el joystick está hacia abajo:
        if ((millis() - SetTime) > (LongInt - (YValue - 600) * (LongInt - ShortInt) / 400)) {  // Si ha pasado suficiente tiempo basado en la deflexión del joystick:
          PhotoNr = PhotoNr - 2;  // Decrementa PhotoNr en 2.
          SetTime = millis();  // Actualiza SetTime al tiempo actual.
          lcd.clear();  // Limpia la pantalla LCD.
        }
      }

      if (PhotoNr < 2){    // Si el número de fotos es menor que 2, establece el número mínimo de fotos en 2.
        PhotoNr = 2;
      }
      
      if (PhotoNr > 200){  // Si el número de fotos es mayor que 200, establece el número máximo de fotos en 200.
        PhotoNr = 200;
      }
    }

    if (SwMenu == 2){ // Si el submenú (SwMenu) es 2, se ha iniciado el programa.

      MaxSwMenu = 1;  // Establece MaxSwMenu a 1 para indicar que se está en el menú máximo.
      
      lcd.setCursor(0, 0);  // Configura el cursor en la posición 0,0 de la pantalla LCD.
      lcd.print("Empezamos");  // Muestra el texto "Empezamos".
      lcd.setCursor(0, 1);  // Configura el cursor en la posición 0,1 (segunda línea).
      lcd.print("Nr. de fotos");  // Muestra el texto "Nr. de fotos".
      lcd.print(PhotoTaken);  // Muestra el número de fotos tomadas (PhotoTaken).
      
      StepPerPhoto = FullRev / PhotoNr;  // Calcula la cantidad de pasos por foto.

      if (PhotoTaken < PhotoNr){  // Mientras no se haya tomado el número total de fotos:
        myStepper.setSpeed(rolePerMinute);  // Configura la velocidad del motor paso a paso.
        myStepper.step(StepPerPhoto);  // Mueve el motor paso a paso la cantidad calculada de pasos.
        PhotoTaken = PhotoTaken + 1;  // Incrementa el contador de fotos tomadas.
        lcd.setCursor(0, 1);  // Configura el cursor en la posición 0,1 (segunda línea).
        lcd.print("Foto Nr.    ");  // Muestra el texto "Foto Nr.".
        lcd.print(PhotoTaken);  // Muestra el número de fotos tomadas.
        digitalWrite(19, LOW);  // Enciende el LED conectado al pin 19.
        delay(300);  // Espera 300 ms.
        digitalWrite(19, HIGH);  // Apaga el LED conectado al pin 19.
        delay(4000);  // Espera 4 segundos.
      }

      if (PhotoTaken == PhotoNr){  // Si el número de fotos tomadas es igual al número de fotos a tomar:
        lcd.setCursor(0, 0);  // Configura el cursor en la posición 0,0 de la pantalla LCD.
        lcd.print("Prog. finalizado");  // Muestra el texto "Prog. finalizado".
        delay(3000);  // Espera 3 segundos.
        lcd.clear();  // Limpia la pantalla LCD.
        PhotoTaken = 0;  // Restablece el contador de fotos tomadas.
        PhotoNr = 2;  // Restablece el número de fotos a 2.
        SwMenu = 0;  // Restablece el submenú a 0.
        MaxSwMenu = 0;  // Restablece MaxSwMenu a 0.
        Steps = 0;  // Restablece el contador de pasos a 0.
      }

      // Nota: La cancelación funciona, pero el retraso es mayor de lo esperado ya que el movimiento del motor paso a paso bloquea.
      if (SwValue == 0 && BtnCancelFlag == 0) {  // Si se presiona el botón y la bandera de cancelación es 0, comienza el temporizador para cancelar.
        BtnCancelFlag = 1;  // Establece BtnCancelFlag a 1 para indicar que se ha comenzado a cancelar.
        SetTime = millis();  // Establece SetTime al tiempo actual en milisegundos.
      }

      if (SwValue == 1 && BtnCancelFlag == 1) {  // Si el botón se suelta antes de que pase el tiempo de retraso para cancelar, restablece la bandera de cancelación.
        BtnCancelFlag = 0;  // Restablece BtnCancelFlag a 0.
      }

      if (SwValue == 0 && BtnCancelFlag == 1 && ((millis() - SetTime) > BtnDelay)) {  // Si el botón se mantiene presionado por más tiempo que BtnDelay, cancela la operación.
        lcd.clear();  // Limpia la pantalla LCD.
        lcd.setCursor(0, 0);  // Configura el cursor en la posición 0,0 de la pantalla LCD.
        lcd.print("Prog. cancelado");  // Muestra el texto "Prog. cancelado".
        delay(3000);  // Espera 3 segundos.
        lcd.clear();  // Limpia la pantalla LCD.
        PhotoTaken = 0;  // Restablece el contador de fotos tomadas.
        PhotoNr = 2;  // Restablece el número de fotos a 2.
        SwMenu = 0;  // Restablece el submenú a 0.
        MaxSwMenu = 0;  // Restablece MaxSwMenu a 0.
        Steps = 0;  // Restablece el contador de pasos a 0.
        BtnCancelFlag = 0;  // Restablece BtnCancelFlag a 0.
      }
    }
}



 //***********************************************Menu1 - Cinematico***********************************************//

  if (MenuNr == 1){  // Si el número del menú es 1, se ejecuta el código del Menú 1.
    
    if (SwMenu == 0){  // Si el submenú (SwMenu) es 0, se muestra el texto "Cinematico" en la pantalla LCD.
      lcd.setCursor(0, 0);  // Configura el cursor en la posición 0,0 de la pantalla LCD.
      lcd.print("Cinematico");  // Muestra el texto "Cinematico".
      CinCancelFlag = 0;  // Restablece la bandera de cancelación cinemática a 0.
      Cntr = 0;  // Restablece el contador a 0.
    }

    if (SwMenu == 1){  // Si el submenú (SwMenu) es 1, se muestra y ajusta la velocidad del motor.
      lcd.setCursor(0, 0);  // Configura el cursor en la posición 0,0 de la pantalla LCD.
      lcd.print("Vel. motor");  // Muestra el texto "Vel. motor".
      lcd.setCursor(7, 1);  // Configura el cursor en la posición 7,1 (segunda línea).
      lcd.print(rolePerMinute);  // Muestra la velocidad del motor (rolePerMinute).

      if (FastChng == 0) {  // Si no se está en modo de cambio rápido, actualiza el tiempo de referencia.
        SetTime = millis();  // Establece SetTime al tiempo actual en milisegundos.
      }
      
      if (YValue < 400 && Flag3 == 0){ // Si el joystick se mueve hacia arriba (YValue < 400) y la bandera 3 es 0, aumenta la velocidad en 1 RPM.
        rolePerMinute = rolePerMinute + 1;  // Incrementa rolePerMinute en 1.
        Flag3 = 1;  // Establece Flag3 a 1 para evitar cambios repetitivos.
        FastChng = 1;  // Activa el modo de cambio rápido.
        lcd.clear();  // Limpia la pantalla LCD.
      }
      
      if (YValue > 600 && Flag3 == 0){ // Si el joystick se mueve hacia abajo (YValue > 600) y la bandera 3 es 0, disminuye la velocidad en 1 RPM.
        rolePerMinute = rolePerMinute - 1;  // Decrementa rolePerMinute en 1.
        Flag3 = 1;  // Establece Flag3 a 1 para evitar cambios repetitivos.
        FastChng = 1;  // Activa el modo de cambio rápido.
        lcd.clear();  // Limpia la pantalla LCD.
      }
      
      if (YValue > 399 && YValue < 599 && Flag3 == 1){  // Si el joystick está en la posición neutral (YValue entre 400 y 600) y Flag3 es 1, restablece Flag3 y desactiva el modo de cambio rápido.
        Flag3 = 0;  // Restablece Flag3.
        FastChng = 0;  // Desactiva el modo de cambio rápido.
      }

      if (YValue < 400 && FastChng == 1) {  // Si el joystick está hacia arriba y el modo de cambio rápido está activado, y se mantiene por más tiempo que el tiempo de demora rápido, entra en el modo de cambio rápido.
        if ((millis() - SetTime) > FastDelay) {  // Si ha pasado más tiempo que FastDelay desde SetTime:
          FastChng = 2;  // Establece FastChng a 2 para indicar el modo de cambio rápido.
          SetTime = millis();  // Actualiza SetTime al tiempo actual.
        }
      }

      if (YValue > 600 && FastChng == 1) {  // Si el joystick está hacia abajo y el modo de cambio rápido está activado, y se mantiene por más tiempo que el tiempo de demora rápido, entra en el modo de cambio rápido.
        if ((millis() - SetTime) > FastDelay) {  // Si ha pasado más tiempo que FastDelay desde SetTime:
          FastChng = 2;  // Establece FastChng a 2 para indicar el modo de cambio rápido.
          SetTime = millis();  // Actualiza SetTime al tiempo actual.
        }
      }

      if (YValue < 400 && FastChng == 2) {  // Si estamos en modo de cambio rápido y el joystick está hacia arriba:
        if ((millis() - SetTime) > LongInt) {  // Si ha pasado suficiente tiempo basado en LongInt:
          rolePerMinute = rolePerMinute + 1;  // Incrementa rolePerMinute en 1.
          SetTime = millis();  // Actualiza SetTime al tiempo actual.
          lcd.clear();  // Limpia la pantalla LCD.
        }
      }

      if (YValue > 600 && FastChng == 2) {  // Si estamos en modo de cambio rápido y el joystick está hacia abajo:
        if ((millis() - SetTime) > LongInt) {  // Si ha pasado suficiente tiempo basado en LongInt:
          rolePerMinute = rolePerMinute - 1;  // Decrementa rolePerMinute en 1.
          SetTime = millis();  // Actualiza SetTime al tiempo actual.
          lcd.clear();  // Limpia la pantalla LCD.
        }
      }
      
      if (rolePerMinute < 1){  // Si la velocidad del motor es menor que 1 RPM, establece el mínimo permitido en 1 RPM.
        rolePerMinute = 1;
      }
      
      if (rolePerMinute > 17){  // Si la velocidad del motor es mayor que 17 RPM, establece el máximo permitido en 17 RPM.
        rolePerMinute = 17;
      }
    }

    if (SwMenu == 2){  // Si el submenú (SwMenu) es 2, se muestra y ajusta el número de turnos.
      lcd.setCursor(0, 0);  // Configura el cursor en la posición 0,0 de la pantalla LCD.
      lcd.print("Nr. de turnos");  // Muestra el texto "Nr. de turnos".
      lcd.setCursor(7, 1);  // Configura el cursor en la posición 7,1 (segunda línea).
      lcd.print(TurnNr);  // Muestra el número de turnos (TurnNr).

      if (FastChng == 0) {  // Si no se está en modo de cambio rápido, actualiza el tiempo de referencia.
        SetTime = millis();  // Establece SetTime al tiempo actual en milisegundos.
      }
      
      if (YValue < 400 && Flag4 == 0){ // Si el joystick se mueve hacia arriba (YValue < 400) y la bandera 4 es 0, aumenta el número de turnos en 1.
        TurnNr = TurnNr + 1;  // Incrementa TurnNr en 1.
        Flag4 = 1;  // Establece Flag4 a 1 para evitar cambios repetitivos.
        FastChng = 1;  // Activa el modo de cambio rápido.
        lcd.clear();  // Limpia la pantalla LCD.
      }
      
      if (YValue > 600 && Flag4 == 0){ // Si el joystick se mueve hacia abajo (YValue > 600) y la bandera 4 es 0, disminuye el número de turnos en 1.
        TurnNr = TurnNr - 1;  // Decrementa TurnNr en 1.
        Flag4 = 1;  // Establece Flag4 a 1 para evitar cambios repetitivos.
        FastChng = 1;  // Activa el modo de cambio rápido.
        lcd.clear();  // Limpia la pantalla LCD.
      }
      
      if (YValue > 399 && YValue < 599 && Flag4 == 1){  // Si el joystick está en la posición neutral (YValue entre 400 y 600) y Flag4 es 1, restablece Flag4 y desactiva el modo de cambio rápido.
        Flag4 = 0;  // Restablece Flag4.
        FastChng = 0;  // Desactiva el modo de cambio rápido.
      }

      if (YValue < 400 && FastChng == 1) {  // Si el joystick está hacia arriba y el modo de cambio rápido está activado, y se mantiene por más tiempo que el tiempo de demora rápido, entra en el modo de cambio rápido.
        if ((millis() - SetTime) > FastDelay) {  // Si ha pasado más tiempo que FastDelay desde SetTime:
          FastChng = 2;  // Establece FastChng a 2 para indicar el modo de cambio rápido.
          SetTime = millis();  // Actualiza SetTime al tiempo actual.
        }
      }

      if (YValue > 600 && FastChng == 1) {  // Si el joystick está hacia abajo y el modo de cambio rápido está activado, y se mantiene por más tiempo que el tiempo de demora rápido, entra en el modo de cambio rápido.
        if ((millis() - SetTime) > FastDelay) {  // Si ha pasado más tiempo que FastDelay desde SetTime:
          FastChng = 2;  // Establece FastChng a 2 para indicar el modo de cambio rápido.
          SetTime = millis();  // Actualiza SetTime al tiempo actual.
        }
      }

      if (YValue < 400 && FastChng == 2) {  // Si estamos en modo de cambio rápido y el joystick está hacia arriba:
        if ((millis() - SetTime) > (LongInt - (400 - YValue) * (LongInt - ShortInt) / 400)) {  // Tiempo proporcional basado en la deflexión del joystick:
          TurnNr = TurnNr + 1;  // Incrementa TurnNr en 1.
          SetTime = millis();  // Actualiza SetTime al tiempo actual.
          lcd.clear();  // Limpia la pantalla LCD.
        }
      }

      if (YValue > 600 && FastChng == 2) {  // Si estamos en modo de cambio rápido y el joystick está hacia abajo:
        if ((millis() - SetTime) > (LongInt - (YValue - 600) * (LongInt - ShortInt) / 400)) {  // Tiempo proporcional basado en la deflexión del joystick:
          TurnNr = TurnNr - 1;  // Decrementa TurnNr en 1.
          SetTime = millis();  // Actualiza SetTime al tiempo actual.
          lcd.clear();  // Limpia la pantalla LCD.
        }
      }
      
      if (TurnNr < 1){  // Si el número de turnos es menor que 1, establece el mínimo permitido en 1.
        TurnNr = 1;
      }
      
      if (TurnNr > 200){  // Si el número de turnos es mayor que 200, establece el máximo permitido en 200.
        TurnNr = 200;
      }
    }

    if (SwMenu == 3){  // Si el submenú (SwMenu) es 3, se inicia el programa.
      MaxSwMenu = 1;  // Activa el indicador de menú máximo.
      StepPoll = 32 * rolePerMinute;  // Establece la tasa de muestreo (StepPoll) basada en la velocidad del motor.
      lcd.setCursor(0, 0);  // Configura el cursor en la posición 0,0 de la pantalla LCD.
      lcd.print("Prog. iniciado");  // Muestra el texto "Prog. iniciado".
      lcd.setCursor(0, 1);  // Configura el cursor en la posición 0,1 (segunda línea).
      lcd.print("Turnos hechos: ");  // Muestra el texto "Turnos hechos: ".
      lcd.print(CurrentTurn);  // Muestra el número de turnos actuales (CurrentTurn).

      if (CurrentTurn < TurnNr){  // Mientras el número de turnos actuales sea menor que el número de turnos necesarios:
        myStepper.setSpeed(rolePerMinute);  // Establece la velocidad del motor al valor de rolePerMinute.
        Cntr = 0;  // Inicializa el contador de pasos.
        while ((Cntr <= FullRev) && (CinCancelFlag == 0)) {  // Mientras el contador sea menor o igual a FullRev y la bandera de cancelación cinemática sea 0:
          myStepper.step(StepPoll);  // Realiza un paso del motor basado en la tasa de muestreo StepPoll.
          Cntr = Cntr + StepPoll;  // Incrementa el contador de pasos.
          SwValue = digitalRead(SW_pin);  // Lee el valor del botón de cancelación (SW_pin).
          if (SwValue == 0 && BtnCancelFlag == 0) {  // Si el botón está presionado y la bandera de cancelación de botón es 0:
            BtnCancelFlag = 1;  // Establece BtnCancelFlag a 1 para iniciar el temporizador de cancelación.
            SetTime = millis();  // Establece SetTime al tiempo actual en milisegundos.
          }
          if (SwValue == 1 && BtnCancelFlag == 1) {  // Si el botón se suelta antes del tiempo de retraso para cancelar, restablece la bandera de cancelación de botón.
            BtnCancelFlag = 0;  // Restablece BtnCancelFlag a 0.
          }
          if (SwValue == 0 && BtnCancelFlag == 1 && ((millis() - SetTime) > BtnDelay)) {  // Si el botón ha sido mantenido presionado por más tiempo que BtnDelay:
            CinCancelFlag = 1;  // Establece la bandera de cancelación cinemática a 1 para cancelar la operación.
            CurrentTurn = TurnNr;  // Establece CurrentTurn al número de turnos necesarios.
          }
        }
        if (CinCancelFlag == 0) {  // Si la operación no fue cancelada:
          myStepper.step(FullRev + StepPoll - Cntr);  // Realiza el resto de los pasos necesarios para completar un giro completo.
          CurrentTurn = CurrentTurn + 1;  // Incrementa el número de turnos actuales.
          lcd.setCursor(0, 1);  // Configura el cursor en la posición 0,1 de la pantalla LCD.
          lcd.print("Turnos hechos: ");  // Muestra el texto "Turnos hechos: ".
          lcd.print(CurrentTurn);  // Muestra el número de turnos actuales.
        }
      }

      if (CurrentTurn == TurnNr){  // Si el número de turnos actuales es igual al número de turnos necesarios:
        lcd.setCursor(0, 0);  // Configura el cursor en la posición 0,0 de la pantalla LCD.
        if (CinCancelFlag == 0) {  // Si la operación no fue cancelada:
          lcd.print("Prog. finalizado");  // Muestra el texto "Prog. finalizado".
        }
        else {  // Si la operación fue cancelada:
          lcd.clear();  // Limpia la pantalla LCD.
          lcd.print("Prog. cancelado");  // Muestra el texto "Prog. cancelado".
        }
        delay(3000);  // Espera 3 segundos.
        lcd.clear();  // Limpia la pantalla LCD.
        CurrentTurn = 0;  // Restablece el número de turnos actuales a 0.
        PhotoNr = 1;  // Restablece el número de fotos a 1.
        rolePerMinute = 15;  // Restablece la velocidad del motor a 15 RPM.
        SwMenu = 0;  // Restablece el submenú a 0.
        MaxSwMenu = 0;  // Restablece MaxSwMenu a 0.
        CinCancelFlag = 0;  // Restablece la bandera de cancelación cinemática a 0.
        Steps = 0;  // Restablece el contador de pasos a 0.
      }
    }
}


 //***********************************************Menu2 - Control Manual***********************************************//

  if (MenuNr == 2){  // Si el número del menú es 2, se ejecuta el código del Menú 2.

    if (SwMenu == 0){  // Si el submenú (SwMenu) es 0:
        lcd.setCursor(0, 0);  // Configura el cursor en la posición 0,0 de la pantalla LCD.
        lcd.print("Control manual");  // Muestra el texto "Control manual".
    }

    if (SwMenu == 1){  // Si el submenú (SwMenu) es 1 (entrada en el menú 2):
        lcd.setCursor(0, 0);  // Configura el cursor en la posición 0,0 de la pantalla LCD.
        lcd.print("Velocidad motor");  // Muestra el texto "Velocidad motor".
        lcd.setCursor(7, 1);  // Configura el cursor en la posición 7,1 (segunda línea).
        lcd.print(rolePerMinute);  // Muestra la velocidad del motor (rolePerMinute).

        if (FastChng == 0) {  // Si no se está en modo de cambio rápido, actualiza el tiempo de referencia.
            SetTime = millis();  // Establece SetTime al tiempo actual en milisegundos.
        }

        if (YValue < 400 && Flag5 == 0){  // Si el joystick se mueve hacia arriba (YValue < 400) y la bandera 5 es 0, aumenta la velocidad en 1 RPM.
            rolePerMinute = rolePerMinute + 1;  // Incrementa rolePerMinute en 1.
            Flag5 = 1;  // Establece Flag5 a 1 para evitar cambios repetitivos.
            FastChng = 1;  // Activa el modo de cambio rápido.
            lcd.clear();  // Limpia la pantalla LCD.
        }

        if (YValue > 600 && Flag5 == 0){  // Si el joystick se mueve hacia abajo (YValue > 600) y la bandera 5 es 0, disminuye la velocidad en 1 RPM.
            rolePerMinute = rolePerMinute - 1;  // Decrementa rolePerMinute en 1.
            Flag5 = 1;  // Establece Flag5 a 1 para evitar cambios repetitivos.
            FastChng = 1;  // Activa el modo de cambio rápido.
            lcd.clear();  // Limpia la pantalla LCD.
        }

        if (YValue > 399 && YValue < 599 && Flag5 == 1){  // Si el joystick está en la posición neutral (YValue entre 400 y 600) y Flag5 es 1, restablece Flag5 y desactiva el modo de cambio rápido.
            Flag5 = 0;  // Restablece Flag5.
            FastChng = 0;  // Desactiva el modo de cambio rápido.
        }

        if (YValue < 400 && FastChng == 1) {  // Si el joystick está hacia arriba y el modo de cambio rápido está activado, y se mantiene por más tiempo que el tiempo de demora rápido, entra en el modo de cambio rápido.
            if ((millis() - SetTime) > FastDelay) {  // Si ha pasado más tiempo que FastDelay desde SetTime:
                FastChng = 2;  // Establece FastChng a 2 para indicar el modo de cambio rápido.
                SetTime = millis();  // Actualiza SetTime al tiempo actual.
            }
        }

        if (YValue > 600 && FastChng == 1) {  // Si el joystick está hacia abajo y el modo de cambio rápido está activado, y se mantiene por más tiempo que el tiempo de demora rápido, entra en el modo de cambio rápido.
            if ((millis() - SetTime) > FastDelay) {  // Si ha pasado más tiempo que FastDelay desde SetTime:
                FastChng = 2;  // Establece FastChng a 2 para indicar el modo de cambio rápido.
                SetTime = millis();  // Actualiza SetTime al tiempo actual.
            }
        }

        if (YValue < 400 && FastChng == 2) {  // Si estamos en modo de cambio rápido y el joystick está hacia arriba:
            if ((millis() - SetTime) > LongInt) {  // Si ha pasado suficiente tiempo basado en LongInt:
                rolePerMinute = rolePerMinute + 1;  // Incrementa rolePerMinute en 1.
                SetTime = millis();  // Actualiza SetTime al tiempo actual.
                lcd.clear();  // Limpia la pantalla LCD.
            }
        }

        if (YValue > 600 && FastChng == 2) {  // Si estamos en modo de cambio rápido y el joystick está hacia abajo:
            if ((millis() - SetTime) > LongInt) {  // Si ha pasado suficiente tiempo basado en LongInt:
                rolePerMinute = rolePerMinute - 1;  // Decrementa rolePerMinute en 1.
                SetTime = millis();  // Actualiza SetTime al tiempo actual.
                lcd.clear();  // Limpia la pantalla LCD.
            }
        }

        if (rolePerMinute < 1){  // Si la velocidad del motor es menor que 1 RPM, establece el mínimo permitido en 1 RPM.
            rolePerMinute = 1;
        }

        if (rolePerMinute > 17){  // Si la velocidad del motor es mayor que 17 RPM, establece el máximo permitido en 17 RPM.
            rolePerMinute = 17;
        }

        if (XValue < 400 ){  // Si el joystick se mueve hacia la derecha (XValue < 400) y la bandera 6 es 0, incrementa el número de pasos y mueve el motor en esa dirección.
            Steps = Steps + 1;  // Incrementa Steps en 1.
            myStepper.setSpeed(rolePerMinute);  // Establece la velocidad del motor.
            myStepper.step(Steps);  // Realiza el movimiento del motor basado en el número de pasos.
            lcd.setCursor(14, 1);  // Configura el cursor en la posición 14,1 de la pantalla LCD.
            lcd.print("->");  // Muestra el texto "->" para indicar movimiento hacia la derecha.
            Flag6 = 1;  // Establece Flag6 a 1 para indicar que se ha realizado un movimiento.
        }

        if (XValue > 600 ){  // Si el joystick se mueve hacia la izquierda (XValue > 600) y la bandera 6 es 0, decrementa el número de pasos y mueve el motor en esa dirección.
            Steps = Steps + 1;  // Incrementa Steps en 1.
            myStepper.setSpeed(rolePerMinute);  // Establece la velocidad del motor.
            myStepper.step(-Steps);  // Realiza el movimiento del motor basado en el número de pasos negativos.
            lcd.setCursor(0, 1);  // Configura el cursor en la posición 0,1 de la pantalla LCD.
            lcd.print("<-");  // Muestra el texto "<-" para indicar movimiento hacia la izquierda.
            Flag6 = 1;  // Establece Flag6 a 1 para indicar que se ha realizado un movimiento.
        }

        if (XValue > 399 && XValue < 599){  // Si el joystick está en la posición neutral (XValue entre 400 y 600) y la bandera 6 es 1, restablece Steps y limpia la pantalla LCD.
            Steps = 0;  // Restablece Steps a 0.
            if (Flag6 == 1){  // Si Flag6 es 1:
                lcd.clear();  // Limpia la pantalla LCD.
                Flag6 = 0;  // Restablece Flag6 a 0.
            }
        }
    }

    if (SwMenu == 2){  // Si el submenú (SwMenu) es 2 (salida del menú 2 al presionar un botón):
        lcd.clear();  // Limpia la pantalla LCD.
        CurrentTurn = 0;  // Restablece el número de turnos actuales a 0.
        PhotoNr = 1;  // Restablece el número de fotos a 1.
        rolePerMinute = 15;  // Restablece la velocidad del motor a 15 RPM.
        SwMenu = 0;  // Restablece el submenú a 0.
        Steps = 0;  // Restablece el número de pasos a 0.
    }
  }
}
