/**
* Projekt do predmetu IMP
* Zaznamnik a prehravac melodii s vnitrni pameti pro skladby a vizualizaci
* Tento program je urcen pro vyukovy kit VUT FIT
* Soubor: main.c
*
* Autor:  Martin Knapovsky
* E-Mail: xknapo02@stud.fit.vutbr.cz, knapovsky@email.cz
* Datum:  29.11.2011
*
* Popis:  Tento program funguje 3 zpusoby dle zvoleneho modu
*           PIANO - Syntetizuje ton zvoleny uzivatelem a prehrava ho
*           REC   - Nahrava tony zvolene uzivatelem na predem zvolenou pozici
*           PLAY  - Prehrava drive ulozenou melodii, kterou si uzivatel zvolil
*
* Tagy pro VUT FIT: "Original"
*/

#include <hidef.h> /* for EnableInterrupts macro */
#include "derivative.h" /* include peripheral declarations */
#include <stdio.h>
#include "main.h"

/* DEFINICE BOOL */
#define TRUE 1
#define FALSE 0

/* MELODIE */
/* definice frekvenci tonu */
/* pouze jedna oktava */
/* Pouzite tony */
#define F_C   1047
#define F_CC  1109
#define F_D   1175
#define F_DD  1245
#define F_E   1319
#define F_F   1397
#define F_FF  1480
#define F_G   1568
#define F_GG  1661
#define F_A   1760
#define F_AA  1865
#define F_H   1976
#define MCU_FREQ 8000000
#define PRESCALER 128

#define C   1
#define CC  2
#define D   3
#define DD  4
#define E   5
#define F   6
#define FF  7
#define G   8
#define GG  9
#define A   10
#define AA  11
#define H   12
#define NO_KEY 13

#define FR F_C

/* Pouzitelne frekvence - 4kHz Max - Lze zmenit frekvece v definicich vyse
a               220
bb             233
b               247
C1            262
D1b          277
D1            294
E1b          311
E1            330
F1            349
G1b          370
G1            392
A1b          416
A1            440
B1b          466
B1            494
C2            523
D2b          554
D2            587
E2b          622
E2            659
F2            698
G2b          740
G2            784
A2b          831
A2            880
B2b          932
B2            988
C3          1047
D3b        1109
D3          1175
E3b         1245
E3           1319
F3           1397
G3b         1480
G3           1568
A3b         1661
A3           1760
B3b         1865
B3           1976
C4           2093
D4b         2217
D4           2349
E4b         2489
E4           2637
F4           2794
G4b         2960
G4           3136
A4b         3322
A4           3520
B4b         3729
B4           3951
*/

/* Konec melodie */
#define MELODY_END 0
/* Maximalni frekvence */
#define FREQ_MAX 4000
/* Delka prehravaneho tonu */
#define TONE_LENGTH 10000
/* Aktualni frekvence prehravaneho tonu */
unsigned int tone_freq = 0;

/* UKLADANI MELODII */
/* maximalni pocet tonu v melodii */
#define MAX_MELODY_SIZE 10
/* maximalni pocet melodii ulozenych v RAM */
#define MAX_MELODY_COUNT 5
/* pole uchovavajici melodie */
/* tony je potreba ukladat ve vhodne reprezentaci pro usetreni mista */
/* posledni pozice urcena pro ulozeni ukoncovaciho znaku melodie */
unsigned char melodies[MAX_MELODY_COUNT][MAX_MELODY_SIZE + 1];
/* Aktualne zvolena melodie */
unsigned char melody = 0;
/* Aktualne prehravany ton */
unsigned char tone = 0;
/* Dalsi melodie */
#define UP 1
/* Predchozi melodie */
#define DOWN 0

/* STAV PROGRAMU */
/* definice stavu programu */
#define PIANO 1
#define REC   2
#define PLAY  3
/* promenna uchovavajici stav programu */
unsigned char state = PIANO;
/* Indikace probihajiciho nahravani */
unsigned char recording = 0;
/* Indikace probihajiciho prehravani */
unsigned char playing = 0;

/* RETEZCE PRO LCD */
/* Maximalni delka retezce zobrazovaneho na radku displaye */
#define MAX_STR_LEN 16
char str_piano[MAX_STR_LEN] = "Piano mode";
char str_rec[MAX_STR_LEN] = "Recording mode";
char str_play[MAX_STR_LEN] = "Playing mode";

/* PODSVETLENI LCD */
/* Maximalni hodnota */
#define LCD_COL_MAX 250

/* DEFINICE K TLACITKUM A LED */
#define BTN_PORT1D PTGD
#define BTN_PORT2D PTDD
#define LED_PORTD PTCD
#define LED_PORTDD PTCDD

#define BTN_LED_UP 4
#define BTN_LED_DOWN 8
#define BTN_LED_LEFT 16
#define BTN_LED_RIGHT 32
#define BTN_LED_OK 64

#define BTN_LED_ALL (BTN_LED_UP | BTN_LED_DOWN | BTN_LED_LEFT | BTN_LED_RIGHT | BTN_LED_OK )

#define BTN_UP 1
#define BTN_DOWN 2
#define BTN_LEFT 4
#define BTN_RIGHT 8
#define BTN_OK 16
/* indikace, zda ma svitit ok tlacitko */
unsigned char ok = 0;

/* PRACE S IIC */
/* Globalni promenna obsahujici adresu slave zarizeni */
byte slave_addr = 0;

/* DEFINICE PRO PRACI S PIEZO BUZZEREM */
/* Zakladni frekvence piezo buzzeru je 4kHz */
#define BUZ_DEF_HZ 4000
/* Piezo buzzer je na PORT D PIN 4 (cislovani od 0) */

/* POMOCNE PROMENNE */
char str_tmp[MAX_STR_LEN];
unsigned int i;
unsigned int j;

/*-------------------------------------------------------*/
/*-------------------INICIALIZACE MCU--------------------*/
/*-------------------------------------------------------*/


/** Inicializace MCU 
*/
void mcu_init()
{
  /* Kill the dog :D, Stop Mode disabled */
  SOPT1 = 0x13;
  /* Enable input filter on SPI1 port pins to eliminate noise and restrict maximum SPI baud rate. */
  SOPT2 = 0x04;
  /* Force an MCU reset when an enabled low-voltage detect event occurs. */
  /* Low-voltage detect enabled during stop mode. */
  /* LVD logic enabled. */
  /* Bandgap buffer enabled. */
  SPMSC1 = 0x1C;
  /* Enable Stop3 */
  SPMSC2 = 0x00;

  /* Vynulovani, vypnuti SPI */
  SPI2C1 = 0x00;
  SPI2C2 = 0x00;
  /* Nastaveni Baud Rate */
  SPI2BR = 0x04;
  /* Vynulovani Match registru */
  SPI2M = 0x00;
  /* System and Interrupt enable */
  SPI2C1 = 0x50;

  // Nastaveni podsviceni LCD
  PTFDD |= (unsigned char) 0x0C;
  PTFD |= (unsigned char) 0x0E;

  PTADD |= (unsigned char) 0x0C;
  PTAD |= (unsigned char) 0x0C;

  return;
}

/*-------------------------------------------------------*/
/*------------------OSTATNI INICIALIZACE-----------------*/
/*-------------------------------------------------------*/

/** Nastaveni casovace TPM1 - ovladani podsviceni displaye pomoci PWN
*/
void timer_init(void){

    /* Vypnuti casovace */
    TPM1SC_CLKSA = 0;
    TPM1SC_CLKSB = 0;

    /* Nulovani casovace */
    TPM1CNT = 0;

    /* Nastaveni koncove hodnoty citani */
    TPM1MOD = LCD_COL_MAX;

    /* Nastaveni kazdeho z kanalu 3(R) 4(G) 5(B) casovace do rezimu Edge-aligned PWN: high-true pulses */
    /* Input-Capture | Output-Capture edge-aligned */
    TPM1SC_CPWMS = 0;

    /* RED CHANNEL */
    TPM1C3SC_MS3B = 1; /* Edge aligned */
    TPM1C3SC_ELS3A = 0; /* High True Pulses */
    TPM1C3SC_ELS3B = 1; /* High True Pulses */
    /* GREEN CHANNEL */
    TPM1C4SC_MS4B = 1; /* Edge aligned */
    TPM1C4SC_ELS4A = 0; /* High True Pulses */
    TPM1C4SC_ELS4B = 1; /* High True Pulses */
    /* BLUE CHANNEL */
    TPM1C5SC_MS5B = 1; /* Edge aligned */
    TPM1C5SC_ELS5A = 0; /* High True Pulses */
    TPM1C5SC_ELS5B = 1; /* High True Pulses */
    
    /* Podsviceni zvoleno podle prehravaneho tonu */
    switch(tone)
    {
    	case C:
    		TPM1C3V = 153;
    		TPM1C4V = 0;
    		TPM1C5V = 0;
    		break;
    	case CC:
			TPM1C3V = 153;
    		TPM1C4V = 0;
    		TPM1C5V = 51;
    		break;
    	case D:
    		TPM1C3V = 153;
    		TPM1C4V = 0;
    		TPM1C5V = 102;
    		break;
    	case DD:
    		TPM1C3V = 153;
    		TPM1C4V = 0;
    		TPM1C5V = 153;
    		break;
    	case E:
    		TPM1C3V = 153;
    		TPM1C4V = 0;
    		TPM1C5V = 204;
    		break;
    	case F:
    		TPM1C3V = 153;
    		TPM1C4V = 0;
    		TPM1C5V = 250;
    		break;
    	case FF:
    		TPM1C3V = 153;
    		TPM1C4V = 51;
    		TPM1C5V = 250;
    		break;
    	case G:
    		TPM1C3V = 153;
    		TPM1C4V = 102;
    		TPM1C5V = 250;
    		break;
    	case GG:
    		TPM1C3V = 153;
    		TPM1C4V = 153;
    		TPM1C5V = 250;
    		break;
    	case A:
    		TPM1C3V = 153;
    		TPM1C4V = 204;
    		TPM1C5V = 250;
    		break;
    	case AA:
    		TPM1C3V = 153;
    		TPM1C4V = 255;
    		TPM1C5V = 255;
    		break;
    	case H:
    		TPM1C3V = 51;
    		TPM1C4V = 250;
    		TPM1C5V = 250;
    		break;			
    }

    /* Zapnuti casovace */
    /* Zdroj hodin = "Bus rate clock" */
    /* Preddeleni = 128 */
    TPM1SC = 128;
    TPM1SC_CLKSA = 1;
    TPM1SC_CLKSB = 0;
    
    return;
}

/** Inicializace casovace TPM2 - Buzzer 
*/
void timer2_init(void ){

    /* Vypnuti casovace */
    TPM2SC_CLKSA = 0;
    TPM2SC_CLKSB = 0;

    /* Nulovani casovace */
    TPM2CNT = 0;  

    /* Nastaveni koncove hodnoty citani */
    TPM2MOD = MCU_FREQ/PRESCALER/tone_freq * 256;

    /* Nastaveni casovace do rezimu Edge-aligned PWN: high-true pulses */
    /* Input-Capture | Output-Capture edge-aligned */
    TPM2SC_CPWMS = 0;

    /* Edge aligned */
    TPM2C0SC_MS0B = 1;
    /* High True Pulses */
    TPM2C0SC_ELS0A = 0;
    /* High True Pulses */
    TPM2C0SC_ELS0B = 1;

    /* Nastaveni hodnoty kanalu */
    TPM2C0V = TPM2MOD / 2;

    /* Zapnuti casovace */
    /* Zdroj hodin = "Bus rate clock" */
    /* Preddeleni = 128 */    
    TPM2SC = PRESCALER;
    TPM2SC_CLKSA = 1;
    TPM2SC_CLKSB = 0;

	/* Vizualizace */
	timer_init();
	init_rtc();

    return;
}

/** Nastavi obvod realneho casu - Preruseni po 0,5s
*/
void init_rtc(void ){

    /* Nastaveni modulo registru */
    RTCMOD = 0x00;
    /* Zdrojem je 1kH LFO */
    RTCSC_RTCLKS0 = 0;
    RTCSC_RTCLKS1 = 0;
    /* Preruseni po 0,5s */
    RTCSC_RTCPS = 0b1110;
    /* Povoleni preruseni */
    RTCSC_RTIE = 1;

}

/** Nastaveni a povoleni preruseni IRQ EXT 
*/
void init_irq_ext(void ){

    /* Edges only */
    //IRQSC_IRQMOD = 0;
    IRQSC_IRQEDG = 0;
    
    /* Internal Pull-up */
    //IRQSC_IRQPDD = 0;
    
    /* Enable */
    IRQSC_IRQPE = 1;
    IRQSC_IRQIE = 1;
    
    return;
}

/** Inicializace Buzzeru 
*/
void init_buzzer(void ){
    
    /* Output */
    PTFDD_PTFDD4 = 1;
    
    return;
}

/** Inicializace LCD
*/
void lcd_init(){

  /* 8 bit data length, 2 lines, instruction table */
  lcd_send_CMD(0x39);
  delay(50);
  /* BS: 1/5, 2 line LCD */
  lcd_send_CMD(0x14);
  delay(50);
  /* booster on, contrast C5, set C4 */
  lcd_send_CMD(0x55);
  delay(50);
  /* set voltage follower and gain */
  lcd_send_CMD(0x6D);
  delay(50);
  /* set contrast C3, C2, C1 */
  lcd_send_CMD(0x78);
  delay(50);
  /* switch back to instruction table 0 */
  lcd_send_CMD(0x38);
  delay(200);
  /* display on, cursor on, cursor blink */
  lcd_send_CMD(0x0F);
  delay(50);
  /* delete display, cursor at home */
  lcd_send_CMD(0x01);
  delay(200);
  /* cursor auto-increment */
  lcd_send_CMD(0x06);
  delay(50);
  /* return home */
  lcd_send_CMD(0x02);
  delay(50);
  /* display on, cursor on, cursor position on */
  lcd_send_CMD(0x0C);
  delay(50);

  /* UVODNI INFORMACE NA LCD */
  /* return home */
  lcd_send_CMD(0x02);
  delay(50);

  i = sprintf(str_tmp, " 8bit Producer ");
  // zaslani retezce str_tmp na LCD (nulty radek, smazat pred zapsanim)
  lcd_send_str(str_tmp, 0, 1);
  // zaslani vyzvy na dalsi radek
  i = sprintf(str_tmp, "<-SELECT MODE->");
  lcd_send_str(str_tmp, 1, 0);

  return;
}

/*-------------------------------------------------------*/
/*----------------FUNKCE PRO PRACI S LCD-----------------*/
/*-------------------------------------------------------*/

/** Zasle byte na SPI
*@byte b - zasilany byte
*/
void spi_send_byte(byte b){

  // cekani na prazdny buffer (predchozi odesilani)
  while(!SPI2S_SPTEF);
  // zapis bytu do SPI datoveho registru
  SPI2DL = b;

  return;
}

/** Zasle prikaz na LCD
*@byte cmd - prikaz pro LCD
*/
void lcd_send_CMD(byte cmd){

  // zapisujeme prikazy
  PTAD_PTAD2 = 0; /*  nahodit -SS */
  PTAD_PTAD3 = 0;
  // zaslani bytu na spi
  spi_send_byte(cmd);
  /* SPI Transmit Buffer Empty Flag */
  // dokud neni posilani dokonceno
  while(!SPI2S_SPTEF);
  // cekani
  delay(50);
  // konec zapisu
  PTAD_PTAD2 = 1;   /*  shodit -SS */
  // cekani na zapis
  delay(50);

  return;
}

/** Zasle data na LCD
*@byte data - zasilany byte
*/
void lcd_send_DATA(byte data){

   // zapisujeme data
  PTAD_PTAD2 = 0; /*  nahodit -SS */
  PTAD_PTAD3 = 1;
  // zaslani bytu na spi
  spi_send_byte(data);
  /* SPI Transmit Buffer Empty Flag */
  // dokud neni posilani dokonceno
  while(!SPI2S_SPTEF);
  // cekani
  delay(50);
  // konec zapisu
  PTAD_PTAD2 = 1;   /*  shodit -SS */
  // cekani na zapis
  delay(50);

  return;
}

/** Smazani LCD
*/
void lcd_clear(){

  // smazani LCD pomoci prikazu 0x01
  lcd_send_CMD(0x01);
  // cekani na LCD
  delay(200);

  return;
}

/** Zobrazuje retezec na LCD
*@unsigned char str[MAX_STR_LEN] - retezec k zobrazeni na LCD
*@unsigned char line - volba radku LCD (0 - horni /1 - spodni)
*@unsigned char clr - volba ponechani obsahu LCD pred zapisem retezece ( 0 - ponechat, 1 - smazat)
*/
void lcd_send_str(unsigned char str[MAX_STR_LEN], unsigned char line, unsigned char clr){

  // pomocna promenna
  unsigned int i;
  // smazat LCD?
  if(clr) lcd_clear();
  // na ktery radek zapisovat?
  // nastaveni prvniho radku
  if(line == 0) lcd_send_CMD(0x80);
  // nastaveni druheho radku
  else lcd_send_CMD(0xC0);
  // cekani pred nastavenim
  delay(200);

  // zapis dat (znaku) na LCD
  for(i = 0; str[i]!= 0; i++){

    lcd_send_DATA(str[i]);

  }

  return;

}

/*-------------------------------------------------------*/
/*------FUNKCE PRO PRACI S KURZOROVYMI TLACITKY A LED----*/
/*-------------------------------------------------------*/


/** FUNKCE PRO PRACI S KLAVESNICI **/
/* Podle zadane masky vypne LED pod tlacitky */
void btn_leds_off(unsigned char mask){

    LED_PORTD &= (~mask);

    return;
}

/** Podle zadane masky zapne LED pod tlacitky 
*/
void btn_leds_on(unsigned char mask){

    LED_PORTD |= mask;

    return;
}

/** Vypne kurzorova tlacitka a OK ponecha aktivni 
*/
void btn_disable(){
	
	/* Necha aktivni pouze OK tlacitko */
	/*0bRIGHT|LEFT|NIC|NIC|NIC|OK|DOWN|UP|*/
	KBIPE &= 0b00111100;
	
	return;
}

/** Zapne kurzorova tlacitka
*/
void btn_enable(){
	
	/* Zapnuti preruseni od vsech kurzorovych tlacitek */
    KBIPE |= 0b11000111;

	return;
}

/** Inicializace tlacitek 
*/
void btn_leds_init(unsigned int del){

    /* Inicializace portu tlacitek KBI */
    /* Povoleni preruseni od KBI */
    KBISC_KBIE = 1;
    /* Aktivace KBI vyvodu 7,6,x,x,x,2,1,0 (maska 1100.0111) */
    KBIPE |= 0b11000111;
    /* Nastaveni citlivosti KBI na sestupnou hranu */
    KBIES = 0;

    /* Inicializace portu a stavu LED */
    /* PTD2-6 na vystup (PORT C Data Direction Register) */
    LED_PORTDD = BTN_LED_ALL;

    /* Animace LED  */
    while(del > 500){
        btn_leds_on(BTN_LED_UP);
        delay(del);
        btn_leds_off(BTN_LED_ALL);
        del -= del/15;
        btn_leds_on(BTN_LED_RIGHT);
        delay(del);
        btn_leds_off(BTN_LED_ALL);
        del -= del/15;
        btn_leds_on(BTN_LED_DOWN);
        delay(del);
        btn_leds_off(BTN_LED_ALL);
        del -= del/15;
        btn_leds_on(BTN_LED_LEFT);
        delay(del);
        btn_leds_off(BTN_LED_ALL);
        del -= del/15;
    }
    
    btn_leds_on(BTN_LED_ALL);             

    del = 20000; 
    btn_leds_off(BTN_LED_OK);
    delay(del);
    btn_leds_on(BTN_LED_OK);
    delay(del);  
    btn_leds_off(BTN_LED_OK);
    delay(del);
    btn_leds_on(BTN_LED_OK);
    delay(del);         
    
    led_change();

    return;
}

/* Zjisteni stavu tlacitka */
unsigned int btn_state(){

    unsigned char btn_state = 0;

    // Nastaveni promenne btn_state dle toho, zda je tlacitko stlaceno, ci nikoliv
    if(~BTN_PORT1D & BTN_UP) btn_state |= BTN_UP;
    if(~BTN_PORT1D & BTN_DOWN) btn_state |= BTN_DOWN;
    if(~BTN_PORT1D & BTN_LEFT) btn_state |= BTN_LEFT;
    if(~BTN_PORT1D & BTN_RIGHT) btn_state |= BTN_RIGHT;
    if(~BTN_PORT2D & 4) btn_state |= BTN_OK;

    return(btn_state);
}

/** Zmeni podsvetleni tlacitek podle soucasneho stavu aplikace
*/
void led_change(){

    /* Podsvetluji se pouze funkcni klavesy */
    unsigned char mask = 0;
    switch(state){

        case PIANO:
            mask |= BTN_LED_LEFT;
            mask |= BTN_LED_RIGHT;
            break;
        /* PLAY a REC maji shodne podvetleni */
        case PLAY:
        case REC:
            mask |= BTN_LED_LEFT;
            mask |= BTN_LED_RIGHT;
            mask |= BTN_LED_UP;
            mask |= BTN_LED_DOWN;
            if(ok){
              mask |= BTN_LED_OK;
            }
            break;
        default:
            break;

    }

    /* Vypnuti podsvetleni vsech kurzorovych tlacitek */
    btn_leds_off(BTN_LED_ALL);
    /* Zapnuti podsvetleni tlacitek dle masky */
    btn_leds_on(mask);

    return;
}

/*-------------------------------------------------------*/
/*----------POMOCNE FUNKCE PROVADENI PROGRAMU------------*/
/*-------------------------------------------------------*/


/** Zmeni soucasny mod aplikace
*   |PIANO|REC|PLAY|
*@unsigned char new_state - stav na ktery se ma prejit - PIANO, REC, PLAY
*/
void change_state_to(unsigned char new_state){

    switch(new_state){

        case PIANO:
            /* Zmena stavu na PIANO a vypsani na display - 1 radek, smazani */
            state = PIANO;
            /* Zmena podsvetleni */
            led_change();
            /* Zapnuti preruseni od klavesnice */
            /* */
            /* Zaslani info o PIANO modu na display */
            lcd_send_str(str_piano, 0, 1);
            /* Smazani udaje o melodii */
            i = sprintf(str_tmp, "");
            lcd_send_str(str_tmp, 1, 0);
            break;
        case REC:
            /* Zmena stavu na REC a vypsani na display - 1 radek, smazani */
            state = REC;
            /* Zmena podsvetleni */
            led_change();
            /* Zapnuti preruseni od klavesnice */
            /* */
            /* Zaslani info o REC modu na display */
            lcd_send_str(str_rec, 0, 1);
            /* Vypsani udaje o melodii */
            i = sprintf(str_tmp, "Melody: %d", melody + 1);
            lcd_send_str(str_tmp, 1, 0);
            break;
        case PLAY:
            /* Zmena stavu na PLAY a vypsani na display - 1 radek, smazani */
            state = PLAY;
            /* Zmena podsvetleni */
            led_change();
            /* PLAY mod na LCD */
            lcd_send_str(str_play, 0, 1);
            /* Vypsani udaje o melodii */
            i = sprintf(str_tmp, "Melody: %d", melody + 1);
            lcd_send_str(str_tmp, 1, 0);
            break;
        default:
            /* Zmena stavu na PIANO a vypsani na display - 1 radek, smazani */
            state = PIANO;
            /* Zmena podsvetleni */
            led_change();
            /* Zapnuti preruseni od klavesnice */
            /* */
            /* Zaslani info o PIANO modu na display */
            lcd_send_str(str_piano, 0, 1);
            /* Smazani udaje o melodii */
            i = sprintf(str_tmp, "");
            lcd_send_str(str_tmp, 1, 1);
            break;
    }

    return;
}

/** Zmeni soucasnou melodii
*@unsigned char direction - udava smer zmeny - 1 nahoru, 0 dolu
*/
void shift_melody(unsigned char direction){

    switch(direction){

        case UP:
            /* Prechod na dalsi melodii */
            melody = (melody + 1) % MAX_MELODY_COUNT;
            /* Vypsani pozice na display - spodni radek, smazat */
            i = sprintf(str_tmp, "Melody: %d", melody + 1);
            lcd_send_str(str_tmp, 1, 0);
            break;
        case DOWN:
            /* Prechod na predchozi melodii */
            melody = (melody - 1);
            /* Osetreni zaporne melodie */
            if(melody < 0 || melody >= MAX_MELODY_COUNT) melody = MAX_MELODY_COUNT - 1;
            /* Vypsani melodie na spodni radek LCD */
            i = sprintf(str_tmp, "Melody: %d", melody + 1);
            lcd_send_str(str_tmp, 1, 0);
            break;
        default:
            break;
    }

    return;
}

/** Prehraje ton o zadane delce a vypise ho na LCD
*@unsigned char tone - prehravany ton
*@unsigned char length - delka tonu
*/
void play_tone(unsigned char myTone, unsigned char length){

    /* jen kvuli warningu*/
	length = 0;
	tone = myTone;

    switch(myTone){

        case C:
            tone = C;
            tone_freq = F_C;
            if(state != PIANO){
            	i = sprintf(str_tmp, "Melody: %d     C ", melody + 1);
            }
            else{
            	i = sprintf(str_tmp, "              C ");
            }
            lcd_send_str(str_tmp, 1, 0);
            timer2_init();
            break;
        case CC:
            tone = CC;
            tone_freq = F_CC;
            if(state != PIANO){
            	i = sprintf(str_tmp, "Melody: %d     C#", melody + 1);
            }
            else{
            	i = sprintf(str_tmp, "              C#");
            }
            lcd_send_str(str_tmp, 1, 0);
            timer2_init();
            break;
        case D:
            tone = D;
            tone_freq = F_D;
            if(state != PIANO){
            	i = sprintf(str_tmp, "Melody: %d     D ", melody + 1);
            }
            else{
            	i = sprintf(str_tmp, "              D ");
            }
            lcd_send_str(str_tmp, 1, 0);
            timer2_init();
            break;
        case DD:
            tone = DD;
            tone_freq = F_DD;
            if(state != PIANO){
            	i = sprintf(str_tmp, "Melody: %d     D#", melody + 1);
            }
            else{
            	i = sprintf(str_tmp, "              D#");
            }
            lcd_send_str(str_tmp, 1, 0);
            timer2_init();
            break;
        case E:
            tone = E;
            tone_freq = F_E;
            if(state != PIANO){
            	i = sprintf(str_tmp, "Melody: %d     E ", melody + 1);
            }
            else{
            	i = sprintf(str_tmp, "              E ");
            }
            lcd_send_str(str_tmp, 1, 0);
            timer2_init();
            break;
        case F:
            tone = F;
            tone_freq = F_F;
            if(state != PIANO){
            	i = sprintf(str_tmp, "Melody: %d     F ", melody + 1);
            }
            else{
            	i = sprintf(str_tmp, "              F ");
            }
            lcd_send_str(str_tmp, 1, 0);
            timer2_init();
            break;
        case FF:
            tone = FF;
            tone_freq = F_FF;
            if(state != PIANO){
            	i = sprintf(str_tmp, "Melody: %d     F#", melody + 1);
            }
            else{
            	i = sprintf(str_tmp, "              F#");
            }
            lcd_send_str(str_tmp, 1, 0);
            timer2_init();
            break;
        case G:
            tone = G;
            tone_freq = F_G;
            if(state != PIANO){
            	i = sprintf(str_tmp, "Melody: %d     G ", melody + 1);
            }
            else{
            	i = sprintf(str_tmp, "              G ");
            }
            lcd_send_str(str_tmp, 1, 0);
            timer2_init();
            break;
        case GG:
            tone = GG;
            tone_freq = F_GG;
            if(state != PIANO){
            	i = sprintf(str_tmp, "Melody: %d     G#", melody + 1);
            }
            else{
            	i = sprintf(str_tmp, "              G#");
            }
            lcd_send_str(str_tmp, 1, 0);
            timer2_init();
            break;
        case A:
            tone = A;
            tone_freq = F_A;
            if(state != PIANO){
            	i = sprintf(str_tmp, "Melody: %d     A ", melody + 1);
            }
            else{
            	i = sprintf(str_tmp, "              A ");
            }
            lcd_send_str(str_tmp, 1, 0);
            timer2_init();
            break;
        case AA:
            tone = AA;
            tone_freq = F_AA;
            if(state != PIANO){
            	i = sprintf(str_tmp, "Melody: %d     A#", melody + 1);
            }
            else{
            	i = sprintf(str_tmp, "              A#");
            }
            lcd_send_str(str_tmp, 1, 0);
            timer2_init();
            break;
        case H:
            tone = H;
            tone_freq = F_H;
            if(state != PIANO){
            	i = sprintf(str_tmp, "Melody: %d     H ", melody + 1);
            }
            else{
            	i = sprintf(str_tmp, "              H ");
            }
            lcd_send_str(str_tmp, 1, 0);
            timer2_init();
            break;
        default:
            break;

    }

    return;
}

/** Prehrava soucasne zvolenou melodii
*   Melodie je ukoncena znakem MELODY_END
*/
void play_melody(){

    /* Lokalni promenne, aby nahodou nepreslo k prepsani promenne pri prehravani */
    unsigned char tmp_melody = melody;
    unsigned char tmp_i = 0;
    /* Nastaveni indikace prehravani */
    playing = 1;
    /* Vypnuti preruseni od kurzorovych tlacitek a dotykove klavesnice - pouze preruseni od OK, ktere vypne prehravani */
    /* Pokud se stiskne tlacitko OK, nastavi se playing na 0 a prehravani se ukonci */
    while((melodies[tmp_melody][tmp_i] != MELODY_END)){

		if(playing == 0) break;
        /* Prehrani tonu s danou delkou a vypsani na LCD */
        play_tone(melodies[tmp_melody][tmp_i], 10000);
        /* Oddeleni dalsiho tonu */
        delay(10000);
        delay(10000);
        delay(10000);
        delay(10000);
        delay(10000);
        delay(10000);
        /* Dalsi ton */
        tmp_i++;
    }
    
    /* Konec prehravani, nastaveni indikace */
    playing = 0;

    /* Zapnuti preruseni od tlacitek */
	btn_enable();
	
    return;
}

/** Inicializuje seznam melodii
*/
void init_melody_list(){

    i = 0;
    for(i; i < MAX_MELODY_COUNT; i++){

            /* Zapsani ukoncovaciho znaku melodie */
            melodies[i][0] = MELODY_END;

    }

    return;
}

/** Nahraje ton do zvoleneho uloziste
*@unsigned char tone - hrany ton
*/
void rec_tone(unsigned char tone){

    unsigned char position = 0;
    unsigned char tmp_melody = melody;
    /* Prohledavani konce melodie */
    /* Slo by udelat efektivneji udrzovanim soucasne pozice v melodii */
    /* Takhle je to ale prehlednejsi a jednodussi */
    while(melodies[tmp_melody][position] != MELODY_END){

        position++;

    }

    /* Melodie by byla delsi nez jeji maximalni delka */
  
	if(position >= (MAX_MELODY_SIZE)) return;
    /* Zapsani tonu */
    melodies[tmp_melody][position] = tone;
    /* Ukonceni melodie */
    position++;
    melodies[tmp_melody][position] = MELODY_END;

    return;
}

/** Nahraje melodii hranou na klavesnici do zvoleneho uloziste
*/
void rec_melody(){

    /* Vypnuti preruseni od kurz tlacitek */
    btn_disable();
    /* Smazani soucasne melodie - nove nahravani */
    recording = 1;
    melodies[melody][0] = MELODY_END;
    /* Zapnuti preruseni od kurz tlacitek */

    return;
}

/** Cekani pomoci NOP instrukci
*@unsigned int n - delka cekani
*/
void delay(unsigned int n){

    // pomocne promenne
    unsigned int i, j;
    for(i = 0; i < n; i++){

        for(j = 0; j < 1; j++){
            // no operation - cekani
            asm nop;
        }
    }

    return;
}

/*-------------------------------------------------------*/
/*---------------FUNKCE PRO PRACI S IIC------------------*/
/*-------------------------------------------------------*/

/** Cteni hodnoty na I2C sbernici
*/
unsigned char iic_read_button (void)
{
    /* Vysledek */
    unsigned char result;
    /* Spodni Byte hodnoty */
    byte data_low;
    /* Horni Byte hodnoty */
    byte data_high;

    /* Cteni spodniho Bytu */
    data_low = iic_read_reg(0x00);
    delay(100);
    /* Cteni horniho Bytu */
    data_high = iic_read_reg(0x01);
    delay(100);

    switch (data_low)
    {
        case 1 :
                 result = C;
                 break;
        case 2 :
                 result = CC;
                 break;
        case 4 :
                 result = D;
                 break;
        case 8 :
                 result = DD;
                 break;
        case 16 :
                 result = E;
                 break;
        case 32 :
                 result = F;
                 break;
        case 64 :
                 result = FF;
                 break;
        case 128 :
                 result = G;
                 break;
        default :
            switch (data_high)
            {
              case 1:
                      result = GG;
                      break;
              case 2:
                      result = A;
                      break;
              case 4:
                      result = AA;
                      break;
              case 8:
                      result = H;
                      break;
              default:
                      result = NO_KEY;
                      break;
            }
    }

    return result;
}

/** Inicializace komunikace pres I2C
*/
void init_iic (void)
{
    /* Zapnuti I2C */
    IICC_IICEN = 1;
    /* ACK se negeneruje */
    IICC_TXAK = 1;
    /* Slave */
    IICC_MST = 0;
    /* Speed */
    IICF = 0x99;
    /* RW bit = 0 */
    IICS_SRW = 0;

    return;
}

/** Zapisuje data na cilovy registr zarizeni pripojeneho
*   na sbernici I2C
*@byte target_reg - cilovy registr
*@byte data - posilana data
*/
void iic_write_reg (byte target_reg, byte data)
{
    /* Adresa */
    byte addr;
    
    /* 7 bit adresa */
    addr = (slave_addr)<<1;
    IICC_TXAK = 0;
    /* Generovani startu a nastaveni TX1*/
    IICC |= 0x30;

    /* Vystaveni adresy slave zarizeni na sbernici */
    IICD = addr;
    /* Cekani */
    while (!IICS_IICIF);
    /* Nulovani priznaku */
    IICS_IICIF=1;
    /* Kontrola RXAK */
    while(IICS_RXAK);

    /* Vystaveni adresy registru slave zarizeni na sbernici */
    IICD = target_reg;
    /* Cekani */
    while (!IICS_IICIF);
    /* Nulovani priznaku */
    IICS_IICIF=1;
    /* Kontrola RXAK */
    while(IICS_RXAK);

    /* Vystaveni dat */
    IICD = data;
    /* Cekani */
    while (!IICS_IICIF);
    /* Nulovani priznaku */
    IICS_IICIF=1;
    /* Kontrola RXAK */
    while(IICS_RXAK);

    /* Nulovani priznaku */
    IICS_IICIF=1;
    /* Generovani STOP */
    IICC_MST = 0;

    return;
}

/** Cteni registru slave zarizeni na I2C
*@byte source_reg - adresa registru pro cteni
*/
byte iic_read_reg(byte source_reg)
{
    /* Prectena data */
    byte read_data;
    /* Adresa */
    byte addr;
    /* Adresa je 7 bitova - posuv */
    addr = (slave_addr)<<1;
    IICC_TXAK = 0;
    /* Generovani Startu */
    IICC |= 0x30;

    /* Vystaveni adresy slave na sbernici */
    IICD = addr;
    /* Cekani */
    while (!IICS_IICIF);
    /* Nulovani Flagu */
    IICS_IICIF=1;
    /* Kontrola RXAK */
    while(IICS_RXAK);

    /* Vystaveni adresy registru slave */
    IICD = source_reg;
    /* Cekani */
    while (!IICS_IICIF);
    /* Nulovani Flagu */
    IICS_IICIF=1;
    /* Kontrola RXAK */
    while(IICS_RXAK);

    /* Repeated Start */
    IICC_RSTA = 1;
    /* Cteni */
    IICD = addr|1;
    /* Cekani */
    while (!IICS_IICIF);
    /* Nulovani */
    IICS_IICIF=1;
    /* Kontrola RXAK */
    while(IICS_RXAK);

    /* Nastaveni prijmu */
    IICC_TX = 0;
    /* Zruseni ACK */
    IICC_TXAK = 1;
    /* Dummy read */
    read_data = IICD;
    /* Cekani */
    while (!IICS_IICIF);
    /* Nulovani */
    IICS_IICIF=1;
    /* Generovani stop */
    IICC_MST = 0;
    read_data = IICD;

    /* Vraceni prectenych dat */
    return read_data;
}

/** Inicializace obvodu MPR121
*/
void init_MPR121(void)
{
    byte i;

    /* AUTO-CONFIG Control Registru 0 */
    iic_write_reg(0x7B, 0x14);
    delay(10);
    /* AUTO-CONFIG USL Registru */
    iic_write_reg(0x7D, 0x9C);
    delay(10);
    /* AUTO-CONFIG LSL Registru */
    iic_write_reg(0x7E, 0x65);
    delay(10);
    /* AUTO-CONFIG Target Level Registru*/
    iic_write_reg(0x7F, 0x8C);
    delay(10);

    //nastavení prahu citlivosti
    for (i = 0x41; i < 0x59; i += 2)
    {
        iic_write_reg(i, 0x0F);
        delay(10);
        iic_write_reg(i + 1, 0x0A);
        delay(10);
    }

    /* Spusteni */
    iic_write_reg(0x5E, 0x0C);
    delay(10);

    return;

}

/** Nastavuje adresu Slave zarizeni do globalni promenne
*@byte addr - adresa zarizeni
*/
void set_slave_addr(byte addr)
{
    /* Nastaveni adresy */
    slave_addr = addr;
    return;
}

/** Vypnuti preruseni od IRQ EXT
*/
void irq_disable(void ){
  
  IRQSC_IRQIE = 0;
  
  return;
}

/** Zapnuti preruseni od IRQ EXT
 * */
void irq_enable(void ){
	
	IRQSC_IRQIE = 1;
	
	return;
}

/*-------------------------------------------------------*/
/*------------------OBSLUHY PRERUSENI--------------------*/
/*-------------------------------------------------------*/


/** Obsluha preruseni od kurzorovych tlacitek (25) 
*/
interrupt VectorNumber_Vkeyboard void ISR_kbi(){

    /* Zjisteni stavu tlacitek */
    unsigned int tmp_i = btn_state();
    //unsigned char value;
    
    /* Potvrzeni preruseni */
    KBISC_KBACK = 1;

    /* Odstaneni zakmitu */
    delay(10000);
    if(tmp_i != btn_state()) tmp_i = 0;

    /* Zhasnout stlacena tlacitka */
    btn_leds_off(tmp_i*4);
    /* Zapnuti podsviceni podle stavu */
    led_change();

    /* FUNKCE TLACITEK */
    /* |PIANO|REC|PLAY| */
    /* Funkce tlacitek v PLAY modu */
    /* Levym a pravym kurzorovym tlacitkem se prepina mod */
    /* Tlacitko "Nahoru" a "Dolu" prepina uloziste */
    if(state == PLAY){

        /* Prepnuti modu */
        /* Leve tlacitko ma prioritu */
        if(tmp_i & BTN_LEFT){

            /* Zmena stavu na REC */
            change_state_to(REC);

        }
        else if(tmp_i & BTN_RIGHT){

            /* Zmena modu na PIANO */
            change_state_to(PIANO);

        }

        /* Nahoru - Prepnuti melodie */
        if(tmp_i & BTN_UP){

            /* Dalsi melodie */
            shift_melody(UP);

        }
        else if(tmp_i & BTN_DOWN){

            /* Predchozi melodie */
            shift_melody(DOWN);

        }

        /* OK - Prehrani melodie */
        if(tmp_i & BTN_OK){

            /* Prehrani melodie */
            if(playing == 0) play_melody();
            else 
            {
            	playing = 0;
            }

        }
    }
    /* Funkce tlacitek v REC modu */
    else if(state == REC){

        /* Prepnuti modu */
        /* Leve tlacitko ma prioritu */
        if(tmp_i & BTN_LEFT){

            /* Zmena stavu na PIANO */
            change_state_to(PIANO);

        }
        else if(tmp_i & BTN_RIGHT){

            /* Zmena stavu na PLAY */
            change_state_to(PLAY);

        }

        /* Nahoru - Prepnuti melodie */
        if(tmp_i & BTN_UP){

            /* Dalsi melodie */
            shift_melody(UP);

        }
        else if(tmp_i & BTN_DOWN){

            /* Predchozi melodie */
            shift_melody(DOWN);

        }

        /* OK - Zaznam melodie */
        if(tmp_i & BTN_OK){

			//lcd_send_str("recording", 1, 1);
            /* Zaznam melodie */
            if(recording == 0) rec_melody();
            else 
            {
            	recording = 0;
            	btn_enable();
            }

        }

    }
    /* Funkce tlacitek v PIANO modu - vychozi mod */
    else{

        /* Prepnuti modu */
        /* Leve tlacitko ma prioritu */
        if(tmp_i & BTN_LEFT){

            /* Zmena stavu na PLAY */
            change_state_to(PLAY);

        }
        else if(tmp_i & BTN_RIGHT){

            /* Zmena stavu na REC */
            change_state_to(REC);

        }
        /* Ostatni tlacitka vypnuta */
    }

  return;
}

/** Obsluha preruseni od klavesnice (0-9*#)
*/
interrupt VectorNumber_Virq void ISR_keyboard(){

	/* Prijeti preruseni */
	IRQSC_IRQACK = 1;

	/* Precteni hodnoty stlaceneho tlacitka */
	tone = iic_read_button();
	/* Zpracovavam pouze platne tony - osetreni preruseni od pusteni tlacitka */
	if(tone >= C && tone <= H){    
	    if(state == PIANO){

	        /* Pouze prehraje zadany ton a vypise ho na display + vizualizace */
	        play_tone(tone, TONE_LENGTH);

	    }
	    else if(state == REC){

	        /* Prehraje zadany ton */
	        play_tone(tone, TONE_LENGTH);
	        /* Nahraje zadany ton, vypise ho na display + vizualizace */
	        if(recording == TRUE) rec_tone(tone);

	    }
	    else if(state == PLAY){

	        ;
	        /* Sem by se program nemel dostat - preruseni by zde melo byt vypnuto */

	    }
	    else return;
	}
	else return;

    return;
}

/** Obsluha preruseni od RTC modulu
*/
interrupt VectorNumber_Vrtc void ISR_RTC(){

    unsigned char leds = btn_state();
    
    /* Shozeni interrupt flagu */
    RTCSC = RTCSC | 0x80;

    /* Blikani OK pri modu PLAY a REC bez nahravani, nebo prehravani */
    if(state == REC || state == PLAY){
        /* Rozsviceni, nebo zhasnuti tlacitka OK */
        /* Rozsviceno -> zhasnout */
        /* Prehrava/Nahrava se - nechat OK zapnute */
        if( (recording == 1) || (playing == 1)){
            ok = 1;
            led_change();
        }
        /* Blikat */
        else{
            if( ok ){
                ok = 0;
                led_change();
            }
            /* Zhasnuto -> Rozsvitit */
            else {
                ok = 1;
                led_change();
            }
        }
    }
    /* Pri PIANO modu je tlacitko vypnuto */
    else{
        ok = 0;
        led_change();
    }
    
    /* Vypnuti prehravaneho tonu */
    TPM2SC_CLKSA = 0;
    TPM2SC_CLKSB = 0;
    /* Zapnuti preruseni od klavesnice */
    init_irq_ext();
    
    /* Vypnuti vizualizace */
    TPM1SC_CLKSA = 0;
    TPM1SC_CLKSB = 0;
    					 
    /* Smazani Indikace melodie z displaye */
    if(state != PIANO){
    	i = sprintf(str_tmp, "Melody: %d", melody + 1);
    }
    else{
    	i = sprintf(str_tmp, "");
    }
    /* Prepsani displaye */
    lcd_send_str(str_tmp, 1, 0);	


    return;
}

/*-------------------------------------------------------*/
/*-----------------------PROGRAM-------------------------*/
/*-------------------------------------------------------*/


/** Samotny program 
*/
void main(void) {
    
    /* Inicializace MCU */
    mcu_init();
    /* Inicializace RTC */
    init_rtc();
    /* Inicializace timeru */ 
    timer_init(); 
    /* Inicializace LCD */
    lcd_init();
    /* Inicializace tlacitek */
    btn_leds_init(20000);       
    /* Inicializace I2C */
    init_iic();
    /* Nastaveni adresy klavesnice */
    set_slave_addr(0x5A);
    /* Inicializace obvodu MPR121 (dotykova klavesnice) */
    init_MPR121();
    /* Nastaveni a povoleni preruseni IRQ EXT */
    init_irq_ext();
    /* Inicializace buzzeru */
    init_buzzer();    
    /* Inicializace seznamu melodii */
    init_melody_list();    
    /* Povoleni preruseni */
    EnableInterrupts;

    /* Programova smycka */
    for(;;) {
        /* Watchdog je vypnut */
        //__RESET_WATCHDOG(); /* kicks the dog in the *** */
    } /* loop forever */
}
