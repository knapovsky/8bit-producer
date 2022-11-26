# 8BIT Producer

# Úvod

Cílem projektu bylo implementovat jednoduchový záznamník a přehrávač melodií s vnitřní pamětí a vizualizací přehrávaného tónu. Projekt byl realizován na vývojovém přípravku s _MCU MC9S08JM60_ od firmy _Freescale_. Osobním cílem bylo využít většinu dostupných periferií MCU a vývojového kitu tak, aby byl program co možná nejméně náročný na výpočetní výkon a paměť.

## Schéma prvků využitých programem

 

![Keyboard](./images/8bit-producer-diagram.jpg)

- LCD - _EA DOGM162_
- Řadič dotykové klávesnice - _MPR121QR2_
- Buzzer - _PS1740P02_
- MCU _- MC9S08JM60_

# Popis funkčnosti programu

# ![State-machine](./images/8bit-producer-state-machine.jpg)

## Režimy programu

Podle zadání projektu má program pracovat ve 3 různých režimech. Mezi režimy je možné přepínat pomocí kurzorových tlačítek. Funkční tlačítka daného módu jsou indikována podsvícením ve formě modrých LED diod. Následuje popis jednotlivých režimů.

### Piano

V prvním režimu, který je označen jako _„Piano mode“_, program přijímá informace od dotykové klávesnice a na základě stlačeného tlačítka přehrává tón a mění podsvícení LCD displaye. Pro přechod do následujícího, nebo předchozího režimu je možné využít kurzorových tlačítek.

![Keyboard](./images/8bit-producer-keyboard-media.png)

### Nahrávání

_„Recording mode“_ pracuje podobně jako _„Piano mode“_, avšak umožňuje uživateli vybrat kurzorovými tlačítky číslo uložiště, do kterého může zaznamenat posloupnost tónu (melodii) pomocí dotykové klávesnice. Tlačítko OK spouští nahrávaní, které je indikováno změnou podsvícení OK tlačítka z blikání na svícení. Náhravání je ukončeno dalším stiskem tlačítka OK, jehož podsvícení se opět vrátí do stavu blikání. Tlačítka na změnu melodie a módu jsou v přůběhu nahrávání blokována. Pokud chce uživatel nahrávat do úložiště, kde je již dříve nahraná melodie, pak je původní melodie odstaněna a přepsána novou. Při nahrávání nejsou ukládány délky tónu a jejich odstupy. Pro přechod do jiného režimu je opět využito kurzorových tlačítek

![KeyboardRec](./images/8bit-producer-keyboard-rec.png)

### Přehrávání

_„Playing mode“_ přehrává předem zvolenou melodii z paměťového úložiště. Tóny jsou odděleny konstantním intervalem. Pro výběr melodie a přechod do jiného režimu slouží opět kurzorová tlačítka. Pro spuštění přehrávání je použito tlačítko OK a jako indikace přehrávání je využito změny podsvícení OK tlačítka z blikání na jeho zhasnutí (pro odlišení s módem nahrávání). Při přehrávání melodie je dotyková klávesnice blokována.

![KeyboardPlay](./images/8bit-producer-keyboard-play.png)

## Zadávání tónů

Jak již bylo několikrát uvedeno, tóny jsou zadávány pomocí dotykové klávesnice. Přehrávané tóny jsou ve frekvenčním rozsahu 1047 – 1976 Hz, což odpovídá třetí oktávě (C3 – H3).

![Keyboard2](./images/8bit-producer-keyboard.png)

Po zadání tónu je jeho název vypsán na display jak je znázorněno na Obr. 1. Tón je vizualizován dočasnou změnou podsvícení displaye, které je nastaveno tak, aby na sebe navazovalo – červená pro tón C až po světle modrou pro tón H, kde tóny mezi nimi jsou vizualizovány lineární interpolací zabarvení těchto dvou tónů. Tóny je možné zadávat bez jakéhokoliv omezení hned za sebou, avšak jejich délka je programově omezena a není možné bez opětovného stisknutí hrát tón delší jak _0,5_s.

# Popis implementace

K implementaci bylo využito prostředí _CodeWarrior 6.3_ a programovací jazyk C, který byl upřednostněn před Assemblerem díky tomu, že je více abstraktní a také kvůli mým dlouholetým zkušenostem s tímto jazykem.

Program není rozdělen do modulů, avšak obsahuje logické části, které je vhodné od sebe v popisu odlišit. Předem upozorňuji na stručnost popisu vzhledem k velmi kvalitním komentařům samotného zdrojového kódu, kde je možno o implementaci zjistit podrobnější informace, které by dokumentaci zbytečně znepřehledňovaly.

## Inicializace

Základní inicializace _MCU_ vypíná modul Watchdog, nastavuje _SPI_ a základní podsvícení _LCD_ displaye na modrou barvu (_0x0C_). Dále je inicializován modul _RTC_, který slouží k změně podsvícení tlačítka _OK_ v režimu nahrávání a přehrávání. _LCD_ je inicializováno sekvencí příkazů (lze je nalézt v dokumentaci k použitému displayi) a nasledným vypsáním informací o programu. Po ukončení animace na kurzorových tlačítkách je započato nastavení _I2C_ sběrnice a modulu _MPR121_, který je na sběrnici adresován hodnotou _0x5A_. Po odpovědi od modulu _MPR121_ jsou adresovány jeho registry, kde jsou vhodně nastaveny parametry dotykové klávesnice, které byli získány z dokumentace k tomuto modulu. Především je nutné nastavit správnou citlivost tlačítek, jelikož ty mohou být nastaveny i jako proximity senzor. Přerušení od modulu _MPR121_ je generováno jako _IRQ EXT_, které je nutné pro jeho přijetí povolit zápisem do registru _IRQSC_. Na konci inicializačního bloku je nastaven 4. bit registru _PTFDD_, čímž je 4. pin portu D nastaven jako výstupní a je tak možné generovat tón na _buzzeru_.

### Inicializace paměťového úložiště (seznamu melodií)

Vzhledem k tomu, že předem nevíme, kolik tónu uživatel nahraje, je potřeba vyhradit jednu pozici pro ukončovací znak (popř. ke každé melodii ukládat její velikost, což je stejně paměťově náročné jako ukládání ukončovacího znaku). Při inicializaci se tedy prochází seznamem melodií a na začátek každé melodie je zápsán tento ukončovací znak.

## Programová smyčka

Po inicializaci se provádění programu nacházi v nekonečné programové smyčce, kde se pomocí obslužných rutin reaguje na přerušení, která mohou přijít od kurzorových tlačítek, dotykové klávesnice, nebo modulu _RTC_, který byl nastaven tak, aby generoval přerušení každé _0,5_ sekundy.

### Obsluha přerušení kurzorových tlačítek

U kurzorových tlačítek může docházet k zákmitům, což je řešeno tím, že se po přijetí přerušení uloží současný stav tlačítek (bitové pole, kde hodnota 1 vyjadřuje stav „stisknuto“) a čeká se po určitý počet cyklů _MCU_ (v programu _10000_). Po uplynutí tohoto intervalu se opět zkontroluje stav tlačítek a následná obsluha je provedena pouze tehdy, když jsou oba stavy stejné.

Funkce tlačítek se liší dle aktuálního režimu programu (piano, nahrávání, přehrávání). Popis funkcí tlačítek v daných režimech byl již uveden, proto ho není potřeba zde znovu uvádět. Pro přechod na předchozí, nebo následující režim je volána funkce _change\_state\_to(STAV)_, která mění globální proměnou _state_ uchovávající informaci o aktuálním režimu programu, přepíná podsvětlení pomocí funkce _led\_change()_ a výpíše aktuální informace na _LCD_ display (režim na první řádek a číslo zvolené melodie na řádek druhý). Podobně jako pro změnu režimu, tak i pro změnu melodie je volána funkce _shift\_melody(direction)_, která podle zadaného parametru mění melodie směrem nahoru, nebo dolů (číslo právě zvolené melodie je uloženo v globální proměnné _melody_) a opět vypíše aktuální informace na _LCD_. Pro ošetření přechodu na melodii, která není v paměti alokována je použita operace _modulo_ _MAX\_MELODY\_COUNT_ (maximální počet melodií).

Pokud je program v režimu nahrávání a je stisknuto tlačítko _OK_, pak jsou vypnuta ostatní kurzorová tlačítka, aby uživatel nemohl v průběhu nahrávání změnit režim na jiný, nebo změnit melodii, do které se právě nahrává. Po stisknutí tohoto tlačítka je nastavena indikace nahrávání (globální proměnná _recording_), která rozhoduje o blikání/neblikání _OK_ tlačítka a o tom, zda má být tón přijatý z klávesnice zaznamenán, či nikoliv. Melodie je smazána (na začátek melodie je zapsána ukončovací hodnota _MELODY\_END_) a je očekáváno zadávání tónů uživatelem. Ty jsou následně zpracovány v obsluze přerušení od _MPR121_ (_IRQ EXT_).

Podobným způsobem probíhá i zpracování přerušení od _OK_ tlačítka v režimu přehrávání. Zde je volána funkce, která nastaví příznak přehrávání (_playing_ - opět pro změnu způsobu podsvětlení _OK_ tlačítka), vypne tlačítka pro přechod na jiný režim či melodii a následně pomocí cyklu prochází aktuálně zvolenou melodii, dokud nenarazí na hodnotu reprezentující konec melodie. Interval mezi přehrávanými tóny je pevně nastaven na _60000_ cyklů _MCU_. Po ukončení přehrávání jsou opět povolena tlačítka, která uživateli umožní změnit melodii, nebo režim. Také je odstraněn přiznak _playing_, který způsobí rozblikání tlačítka _OK_. Způsob přehrávání tónu a jeho generování bude popsán dále.

### Obsluha přerušení od modulu _MPR121_ (Dotykových tlačítek)

Vzhledem k tomu, že na 1 stisk tlačítka jsou generována 2 přerušení (stisk a puštění tlačítka), je nutné zjišťovat, o které přerušení se jedná. To je poměrně snadné, jelikož při puštění tlačítka již nejsou na sběrnici _I2C_ data, která by reprezentovala některé ze stlačených tlačítek. Opět se zde rozlišuje mezi režimy nahrávaní, přehrávání a piana a jsou prováděny jim odpovídající činnosti. V režimu piana je volána funkce _play\_tone()_, která zprostředkovává přehrávání tonu. Stejná funkce je volána i režimu nahrávání s tím rozdílem, že přímo za ní následuje volání funkce _rec\_tone()_, která podle toho, zda je nahrávání zapnuto (_recording_ = 1) zapíše aktuální tón na konec zvoleného paměťového uložiště (na konec právě zvolené melodie). Zápis se provádí vyhledáním pozice ukončovací hodnoty v melodii a následným zapsáním tónu na tutu pozici a ukončení melodie posunem ukončovací hodnoty na pozici následující. Pokud by chtěl uživatel nahrát melodii delší než je její maximální délka, pak funkce aktuální tón nezapíše a ponechá ukončovací hodnotu na pozici _MAX\_MELODY\_SIZE + 1_ (za posledním tónem melodie).

### Obsluha přerušení od modulu _RTC_

Jak již bylo dříve uvedeno, modul _RTC_ slouží v programu k indikaci stavu nahrání/přehrávání, kde pokud jsou dané příznaky nastaveny, pak na ně obsluha přerušení od modulu _RTC_ (přerušení nastaveno na _0,5_s) reaguje dle následující tabulky:

<table><tbody><tr><td width="96"><strong>Režim</strong></td><td width="96"><strong>playing = 1</strong></td><td width="96"><strong>recording = 1</strong></td><td width="96"><strong>Playing = 0</strong></td><td width="96"><strong>Recording = 0</strong></td></tr><tr><td width="96"><strong>Přehrávání</strong></td><td width="96">Nesvítí</td><td width="96">Nedefinováno</td><td width="96">Blikání</td><td width="96">Nedefinováno</td></tr><tr><td width="96"><strong>Nahrávání</strong></td><td width="96">Nedefinováno</td><td width="96">Svítí</td><td width="96">Nedefinováno</td><td width="96">Blikání</td></tr></tbody></table>

 

Dále je modul _RTC_ použit ke generování tónu, které je popsáno v následující podkapitole.

## Generování tónu a jeho vizualizace

Generování tónu zprostředkovává funkce _play\_tone(tone)_, která jako parameter přijímá typ přehrávaného tónu, podle kterého nastaví do globální proměnné _tone\_freq_ jeho frekvenci, vypíše jeho název do pravého rohu spodního řádku displaye a zavolá funkci _timer2\_init()_. Tato funkce se stará o syntézu tónu pomocí časovače _TPM2_, jehož výstup 0 se zapisuje přímo _do data registru portu F_ a určuje tak zda má _buzzer_ produkovat tón (1), či nikoliv (0). Časovač je nastaven do režimu _Edge-aligned PWM: high-true-pulses_, což umožnuje generovat tón pomocí pulsně šířkové modulace. Jako zdroj hodin časovače slouží _BUS Rate Clock MCU_, který je dělen hodnotou _128_. Nastavení Modulo registru _TPM2MOD_ a hodnoty kanálu _TPM2C0V_ pak probíhá podle následujících vztahů.

**_TPM2MOD = MCU\_FREQ/PRESCALER/tone\_freq \* 256_**

**_TPM2C0V = TPM2MOD / 2_****,**

kde _MCU\_FREQ_ je frekvence na které běži mikrokontrolér (nastaveno na _8000000_), _PRESCALER_ je předdělič této frekvence (_128_), _tone\_freq_ je frekvence tónu, který chceme generovat . Hodnota kanálu _TPM2C0V_ je polovina hodnoty _TPM2MOD_ – generovaný signál tónu totiž vždy v polovině svého intervalu mění hodnotu z 1 na 0.  Funkce _timer2\_init()_ po inicializaci časovače resetuje čítač modulu _RTC_ na hodnotu _0_ a ve chvíli, kdy od časovače přijde přerušení (_0,5s_) je přehrávání tónu ukončeno. _MCU_ tak nečeká v žádné smyčce a program není tak náročný.

Pro vizualizaci volá funkce _timer2\_init()_ funkci sobě velmi podobnou. Jedná se o funkci _timer\_init()_, která však oproti _timer2\_init()_ zprozdředkovává pulsně šířkovou modulaci na portu F, na který je na pinech _0_, _1_, _2_, _3_ a _5_ připojen _LCD_ display (resp. jeho podvětlení. Každý z výstupů časovače _TPM1_, který tato funkce ovládá je nastaven do stejného režimu jako časovač _TPM2_, do jeho modulo registru _TPM1MOD_ je vložena hodnota _LCD\_COL\_MAX_ (_250_)  a hodnoty registrů _TPM1C3V_, _TPM1C4V_ a _TPM1C5V_ jsou nastaveny dle odpovídajících hodnot požadované barvy. Změna podsvícení na původní hodnotu je opět provedena v obsluze přerušení od modulu _RTC_.

# Analýza paměťových nároků

Přestože má mikrokontrolér _MC9S08JM60_ velké množství paměti (_60KB_), je program implementován tak, aby v paměti nezabíral zbytečné místo. Kde to bylo možné, byly voleny datové typy _unsigned char_, avšak bylo nutné deklarovat i několik proměnných typu _unsigned int_ (pro uložení frekvence tónu a pro pomocné proměnné které byli použity ve for cyklech). Nejvíce paměti zabraly řetězce pro výpis informací na display – zde je stále prostor pro optimalizaci.

Program zabere v paměti _ROM 6072_ bajtů  a v paměti _RAM 206_ bajtů při nastavení velikosti úložiště na 5 melodií o deseti tónech.

# Nastavení programu

Ve zdrojovém souboru _main.c_ jsou vypsány použitelné frekvence, které je možné nahradit za aktuálně definované, aby mohl program generovat jinou oktávu, či jakékoliv jiné kombinace tónu. Je však potřeba dávat si pozor na příliž nízké frekvence, jelikož v závislosti na kvalitě a typu _buzzeru_ se může generovaný tón stát neslyšitelným.

Je také možné přizpůsobit syntézu tónu jiné frekvenci _MCU_, avšak není zaručeno, že při změně frekvence _MCU_ bude funkční komunikace na sběrnici _I2C_.

Z uživatelského hlediska je nejpodstatnějším nastavením velikost seznamu melodií a maximální délka melodie, kterou lze uložit. Toto nastavení se provádí změnou hodnot v definicích _MAX\_MELODY\_SIZE_ a _MAX\_MELODY\_COUNT_. Je nutno brát ohled na paměťové nároky programu při nastavení vysokých hodnot těchto definic.

# Použité zdroje

- Přednášky IMP
- Cvičení IMP
- [Datasheet MC9S08JM60](http://www.freescale.com/files/microcontrollers/doc/data_sheet/MC9S08JM60.pdf)
- [Datasheet DOG Series 3.3V](http://www.lcd-module.de/eng/pdf/doma/dog-me.pdf)
- [Datasheet PS1740P02-TDK](http://octopart.com/ps1740p02-tdk-154428)
- [Datasheet MPR121](http://www.freescale.com/files/sensors/doc/data_sheet/MPR121.pdf)
- Rady pana Ing. Josefa Stranadela, Ph.D. a pana Ing. Václava Šimka

# Metriky Projektu

1860 řádků kódu
