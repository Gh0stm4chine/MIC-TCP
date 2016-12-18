#include <mictcp.h>
#include <api/mictcp_core.h>
#include <math.h>

#define WINDOW_SIZE 500
#define LOSS_RATE 0.50

static const int MAX_ESSAI = 10;
int fab1 = 500;
int fab2 = 700;
int setting = 1;
int counter = 0;
float fiabilite = 0;
int connexion = 1 ;
int timer = 1000;

//Ajout du bonus calcul RTT

unsigned long RTO = 1000; //Timer par default = 1s
unsigned long SRTT = -1;
// [JK88]  Jacobson, V. and M. Karels, "Congestion Avoidance and Control",
double alpha = 1/8; 
double beta = 1/4;
int first = 1 ;
  


// **** Bonus Calcul RTT   ***** //
void calculRTO(unsigned long RTT) {
  if(first) {
    SRTT = RTT;
    RTO = SRTT;
  } else {
    RTO = 0.9*SRTT + 0.1*RTT;
    SRTT = RTT;
  }
}



// **** Fiabilité partielle **** //

void print_window() {
  int i = 0;
  for(i = 0 ; i < WINDOW_SIZE ; i++) {
    printf("%d ",window[i]);
  }
  printf("\n");
}

void decalage() {
  int i = 0;
  for(i = 1 ; i < WINDOW_SIZE ; i++) {
    window[i-1] = window[i];
  }
}

void ajout(int val) {
  int i = 0 ;
  int length = WINDOW_SIZE;
  if(window[length-1] != -1) {
    decalage();
    window[length-1] = val;
  } else {
    while(i < WINDOW_SIZE) {
      if(window[i] == -1) {
	window[i] = val;
	return;
      }
      i++;
    }
  }
}

void init_window() {
  window = malloc(WINDOW_SIZE*sizeof(char));
  int i = 0 ;
  for(i = 0 ; i < WINDOW_SIZE ; i++) {
    window[i] = -1;
  }
}

float ratio() {
  char somme = 0 ;
  int i = 0;
  while(window[i] != -1 && i< WINDOW_SIZE) {
    somme += window[i];
    i++;
  }
  return ((float)somme/(float)(i));
}

// ********************** //

int mic_tcp_socket(start_mode sm) 
// Permet de créer un socket entre l’application et MIC-TCP
// Retourne le descripteur du socket ou bien -1 en cas d'erreur
{
  printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");
  int fd;
  local_sock.fd = 1;
  set_loss_rate(1000-(LOSS_RATE * 1000));
  local_sock.state = ESTABLISHED;
  if(sm == CLIENT) {
    PA = 0;
    init_window();
    //set_loss_rate(1000-(LOSS_RATE * 1000));
    //set_loss_rate(1000-fab1);
    //set_loss_rate(200);
  }
  else if (sm == SERVER) {
    //set_loss_rate(1000-fab2);
    PE = 0;
  }
  initialize_components(sm); // Appel obligatoire
  return local_sock.fd;
}

int mic_tcp_bind(int socket, mic_tcp_sock_addr addr)
// Permet d’attribuer une adresse à un socket. Retourne 0 si succès, et -1 en cas d’échec
{
  printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");
  local_sock.addr = addr;
  return 0;
}

int mic_tcp_accept(int socket, mic_tcp_sock_addr* addr)
// Met l’application en état d'acceptation d’une requête de connexion entrante
// Retourne 0 si succès, -1 si erreur
{
  printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");
  printf("Fiabilité server initiale %d \n", fab2);
  local_sock.state = ATTENTE_SYN;
  while(local_sock.state == ATTENTE_SYN) {}
  printf("SYN RECU \n");
  struct mic_tcp_pdu syn_ack;
  syn_ack.hd.source_port = local_sock.addr.port;
  syn_ack.hd.dest_port = dist_sock_addr.port;
  syn_ack.payload.size = 0;
  syn_ack.payload.data = malloc(15);
  syn_ack.hd.syn = 1;
  syn_ack.hd.ack = 1;
  //Negociation fiabilite
  syn_ack.hd.seq_num = fab2;
  printf("Envoi du SYN_ACK..... \n");
  if(IP_send(syn_ack,dist_sock_addr) == -1) 
    printf("error send \n");
  local_sock.state = SYN_ACK_SENT;
  while(local_sock.state != CONNECTED);
  printf("Fiabilité server final %d \n", fab2);
  //set_loss_rate(1000-fab2);
  connexion = 0;
  printf("Connexion client réussie \n"); 
  return 0;
}

int mic_tcp_connect(int socket, mic_tcp_sock_addr addr)
// Permet de réclamer l’établissement d’une connexion
// Retourne 0 si la connexion est établie, et -1 en cas d’échec
{   
  printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");
  printf("Fiabilité client initiale %d \n", fab1);
  int nb_essai = 0 ;
  //Construction du SYN
  struct mic_tcp_pdu syn;
  syn.hd.syn = 1;
  syn.hd.source_port = local_sock.addr.port;
  syn.hd.dest_port = dist_sock_addr.port;
  syn.payload.data = malloc(15);
  syn.payload.size = 15;
  //Negociation fiabilite
  syn.hd.seq_num = fab1;
  printf("syn.hd.seq_num => %d \n",syn.hd.seq_num);
  //Envoi du SYN
  printf("Envoi SYN..... \n");
  //Start timer (Calcul RTT)
  unsigned long start_timer = get_now_time_usec();
  printf("start timer = %lu \n",start_timer);
  int sizeSyn = IP_send(syn,dist_sock_addr);
  if(sizeSyn == -1)
    printf("Error send SYN \n");
  else 
    printf("Envoi syn réussie : %d \n",sizeSyn);
  local_sock.state = SYN_SENT;
  struct mic_tcp_payload syn_ack;
  syn_ack.data = malloc(15);
  syn_ack.size = 15;
  while(local_sock.state == SYN_SENT && nb_essai < MAX_ESSAI) {
    if(IP_recv(&syn_ack,&dist_sock_addr,RTO) == -1) {
      printf("Expiration Timer ..SynAck pas recu... Resend Syn\n");
      fflush(stdout);
      IP_send(syn,dist_sock_addr);
      start_timer = get_now_time_usec();
      nb_essai++;
    } else {
      struct mic_tcp_header hd;
      hd = get_header(syn_ack.data);
      if(hd.syn == 1 && hd.ack == 1) {
	printf("SYN_ACK recu.\n");
	if(fab1 != hd.seq_num) {
	  fab1 = hd.seq_num;
	  printf("Nouvelle Fiabilité = %d \n", fab1);
	  //set_loss_rate(1000-fab1);
	}
	local_sock.state = SYN_ACK_RECEIVED;
      } else {
	printf("Didn't get a syn_ack \n");
	IP_send(syn,dist_sock_addr);
	start_timer = get_now_time_usec();
	nb_essai++;
      }
    }
  }
  unsigned long now_time = get_now_time_usec();
  printf("now time = %lu \n",now_time);
  unsigned long RTT = now_time - start_timer; // ==> RTT = 0
  printf("difference = %ld \n",RTT);
  printf("RTT apres l'envoie du syn %lu \n", RTT);
  calculRTO(RTT);
  printf("Nouveau RTO %lu \n",RTO);
  struct mic_tcp_pdu ack;
  ack.hd.ack = 1;
  ack.hd.syn = 0;
  ack.hd.source_port = local_sock.addr.port;
  ack.hd.dest_port = dist_sock_addr.port;
  ack.payload.data = malloc(15);
  ack.payload.size = 15;
  //Negociation fiabilite
  ack.hd.seq_num = fab1;
  printf("Envoi ACK..... \n");
  printf("Fiabilité client finale %d \n", fab1);
  if(IP_send(ack,dist_sock_addr) == -1)
    printf("Error send ACK \n");
  else
    local_sock.state = CONNECTED;
  if(nb_essai == MAX_ESSAI) {
    printf("Error connect : Nb essai max atteints, pas de syn_ack recu \n");
    return -1 ;
  } else {
    printf("Connexion server réussie \n");
    return 0;
  }
}

int mic_tcp_send (int mic_sock, char* mesg, int mesg_size)
// Permet de réclamer l’envoi d’une donnée applicative
// Retourne la taille des données envoyées, et -1 en cas d'erreur
{
  printf("[MIC-TCP] Appel de la fonction: "); printf(__FUNCTION__); printf("\n");
  //printf("Window avant le send :  ");print_window();
  printf("Timer = %lu .............", RTO);
  int size_pdu;    
  int nb_essai = 0;
  local_sock.state = ATTENTE_SEND;
  //Construction du PDU
  DT.hd.source_port = local_sock.addr.port;
  DT.hd.dest_port = dist_sock_addr.port;
  DT.payload.data = mesg;
  DT.payload.size = mesg_size;
  DT.hd.seq_num = PE;
  PE = (PE+1)%2;
  struct mic_tcp_payload ack;
  ack.data = malloc(15);
  ack.size = 0;
  //printf("Envoi PDU .......... seq_num = %d\n",DT.hd.seq_num);
  unsigned long start_timer = get_now_time_usec();
  size_pdu = IP_send(DT,dist_sock_addr);
  local_sock.state = ATTENTE_ACK;
  while(local_sock.state == ATTENTE_ACK && nb_essai < MAX_ESSAI) {
    //print_window();
    //printf("ratio = %lf \n", ratio());
    if(isnan(ratio()))
      fiabilite = 0 ;
    else
      fiabilite = ratio();
    //printf("fiabilite %lf \n", fiabilite);
    if(IP_recv(&ack,&dist_sock_addr,RTO) == -1) {
      //printf("Expiration Timer ..... Le paquet est perdu \n");
      double diff = fiabilite - ((float)fab1)/((float)(1000));
      if(diff < 0.001 && diff != 0) {
	//printf("Il faut reenvoyer \n");
	IP_send(DT,dist_sock_addr);
	nb_essai++;
	start_timer = get_now_time_usec();
      } else {
	//printf("fiabilite = loss_rate \n");
	PE = (PE+1)%2;
	//printf("On laisse tombé \n");
	ajout(0);
	local_sock.state = ATTENTE_SEND;
	break;
	//size_pdu = 0 ;
	//PE = (PE+1)%2;
      }
    } else {
      //printf("Envoi réussi \n");
      struct mic_tcp_header hd;
      //print_header(ack);printf("\n\n");
      hd = get_header(ack.data);
      //printf("PE = %d ack num %d seq_num %d\n", PE,hd.ack_num,hd.seq_num);
      if(hd.syn == 1 && hd.ack == 1) {
	printf("Recepetion d'un syn_ack \n");
	struct mic_tcp_pdu ack;
	ack.hd.ack = 1;
	ack.payload.data = malloc(15);
	ack.payload.size = 15;
	IP_send(ack,dist_sock_addr);
	local_sock.state = ATTENTE_SEND;
	PE = (PE+1)%2;
	init_window();
      } else if(hd.ack == 1 && hd.ack_num == PE) {
	//printf("Ajout 1 \n");
	ajout(1);
	local_sock.state = ATTENTE_SEND;
      } else {
	//printf("Recepetion mauvais ack, .... Resend\n");
	nb_essai++;
	size_pdu = IP_send(DT,dist_sock_addr); 
      }
    }
  }
  unsigned long now_time = get_now_time_usec();
  unsigned long RTT = now_time - start_timer;
  calculRTO(RTT);
  //printf("Nouveau RTT %lu \n",get_now_time_usec() - start_timer);
  printf("Nouveau RTO %lu \n", RTO);
  if(nb_essai == MAX_ESSAI)
    printf("Echec envoi : Nb tentatives atteints\n");
  //printf("Window apres le send :   ");print_window();
  return size_pdu;
}


int mic_tcp_recv (int socket, char* mesg, int max_mesg_size)
// Permet à l’application réceptrice de réclamer la récupération d’une donnée 
// stockée dans les buffers de réception du socket
// Retourne le nombre d’octets lu ou bien -1 en cas d’erreur
// NB : cette fonction fait appel à la fonction app_buffer_get() 

{
  printf("[MIC-TCP] Appel de la fonction: "); printf(__FUNCTION__); printf("\n");
  struct mic_tcp_payload data;
  data.data = mesg;
  data.size = max_mesg_size;
  //Appel de la fonction APP_buffer_get()
  return app_buffer_get(data);
}

int mic_tcp_close (int socket)
// Permet de réclamer la destruction d’un socket. 
// Engendre la fermeture de la connexion suivant le modèle de TCP. 
// Retourne 0 si tout se passe bien et -1 en cas d'erreur

{
  printf("[MIC-TCP] Appel de la fonction: "); printf(__FUNCTION__); printf("\n");
  //close coté server detecté avec le state
  if(local_sock.state == SEND_FIN_ACK) {
    printf("CLOSE SERVER \n");
    while(local_sock.state != FIN_ACK_SENT);
    printf("FIN_ACK SENT \n");
    while(local_sock.state != CLOISING);
    return 0 ;
  }
  //close coté client 
  printf("ENVOIE FIN ....\n");
  struct mic_tcp_pdu FIN;
  int nb_essai = 0 ;
  FIN.payload.data = "";
  FIN.payload.size = 0;
  FIN.hd.fin = 1;
  IP_send(FIN,dist_sock_addr);
  local_sock.state = FIN_SENT;
  //boucle d'attente pour compenser celle dans process received
  unsigned long start_time = get_now_time_msec();
  unsigned long current_time = get_now_time_msec();
  while(current_time < start_time + 500) {
    current_time = get_now_time_msec();
  }
  while(local_sock.state != FIN_ACK_RECEIVED && nb_essai < MAX_ESSAI) {
    struct mic_tcp_payload fin_ack;
    fin_ack.data = malloc(15);
    fin_ack.size = 15;
    if(IP_recv(&fin_ack,&dist_sock_addr,RTO) == -1) {
      printf("Expiration Timer ..... fin_ack pas reçu \n"); 
      IP_send(FIN,dist_sock_addr);
      nb_essai++;
    } else {
      struct mic_tcp_header hd;
      hd = get_header(fin_ack.data);
      if(hd.fin == 1 && hd.ack == 1) {
	printf("FIN_ACK recu ... ");
	local_sock.state = FIN_ACK_RECEIVED;
      }
    }
  }
  printf("ENVOI ACK\n");
  struct mic_tcp_pdu ACK;
  ACK.hd.fin = 0;
  ACK.hd.ack = 1;
  ACK.payload.data = "";
  ACK.payload.size = 15;
  IP_send(ACK,dist_sock_addr);
  //Boucle attente de "ne rien recevoir" avant de fermer la connexion pendant un certain temps
  //On s'assure de la bonne récepetion de l'ACK par le server
  start_time = get_now_time_msec();
  int attente = 0 ;
  while(attente < 5) {
    current_time = get_now_time_msec();
    while(current_time < start_time + 500) {
      current_time = get_now_time_msec();
    }
    struct mic_tcp_payload fin_ack;
    fin_ack.data = malloc(15);
    fin_ack.size = 15;
    if(IP_recv(&fin_ack,&dist_sock_addr,timer) != -1) {
      printf("ACK perdu... reenvoie de l'ack \n");
      IP_send(ACK,dist_sock_addr);
      attente = 0 ;
    }
    attente++;
  }
  return 0;
}


void process_received_PDU(mic_tcp_pdu pdu)
// Gère le traitement d’un PDU MIC-TCP reçu (mise à jour des numéros de séquence
// et d'acquittement, etc.) puis insère les données utiles du PDU dans le buffer 
// de réception du socket. Cette fonction utilise la fonction app_buffer_set().   
{
  //printf("[MIC-TCP] Appel de la fonction: "); printf(__FUNCTION__); printf(".........");
  //printf("Reception : fin  = %d et ack = %d ",pdu.hd.fin,pdu.hd.ack);  
  //printf("size pdu %d \n", pdu.payload.size);
  
  //Mode Fermeture
  if(pdu.payload.size == 0 && pdu.hd.fin == 1 && local_sock.state != FIN_ACK_SENT) {
    //Première recepetion du "FIN", envoi du fin_ack
    local_sock.state = SEND_FIN_ACK;
    //printf("Detection du close coté server (FIN RECU) \n");
    app_buffer_set(pdu.payload);
    //Boucle d'attente que le mic_tcp_recv appelle app_buffer_get()
    //Dans server.c, rcv_size = mic_tcp_recv() < 0
    //Appelle de mic_tcp_close (coté server) (state = send_fin_ack)
    unsigned long start_time = get_now_time_msec();
    unsigned long current_time = get_now_time_msec();
    while(current_time < start_time + 500) {
      current_time = get_now_time_msec();
    }
    printf("ENVOI FIN_ACK \n");
    struct mic_tcp_pdu fin_ack;
    fin_ack.payload.data = "";
    fin_ack.payload.size = 15;
    fin_ack.hd.fin = 1 ;
    fin_ack.hd.ack = 1 ;
    IP_send(fin_ack,dist_sock_addr);
    local_sock.state = FIN_ACK_SENT;
    return;
  }
  
  if(local_sock.state == FIN_ACK_SENT && pdu.hd.fin == 1) {
    //fin_ack perdu, recepetion d'un nouveau "FIN", reenvoie du fin_ack
    printf("REENVOIE FIN_ACK \n");
    struct mic_tcp_pdu fin_ack;
    fin_ack.payload.data = "";
    fin_ack.payload.size = 15;
    fin_ack.hd.fin = 1 ;
    fin_ack.hd.ack = 1 ;
    IP_send(fin_ack,dist_sock_addr);
  }
  
  if(local_sock.state == FIN_ACK_SENT && pdu.hd.ack == 1) {
    //Bonne récepetion de l'ack de fermeture
    printf("ACK RECU \n");
    local_sock.state = CLOISING;
    return;
  }
  // *** Fin mode fermeture *** //
  
  //Mode Connexion
  if(connexion == 1) {
    printf("Mode Connexion .. ");
    if(local_sock.state == SYN_ACK_SENT && pdu.hd.ack != 1) {
      //Cas ou le syn_ack a été perdu
      printf("Renvoi syn_ack \n");
      struct mic_tcp_pdu syn_ack;
      syn_ack.hd.source_port = local_sock.addr.port;
      syn_ack.hd.dest_port = dist_sock_addr.port;
      syn_ack.payload.size = 15;
      syn_ack.payload.data = malloc(15);
      syn_ack.hd.syn = 1;
      syn_ack.hd.ack = 1;
      if(IP_send(syn_ack,dist_sock_addr) == -1) 
	printf("error send \n");
      return;
    }
    if(pdu.hd.syn == 1 && pdu.hd.ack != 1) {
      //Negociation fiabilite
      int fiabilite_client = pdu.hd.seq_num;
      //printf("\n Fiabilite client %d \n",fiabilite_client);
      if(fab2 < fiabilite_client) {
	printf("Nouvelle fiabilite server ! \n");
	fab2 = fiabilite_client;
      }
      local_sock.state = SYN_RECEIVED;
      return;
    }
    if(pdu.hd.ack == 1 && pdu.hd.syn != 1) {
      printf("ACK recu.\n");
      local_sock.state = CONNECTED;
      return;
    }
    // *** Fin mode Connexion *** //

    //Mode Data
  } else {
    //printf("Mode Data \n");
    if(pdu.hd.seq_num == PA) {
      //printf("DATA RECU ............... ");
      app_buffer_set(pdu.payload);
      PA = (PA+1)%2; 
    } else {
      //printf("Duplicate ..... ");
    }
    struct mic_tcp_pdu ack;
    ack.hd.source_port = local_sock.addr.port;
    ack.hd.dest_port = dist_sock_addr.port;
    ack.hd.ack_num = PA;
    ack.hd.ack = 1;
    ack.hd.syn = 0;
    //printf("(RE)ENVOI ACK, ack num = %d\n",ack.hd.ack_num);
    IP_send(ack,dist_sock_addr);
  } 
}
