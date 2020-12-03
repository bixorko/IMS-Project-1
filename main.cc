/*
 * LIBRARIES *
             */

#include <iostream>
#include "simlib.h"

/*
 * C STD LIBRARIES *
                   */

#include <string.h>
#include <stdio.h>
#include <math.h>

/*
 * GLOBAL VARIABLES *
                    */

int doVyroby = 0;
int prerobit = 0;
int leaving = 0;
int wantChangeG = 0;
int createdSito = 0;
int createdDigi = 0;
long customer_leave_count = 0;

double RealOrdersCount;

/*
 * DEFINES OF CONSTANTS *
                        */

#define HOUR 60
#define WORKDAY 60*8
#define WORKYEAR 60*8*251 // 251 workdays in year 2020 -> Source: https://www.mzdovecentrum.sk/kalendar-podnikatela/planovaci-kalendar-na-rok-2019-.htm
#define CUSTOMER_PATIENCE 48*60

// uniform(WORKDAY*2, WORKDAY*4) - dlzka tlacenia

/*
 * FACILITIES *
 * AND STORES *
              */

Store kalkulant("Kalkulant", 2);
Store veduci("Veduci", 3);
Store sitotisk("SitoTisk", 2);
Facility digitisk("DigiTisk");

/**
 * This Class is Process for simulating printing pictures on bags
 * It is activated whenever "konzultant" approves and confirms 
 * new order and moves it to manufacturing part.
 **/ 

class Vyroba : public Process
{
    void Behavior()
    {
        Enter(veduci, 1);
        double digiOrSito = Random();
        if (digiOrSito > 0.2)
        {                                                       // potrebujeme sitotisk

            Enter(sitotisk, 1);
            Wait(Uniform(WORKDAY*0.5, WORKDAY*2));

            Leave(sitotisk, 1);
            Leave(veduci, 1);

            createdSito++;

        }
        else 
        {                                                       // potrebujeme digitalni tisk

            Seize(digitisk);
            Wait(Uniform(WORKDAY*0.5, WORKDAY*2));

            Release(digitisk);
            Leave(veduci, 1);
            
            createdDigi++;

        }
    }
};

/**
 * CLASS TIMEOUT
 * Cast kodu prevzaty od pana Dr. Ing. Petra Peringera 
 * z oficialnej dokumentacnej stranky pre SIMLIB
 * https://www.fit.vutbr.cz/~peringer/SIMLIB/examples/model2-timeout.txt
 **/

class Timeout : public Event 
{
    Process *ptr;        
    public:
        Timeout(double t, Process *p): ptr(p) 
        {
            Activate(Time+t);                                   // kdy vyprší timeout
        }
        void Behavior() 
        {
            ptr->Out();                                         // vyjmout z fronty 
            delete ptr;                                         // likvidace
            customer_leave_count++;                             // počitadlo
            Cancel();                                           // konec události (SIMLIB BUG)
        }
};

/**
 * CLASS ZAKAZKA
 * Tato trieda simuluje proces prichadzajucej zakazky 
 * 
 **/ 
class Zakazka : public Process 
{
    void Behavior()
    {
        Event *timeout = new Timeout(CUSTOMER_PATIENCE, this);  // nastavit timeout
        Enter(kalkulant, 1);                                    // ZABRATIE KALKULANTA ZO STORE
        delete timeout;
        Wait(Uniform(WORKDAY/2,WORKDAY));                       // PRIPRAVA CENOVEHO / GRAFICKEHO NAVRHU

        isGoodCheck:                                            // GOTO LABEL... IM SORRY THAT I USED IT :'(

        double isGood = Random();
        if (isGood > 0.1)
        {                                                       // navrh je dobry
            (new Vyroba)->Activate();                           // poslany do vyroby - zacina novy proces
            doVyroby++;
        }
        else
        {                                                       // navrh je zly
            prerobit++;
            double wantChange = Random();
            if (wantChange <= 0.05)
            {                                                   // nechce zmenit
                leaving++;
                /* pass */ 
            }
            else
            {                                                   // chce zmenit
                wantChangeG++;
                Wait(Uniform(WORKDAY/2,WORKDAY));               // znovaplanovanie navrhu ceny / grafiky
                goto isGoodCheck;                               // skocit na ci navrh vyhovuje alebo nie
            }
        }
        
        Leave(kalkulant, 1);                                    // UVOLNENIE KALKULANTA ZO STORE
    }
};

class Generator : public Event 
{
    void Behavior() 
    {
        if (Time != WORKYEAR)
        {                                                       // zabranenie aby sa zakazka z noveho roku nezapocitalo do stareho (do skumaneho)
            (new Zakazka)->Activate();
            this->Activate(Time + (WORKDAY*RealOrdersCount));   // premenna RealOrdersCount je vypocitana na zaklade zisku z daneho roku podla
                                                                // obchodneho registra a na zaklade nej mozme prepocitat, kolko zakaziek bolo v danom roku
                                                                // zadanych a tak vieme, kolko ich za dany rok vygenerovat do simulacie...
                                                                // potom prichadza na rad samotna simulacia, ktorej vysledok (Generovany ZISK v outpute v maine)
                                                                // musi byt obdobny s povodnou sumou, ktora bola zadana. Obcasna nepresnost moze byt zapricinena
                                                                // kvoli pouzivaniu datoveho typu double, ale tieto nepresnosti su pocitane v par tisicoch, co je pri
                                                                // milionovych (prip. statisicovych obratoch) zanedbatelne.
                                                                // v pripade ze je vysledna suma (Generovany ZISK) +- obdobna so sumou na vstupe, znamena to, ze simulacia
                                                                // je validna a presla validaciou a mozme pokracovat na simulaciu zlepsenia 
                                                                // pre pripravenie 383+- objednavok pri zisku 17.240.000 czk za rok 2011
                                                                // pri cene za potlac (spriemerovana podla cien digitalnej a sietotlace v pomere 20:80) +-6 czk 
                                                                // [0,13 sito, 0.60 digi] jednotkova cena
                                                                // pricom jedna zakazka ma v priemere 750 kusov      
                                                                // 1.724.000/6/750 == 383 zakaziek za rok priemerne
                                                                // takze za den musime vygenerovat priemerne WORKDAY*(251/383) resp. RealOrdersCount
                                                                // (80*0,13 + 20*0.6)/100 == 0.23 eur == 6.1 czk == 6 czk priemerne za jeden kus                                   
        }
    }
};

int main(int argc, char *argv[])
{
    if (argc == 2){

        /*
         * VALIDITY OF SIMULATION *
         *        CONTROL         *
                                  */
        if (!strcmp(argv[1], "--validityCheck1"))
        {
            SetOutput("validityCheck1.out");
            
            double orders = 117000/6/750;                           //popis nasledujucich dvoch vypoctov je v triede Generator                      
            RealOrdersCount = 251/orders;                           //popis v triede Generator
            Init(0, WORKYEAR); 
            (new Generator)->Activate();
            Run();

            kalkulant.Output();
            veduci.Output();
            sitotisk.Output();
            digitisk.Output();

            Print("+----------------------------------------------------------+\n");
            Print("| MY STATS                                                 |\n");
            Print("+----------------------------------------------------------+\n");
            Print("|  * Prijatych objednavok: %d                              |\n", doVyroby);
            Print("|  * Prerobit ziadalo: %d                                  |\n", prerobit);
            Print("|    # Chceli prerobit navrh ANO: %d                       |\n", wantChangeG);
            Print("|    # Chceli prerobit navrh NIE: %d                       |\n", leaving);
            Print("|  * Dokoncenych sitotisk zakazek: %d                      |\n", createdSito);
            Print("|  * Dokoncenych digitisk zakazek: %d                      |\n", createdDigi);
            Print("|  * Dokoncenych zakaziek SPOLU: %d                        |\n", createdDigi + createdSito);
            Print("|  * Generovany ZISK: %lu                                  |\n", (createdDigi + createdSito)*6*750);
            Print("|  * Pocet zakaznikov, ktorym nebolo odpovedane na cas: %d |\n", customer_leave_count);
            Print("|  * Pocet zakaznikov, ktory ukoncili spolupracu z dovodu  |\n");
            Print("|    komunikacie / zlemu navrhu: %d                        |\n", leaving + customer_leave_count);
            Print("+----------------------------------------------------------+\n");
        }

        else if (!strcmp(argv[1], "--validityCheck2"))
        {
            SetOutput("validityCheck2.out");

            double orders = 151000/6/750;                           //popis nasledujucich dvoch vypoctov je v triede Generator                      
            RealOrdersCount = 251/orders;                           //popis v triede Generator
            Init(0, WORKYEAR); 
            (new Generator)->Activate();
            Run();

            kalkulant.Output();
            veduci.Output();
            sitotisk.Output();
            digitisk.Output();

            Print("+----------------------------------------------------------+\n");
            Print("| MY STATS                                                 |\n");
            Print("+----------------------------------------------------------+\n");
            Print("|  * Prijatych objednavok: %d                              |\n", doVyroby);
            Print("|  * Prerobit ziadalo: %d                                  |\n", prerobit);
            Print("|    # Chceli prerobit navrh ANO: %d                       |\n", wantChangeG);
            Print("|    # Chceli prerobit navrh NIE: %d                       |\n", leaving);
            Print("|  * Dokoncenych sitotisk zakazek: %d                      |\n", createdSito);
            Print("|  * Dokoncenych digitisk zakazek: %d                      |\n", createdDigi);
            Print("|  * Dokoncenych zakaziek SPOLU: %d                        |\n", createdDigi + createdSito);
            Print("|  * Generovany ZISK: %lu                                  |\n", (createdDigi + createdSito)*6*750);
            Print("|  * Pocet zakaznikov, ktorym nebolo odpovedane na cas: %d |\n", customer_leave_count);
            Print("|  * Pocet zakaznikov, ktory ukoncili spolupracu z dovodu  |\n");
            Print("|    komunikacie / zlemu navrhu: %d                        |\n", leaving + customer_leave_count);
            Print("+----------------------------------------------------------+\n");
        }

        else if (!strcmp(argv[1], "--validityCheck3"))
        {
            SetOutput("validityCheck3.out");

            double orders = 1724000/6/750;                           //popis nasledujucich dvoch vypoctov je v triede Generator                      
            RealOrdersCount = 251/orders;                           //popis v triede Generator
            Init(0, WORKYEAR); 
            (new Generator)->Activate();
            Run();

            kalkulant.Output();
            veduci.Output();
            sitotisk.Output();
            digitisk.Output();

            Print("+----------------------------------------------------------+\n");
            Print("| MY STATS                                                 |\n");
            Print("+----------------------------------------------------------+\n");
            Print("|  * Prijatych objednavok: %d                              |\n", doVyroby);
            Print("|  * Prerobit ziadalo: %d                                  |\n", prerobit);
            Print("|    # Chceli prerobit navrh ANO: %d                       |\n", wantChangeG);
            Print("|    # Chceli prerobit navrh NIE: %d                       |\n", leaving);
            Print("|  * Dokoncenych sitotisk zakazek: %d                      |\n", createdSito);
            Print("|  * Dokoncenych digitisk zakazek: %d                      |\n", createdDigi);
            Print("|  * Dokoncenych zakaziek SPOLU: %d                        |\n", createdDigi + createdSito);
            Print("|  * Generovany ZISK: %lu                                  |\n", (createdDigi + createdSito)*6*750);
            Print("|  * Pocet zakaznikov, ktorym nebolo odpovedane na cas: %d |\n", customer_leave_count);
            Print("|  * Pocet zakaznikov, ktory ukoncili spolupracu z dovodu  |\n");
            Print("|    komunikacie / zlemu navrhu: %d                        |\n", leaving + customer_leave_count);
            Print("+----------------------------------------------------------+\n");
        }

        /*
         * SIMULATION TESTING *
         *   WITH NEW VALUES  *
                              */

        /*
         * BAD INPUT ARGUMENTS *
                               */
        
        else
        {
            fprintf(stderr, "Bad Input Arguments!\n");
            return -1;
        }
    }

    return 0;
}