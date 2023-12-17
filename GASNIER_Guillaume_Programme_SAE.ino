/* *************************************************************
 *  
 *  MATIERE : SAE 2.02 Mesure de grandeurs électrique
 *  
 *  AUTEUR : GASNIER Guillaume
 *  FILIERE : BUT GEII (1ère année)
 *  GROUPE : TD2 TP4
 *  ETABLISSEMENT : IUT de Chartres
 *  
 *  LANGAGE PROGRAMMATION UTILISE : C++ code
 *  
 *  ************************************************************
*/

#include <LiquidCrystal.h>


const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2; // Définition des branchements du lecteur LCD à la carte Arduino
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

// Programme timer1 avec IRQ mode CTC

int i;

volatile int flag_data_ready = 1; // Clignotement de la led autorisé
volatile int flag_stop_acqui = 0; // Extinction de la led

volatile unsigned int compteur = 0;
// Pour version 1 et 2
// volatile int nb_points = 3000; // Nombre de clignotements, soit environ 6 secondes 

/////////////////////////////////
// Pour version 3

// Déclaration des tableaux

  float COURANT[80];
  float TENSION[80];
  
volatile int nb_points = 80; // Nombre de points pour l'acquisition
volatile float *ptr_tension = TENSION; // Même chose que *prt_tension = &TENSION[0]
volatile float *ptr_courant = COURANT; // Même chose que *prt_courant = &COURANT[0]

ISR(TIMER1_COMPA_vect)
{  
if(flag_stop_acqui == 0)
  {
    // Acquisition
    *ptr_tension++ = analogRead(A0); // Sur A0, tension image tension

    // Cette instruction ne fonctionne pas ! 
    // *prt_tension++ = analogRead(A0) * 5. / 1023 - 2.6; (met trop de temps) // Alim en 5.2 V !!!!

    *ptr_courant++ = analogRead(A1); // Sur A1, tenson image courant

    if(++compteur > nb_points) // Le nombre de points n'est pas dépassé
    {
    flag_data_ready = 0; // On arrete le clignotement
    }
  }
} 

// CHARGE RLC  
float tension0[80] = {
      0.59, 0.63, 0.74, 0.85, 0.96, 1.16, 1.54, 1.92, 2.26, 2.60,
      2.94, 3.29, 3.55, 3.77, 3.99, 4.15, 4.22, 4.18, 4.08, 3.97,
      3.86, 3.66, 3.28, 2.90, 2.56, 2.22, 1.88, 1.53, 1.27, 1.06,
      0.83, 0.67, 0.59, 0.64, 0.74, 0.84, 0.95, 1.14, 1.53, 1.91,
      2.25, 2.59, 2.93, 3.28, 3.54, 3.76, 3.98, 4.14, 4.22, 4.18,
      4.08, 3.97, 3.86, 3.67, 3.28, 2.92, 2.57, 2.23, 1.90, 1.53,
      1.28, 1.07, 0.84, 0.67, 0.59, 0.63, 0.74, 0.84, 0.95, 1.14,
      1.52, 1.90, 2.25, 2.58, 2.92, 3.27, 3.54, 3.75, 3.97, 4.14};

float courant0[80] = {
      1.97, 1.67, 1.39, 1.14, 0.94, 0.82, 0.72, 0.70, 0.75, 0.87,
      1.06, 1.28, 1.54, 1.87, 2.17, 2.51, 2.82, 3.10, 3.36, 3.60,
      3.80, 3.91, 4.02, 4.03, 3.99, 3.87, 3.71, 3.48, 3.24, 2.93,
      2.61, 2.28, 1.97, 1.68, 1.39, 1.16, 0.94, 0.83, 0.72, 0.70, 
      0.75, 0.86, 1.05, 1.26, 1.54, 1.85, 2.17, 2.49, 2.82, 3.09,
      3.36, 3.59, 3.79, 3.91, 4.01, 4.04, 3.99, 3.89, 3.70, 3.50,
      3.23, 2.95, 2.61, 2.30, 1.97, 1.69, 1.40, 1.15, 0.96, 0.82,
      0.73, 0.70, 0.75, 0.86, 1.05, 1.26, 1.54, 1.85, 2.17, 2.49};



// Déclaration des fonctions
  float Val_Eff_Tension(int , int );
  float Val_Eff_Courant(int , int );


  
void setup()
{
  
  Serial.begin(9600); // Vitesse de transmission de la carte Arduino (9 600 bits par seconde)

    // Desactiver les interruptions, pour commencer
  // NoInterrupts ();
  // Mettre à 0 PB0
  SREG = SREG &~(1 << 7); // On stoppe les interruptions en mettant à 0
  

  // Mettre à 1 pour DDRB (on utilise un "OU")
  DDRB = DDRB | (1 << PB0); // B0 en sortie
  _NOP();

  // Mettre à l'état bas (donc à 0)
  PORTB = PORTB & ~(1 << PB0); // B1 à l'état bas
  _NOP();

  // On met une fréquence de 16 MHz
  CLKPR = 0b10000000; // CLKPCE = 1, tous les autres bits à 0
  CLKPR = 0b00000000; // CLKPS0 = 0, CLKPS1 = 0, CLKPS2 = 0, CLKPS3 = 0

  TCCR1B = 0b10000000;

  // On divise par 256 la fréquence (donc bit 0 à 0, bit 1 à 0 et le bit 2 à 1)
  TCCR1B = TCCR1B | (1 << CS12);
  _NOP();
  TCCR1B = TCCR1B & ~(1 << CS11);
  _NOP();
  TCCR1B = TCCR1B & ~(1 << CS10);
  _NOP();

  // On va régler la fréquence (multiplier par 2). Et on souhaite utiliser le registre OCR1A
  // Donc WGM13 à 0, WGM12 à 1 (tous les deux dans le registre TCCR1B)
  // Et WGM11 à 0, WGM10 à 0 (tous les deux dans le registre TCCR1A)
  TCCR1B = TCCR1B & ~(1 << WGM13);
  _NOP();
  TCCR1B = TCCR1B | (1 << WGM12);
  _NOP();
  TCCR1A = TCCR1A & ~(1 << WGM11);
  _NOP();
  TCCR1A = TCCR1A & ~(1 << WGM10);
  _NOP();

  // On convertit 127 en décimal en binaire : 127 = 0b01111111
  OCR1A = 0b01111111;
  
  // On met à 1 le drapeau OCIEA1 du registre TIMSK1 (donc utilisation d'un "OU")
  // Activation interruption locale
  TIMSK1 = TIMSK1 | (1 << OCIE1A);

  // activation interruption globale
  SREG = SREG | (1 << 7); // On active les interruptions en mettant à 1

};



void loop()
{
  
  int indice_deb_tensU;
  int indice_fin_tensU;
  int indice_deb_tensI;
  int indice_fin_tensI;

  int indice_tab_U;
  int indice_tab_I;
  int ecartU_I;
  
  float dephasage_rad;
  float dephasage_deg;
  float Val_Eff_U = 0.0;
  float Val_Eff_I = 0.0;

  float maxiU;
  float maxiI;
  
  // Variable puissance (S et P)
  float Puissance_apparente_S = 0.0;
  float Puissance_P = 0.0;
  float Facteur_puissance = 0.0;
  float Puissance_reactive_Q = 0.0;
  
  while (flag_data_ready) {}
  // On arrete le programme
  flag_stop_acqui = 1;

 // Calcul période

  // Détection premier max (tableau tensionU)
  for(i = 1, indice_deb_tensU = 0, maxiU = TENSION[i]; i < 40; i++)
  {

      if(TENSION[i] >= maxiU)
      {
        maxiU = TENSION[i];
        indice_deb_tensU = i;
      };
  };

   // Détection deuxieme max (tableau tensionU)
   for(i = indice_deb_tensU + 2, maxiU = TENSION[indice_deb_tensU+1]; i < 70; i++)
  {
    
   if (TENSION[i] >= maxiU)
    {
      maxiU = TENSION[i];
      indice_fin_tensU = i;
    };
  };
  
  indice_tab_U = indice_fin_tensU - indice_deb_tensU;
  Serial.println("Courbe U:");
  Serial.println("");
  Serial.print("Indice du premier max : ");
  Serial.println(indice_deb_tensU);
  Serial.print("Indice du deuxieme max : ");
  Serial.println(indice_fin_tensU);
  Serial.print("Différence d'indice tableau U : ");
  Serial.println(indice_tab_U);




  // Détection premier max (tableau tensionI)
   for(i = 1, indice_deb_tensI = 0, maxiI = COURANT[i]; i < 40; i++)
  {
      if(COURANT[i] >= maxiI)
      {
        maxiI = COURANT[i];
        indice_deb_tensI = i;
      }; 
  };

  // Détection deuxieme max (tableau tensionI)
   for(i = indice_deb_tensI + 2, maxiI = COURANT[indice_deb_tensI+1]; i < 70; i++)
  {
    if (COURANT[i] >= maxiI)
    {
      maxiI = COURANT[i];
      indice_fin_tensI = i;
    };
  };
  
  indice_tab_I = indice_fin_tensI - indice_deb_tensI;

  Serial.println("Courbe I:");
  Serial.println("");
  Serial.print("Indice du premier max : ");
  Serial.println(indice_deb_tensI);
  Serial.print("Indice du deuxieme max : ");
  Serial.println(indice_fin_tensI);
  Serial.print("Différence d'indice tableau I : ");
  Serial.println(indice_tab_I);


   // Calcul et affichage du déphasage 
   ecartU_I = indice_deb_tensI - indice_deb_tensU ;
   dephasage_rad = (ecartU_I * (2*3.14) ) / indice_tab_U; // Résultat en radian
   dephasage_deg = dephasage_rad * 180 / 3.14; // Conversion radian --> degré

   Serial.print("Déphasage de U par rapport à I: ");
   Serial.println(dephasage_deg);

   Val_Eff_U = Val_Eff_Tension(indice_tab_U, indice_deb_tensU);
   Val_Eff_I = Val_Eff_Courant(indice_tab_U, indice_deb_tensI);
  
   // Calcul puissances
   Serial.println("");
   Serial.println("Puissances :");
   Serial.println("");
   
   // Calcul et affichage puissance apparente
   Puissance_apparente_S = Val_Eff_U * Val_Eff_I;
   Serial.print("La puissance apparente S est de : ");
   Serial.println(Puissance_apparente_S);

   // Calcul et affichage puissance réactive
   Puissance_reactive_Q = Val_Eff_U * Val_Eff_I * sin(dephasage_rad);
   Serial.print("La puissance réactive Q est de : ");
   Serial.println(Puissance_reactive_Q);


   // Calcul et affichage puissance P
   Puissance_P = Val_Eff_U * Val_Eff_I * cos(dephasage_rad);
   Serial.print("La puissance P est de : ");
   Serial.println(Puissance_P); 


   // Calcul et affichage facteur de puissance 
   Facteur_puissance = Puissance_P / Puissance_apparente_S;
   Serial.print("le facteur de puissance de cette installation est de : ");
   Serial.println(Facteur_puissance);
   Serial.println(" ---------------------------------- ");


  delay(3000); // Tempo pour lire les données
  // Rénitialiser toutes les variables

  compteur = 0;
  ptr_tension = TENSION;
  ptr_courant = COURANT;

  flag_data_ready = 1;
  flag_stop_acqui = 0;

};


 
// Fonction calcul valeur efficace courbe tension
float Val_Eff_Tension(int indice_tab_U, int indice_deb_tensU)
{

  float somme_U = 0.0;

  for(i = 0, somme_U = 0; i < indice_tab_U; i++)
  {
    // somme_U = somme_U + TENSION[i] * TENSION[i];
    somme_U = somme_U + (TENSION [i] *5. / 1023. -2.6) * (TENSION [i] *5. / 1023. -2.6); // Somme des valeurs (convertit en valeur numérique)
  }

  return sqrt(somme_U / indice_tab_U);
}; 


// Fonction calcul valeur efficace courbe courant
float Val_Eff_Courant(int indice_tab_U, int indice_deb_tensI)
{


  float somme_I = 0.0;

  for(i = 0, somme_I = 0; i < indice_tab_U; i++)
  {
    // somme_I = somme_I + COURANT[i] * COURANT[i];
    somme_I = somme_I + (COURANT [i] *5. / 1023. -2.6) * (COURANT [i] *5. / 1023. -2.6); // Somme des valeurs (convertit en valeur numérique)
  }

  return sqrt(somme_I / indice_tab_U);
}; 
