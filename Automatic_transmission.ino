

//Variables
byte statePin8;
byte statePin9; // ou 12
byte maskPB0 = 0b00000001; //masque pour la pin digital 8 (PB0) (LSB on PINx register)
byte maskPB1 = 0b00000010; //masque pour la pin digital 9 (PB1)
byte maskpd4 = 0b00010000;
byte maskpb5 = 0b00100000;

//state watcher
statePin8 = PINB; //should be in setup 
statePin9 = PINB;

byte lastState; //should be boolean i suppose
byte lastState1;

boolean pb3;
boolean pb4;

// definission


//Compteurs
int cntRpmFlyWheel = 0; //remember can't do arithmetics with unsigned on arduino for some reason 
int cntRpmWheel = 0;
int counterShift = 0;
int counterUpShift = 0;
int counterDownShift = 0;
//variable volatile pour récupérer le compteur plus tard dans la loop
volatile unsigned int compaCounter = 0;

int rpmFlyWheel = 0;
int numTeeth = 35;
int rpmWheel = 0;
unsigned int oldTime; //remember can't do arithmetics with unsigned on arduino for some reason
unsigned int currenTime;

//time constant/variable
int timeConst = 18;
int converTime = 60000;

//So i want to count the number of times the signal goes from high to low in a fixed period of time....
//it's TIME to use the timer register (badumtss)

void setup() {
 Serial.begin(9600);
 cli();//stop interrupts
 //Let's setup the 16 bits timer to interrupt at 4 hz 
 TCCR1A = 0;//disable all the registers in TCCR1 A and B
 TCCR1B = 0;
 TCNT1 = 0;//setup counter value
 //Set compare match register for 55 hz increments (can be whatever frequency we want)
 OCR1A = 1249;//(16*10^6) / (4*64) - 1 (formule pour calculer la valeur du match)
 //CTC enabled
 TCCR1B |= (1<<WGM12);
 //setting up the 64 prescaler 
 TCCR1B |= (1<<CS11) | (1<<CS10); 
 //setup timer ovf 
 TIMSK1 |= (1<<OCIE1A);
 sei();//start interrupts
 counterShift = 11;
 //Downshift 11 times
 while(counterShift > 1){
   PORTB = 0b00001000;//downshift
   delay(150);//delay 50 ms peut ne pas marcher a 50 ms
   PORTB = 0b00000000;
   counterShift--;
 }
 counterShift = 1; 
}

//sous-routine qui s'execute a chaque fois quil y a un match (à chaque 18 ms selon le setup)
ISR(TIMER1_COMPA_vect){
  compaCounter++; 
}

void manualMode(){
 while(1){//boucle qui ne sarrete jaaaamaaaaiiiisssss
  //ouvre led verte pour indiquer mode man
  //verifier les derniers etats pour pas toujours entrer dans la condition si on garde le bouton enfoncer
  if(pb3 && !pb4){
   PORTB = 0b00000100;//upshift (1<<3)
   delay(70);//delay 50 ms peut ne pas marcher a 50 ms
   PORTB = 0b00000000;
   //counterUpShift++;
  }
  if(!pb3 && pb4){
   PORTB = 0b00001000;//downshift
   delay(70);//delay 50 ms peut ne pas marcher a 50 ms
   PORTB = 0b00000000;
   //counterDownShift++;
  }
 }
 //ferme led verte ouvre led jaune pour indiquer mode auto
}

void loop() {
 
 /*
 ce qu'il reste à faire : 
 1- calculer le taux d'accélération / ou déccel
 2- mode manuel/automatique
 3- changer les vitesses
 4- tester le calcul de rpm
 */
 
 /*Section 1 : Mode Man/Auto*/
 //check the states of pin 3 and 4 
 //pb3 = PIND & maskpd4;
 //pb4 = PIND & maskpd5;
 //if(pb4 && pb3){
 // manualMode();
 //}
 //}
 
 
 /*Section 2 : Calcul rpm roue + fly wheel */
 //Boucle while qui mesure le nombre de dents qui passe devant le capteur en 18 ms
 //la boucle while fait 1 tour en ~ 25 Micro secondes 
  compaCounter = 0;//Always before the while loop
  while(compaCounter < 3){
 
      if(statePin8 != lastState){
      //Quand une dent passe devant le capteur, celui-ci envoi un 0 logique (transition High to Low)
        if( (statePin8 &= maskPB0) == LOW){
          cntRpmFlyWheel++;
        }
      }
      if(statePin9 != lastState1){
        if( (statePin9 &= maskPB1) == LOW){ // pas sure de la transition pour ce capteur (high to low ou inverse)
           cntRpmWheel++;
        }
      }
      //Pour enregistrer le dernier état
      lastState = statePin8;
      lastState1 = statePin9;
      lastState &= maskPB0;
      lastState1 &= maskPB1;
 
  }
  
  //RPM measurement + reset of all the counters 
  /*Section 3 : Calcul du taux d'accélération*/
  rpmFlyWheel = (cntRpmFlyWheel*converTime)/(numTeeth*timeConst);
  rpmWheel = (cntRpmWheel*converTime)/(numTeeth*timeConst);
  
 
  /*
  To calculate the rate of acceleration, we need the speed of the vehicle and the time (so we need another timer)
  delta V2(t2) - delta V1(t1)/(t2-t1)
  if the delta is negative then we are deccelerating, if the delta is positive, then we are accelerating.
  
  Instead of calculating a delta for the speed, we could use an acceleromter hardware setup probably takes less code
  meaning less time.
  
  to check port manipulation for analog reading
  
  @Input : array of speed
  @output : void
  
  steps : 
  1- initialise array
  2- input speed (5 values)
  3- check each values if they go lower
  4- 
  
  
  
  
  */
  
 
  cntRpmFlyWheel = 0;
  cntRpmWheel = 0;
  


  //Ce qu'il reste à faire c'est l'algorithme des brackets
  Serial.print("rpm Wheel = ");
  Serial.println(rpmFlyWheel);
  Serial.print("rpm Roue = ");
  Serial.println(rpmWheel);
  

  
  /*Section 4 : Changement des vitesses*/
  /*
  //en acceleration
  if(accelValue > 0 && rpmFlyWheel > 4500){
   //jupshfift pour descendre mon rpm qui reste entre 4000 et 4500
   PORTB = 0b00000100;//upshift (1<<3)
   delay(70);//delay 50 ms peut ne pas marcher a 50 ms
   PORTB = 0b00000000;
   counterUpShift++;
  }
  
  if(accelValue > 0 && rpmFlyWheel < 4000){
   PORTB = 0b00001000;//downshift
   delay(50);//delay 50 ms peut ne pas marcher a 50 ms
   PORTB = 0b00000000;
   counterDownShift++;
  }
  
  //en decceleration
  if(acceleration <= 0 && rpmFlyWheel != targetRpm){
   //si l'accel est nulle alors on shift en fonction du rpm target du tableau excel
   //
   //a developper
   //
  }
  
  
  */
 
 
}





