#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>

#define EXERR -1

int pidPadre, numProc = 0, pidSiguiente;
int entrar = 0, modoD = 0, sonoAlarma = 0;
sigset_t blockAlarm;

void morir(int);
void testigo(int);
void alarma(int);
void error(int);

int main(int argc, char * argv[]){

       int i = 0, n;
       int pid, pidAnterior;
       char mensaje[50];
       struct sigaction manejadora;
       sigset_t bloqueo;

       if (sigfillset(&bloqueo) != 0) return EXERR;
       if (sigemptyset(&blockAlarm) != 0) return EXERR;
       if (sigprocmask(SIG_SETMASK, &bloqueo, NULL) != 0) return EXERR;

       manejadora.sa_handler = morir;
       if (sigemptyset(&manejadora.sa_mask) != 0) return EXERR;
       if (sigaddset(&manejadora.sa_mask, SIGUSR1) != 0) return EXERR;
       manejadora.sa_flags = 0;
       if (sigaction(SIGINT, &manejadora, NULL) != 0) return EXERR;
      
       manejadora.sa_handler = testigo;
       if (sigemptyset(&manejadora.sa_mask) != 0) return EXERR;
       if (sigaddset(&manejadora.sa_mask, SIGALRM) != 0) return EXERR;
       manejadora.sa_flags = 0;
       if (sigaction(SIGUSR1, &manejadora, NULL) != 0) return EXERR;
      
       manejadora.sa_handler = alarma;
       if (sigemptyset(&manejadora.sa_mask) != 0) return EXERR;
       if (sigaddset(&manejadora.sa_mask, SIGUSR1) != 0) return EXERR;
       manejadora.sa_flags = 0;
       if (sigaction(SIGALRM, &manejadora, NULL) != 0) return EXERR;

       manejadora.sa_handler = error;
       manejadora.sa_mask = bloqueo;
       if (sigdelset(&manejadora.sa_mask, SIGINT) != 0) return EXERR;
       manejadora.sa_flags = 0;
       if (sigaction(SIGTERM, &manejadora, NULL) != 0) return EXERR;
      
       pidPadre = getpid();
       pidAnterior = pidPadre;       
      
       if(argc == 1){
               if(sprintf(mensaje, "Faltan argumentos\n") < 0)return EXERR;
               if (write(0, mensaje, strlen(mensaje)) == -1)return EXERR;
               return EXERR;
       }
       if(argc == 3){
               if(strcmp(argv[2], "debug")== 0){
                       modoD = 1;
                       }
               else{
                       if(sprintf(mensaje, "Segundo argumento invalido\n") < 0)return EXERR;
                       if(write(0, mensaje, strlen(mensaje)) == -1)return EXERR;
                       return EXERR;
                       }
               }
       n = atoi(argv[1]);
       if(n <= 0){
               if(sprintf(mensaje, "N?? procesos inv??lido, debe ser mayor que 0\n") < 0)return
EXERR;
               if(write(0, mensaje, strlen(mensaje)) == -1)return EXERR;
               return EXERR;
       }

       for(i = 0; i < n; i++){
               pid = fork();
               if(pid == -1){
                       raise(SIGTERM);
               }
               else if(pid == 0){
                       pidSiguiente = pidAnterior;
                       break;
               }
               else{
                       pidAnterior = pid;
                       numProc++;
               }
       }
       if(getpid() == pidPadre){
               pidSiguiente = pidAnterior;
               if (kill(pidSiguiente, SIGUSR1) != 0){
                       kill(0, SIGINT);
                       for(i = 0; i < numProc; i++){
                               wait(&n);
                       }
                       sprintf(mensaje, "\nEjecucion concluida erroneamente\n");
                       write(0, mensaje, strlen(mensaje));
                       _Exit(-1);               
               }
       }
      
       if (sigdelset(&bloqueo, SIGUSR1) != 0) kill(pidPadre, SIGTERM);
       if (sigdelset(&bloqueo, SIGTERM) != 0) kill(pidPadre, SIGTERM);
       if (sigdelset(&bloqueo, SIGINT) != 0) kill(pidPadre, SIGTERM);       
       if (sigprocmask(SIG_SETMASK, &bloqueo, NULL) != 0) kill(pidPadre, SIGTERM);

       while(1){
               entrar = 0;
               if(modoD){
                       sonoAlarma = 0;
                       alarm(1);
                       while(!sonoAlarma){
                               sigsuspend(&blockAlarm);               
                       }
               }
               entrar = 1;
               sigsuspend(&bloqueo);
       }
       return 0;
       }

void morir(int sennal){
       int i;
       int codigo;
       char mensaje[85];
       if(getpid() != pidPadre){
               _Exit(0);       
       }
       else{
               for(i = 0; i < numProc; i++){
                       wait(&codigo);
               }
               sprintf(mensaje, "\nEjecucion concluida correctamente\n");
               write(0, mensaje, strlen(mensaje));
               _Exit(0);               
       }
}

void testigo(int sennal){
       char mensaje[40];
       struct timespec duerme;

       if(entrar == 1){
               entrar = 0;
               if(modoD){
                       sonoAlarma = 0;
                       if(sprintf(mensaje, "E(%d)", getpid()) < 0) kill(SIGTERM, pidPadre);
                       if(write(0, mensaje, strlen(mensaje)) == -1) kill(SIGTERM, pidPadre);
                       alarm(2);
                       while(!sonoAlarma){
                               sigsuspend(&blockAlarm);
                       }
               }       
               else{
                       if(sprintf(mensaje, "E") < 0) kill(SIGTERM, pidPadre);
                       if(write(0, mensaje, strlen(mensaje)) == -1) kill(SIGTERM, pidPadre);
               }               
               if(modoD) {
                       if(sprintf(mensaje, "S(%d)", getpid()) < 0) kill(SIGTERM, pidPadre);
               }
               else {
                       if(sprintf(mensaje, "S") < 0) kill(SIGTERM, pidPadre);
               }
               if(write(0, mensaje, strlen(mensaje)) == -1) kill(SIGTERM, pidPadre);
       }
       if(modoD){
               duerme.tv_sec = 0;
               duerme.tv_nsec = 100000;
               while(nanosleep(&duerme, &duerme));
       }
       if (kill(pidSiguiente, SIGUSR1) != 0) kill(pidPadre, SIGTERM);
       return;
}

void alarma(int sennal){
       sonoAlarma = 1;
       return;
};

void error(int sennal){
       kill(0, SIGINT);
       raise(SIGINT);
}
