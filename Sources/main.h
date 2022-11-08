/**
* Projekt do predmetu IMP
* Zaznamnik a prehravac melodii s vnitrni pameti pro skladby a vizualizaci
* Tento program je urcen pro vyukovy kit VUT FIT
* Soubor: main.h
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

#ifndef MAIN_H
#define MAIN_H

/*-------------------------------------------------------*/
/*-------------------INICIALIZACE MCU--------------------*/
/*-------------------------------------------------------*/


/** Inicializace MCU 
*/
void mcu_init();

/*-------------------------------------------------------*/
/*------------------OSTATNI INICIALIZACE-----------------*/
/*-------------------------------------------------------*/

/** Nastaveni casovace TPM1 - ovladani podsviceni displaye pomoci PWN
*/
void timer_init(void );
/** Inicializace casovace TPM2 - Buzzer 
*/
void timer2_init(void );
/** Nastavi obvod realneho casu - Preruseni po 0,5s
*/
void init_rtc(void );
/** Nastaveni a povoleni preruseni IRQ EXT 
*/
void init_irq_ext(void );
/** Inicializace Buzzeru 
*/
void init_buzzer(void );

/*-------------------------------------------------------*/
/*----------------FUNKCE PRO PRACI S LCD-----------------*/
/*-------------------------------------------------------*/

/** Zasle byte na SPI
*@byte b - zasilany byte
*/
void spi_send_byte(byte );
/** Zasle prikaz na LCD
*@byte cmd - prikaz pro LCD
*/
void lcd_send_CMD(byte );
/** Zasle data na LCD
*@byte data - zasilany byte
*/
void lcd_send_DATA(byte );
/** Smazani LCD
*/
void lcd_clear();
/** Zobrazuje retezec na LCD
*@unsigned char str[MAX_STR_LEN] - retezec k zobrazeni na LCD
*@unsigned char line - volba radku LCD (0 - horni /1 - spodni)
*@unsigned char clr - volba ponechani obsahu LCD pred zapisem retezece ( 0 - ponechat, 1 - smazat)
*/
void lcd_send_str(unsigned char* , unsigned char , unsigned char );
/** Inicializace LCD
*/
void lcd_init();

/*-------------------------------------------------------*/
/*------FUNKCE PRO PRACI S KURZOROVYMI TLACITKY A LED----*/
/*-------------------------------------------------------*/

/** FUNKCE PRO PRACI S KLAVESNICI **/
/* Podle zadane masky vypne LED pod tlacitky */
void btn_leds_off(unsigned char );
/** Podle zadane masky zapne LED pod tlacitky 
*/
void btn_leds_on(unsigned char );
/** Vypne kurzorova tlacitka a OK ponecha aktivni 
*/
void btn_disable(void );
/** Zapne kurzorova tlacitka
*/
void btn_enable(void );
/** Inicializace tlacitek 
*/
void btn_leds_init(unsigned int );
/* Zjisteni stavu tlacitka */
unsigned int btn_state();
/** Zmeni podsvetleni tlacitek podle soucasneho stavu aplikace
*/
void led_change(void );

/*-------------------------------------------------------*/
/*----------POMOCNE FUNKCE PROVADENI PROGRAMU------------*/
/*-------------------------------------------------------*/

/** Zmeni soucasny mod aplikace
*   |PIANO|REC|PLAY|
*@unsigned char new_state - stav na ktery se ma prejit - PIANO, REC, PLAY
*/
void change_state_to(unsigned char );
/** Zmeni soucasnou melodii
*@unsigned char direction - udava smer zmeny - 1 nahoru, 0 dolu
*/
void shift_melody(unsigned char );
/** Prehraje ton o zadane delce a vypise ho na LCD
*@unsigned char tone - prehravany ton
*@unsigned char length - delka tonu
*/
void play_tone(unsigned char , unsigned char );
/** Prehrava soucasne zvolenou melodii
*   Melodie je ukoncena znakem MELODY_END
*/
void play_melody();
/** Inicializuje seznam melodii
*/
void init_melody_list();
/** Nahraje ton do zvoleneho uloziste
*@unsigned char tone - hrany ton
*/
void rec_tone(unsigned char );
/** Nahraje melodii hranou na klavesnici do zvoleneho uloziste
*/
void rec_melody();
/** Cekani pomoci NOP instrukci
*@unsigned int n - delka cekani
*/
void delay(unsigned int n);

/*-------------------------------------------------------*/
/*---------------FUNKCE PRO PRACI S IIC------------------*/
/*-------------------------------------------------------*/

/** Cteni hodnoty na I2C sbernici
*/
unsigned char iic_read_button (void );
/** Inicializace komunikace pres I2C
*/
void init_iic (void );
/** Zapisuje data na cilovy registr zarizeni pripojeneho
*   na sbernici I2C
*@byte target_reg - cilovy registr
*@byte data - posilana data
*/
void iic_write_reg (byte , byte );
/** Cteni registru slave zarizeni na I2C
*@byte source_reg - adresa registru pro cteni
*/
byte iic_read_reg(byte );
/** Inicializace obvodu MPR121
*/
void init_MPR121(void);
/** Nastavuje adresu Slave zarizeni do globalni promenne
*@byte addr - adresa zarizeni
*/
void set_slave_addr(byte );
/** Vypnuti preruseni od IRQ EXT
*/
void irq_disable(void );
/** Zapnuti preruseni od IRQ EXT
 * */
void irq_enable(void );

/*-------------------------------------------------------*/
/*------------------OBSLUHY PRERUSENI--------------------*/
/*-------------------------------------------------------*/

/** Obsluha preruseni od kurzorovych tlacitek (25) 
*/
interrupt VectorNumber_Vkeyboard void ISR_kbi();
/** Obsluha preruseni od klavesnice (0-9*#)
*/
interrupt VectorNumber_Virq void ISR_keyboard();
/** Obsluha preruseni od RTC modulu
*/
interrupt VectorNumber_Vrtc void ISR_RTC();

#endif
