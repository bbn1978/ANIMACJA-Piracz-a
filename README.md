# ANIMACJA Piracz-a — Raspberry Pi Pico i HUB75 512×64

Po uruchomieniu projekt przewija kolorowy napis „WESOŁYCH ŚWIĄT BOŻEGO
NARODZENIA” na szesnastu modułach `DH5-64X32-8S-A-V1.0` ułożonych w dwóch
rzędach po osiem (512×64). Dotychczasowy ekran 256×64 pozostaje po prawej,
a nowy ekran 256×64 znajduje się po lewej. Litery mają 44 piksele wysokości i są
wyśrodkowane pionowo. Szerokość liter i odstępy skalują się proporcjonalnie
do ich wysokości; obrys liter jest umiarkowany, a glif „O” zaokrąglony.
Kolor napisu zmienia się po każdym pełnym przewinięciu, według palety RGB
i mieszanek kolorów.

## Logiczna numeracja paneli

Widok od przodu, numeracja od lewej do prawej niezależnie od kolejności
połączenia szeregowego:

| Rząd | Nowy ekran po lewej | Dotychczasowy ekran po prawej |
| --- | --- | --- |
| Górny | 1, 2, 3, 4 | 5, 6, 7, 8 |
| Dolny | 9, 10, 11, 12 | 13, 14, 15, 16 |

Po zakończeniu każdego pełnego przewinięcia uruchamia się 30-sekundowa
animacja: kolorowy Mikołaj idzie przez ekran, trzymając powiewającą flagę
Polski. Na białym polu znajduje się pogrubiony, czerwony napis „WESOŁYCH”,
a na czerwonym — pogrubiony, biały napis „ŚWIĄT”. W tle pada śnieg i świeci
choinka.
Po niej wyświetla się kolejna, 30-sekundowa animacja: ulica rozświetla się
przed przejazdem czerwonego tira z napisem „COCA COLA”. Świecą się okna
trójwymiarowych domów z białym szkieletem, lampy i świąteczne dekoracje,
jezdnia pod tirem jest magenta, a na ekranie pada śnieg. Czerwone dachy i kolorowe ściany rysują się
piksel po pikselu od strony tira, a okna, lampki i wieńce rozświetlają się stopniowo
80 pikseli przed nim. Domy po prawej są przesunięte za lampę. Żółte, okrągłe koła
tira obracają się podczas
przejazdu, a kabinę od naczepy oddziela dwupikselowa przerwa.
Potem wyświetla się kolejna, 30-sekundowa animacja sześciu kolorowych,
pięcioramiennych gwiazd, które poruszają się po ekranie i odbijają od jego
krawędzi jak piłki.
Następnie przez 30 sekund po całym ekranie latają 24 okrągłe piłki o różnych
średnicach, których kolory zmieniają się w trakcie animacji.
Na końcu uruchamia się gra Breakout: deseczka odbija piłkę, która rozbija
40 kolorowych cegiełek. Deseczka automatycznie podąża za piłką i ją odbija,
a gra trwa do zbicia ostatniej cegiełki. Gdy zostaje kilka klocków, piłka
celuje w te, które można trafić, a tempo gry przyspiesza, szczególnie przy
ostatnim klocku. Po grze przez 10 sekund po całym ekranie biegnie renifer
z animowanymi nogami, ciągnąc za sobą napis „KONIEC” o wysokości 38 pikseli,
zachowujący proporcje liter i oddalony od renifera o 10 pikseli. Renifer
przebiega ekran raz w dotychczasowym tempie, zatrzymuje się na 5 sekund,
aby napis był widoczny. Następnie przez 30 sekund po ekranie w obie strony
przesuwa się biały kontur z przesłanego przez użytkownika rysunku sań,
Mikołaja i zaprzęgu reniferów, dopasowany do rozdzielczości 256×64. Wnętrze
sań i Mikołaj są czerwoni, a renifery pomarańczowo-brązowe;
oryginalny biały kontur pozostaje widoczny, a kopyta poruszają się
naprzemiennie. Po tej animacji napis zaczyna przewijać się ponownie.
Klawiatura PS/2 nadal działa podczas animacji; F1–F10 wybierają kolor
i rozpoczynają przewijanie od nowa.

Mapowanie panelu 32×8 pozostaje bez zmian. Wiersze są mapowane tak jak
w referencyjnym projekcie 64×32: fizyczne 1–8 pobierają kolejno źródłowe
2, 3, 4, 5, 6, 7, 8, 1. Ten sam wzór powtarza się w blokach 9–16,
17–24 i 25–32, a w dolnej połowie z przesunięciem o 32.

Mapowanie paneli pozostaje zachowane. Test pojedynczej linii
(`ROW_MAPPING_TEST_MODE`) jest wyłączony; po uruchomieniu odtwarzane są
animacje. Test diagnostyczny (`DIAGNOSTIC_MODE`) również pozostaje wyłączony.
W razie włączenia test diagnostyczny pokazuje kolejno wiersze sterowane
przez adresy A/B/C; adresy 0–6 mają kolory: czerwony, zielony, niebieski,
żółty, magenta, cyjan i biały, a adres 7 miga na czerwono i niebiesko.

## Klawiatura PS/2

Można podłączyć klawiaturę PS/2:

| Sygnał klawiatury | Raspberry Pi Pico |
| --- | --- |
| CLK | GP21 |
| DATA | GP22 |
| GND | GND |
| VCC | Zasilanie zgodne z klawiaturą |

Obsługa została potwierdzona z działającą klawiaturą PS/2 zasilaną napięciem
3,3 V.

Połącz masy. Linie CLK i DATA klawiatury PS/2 mogą mieć poziom 5 V, a GPIO Pico
nie są odporne na 5 V. Zastosuj konwerter poziomów logicznych albo upewnij się,
że linie typu open-collector mają podciąganie wyłącznie do 3,3 V. Nie podłączaj
sygnału 5 V bezpośrednio do Pico.

Podczas wpisywania tekst jest od razu widoczny jako nieruchomy podgląd
wyrównany do prawej; gdy jest dłuższy od ekranu, widoczny jest jego końcowy
fragment. Backspace natychmiast odświeża podgląd. Naciśnij Enter, aby
zatwierdzić tekst i rozpocząć jego przewijanie. Pierwszy znak nowego wpisu
zastępuje poprzedni tekst. Shift i Caps Lock zmieniają
wielkość liter, a w układzie polskim programisty AltGr+A/C/E/L/O/S/X/Z wpisuje
odpowiednio ą/ć/ę/ł/ó/ś/ź/ż. Wielkie litery uzyskuje się z Shift+AltGr.
Skrót Alt+L wpisuje wielkie Ł, a Alt+Shift+L — małe ł. Glify obu liter
mają osobne, wyraźne ukośne kreski.
Litery F/f, H/h, V/v i X/x mają glify i obsługę kodów PS/2 Set 2.
Obsługiwane są tylko znaki mające glif w obecnej czcionce; pozostałe są
pomijane. Można wpisać do 1000 znaków; bufor uwzględnia kodowanie UTF-8.
Strzałka w prawo przechodzi do następnego elementu, a strzałka w lewo do
poprzedniego w cyklu: przewijany tekst → Mikołaj → tir → gwiazdy → piłki
→ Breakout → renifer → sanie z reniferami. Nawigacja działa także podczas gry.
Wybrany element uruchamia się od początku; automatyczne odtwarzanie cyklu
pozostaje włączone. Podczas gry deseczka porusza się automatycznie za piłką.
Strzałka w górę lub PageUp zwiększa, a strzałka w dół lub PageDown zmniejsza
prędkość przewijania. Dostępne poziomy to 1/32×, 1/16×, 1/8×, 1/4×, 1/2×,
1×, 2×, 4×, 8×, 16×, 25× i 32×. Wszystkie poziomy są teraz o 50% szybsze
niż przed zmianą; pięć naciśnięć strzałki w górę od 1× ustawia 25×.

Klawisze F1–F10 wybierają kolor początkowy przewijania i uruchamiają napis
od początku w wybranym kolorze. Potem kolory nadal zmieniają się cyklicznie
po każdym pełnym przewinięciu. Kolejność to: F1 czerwony, F2 zielony,
F3 niebieski, F4 żółty, F5 magenta, F6 cyjan, F7 biały, F8 czerwony,
F9 zielony, F10 niebieski. Sterownik matrycy używa 3-bitowego RGB, więc ma
7 różnych nieczarnych kolorów; ostatnie trzy pozycje powtarzają podstawowe.

Połącz wszystkie szesnaście modułów szeregowo i zachowaj dotychczasowe
okablowanie obecnego ekranu. Połącz OUT górnego lewego panelu obecnego ekranu
z IN dolnego prawego panelu nowego ekranu po lewej. W nowym ekranie łańcuch
biegnie dolnym rzędem od prawej do lewej, a następnie górnym rzędem od lewej
do prawej. Program mapuje kolumny nowego ekranu od lewej do prawej, jako
1, 2, 3, 4. Zachowaj dotychczasową orientację modułów.

## Pinout gniazda HUB75

Tabela odpowiada legendzie widocznej na zdjęciu. Podane numery Pico to GPIO
(`GP`), nie numery fizycznych wyprowadzeń płytki.

| Pin złącza | Sygnał | Pico |
| ---: | --- | --- |
| 1 | R1 | GP2 |
| 2 | B1 | GP4 |
| 3 | G1 | GP3 |
| 4 | GND | GND |
| 5 | R2 | GP5 |
| 6 | B2 | GP7 |
| 7 | G2 | GP6 |
| 8 | GND | GND |
| 9 | A | GP18 |
| 10 | B | GP19 |
| 11 | C | GP20 |
| 12 | NC — nie podłączać | — |
| 13 | CLK | GP13 |
| 14 | LAT | GP16 |
| 15 | OE | GP17 |
| 16 | GND | GND |

Przed podłączeniem sprawdź orientację numeracji pinów na swoim przewodzie
i złączu. Nie łącz żadnego sygnału z pinem opisanym `NC`.

## Zasilanie i poziomy logiczne

- Zasil wszystkie szesnaście paneli z osobnego, stabilizowanego zasilacza 5 V
  o odpowiedniej wydajności prądowej. Nie zasilaj ich z USB ani pinu 5 V Pico.
- Połącz masę zasilacza paneli z GND Pico.
- GPIO Pico pracują z napięciem 3,3 V. Zalecany jest bufor 3,3→5 V, na
  przykład 74AHCT245. Nie podawaj 5 V na GPIO Pico. Bezpośrednie sterowanie
  z 3,3 V może działać z niektórymi panelami, ale nie jest gwarantowane.
- Zegar Pico pozostaje na standardowych 125 MHz. Sygnały danych i zegara HUB75
  są sterowane bezpośrednio przez GPIO, bez PIO/DMA. Czas slotu wiersza to
  1000 µs, co daje do 125 Hz dla 8 adresów; rzeczywista częstotliwość zależy
  także od czasu przesyłania danych. Czas świecenia wynosi 3 µs.

## Kompilacja i wgrywanie

Wymagany jest Arduino CLI i rdzeń Arduino-Pico firmy Earle Philhower.
Z głównego katalogu projektu skompiluj firmware poleceniem:

```powershell
arduino-cli compile --clean --fqbn rp2040:rp2040:rpipico --build-path build .
```

Plik UF2 znajdzie się w `build` pod nazwą zgodną z nazwą pliku `.ino`.
W repozytorium dołączono też gotowy firmware:
`ANIMACJA_Piracz_a.ino.uf2`. Aby wgrać go ręcznie, przytrzymaj BOOTSEL
podczas podłączania Pico przez USB, a następnie skopiuj UF2 na dysk `RPI-RP2`.
