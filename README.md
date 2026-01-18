# Symulacja dyskontu

**Po okazaniu projektu zostały poprawione 3 rzeczy:**

1. Procesy czekają zamknięcie się dzieci
2. Kolejki do kas i do logera są lepiej zabezpieczone przed przepełnieniem
3. Kierownik używa własnego czasu, co zapobiega przedwczesnemu kończeniu symulacji

## 1. Użyty soft

| Kat.             | Nazwa                        |
| ---------------- | ---------------------------- |
| System           | openSUSE Kalpa               |
| Kontener         | openSUSE Tumbleweed (Podman) |
| Język            | C11                          |
| Edytor kodu      | Kate                         |
| Kompilator       | GCC 15.2.1                   |
| System budowania | CMake 4.1.3 + GNU Make 4.4.1 |

## 2. Zależności

### openSUSE Tumbleweed / Slowroll / Leap

```sh
sudo zypper install git gh gcc cmake make
sudo zypper install -t pattern devel_basis
```

### openSUSE MicroOS / Aeon / Kalpa

```sh
sudo transactional-update run zypper install git gh gcc cmake make
sudo transactional-update run zypper install -t pattern devel_basis
```

## 3. Budowanie

```sh
gh repo clone Azafanatic/dyskont-sim
cd dyskont-sim
chmod +x buildnrun.sh
./buildnrun.sh
```

## 4. Wymagania

* Dokumentacja wymaganych w użyciu rozwiązań
* Walidacja danych
* Użycie `perror()` i `errno`
* Minimalne prawa dostępu dla utworzonych struktur
* Wyczyszczenie pamięci po zamknięciu programu
* Kod w C/C++
* Użycie `fork()` i `exec()`

## 5. Opis kodu
### sim (main.c)

Program główny, odpowiada za:

* utworzenie pamięci dzielonej, kolejek i semaforów,
* ustawienie wartości domyślnych,
* walidację danych,
* uruchomienie innych programów,
* powiadomienie innych programów o konieczności zakończenia działania,
* usunięcie pamięci dzielonej, kolejek i semaforów.

Jest to w zasadzie program czuwający nad rozpoczęciem i zakończeniem symulacji.

### utils.c / utils.h

Zawiera definicje kluczy, domyślnych wartości oraz struktur. Ustawia też domyślne wartości dla języka. Posiada funkcje potrzebne w wielu miejscach, takie jak podłączanie pamięci, zapisywanie logów, sprawdzanie długości kolejek czy inicjalizacja tłumaczenia.

### logger.c / logger.h

Pozwala zapisywać, w kolejności wykonywania funkcji, kolorowe logi, które wyświetlają się zarówno na wyjściu standardowym, jak i są zapisywane do pliku. Robi to za pomocą kolejki komunikatów, do której logi dodawane są przez użycie save_a_log().

### manager.c

Otwiera i zamyka sklep, decyduje też, kiedy jakie kasy należy otworzyć oraz przeprowadza (jeśli jest taka potrzeba) ewakuację.

### checkout.c

Program uruchamia MAX_CHECKOUTS procesów kas. Główna pętla czeka na zakończenie programu, aż będzie mogła odpiąć pamięć współdzieloną. Kasy stacjonarne same sprawdzają wiek klienta w przypadku próby kupienia alkoholu i nie zacinają się. Jeśli są otwarte, odczytują klientów z kolejki i wystawiają im paragony, jeśli transakcja przebiegnie pomyślnie.

### self_service_checkout.c

Kasy samoobsługowe działają podobnie jak stacjonarne, z tym że w przypadku problemów wysyłają, przy pomocy kolejki komunikatów, wiadomość do procesu obsługi i czekają na odpowiedź. Kiedy ją otrzymają, decydują, czy wystawić klientowi paragon, czy odmówić zakupów.

### staff.c

Program czeka na pojawienie się próśb o zatwierdzenie wieku lub odblokowanie kasy (np. w przypadku nieprawidłowej wagi) i je obsługuje.

## 6. Opis implementacji

### IPC

Program główny (sim) tworzy wszystko, co potrzebne do działania komunikacji między procesami. Tworzy semafory, pamięć współdzieloną oraz kolejki komunikatów. Pozostałe programy jedynie się pod nie podpinają. Podobnie wygląda to w przypadku usuwania i odpinania zasobów. Wszystkie klucze zdefiniowane są w `utils.h`.

Pamięć współdzielona przechowuje rozmaite dane — począwszy od ustawień symulacji, poprzez identyfikatory wszystkich kolejek, a na danych kas kończąc. Dodatkowo semafory zabezpieczają sekcje krytyczne, takie jak kolejki czy stany kas.

### Algorytm zarządzania kasami

Kasy są otwierane i zamykane zgodnie z następującymi zasadami:

* Min. 3 kasy samoobsługowe są zawsze otwarte
* Kasy stacjonarne są domyślnie zamknięte
* Kasa stacjonarna nr 1 otwiera się, jeśli w kolejce znajduje się ≥ 3 osoby
* Kasy stacjonarne zamykają się po 30 s od obsłużenia wszystkich klientów
* Pozostałe kasy (samoobsługowe: od 4 do `MAX_SS_CHECKOUTS`, stacjonarne: od 2) otwierają się kolejno, gdy zajdzie taka potrzeba (min. jedna kasa na K klientów)
* Zamykają się, gdy liczba klientów spadnie poniżej `K · (N − 3)`

```sh
FUNKCJA menage_checkouts
    UTWÓRZ tablicę prev_open_checkout
    DLA każdego checkoutu
        zapamiętaj poprzedni stan otwarcia

    // Wyznaczenie liczby aktywnych kas
    active ← floor(liczba_klientów / K)

    JEŻELI liczba_klientów < K · (active − 3)
        active ← active − 1

    JEŻELI active < 3
        active ← 3

    JEŻELI active > maksymalna_liczba_kas
        active ← maksymalna_liczba_kas

    // Sterowanie pierwszą kasą tradycyjną na podstawie kolejki
    JEŻELI długość_kolejki > 3
        otwórz pierwszą kasę tradycyjną
    WPP JEŻELI długość_kolejki = 0
        zamknij pierwszą kasę tradycyjną

    // Otwieranie odpowiedniej liczby kas
    i ← 0
    DOPÓKI active > 0
        JEŻELI kasa[i] jest zamknięta
            otwórz kasę[i]
        active ← active − 1
        i ← i + 1

    // Sekcja krytyczna
    ZABLOKUJ semafor kas

    // Aktualizacja kas samoobsługowych
    DLA każdej kasy samoobsługowej
        JEŻELI jej stan się zmienił
            zapisz nowy stan do pamięci współdzielonej

    // Aktualizacja kas tradycyjnych
    DLA każdej kasy tradycyjnej
        JEŻELI jej stan się zmienił
            JEŻELI kasa została otwarta
                wyślij sygnał OTWARCIA
            WPP JEŻELI kasa została zamknięta ORAZ
                minął odpowiedni czas od ostatniego klienta
                wyślij sygnał ZAMKNIĘCIA

    // Zliczanie otwartych kas samoobsługowych
    policz otwarte kasy samoobsługowe
    zapisz wynik do pamięci współdzielonej

    // Zliczanie otwartych kas tradycyjnych
    policz otwarte kasy tradycyjne
    zapisz wynik do pamięci współdzielonej

    ODBLOKUJ semafor kas

KONIEC FUNKCJI

```

### Obsługa błędów

Błędy są obsługiwane przez sprawdzanie wartości zwracanych funkcji; w razie ich wystąpienia wywoływana jest funkcja `perror()` z przetłumaczonym komunikatem. Dane wejściowe walidowane są przez funkcję `parse_int`, która w razie problemów zamyka program i wypisuje błąd.

## 7. Napotkane problemy

W zasadzie udało się zrealizować wszystkie wymagania projektu. Spośród napotkanych problemów można wymienić:

* proces zmiany kolejki, w której czeka klient,
* powracający problem kas, które nie chciały obsługiwać klientów,
* proces otwierania i zamykania kas.

Pierwszy problem udało się rozwiązać poprzez zabezpieczenie dostępu do kolejek semaforami oraz ich usuwanie i odbudowę w momencie, gdy klient chce je opuścić.
Drugi problem za każdym razem wynikał z czegoś innego, więc wymagał różnych rozwiązań. Jednym z nich było otwieranie i zamykanie kas samoobsługowych przy pomocy zmiennych w pamięci dzielonej zamiast sygnałów.
Trzeci problem wymagał napisania dość skomplikowanego algorytmu, który spełniał wszystkie warunki i nie tylko zliczał, ile kas jest potrzebnych, ale także informował kasy o otwarciu lub zamknięciu tylko wtedy, gdy ich stan faktycznie się zmieniał.

## 8. Elementy dodatkowe

* Logger jako osobny proces umożliwiający wypisywanie zsynchronizowanych, podpisanych i pokolorowanych wiadomości na standardowym wyjściu oraz zapisywanie ich do pliku w bardzo prosty sposób.
* Cały program jest przetłumaczony przy użyciu `gettext`, co pozwala na jego uruchamianie w języku systemowym. Dodatkowo pliki językowe są wczytywane z katalogu, w którym znajduje się plik wykonywalny, dzięki czemu nie jest konieczna integracja z systemem.
* Pełna dokumentacja wykonana przy pomocy narzędzia Doxygen.

## 9. Testy
Czy klient może kupić inną ilość rzeczy niż przewidziano (3-10)?
```sh
number_of_products = MIN_PRODUCTS + rand() % (MAX_PRODUCTS - MIN_PRODUCTS + 1);
```
Nie, klient wybiera losową ilość produktów z tego zakresu

Czy kasa blokuje się przy zakupie alkoholu?
```sh
[OBSŁUGA] Weryfikuję klienta 308531... Ma 55 lat.
[OBSŁUGA] Klient 308531 ma co najmniej 18 lat.
[KASA SAMOOBSŁUGOWA] (5) Witaj (308531)!
Twoja lista zakupów:
Batonik Wino Jägermeister Piwo Sok pomarańczowy Masło Szynka Makaron 
Do zapłaty: 163,02 PLN.
Dziękujemy!

...

[KASA] (0) Witaj (308998)!
Twoja lista zakupów:
Chleb Chleb Jägermeister Woda mineralna Musztarda Dżem 
Masz tylko 17 lat, nie mogę sprzedać Ci tego produktu!
[KLIENT] (308998): :C
```
Tak, samoobsługowa wzywa pracownika, w stacjonarnej sprawdza kasjer.


Czy kasa poprawnie generuje raport?
```sh
[KASA SAMOOBSŁUGOWA] (3) Witaj (308431)!
Twoja lista zakupów:
Chleb Wino Ser żółty Ryż 
Do zapłaty: 69,06 PLN.
Dziękujemy!
[KLIENT] (308784): Stanę w kolejce do kasy. Mój numer to 100.
[KLIENT] Paragon
PID klienta: 308431
ID kasy: SS_3
Zakupione produkty:
Chleb – 4,29 PLN
Wino – 39,99 PLN
Ser żółty – 19,99 PLN
Ryż – 4,79 PLN
Suma: 69,06 PLN
```
Tak.

Czy zawsze są otwarte min. 3 kasy samoobsługowe?
```sh
if (active < 3) active = 3;
```
Tak.

Czy kasa otwiera się, gdy liczba klientów przekracza wymaganych 3?
```sh
[INFO] Klienci w sklepie: 296	 Kolejka do kas samoobsługowych: 56	 Otwarte kasy samoobsługowe: 0
[INFO] Kolejka do kasy 1.: 3	 Kolejka do kasy 2.: 0	 Otwarte kasy: 0
[KLIENT] (338494): Dzień dobry!
[KLIENT] (338494): Kupię 7 produktów.
[KASA SAMOOBSŁUGOWA] Wymagana pomoc obsługi: Nieprawidłowa waga produktu
(1) Witaj (338253)!
Twoja lista zakupów:
Orzechy Szynka Masło Szynka 
Do zapłaty: 74,96 PLN.
Dziękujemy!
```
Tak.

Czy kasa się zamyka kiedy przez 30 s nie ma klientów?
```sh
[KIEROWNIK] Closing CASHIER checkout 1 after 30s inactivity
```
Tak.

Czy klienci, którzy stali przy kasie przed podjęciem decyzji o jej zamknięciu, są poprawnie obsługiwani?
```sh
//	SYGNAŁ
shm_checkouts->checkout[id].open = 0;
if (id == 0) {
	clients_left = queue_length(shm_queues->msq_checkout_one);
} else {
	clients_left = queue_length(shm_queues->msq_checkout_two);
}
 
---
// PĘTLA GŁÓWNA
if (shm_checkouts->checkout[id].open == 0 && clients_left == 0) {
	usleep(500000 / shm_sim_settings->sim_speed);
	continue;
};

//	*OBSŁUGA*
if (shm_checkouts->checkout[id].open == 0 && clients_left > 0) {
	clients_left--;
} else {
	clients_left = 0;
}

```
Tak, kasa zapamiętuje liczbę klientów w kolejce przed zamknięciem.

Czy sygnały 1, 2 i 3 są poprawnie obsługiwane?
```sh
//	SYGNAŁ 1.
[KASA] Otwieram kasę
//	SYGNAŁ 2.
[KASA] Zamykam kasę
//	SYGNAŁ 3.
Dżem – 7,49 PLN
Suma: 161,52 PLN
[KLIENT] (856491): Do widzenia!
[KIEROWNIK] Czas na ewakuację!
[KIEROWNIK] Zamykanie wszystkich kas...
[INFO] Zatrzymywanie symulacji...
[INFO] Obsłużeni klienci:
Kasy samoobsługowe:
[INFO] (0) : 119 klientów.
[INFO] (1) : 115 klientów.
[INFO] (2) : 118 klientów.
[INFO] (3) : 118 klientów.
[INFO] (4) : 125 klientów.
[INFO] (5) : 116 klientów.
[INFO] Kasy:
[INFO] (0) : 30 klientów.
[INFO] (1) : 0 klientów.
📦[kacper@tumbleweed dyskont-sim]$ 

```
Tak.

Czy pamięć jest poprawnie czyszczona?
```sh
[INFO] (5) : 1237 klientów.
[INFO] Kasy:
[INFO] (0) : 397 klientów.
[INFO] (1) : 30 klientów.
📦[kacper@tumbleweed dyskont-sim]$ ipcs

------ Message Queues --------
key        msqid      owner      perms      used-bytes   messages    

------ Shared Memory Segments --------
key        shmid      owner      perms      bytes      nattch     status      

------ Semaphore Arrays --------
key        semid      owner      perms      nsems     

```
Tak.

## 10. Wymagane funkcje

#### 10.1. Tworzenie i obsługa plików:
* `open()`: https://github.com/Azafanatic/dyskont-sim/blob/a1b74a8e0b14b351b1c418077e9262e8b459b9b4/src/logger.c#L59
* `close()`: https://github.com/Azafanatic/dyskont-sim/blob/a1b74a8e0b14b351b1c418077e9262e8b459b9b4/src/logger.c#L69
* `write()`: https://github.com/Azafanatic/dyskont-sim/blob/a1b74a8e0b14b351b1c418077e9262e8b459b9b4/src/logger.c#L157

#### 10.2. Tworzenie procesów: 
* `fork()`: https://github.com/Azafanatic/dyskont-sim/blob/a1b74a8e0b14b351b1c418077e9262e8b459b9b4/src/self_service_checkout.c#L56
* `execlp()`: https://github.com/Azafanatic/dyskont-sim/blob/a1b74a8e0b14b351b1c418077e9262e8b459b9b4/src/main.c#L119
* `exit()`: https://github.com/Azafanatic/dyskont-sim/blob/a1b74a8e0b14b351b1c418077e9262e8b459b9b4/src/main.c#L121
* `waitpid()`: https://github.com/Azafanatic/dyskont-sim/blob/a1b74a8e0b14b351b1c418077e9262e8b459b9b4/src/main.c#L159

#### 10.3. Obsługa sygnałów:
* `kill()`: https://github.com/Azafanatic/dyskont-sim/blob/1dae76928d154fdad4c57e0fb9e9698b5e2cdde1/src/manager.c#L85
* `signal()`: https://github.com/Azafanatic/dyskont-sim/blob/1dae76928d154fdad4c57e0fb9e9698b5e2cdde1/src/checkout.c#L71

#### 10.4. Synchronizacja procesów:
* `ftok()`: https://github.com/Azafanatic/dyskont-sim/blob/a1b74a8e0b14b351b1c418077e9262e8b459b9b4/src/utils.c#L33
* `semget()`: https://github.com/Azafanatic/dyskont-sim/blob/1dae76928d154fdad4c57e0fb9e9698b5e2cdde1/src/utils.c#L17
* `semctl()`: https://github.com/Azafanatic/dyskont-sim/blob/1dae76928d154fdad4c57e0fb9e9698b5e2cdde1/src/utils.c#L22
* `semop()`: https://github.com/Azafanatic/dyskont-sim/blob/1dae76928d154fdad4c57e0fb9e9698b5e2cdde1/src/utils.c#L42

#### 10.5. Segmenty pamięci dzielonej:
* `shmget()`: https://github.com/Azafanatic/dyskont-sim/blob/1dae76928d154fdad4c57e0fb9e9698b5e2cdde1/src/utils.c#L128
* `shmat()`: https://github.com/Azafanatic/dyskont-sim/blob/1dae76928d154fdad4c57e0fb9e9698b5e2cdde1/src/utils.c#L134C15-L134C20
* `shmdt()`: https://github.com/Azafanatic/dyskont-sim/blob/1dae76928d154fdad4c57e0fb9e9698b5e2cdde1/src/utils.c#L206
* `shmctl()`: https://github.com/Azafanatic/dyskont-sim/blob/1dae76928d154fdad4c57e0fb9e9698b5e2cdde1/src/utils.c#L198

#### 10.6. Kolejki komunikatów:
* `msgget()`: https://github.com/Azafanatic/dyskont-sim/blob/a1b74a8e0b14b351b1c418077e9262e8b459b9b4/src/main.c#L225
* `msgsnd()`: https://github.com/Azafanatic/dyskont-sim/blob/1dae76928d154fdad4c57e0fb9e9698b5e2cdde1/src/utils.c#L75C9-L75C15
* `msgrcv()`: https://github.com/Azafanatic/dyskont-sim/blob/a1b74a8e0b14b351b1c418077e9262e8b459b9b4/src/logger.c#L104
* `msgctl()`: https://github.com/Azafanatic/dyskont-sim/blob/1dae76928d154fdad4c57e0fb9e9698b5e2cdde1/src/utils.c#L85

#### 10.7. Obsługa błędów:
* `perror()`: https://github.com/Azafanatic/dyskont-sim/blob/1dae76928d154fdad4c57e0fb9e9698b5e2cdde1/src/utils.c#L19