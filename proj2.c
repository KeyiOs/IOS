#include <time.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>             //Atoi
#include <unistd.h>             //Fork
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <semaphore.h>

//Deklarácia zdieľanej pamäte, výstupného súboru a semafórov
int *sharedMem = NULL;
FILE *output= NULL;
sem_t *orderSem = NULL;
sem_t *queueElfSem = NULL;
sem_t *queueSobSem = NULL;
sem_t *santaSem = NULL;
sem_t *santaSleepSem = NULL;
sem_t *elfIDSem = NULL;
sem_t *sobIDSem = NULL;
sem_t *lastRDSem = NULL;
sem_t *lastElfSem = NULL;
sem_t *holidaySem = NULL;
sem_t *elfQuitSem = NULL;
//Koniec deklarácie

int kontrola(int argc, char *argv[]){
    for(int a = 1; a < argc; a++){
        int b = 0;
        while(argv[a][b] != 0){
            if(argv[a][b] < 48 || argv[a][b] > 57){
                fprintf(stderr, "Neplatný znak argumentu\n");
                return 1;
            }
            b++;
        }
    }
    return 0;
}
int kontrola2(int elfs, int soby, int max, int dovolenka){
    if(elfs <= 0 || elfs >= 1000){
        fprintf(stderr, "Neplatný počet škriatkov.\n");
        return 1;
    }
    else if(soby <= 0 || soby >= 20){
        fprintf(stderr, "Neplatný počet sobov.\n");
        return 1;
    }
    else if(max < 0 || max > 1000){
        fprintf(stderr, "Maximálna práca mimo interval.\nPovolený interval <0;1000>\n");
        return 1;
    }
    else if(dovolenka < 0 || dovolenka > 1000){
        fprintf(stderr, "Dovolenka mimo interval.\nPovolený interval <0;1000>\n");
        return 1;
    }

    return 0;
}
void clear(){
    fclose(output);
    sem_destroy(orderSem);
    sem_destroy(queueElfSem);
    sem_destroy(queueSobSem);
    sem_destroy(santaSem);
    sem_destroy(santaSleepSem);
    sem_destroy(elfIDSem);
    sem_destroy(sobIDSem);
    sem_destroy(lastRDSem);
    sem_destroy(lastRDSem);
    sem_destroy(lastElfSem);
    sem_destroy(holidaySem);
    sem_destroy(elfQuitSem);
    munmap(sharedMem, 8*sizeof(int));
}
void processCounter(){
    sem_wait(orderSem);

    sharedMem[0] = sharedMem[0] + 1;
}
void santa(int soby){        
    sem_wait(santaSem);
    
    if(sharedMem[4] != soby){                                                           //Vstup do if pokial je dielňa otvorená
        processCounter();                                                               //Výpočet poradia operácie
        fprintf(output, "%i: Santa: helping elves\n", sharedMem[0]);                    //Vypíše pomoc elfom
        fflush(output);                                                                 //Výpis v reálnom čase
        
        sem_post(orderSem);                                                             //Uvoľnenie semafóru
        
        for(int a = 0; a < 3; a++){                                                     //Simulácia troch elfov
            sem_post(queueElfSem);                                                      //Elfovia vstúpia do dielne
        }

        sem_wait(santaSleepSem);                                                        //Čaká na odchod posledného elfa
        
        processCounter();                                                               //Výpočet poradia operácie
        fprintf(output, "%i: Santa: going to sleep\n", sharedMem[0]);                   //Vypíše, že santa ide spať
        fflush(output);                                                                 //Výpis v reálnom čase
        
        sem_post(orderSem);                                                             //Uvoľnenie semafóru

        santa(soby);                                                                    //Opakovanie procesu
    }
    else{
        processCounter();                                                               //Výpočet poradia operácie
        fprintf(output, "%i: Santa: closing workshop\n", sharedMem[0]);                 //Vypíše, že sa dielňa zatvára
        fflush(output);                                                                 //Výpis v reálnom čase
        
        sharedMem[7] = 0;                                                               //Signál pre santu zavrieť dielňu

        sem_post(orderSem);                                                             //Uvoľnenie semafóru
        sem_post(queueElfSem);                                                          //Uvoľnenie semafóru
        sem_post(queueSobSem);                                                          //Začne zapriahavať soby

        sem_wait(holidaySem);                                                           //Čaká na zapriahnutie posledného soba

        processCounter();                                                               //Výpočet poradia operácie
        fprintf(output, "%i: Santa: Christmas started\n", sharedMem[0]);                //Vypíše, že sa dielňa zatvára
        fflush(output);                                                                 //Výpis v reálnom čase
        
        sem_post(orderSem);                                                             //Uvoľnenie semafóru
        sem_post(holidaySem);                                                           //Uvoľnenie semafóru
        sem_post(santaSleepSem);                                                        //Uvoľnenie semafóru
        
        exit(0);                                                                        //Ukončenie procesu
    }
}
void elf(int elfID, int max){
    srand (time(NULL)*elfID);                                                           //Seedovanie náhodnéhných čísel
    usleep((rand() % (max + 1))*1000);                                                  //Uspanie procesu na náhodnú dobu

    processCounter();                                                                   //Výpočet poradia operácie
    fprintf(output, "%i: Elf %i: need help\n", sharedMem[0], elfID);                    //Vypíše, že elf potrebuje pomoc
    fflush(output);                                                                     //Výpis v reálnom čase
    
    sem_post(orderSem);                                                                 //Uvoľnenie semafóru

    if(sharedMem[7] == 0){                                                              //Vstup do if pokial je dielňa zatvorená
        processCounter();                                                               //Výpočet poradia operácie
        fprintf(output, "%i: Elf %i: taking holidays\n", sharedMem[0], elfID);          //Vypíše, že elf ide na dovolenku
        fflush(output);                                                                 //Výpis v reálnom čase

        sem_post(orderSem);                                                             //Uvoľnenie semafóru
    
        exit(0);                                                                        //Ukončenie procesu
    }

    sem_wait(lastElfSem);                                                               //Zablokovanie vstupu pre dalsie procesy

    sharedMem[3] = sharedMem[3] + 1;                                                    //Pridanie elfa do poradia
    if(sharedMem[3] >= 3 && sharedMem[5] == 0){                                         //Vstup do if pokial čakajú traja elfovia a dielňa je prázdna
        sem_post(santaSem);                                                             //Zobudí santu
        sharedMem[3] = sharedMem[3] - 3;                                                //Traja elfovia vstúpia do dielne
        sharedMem[5] = 1;                                                               //V dielni sú elfovia
    }
    
    sem_post(lastElfSem);                                                               //Uvoľnenie semafóru

    sem_wait(queueElfSem);                                                              //Zablokovanie vstupu pre dalsie procesy
    
    //Zvyšný elfovia z rady pred dielňou odídu na dovolenku po uzavretí dielne
    if(sharedMem[7] == 0){                                                              //Vstup do if pokial je dielňa zatvorená
        sem_post(queueElfSem); 
        
        processCounter();                                                               //Výpočet poradia operácie
        fprintf(output, "%i: Elf %i: taking holidays\n", sharedMem[0], elfID);          //Vypíše, že elf ide na dovolenku
        fflush(output);                                                                 //Výpis v reálnom čase

        sem_post(orderSem);                                                             //Uvoľnenie semafóru
    
        exit(0);                                                                        //Ukončenie procesu
    }
    //Koniec odchodu elfov

    processCounter();                                                                   //Výpočet poradia operácie
    fprintf(output, "%i: Elf %i: get help\n", sharedMem[0], elfID);                     //Vypíše pomoc elfovi
    fflush(output);                                                                     //Výpis v reálnom čase

    sem_post(orderSem);                                                                 //Uvoľnenie semafóru

    sem_wait(elfQuitSem);
    sharedMem[6] = sharedMem[6] + 1;                                                    //Elf vyšiel z dielne
    if(sharedMem[6] == 3 && sharedMem[3] >= 3){                                         //Vstup do if ak odchádza posledný elf a pred dielňov čakajú traja elfovia
        sem_post(santaSleepSem);                                                        //Santa môže ísť spať
        sharedMem[6] = 0;                                                               //Z dielne už nemá kto odísť
        for(int a = 0; a < 3; a++){                                                     //Simulácia troch elfov
            sharedMem[3] = sharedMem[3] - 1;                                            //Pred dielňov čaká o jedného elfa menej
        }
        sem_post(santaSem);                                                             //Zobudí santu
    }
    else if(sharedMem[6] == 3 && sharedMem[3] < 3){                                     //Vstup do if ak čakajú pred dielňou menej ako traja elfovia a dielňa je prázdna
        sem_post(santaSleepSem);                                                        //Santa môže ísť spať
        sharedMem[5] = 0;                                                               //V dielni nikto neni
        sharedMem[6] = 0;                                                               //Z dielne už nemá kto odísť
    }
    sem_post(elfQuitSem);                                                               //Uvoľnenie semafóru

    elf(elfID, max);                                                                    //Opakované volanie funkcie
}
void sob(int sobID, int dovolenka, int soby){
    srand (time(NULL)*sobID);                                                           //Seedovanie náhodnéhných čísel
    usleep(((rand() % (dovolenka - dovolenka/2 + 1)) + dovolenka/2)*1000);              //Uspanie procesu na náhodnú dobu

    processCounter();                                                                   //Výpočet poradia operácie
    fprintf(output, "%i: RD %i: return home\n", sharedMem[0], sobID);                   //Vypíše návrat soba
    fflush(output);                                                                     //Výpis v reálnom čase
    
    sem_post(orderSem);                                                                 //Uvoľnenie semafóru

    sem_wait(lastRDSem);                                                                //Zablokovanie vstupu pre dalsie procesy
    
    sharedMem[4] = sharedMem[4] + 1;                                                    //Počítanie sobov
    if(sharedMem[4] == soby){                                                           //Filtruje posledného soba
        sem_post(santaSem);                                                             //Zobudí santu
    }
    
    sem_post(lastRDSem);                                                                //Uvoľnenie semafóru

    sem_wait(queueSobSem);                                                              //Semafór čaká na všetkých sobov
    
    processCounter();                                                                   //Výpočet poradia operácie
    fprintf(output, "%i: RD %i: get hitched\n", sharedMem[0], sobID);                   //Výpíše zapriahnutie soba
    fflush(output);                                                                     //Výpis v reálnom čase

    sem_post(orderSem);                                                                 //Uvoľnenie semafóru
    sem_post(queueSobSem);                                                              //Uvoľnenie semafóru

    sharedMem[4] = sharedMem[4] - 1;                                                    //Počítanie ostávajúcich sobov
    if(sharedMem[4] == 0){                                                              //Filtruje posledného zapriahnutého soba
        sem_post(holidaySem);                                                           //Signál pre santu
    }
}

int main(int argc, char *argv[]){
    //Otvorenie súboru pre výpis
    output = fopen("proj2.out", "w");
    if(output == NULL){
        fprintf(stderr, "Chyba pri otváraní proj2.out\n");
        return 1;
    }
    //Koniec otvárania súboru
    
    //Kontrola počtu argumentov
    if(argc != 5){
        fprintf(stderr, "Neplatné argumenty\n");
        fclose(output);
        return 1;
    }
    //Koniec kontroli počtu arguentov

    //Kontrola na neplatné znaky v argumentoch
    int control = kontrola(argc, argv);
    if(control != 0){
        fclose(output);
        return 1;
    }
    //Koniec kontroli znakov

    //Zápis argumentov do premenných
    int dovolenka = atoi(argv[4]);
    int elfs = atoi(argv[1]);
    int soby = atoi(argv[2]);
    int max = atoi(argv[3]);
    //Ukončenie zápisu argumentov

    //Kontrola hodnôt argumentov
    int control2 = kontrola2(elfs, soby, max, dovolenka);
    if(control2 != 0){
        fclose(output);
        return 1;
    }
    //Koniec kontroli hodnôt

    //Mapovanie zdieľanej pamäte
    if((sharedMem = mmap(NULL, 8*sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0)) == MAP_FAILED){
        fprintf(stderr, "Error creating shared memory for variables");
        fclose(output);
        return 1;
    }
    //Inicializácia hodnôt zdieľanej pamäte
    sharedMem[0] = 0;   //Číslo operácie
    sharedMem[1] = 0;   //elfID
    sharedMem[2] = 0;   //sobID
    sharedMem[3] = 0;   //Počet čakajúcich elfov
    sharedMem[4] = 0;   //Počet čakajúcich sobov
    sharedMem[5] = 0;   //Elfovia v dielni
    sharedMem[6] = 0;   //Počet elfov ktorý vyšli z dielne
    sharedMem[7] = 1;   //Status dielne
    //Ukončenie inicializácie zdieľanej pamäte

    //Inicializácia semáfórou
    if((orderSem = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0)) == MAP_FAILED){
        fprintf(stderr, "Error creating shared memory for semaphore");
        fclose(output);
        munmap(sharedMem, 8*sizeof(int));
        return 1;
    }
    if(sem_init(orderSem, 1, 1) == -1){
        fprintf(stderr, "Error creating semaphore");
        fclose(output);
        sem_destroy(orderSem);
        munmap(sharedMem, 8*sizeof(int));
        return 1;
    }

    if((queueElfSem = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0)) == MAP_FAILED){
        fprintf(stderr, "Error creating shared memory for semaphore");
        fclose(output);
        sem_destroy(orderSem);
        munmap(sharedMem, 8*sizeof(int));
        return 1;
    }
    if(sem_init(queueElfSem, 1, 0) == -1){
        fprintf(stderr, "Error creating semaphore");
        fclose(output);
        sem_destroy(orderSem);
        sem_destroy(queueElfSem);
        munmap(sharedMem, 8*sizeof(int));
        return 1;
    }

    if((queueSobSem = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0)) == MAP_FAILED){
        fprintf(stderr, "Error creating shared memory for semaphore");
        fclose(output);
        sem_destroy(orderSem);
        sem_destroy(queueElfSem);
        munmap(sharedMem, 8*sizeof(int));
        return 1;
    }
    if(sem_init(queueSobSem, 1, 0) == -1){
        fprintf(stderr, "Error creating semaphore");
        fclose(output);
        sem_destroy(orderSem);
        sem_destroy(queueElfSem);
        sem_destroy(queueSobSem);
        munmap(sharedMem, 8*sizeof(int));
        return 1;
    }

    if((santaSem = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0)) == MAP_FAILED){
        fprintf(stderr, "Error creating shared memory for semaphore");
        fclose(output);
        sem_destroy(orderSem);
        sem_destroy(queueElfSem);
        sem_destroy(queueSobSem);
        munmap(sharedMem, 8*sizeof(int));
        return 1;
    }
    if(sem_init(santaSem, 1, 0) == -1){
        fprintf(stderr, "Error creating semaphore");
        fclose(output);
        sem_destroy(orderSem);
        sem_destroy(queueElfSem);
        sem_destroy(queueSobSem);
        sem_destroy(santaSem);
        munmap(sharedMem, 8*sizeof(int));
        return 1;
    }

    if((elfIDSem = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0)) == MAP_FAILED){
        fprintf(stderr, "Error creating shared memory for semaphore");
        fclose(output);
        sem_destroy(orderSem);
        sem_destroy(queueElfSem);
        sem_destroy(queueSobSem);
        sem_destroy(santaSem);
        munmap(sharedMem, 8*sizeof(int));
        return 1;
    }
    if(sem_init(elfIDSem, 1, 1) == -1){
        fprintf(stderr, "Error creating semaphore");
        fclose(output);
        sem_destroy(orderSem);
        sem_destroy(queueElfSem);
        sem_destroy(queueSobSem);
        sem_destroy(santaSem);
        sem_destroy(elfIDSem);
        munmap(sharedMem, 8*sizeof(int));
        return 1;
    }

    if((sobIDSem = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0)) == MAP_FAILED){
        fprintf(stderr, "Error creating shared memory for semaphore");
        fclose(output);
        sem_destroy(orderSem);
        sem_destroy(queueElfSem);
        sem_destroy(queueSobSem);
        sem_destroy(santaSem);
        sem_destroy(elfIDSem);
        munmap(sharedMem, 8*sizeof(int));
        return 1;
    }
    if(sem_init(sobIDSem, 1, 1) == -1){
        fprintf(stderr, "Error creating semaphore");
        fclose(output);
        sem_destroy(orderSem);
        sem_destroy(queueElfSem);
        sem_destroy(queueSobSem);
        sem_destroy(santaSem);
        sem_destroy(elfIDSem);
        sem_destroy(sobIDSem);
        munmap(sharedMem, 8*sizeof(int));
        return 1;
    }

    if((lastRDSem = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0)) == MAP_FAILED){
        fprintf(stderr, "Error creating shared memory for semaphore");
        fclose(output);
        sem_destroy(orderSem);
        sem_destroy(queueElfSem);
        sem_destroy(queueSobSem);
        sem_destroy(santaSem);
        sem_destroy(elfIDSem);
        sem_destroy(sobIDSem);
        munmap(sharedMem, 8*sizeof(int));
        return 1;
    }
    if(sem_init(lastRDSem, 1, 1) == -1){
        fprintf(stderr, "Error creating semaphore");
        fclose(output);
        sem_destroy(orderSem);
        sem_destroy(queueElfSem);
        sem_destroy(queueSobSem);
        sem_destroy(santaSem);
        sem_destroy(elfIDSem);
        sem_destroy(sobIDSem);
        sem_destroy(lastRDSem);
        munmap(sharedMem, 8*sizeof(int));
        return 1;
    }

    if((lastElfSem = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0)) == MAP_FAILED){
        fprintf(stderr, "Error creating shared memory for semaphore");
        fclose(output);
        sem_destroy(orderSem);
        sem_destroy(queueElfSem);
        sem_destroy(queueSobSem);
        sem_destroy(santaSem);
        sem_destroy(elfIDSem);
        sem_destroy(sobIDSem);
        sem_destroy(lastRDSem);
        munmap(sharedMem, 8*sizeof(int));
        return 1;
    }
    if(sem_init(lastElfSem, 1, 1) == -1){
        fprintf(stderr, "Error creating semaphore");
        fclose(output);
        sem_destroy(orderSem);
        sem_destroy(queueElfSem);
        sem_destroy(queueSobSem);
        sem_destroy(santaSem);
        sem_destroy(elfIDSem);
        sem_destroy(sobIDSem);
        sem_destroy(lastRDSem);
        sem_destroy(lastElfSem);
        munmap(sharedMem, 8*sizeof(int));
        return 1;
    }

    if((santaSleepSem = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0)) == MAP_FAILED){
        fprintf(stderr, "Error creating shared memory for semaphore");
        fclose(output);
        sem_destroy(orderSem);
        sem_destroy(queueElfSem);
        sem_destroy(queueSobSem);
        sem_destroy(santaSem);
        sem_destroy(elfIDSem);
        sem_destroy(sobIDSem);
        sem_destroy(lastRDSem);
        sem_destroy(lastElfSem);
        munmap(sharedMem, 8*sizeof(int));
        return 1;
    }
    if(sem_init(santaSleepSem, 1, 0) == -1){
        fprintf(stderr, "Error creating semaphore");
        fclose(output);
        sem_destroy(orderSem);
        sem_destroy(queueElfSem);
        sem_destroy(queueSobSem);
        sem_destroy(santaSem);
        sem_destroy(elfIDSem);
        sem_destroy(sobIDSem);
        sem_destroy(lastRDSem);
        sem_destroy(lastElfSem);
        sem_destroy(santaSleepSem);
        munmap(sharedMem, 8*sizeof(int));
        return 1;
    }
    
    if((holidaySem = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0)) == MAP_FAILED){
        fprintf(stderr, "Error creating shared memory for semaphore");
        fclose(output);
        sem_destroy(orderSem);
        sem_destroy(queueElfSem);
        sem_destroy(queueSobSem);
        sem_destroy(santaSem);
        sem_destroy(elfIDSem);
        sem_destroy(sobIDSem);
        sem_destroy(lastRDSem);
        sem_destroy(lastElfSem);
        sem_destroy(santaSleepSem);
        munmap(sharedMem, 8*sizeof(int));
        return 1;
    }
    if(sem_init(holidaySem, 1, 0) == -1){
        fclose(output);
        sem_destroy(orderSem);
        sem_destroy(queueElfSem);
        sem_destroy(queueSobSem);
        sem_destroy(santaSem);
        sem_destroy(elfIDSem);
        sem_destroy(sobIDSem);
        sem_destroy(lastRDSem);
        sem_destroy(lastElfSem);
        sem_destroy(santaSleepSem);
        sem_destroy(holidaySem);
        munmap(sharedMem, 8*sizeof(int));
        return 1;
    }
    
    if((elfQuitSem = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0)) == MAP_FAILED){
        fprintf(stderr, "Error creating shared memory for semaphore");
        fclose(output);
        sem_destroy(orderSem);
        sem_destroy(queueElfSem);
        sem_destroy(queueSobSem);
        sem_destroy(santaSem);
        sem_destroy(elfIDSem);
        sem_destroy(sobIDSem);
        sem_destroy(lastRDSem);
        sem_destroy(lastElfSem);
        sem_destroy(santaSleepSem);
        sem_destroy(holidaySem);
        munmap(sharedMem, 8*sizeof(int));
        return 1;
    }
    if(sem_init(elfQuitSem, 1, 1) == -1){
        fprintf(stderr, "Error creating semaphore");
        clear();
        return 1;
    }
    //Ukončenie inicializáce semafórou

    //Vytváranie paralelného procesu
    int id = fork();

    if(id == -1){
        fprintf(stderr, "Error creating child");
        clear();
        return 1;
    }
    //Koniec vytvárania paralelného procesu

    //Inicializácia santu
    if(id == 0){
        processCounter();                                                               //Výpočet poradia operácie
        fprintf(output, "%i: Santa: going to sleep\n", sharedMem[0]);                   //Inicializačný výpis
        fflush(output);                                                                 //Výpis v reálnom čase
        
        sem_post(orderSem);                                                             //Uvoľnenie semafóru
        
        santa(soby);                                                                        //Spustenie funkcie santu
        
        exit(0);                                                                        //Ukončenie procesu
    }
    //Koniec inicializácie santu
    
    //Inicializácia elfov
    for(int a = 0; a < elfs; a++){
        id = fork();                                                                    //Vytvorenie podprocesu

        if(id == -1){
            fprintf(stderr, "Error creating child");
            clear();
            return 1;
        }

        if(id == 0){                                                                    //Povolenie vstupu iba podprocesom
            sem_wait(elfIDSem);                                                         //Zablokovanie vstupu pre dalsie procesy
            
            sharedMem[1] = sharedMem[1] + 1;                                            //Výpočet elfID
            int elfID = sharedMem[1];                                                   //Uloženie elfID
            
            processCounter();                                                           //Výpočet poradia operácie
            fprintf(output, "%i: Elf %i: started\n", sharedMem[0], elfID);              //Inicializačný výpis
            fflush(output);                                                             //Výpis v reálnom čase
            
            sem_post(orderSem);                                                         //Uvoľnenie funkcie pre dalšie procesy
            sem_post(elfIDSem);                                                         //Uvoľnenie funkcie pre dalsie procesy

            elf(elfID, max);                                                            //Spustenie funkcie elfa
            
            exit(0);                                                                    //Ukončenie procesu
        }
    }

    //Inicializácia sobov
    for(int a = 0; a < soby; a++){
        id = fork();                                                                    //Vytvorenie podprocesu
        
        if(id == -1){
            fprintf(stderr, "Error creating child");
            clear();
            return 1;
        }
        
        if(id == 0){                                                                    //Povolenie vstupu iba podprocesom
            sem_wait(sobIDSem);                                                         //Zablokovanie vstupu pre dalsie procesy
            
            sharedMem[2] = sharedMem[2] + 1;                                            //Výpočet sobID
            int sobID = sharedMem[2];                                                   //Uloženie sobID
            
            processCounter();                                                           //Výpočet poradia operácie
            fprintf(output, "%i: RD %i: rstarted\n", sharedMem[0], sobID);              //Inicializačný výpis
            fflush(output);                                                             //Výpis v reálnom čase
            
            sem_post(orderSem);                                                         //Uvoľnenie funkcie pre dalšie procesy
            sem_post(sobIDSem);                                                         //Uvoľnenie funkcie pre dalsie procesy

            sob(sobID, dovolenka, soby);                                                //Spustenie funkcie soba
            
            exit(0);                                                                    //Ukončenie procesu
        }
    }
    //Koniec inicializácií

    //Čakanie na ukončenie všetkých child procesov
    while(wait(NULL) != -1 || errno != ECHILD);

    //Uvoľnenie zdrojov
    clear();

    return 0;
}
