/**
 * Lukasz Bany
 * Emil Hotkowski
 *
 * Rozklad przyciskow na klawiaturze:
 *      0   1   2   3
 *      4   5   6   7
 *      8   9   A   B
 *      C   D   E   F
 *  X   ~   ~   ~
 *
 *  X - zatwierdzenie wpisanej liczby
 *  ~ - brak funkcji
 */

#include <8051.h>

#define uint_16_t unsigned short

/**
 * Definicje segmentow wyswietlacza LCD
 */
#define SEG_A 0x02
#define SEG_B 0x04
#define SEG_C 0x40
#define SEG_D 0x10
#define SEG_E 0x08
#define SEG_F 0x01
#define SEG_G 0x20
#define SEG_H 0x80

/**
 * Definicje wygladu cyfr na wyswietlaczu (od 0 do F).
 */
#define ZERO SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F
#define ONE SEG_B | SEG_C
#define TWO SEG_A | SEG_B | SEG_D | SEG_E | SEG_G
#define THREE SEG_A | SEG_B | SEG_C | SEG_D | SEG_G
#define FOUR SEG_B | SEG_C | SEG_F | SEG_G
#define FIVE SEG_A | SEG_C | SEG_D | SEG_F | SEG_G
#define SIX SEG_A | SEG_C | SEG_D | SEG_E | SEG_F | SEG_G
#define SEVEN SEG_A | SEG_B | SEG_C
#define EIGHT SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F | SEG_G
#define NINE SEG_A | SEG_B | SEG_C | SEG_D | SEG_F | SEG_G
#define CHAR_A SEG_A | SEG_B | SEG_C | SEG_E | SEG_F | SEG_G
#define CHAR_B SEG_C | SEG_D | SEG_E | SEG_F | SEG_G
#define CHAR_C SEG_A | SEG_D | SEG_E | SEG_F
#define CHAR_D SEG_B | SEG_C | SEG_D | SEG_E | SEG_G
#define CHAR_E SEG_A | SEG_D | SEG_E | SEG_F | SEG_G
#define CHAR_F SEG_A | SEG_E | SEG_F | SEG_G

xdata at 0xFFFF unsigned char U10; // okresla, ktore segmenty swieca sie na wybranej pozycji wyswietlacza
xdata at 0x8000 unsigned char U12; // okresla wcisniety klawisz
xdata at 0x8000 unsigned char U15; // okresla pozycje liczby na wyswietlaczu lub kolumne wcisnisnietego klawisza

// tablica zawierajaca definicje wszystkich cyfr
char code digits[10] = {ZERO, ONE, TWO, THREE, FOUR, FIVE, SIX, SEVEN, EIGHT, NINE,
CHAR_A, CHAR_B, CHAR_C, CHAR_D, CHAR_E, CHAR_F};

// tablica pozycji na wyswietlaczu
int position[4] = {0xDF, 0xEF, 0x7F, 0xBF};

// tablica zawierajace wyswietlane w danym momencie cyfry (poczatkowo 0000)
char toDisplay[4] = {digits[0], digits[0], digits[0], digits[0]};

// zmienna iterujaca po pozycjach wyswietlacza
int which_display = 0;

// okresla jaki klawisz zostal wcisniety
int pressed = -1;

// steruje logowaniem w konsoli wcisnietego klawisza
int debug = 1;

// wartosc klawisza, ktory byl wcisniety podczas ostatniego sprawdzenia (zmienna pomocnicza, do obslugi przytrzymanego klawisza)
int lastPressed = -1;

//ilosc cykli, podczas ktorych klawisz byl wcisniety
int timePressed = 0;

// wartosc kolejnych kolumn na klawiaturze ustawiana w U15
int columns[4] = {0x07, 0x0B, 0x0D, 0x0E};

// zmienna do iterowania po kolumnach
int colIndex = 0;

// indeks kolumny do ktorej nalezy przycisk wcisniety podczas ostatniego sprawdzenia (zmienna pomocnicza, do obslugi przytrzymanego klawisza)
int lastCol = -1;

// bufor, w ktorym przechowywana jest wpisywana liczba
uint_16_t writeBuffer = 0;

// indeks wskazujacy, ktora cyfra liczby jest wpisywana (liczac od konca)
int writeBufferIndex = 3;

/**
 * Ustawia ilosc zliczanych impulsow zanim nastapi przetwanie.
 * Funkcja musi byc wykonywana po kazdym przerwaniu.
 */
void setCounter() {
    // ilosc zliczonych impulsow = 65535 - THL0
    TH0 = 0xF0;
    TL0 = 0x0;
}

/**
 * Ustawia wartosci rejestrow.
 */
void init() {
    SCON = 0x50;
    TMOD &= 0x0f;
    TMOD |= 0x21;
    TCON = 0x40;
    PCON |= 0x80;
    TH1=TL1 = 0xFD;
    TI = 0;

    EA=1;
    ET0=1; // bit zezwolenia na obsluge przerwan zwiazanych z licznikiem T0
    TR0=1; // wlacza uklad licznikowy T0

    setCounter();
}

/**
 * Funkcja wyswietla podany znak na terminalu.
 * @param znak do wyswietlenia
 */
void putchar(char znak){
    SBUF=znak;
    while(TI==0);
    TI=0;
}

/**
 * Wyswietla cyfrę na pozycji okreslonej przez zmienna which_display
 * (przyjmujacej wartosc od 0 do 3) i ja inkrementuje (lub zeruje,
 * jesli wynosi 3).
 */
void handleDisplay() {
    U15 = position[which_display];
    U10 = toDisplay[which_display];
    which_display = (which_display + 1) % 4;
}

/**
 * Funkcja wyswietla na terminalu wcisniety klawisz, jesli zmienna debug jest ustawiona.
 */
void logKeyPressed() {
    if (!debug) {
        return;
    }

    if (pressed >= 0 && pressed <= 9) putchar('0' + pressed);
    if (pressed == 10) putchar('a');
    if (pressed == 11) putchar('b');
    if (pressed == 12) putchar('c');
    if (pressed == 13) putchar('d');
    if (pressed == 14) putchar('e');
    if (pressed == 15) putchar('f');
    if (pressed == 20) putchar('~');
}

/**
 * Funkcja wykonywana podczas przerwania. Sprawdza stan klawiatury
 * i jesli ktorys z przyciskow zostanie przytrzymany odpowiednio
 * dlugo, ustawia zmienna pressed na jego wartosc.
 * Jesli przycisk zostanie przytrzymany po zanotowaniu jego nacisniecia,
 * to czas do ponownego zarejestrowania jego wartosci jest dluzszy.
 */
void handleNumbersOnKeyboard() {
    int keys;
    int holdKey = 0;

    U15 = 0x00;
    U15 = columns[colIndex];

    keys = (U12 & 0xf0) >> 4;
    if (keys != 0x0f) {
        lastCol = colIndex;
        if (keys == 0x0E) holdKey = (0 * 4) + (3 - colIndex);
        if (keys == 0x0D) holdKey = (1 * 4) + (3 - colIndex);
        if (keys == 0x0B) holdKey = (2 * 4) + (3 - colIndex);
        if (keys == 0x07) holdKey = (3 * 4) + (3 - colIndex);

        // Jesli obecnie wcisniety przycisk jest tym samym,
        // ktory byl wcisniety ostatnio, zwieksz czas wcisniecia.
        // Jesli nie - zmien ostatnio wcisniety klawisz na obecnie
        // wcisniety i wyzeruj czas.
        if (holdKey == lastPressed) {
            timePressed++;
        } else {
            lastPressed = holdKey;
            timePressed = 0;
        }

        // jesli klawisz byl wcisniety przez dluzej niz 2 cykle
        // ustaw zmienna pressed na jego wartosc.
        if (timePressed >= 2) {
            pressed = holdKey;
            logKeyPressed();
            timePressed = -20;  // wydluzenie czasu, po ktorym nastapi ponowne
                                // zarejestrowanie klawisza w przypadku jego przytrzymania.
        }
    } else if (colIndex == lastCol) {   // Jesli zaden klawisz nie jest wcisniety,
        timePressed = 0;                // zresetuj czas i ostatnio wcisnieta kolumne
        lastCol = -1;
    }

    U15 = 0xFF;                         // usuniecie z U15 wyzerowanych bitow, aby nie wplynac na dzialanie wyswielacza
    colIndex = (colIndex + 1) % 4;
}

/**
 * Funkcja wykonywana podczas przerwania. Sprawdza stan klawisza
 * potwierdzenia (dzialanie analogiczne do funkcji handleNumbersOnKeyboard()).
 */
void handleConfirmButton() {
    int holdKey = 0;

    if ((U12 & 0x0f) == 0x0e) {
        holdKey = 20;

        if (holdKey == lastPressed) {
            timePressed++;
        } else {
            lastPressed = holdKey;
            timePressed = 0;
        }

        if (timePressed >= 2) {
            pressed = holdKey;
            logKeyPressed();
            timePressed = -100;
        }
    }
}

/**
 * Funkcja wykonywana podczas przerwania. Sprawdza stan klawiatury,
 * wyswietla jedna liczbe na jednej pozycji i ustawia wartosc zliczanych impulsow.
 */
void handleDisplayAndKeyboard(void) interrupt 1 {
    handleNumbersOnKeyboard();
    handleConfirmButton();
    handleDisplay();
    setCounter();
}

/**
 * Funkcja zwracajaca wartosc potegi podanej liczby.
 * @param base liczba do potegowania
 * @param power potega
 * @return wynik potegowania
 */
int pow(int base, int power) {
    int i;
    int result = 1;
    for (i = 1; i <= power; i++) {
        result = result * base;
    }

    return result;
}

/**
 * Funkcja czeka na zarejestrowanie wcisniecia przycisku
 * i w zaleznosci od jego wartosci dopisuje cyfre do bufora
 * lub zwraca wartosc zapisana w buforze i resetuje bufor
 * (to samo robi w przypadku przepelnienia bufora).
 *
 * @return wartosc do wyswietlenia
 */
uint_16_t get_number_HEX() {
    int newDigit, numberToDisplay, multiplier;

    while (writeBufferIndex >= 0) {
        while (pressed == -1);
        if (pressed != 20) {
            newDigit = pressed;
            pressed = -1;

            multiplier = pow(16, writeBufferIndex);

            writeBuffer = (newDigit * multiplier) + writeBuffer;
            writeBufferIndex--;
        } else {
            pressed = -1;
            while (writeBufferIndex >= 0) {
                writeBuffer /= 16;
                writeBufferIndex--;
            }
        }
    }

    numberToDisplay = writeBuffer;
    writeBuffer = 0;
    writeBufferIndex = 3;
    return numberToDisplay;
}

/**
 * Funkcja ustawia podana liczbe jako liczbe do wyswietlenia
 * na wyswietlaczu LCD.
 * @param t liczba do wyswietlenia (nie moze byc ujemna - typ unsigned short)
 */
void print_LED_HEX(uint_16_t t) {
    char d0 = t / 4096 % 16;
    char d1 = t / 256 % 16;
    char d2 = t / 16 % 16;
    char d3 = t % 16;

    toDisplay[0] = digits[d0];
    toDisplay[1] = digits[d1];
    toDisplay[2] = digits[d2];
    toDisplay[3] = digits[d3];
}

void main() {
    uint_16_t t;
    init();

    for(;;) {
        t = get_number_HEX();
        t = t - 2;
        print_LED_HEX(t);
    }

}
