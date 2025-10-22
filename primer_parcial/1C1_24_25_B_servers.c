// ------------------------------------------------------------------
// --        EXERCICI EXAMEN B 1a CONVOCATORIA 2024-2025           --
// --        Autor: Guillermo Pinteño			           --
// --        Corrección: SÍ			                   --
// ------------------------------------------------------------------

/*
 * En una empresa hi ha diversos servidors que poden processar tasques
 * de computació pesada. Hi ha setze servidors disponibles, cadascun amb les
 * mateixes capacitats de càlcul. Els usuaris poden triar el servidor que volen
 * utilitzar per processar les seves dades, o poden deixar l'elecció del servidor al
 * sistema. A cada servidor només se li pot assignar una tasca a la vegada. Si
 * l'usuari ha escollit el servidor el sistema ho ha de respectar. Les tasques a
 * processar s'han de realitzar en ordre FIFO sempre que sigui possible. Les
 * operacions que ofereix el dispositiu pels codi d'usuaris són:
 *
 * 	• seleccionar_servidor(servidor): rutina que selecciona el servidor on es vol
 * 	processar la informació, si el servidor està ocupat retorna error. Si a la
 * 	variable servidor hi ha GENERIC, llavors el sistema li assigna qualsevol
 * 	dels servidors que estiguin lliures.
 *
 * 	• enviar_tasca(fitxer): rutina que envia el fitxer de tasques perquè el servidor
 * 	escollit amb anterioritat les processi, si no s'ha escollit servidor retornarà
 * 	error.
 *
 * Les operacions internes del kernel que es poden fer en el dispositiu són:
 * 	• processar_tasca_K(fitxer, servidor): funció síncrona, llarga en execució,
 * 	que processa la tasca enviada en el servidor introduït per paràmetre, i no
 * 	retorna fins que s'hagi acabat el processament o es produeixi un error. Si es
 * 	produeix un error retorna -1.
 *
 * Responeu les següents preguntes, justificant totes les respostes que feu i fent
 * èmfasis en el sincronisme i en les dades concretes del problema a resoldre.
*/

/*
 * a) Escriu un programa d'usuari que enviï els fitxers proj1.dat i
 * proj2.dat per a ser processats en el servidor-4 i prova1.dat i prova2,dat
 * per a ser processats pel qualsevol dels servidors. Els quatre fitxers es volen
 * processar el més ràpid i eficientment possible. */

void programa_usuari()
{
	int id = obrir("gestor_servers");	//Obtenir el DD de servidors
	semafor_t sems[4];
	for(int i = 0; i < 4; i++)
		sems[i] = ini_sem(0);
	
	//Suposem que la rutina de seleccionar el servidor és ràpida
	if(doio(id, SELECCIONAR_SERVIDOR, "server4", size("server4"), NULL))
	{
		//Crida asíncrona
		doio(id, ENVIAR_TASCA, "proj1.dat", size("proj1.dat"), sem[0];
		doio(id, ENVIAR_TASCA, "proj2.dat", size("proj2.dat"), sem[1];
	}
	if(doio(id, SELECCIONAR_SERVIDOR, "GENERIC", size("GENERIC"), NULL))
	{
		doio(id, ENVIAR_TASCA, "prova1.dat", size("prova1.dat"), sem[2];
		doio(id, ENVIAR_TASCA, "prova2.dat", size("prova2.dat"), sem[3];
	}

	//S'espera a la finalització de totes les tasques	
	for(int i = 0; i < 4; i++)
		waitS(sems[i]);
	
	tancar(id);

}


// Fem primer exercici D ja que facilita la feina pels següents exercicis

/* d) Especifica els camps que hauria de tenir el descriptor del
 * dispositiu, la taula de canals oberts i els iorbs. Quan i qui modifica cada un
 * dels camps que has especificat en les diferents estructures de dades? */

#define NUM_SERVERS 16

typedef struct
{
	pid_t pid;	// PID del procés que fa la petició
	char* buffer;	// Serà "servidor" / "fitxer"
	int mida;	// Mida del buffer
	int err;	// Codi de retorn
	semafor_t sem;
} IORB_t


typedef struct
{
	int estat;		// Estat del servidor respecte càlculs (OCUPAT / LLIURE) 
	IORB_t* iorb_act;	// Petició que s'està atenent en un instant
	pid_t pid_prop;		// PID del procés que és propietari del servidor en un instant
	IORB_t* cua_pet_server	// Cua de peticions concretes per aquest servidor

} DD_sever_t


struct
{
	DD_server_t* servers[NUM_SERVERS];	// DDs servidors gestionats
	IORB_t* cua_pet_pend;			// Cua de peticions qualsevols
	semafor_t sem_pet_pend;			// Semafor control de peticions pendents
	semafor_t sem_servers_lliures;		// Semafor control de servidors lliures
} DD_gestor_servers;



/* b) Implementa en pseudocodi un manegador i tots els elements
 * necessaris per a controlar aquests servidors tal com es demana en
 * l’enunciat. Heu de tenir en compte que el manegador s'ha de quedar
 * bloquejat quan tots els servidors estiguin ocupats. */

/* COMENTARI: L'enunciat indica que el manegador s'ha de quedar bloquejat quan tots els
 * servidors estiguin ocupats, per tant, suposarem que si tots els servidors estan treballant
 * llavors el manegador no accepta cap peticio (ni de enviar tasca ni de seleccionar servidor).
 */

void manegador_gestor_servers()
{
	inicialitzar_dispositius();
	DD_gestor_servers = ini_sem_k(NUM_SERVERS);
	while(1)
	{
		wait_k(DD_gestor_servers.sem_pet_pend);		//Esperar nova petició
	
		//Obtenir última petició
		IORB_t* iorb_m = cua_desencuarFIFO(DD_gestor_servers.cua_pet_pend);

		unsigned char despertar_proces = 0;
		

		DD_server_t* server_m;	//Servidor amb el que es treballarà
		if(iorb_m->op == SELECCIONAR_SERVER)	//Cas operació de seleccionar servidor
		{
			if(iorb_m->buffer == "GENERIC")		// Cas manegador determina el server
				server_m* = obtenir_server_lliure(DD_gestor_server.servers);	
			else					// Cas usuari determina el server
				//Obtenir servidor segons el nom (comprova si ja té propietari)
				server_m* = obtenir_server_id(iorb_m->buffer);

			if(server_m == NULL)	// No es pot asignar cap servidor (ja te propietari)
				iorb_m->err = ERR_NO_ASSIGNAT;
			else
			{	//Si el procés que vol apropiar-se d'un servidor ja tenia un 
				//asignat, cal desassignar-ho
				if(server_assignat(iorb_m->pid))
					desassignar_server(iorb_m->pid);
				server_m->pid_prop = iorb_m->pid;
			}
			
			//En tot dos casos cal despertar al procés
			despertar_proces = 1;	
		}
		else if(iorb_m->op == ENVIAR_TASCA)	//Cas operació d'enviar tasca
		{	
			// Obtenir servidor asignat
			server_m = server_assignat(iorb_m->pid);
			if(server_m = NULL)	// Si el procés no té cap servidor assignat
			{
				iorb_m->err = ERR_CAP_SERVIDOR_ASSIGNAT_PER_OPERAR;
				despertar_proces = 1;
			}
			else
			{	//Només s'assigna càlcul si tots estan lliures
				wait_k(DD_gestor_servers.sem_servers_lliures);

				if(server_m->estat == OCUPAT)
					cua_encuar(server_m->cua_pet_server);
				else	/* Si no està ocupat es procesarà amb un working thread
					 * Dins d'aquest es farà signal una vegada no queden
					 * peticions pendents. */
					crear_thread(working_thread(), server_m, iorb_m->buffer);
			}
		}
		
		// Despertar el procés si cal
		if(despertar_proces)
			signal_k(iorb_m->sem);

	}
}


void working_thread(DD_server_t* server_wt, IORB_t* iorb_wt)
{
	unsigned char pet_pend = 0;	// Booleà per sabe si hi ha més peticions pendents
					// per aquest concret servidor
	server_wt->estat = OCUPAT;
	do
	{
		// Procesar la tasca en el fitxer demanat amb el servidor asignat al procés
		if(processar_tasca(iorb_wt->buffer, servert_wt) == -1)
			iorb_wt->err = ERR_CALCUL;
		else
			iorb_wt->err = CALCUL_OK;
		signal_k(iorb_wt->sem);
			

		if(!cua_buida(server_wt->cua_pet_server))
		{
			iorb_wt = cua_desencuarFIFO(server_wt->cua_pet_server);
			pet_pend = 1;
		}
	} while(pet_pend);
	
	server_wt->estat = LLIURE;
	// Indica que el servidor està lliure
	signal_k(DD_gestor_servers.sem_severs_lliures);
	destruir_thread();
}



/*
 * d) Implementa en pseudocodi una possible rutina obrir i tancar per a
 * usar amb aquests servidors.*/

// Considerem el gestor de servidors compartible (pero els servidors individuals NO)
int obrir(char* nom_dispositiu)
{	
	//Asignar a la Taula de Canals Oberts d'aquest procés el DD del gestor de servidors
	int id = assignar_TCO(obtenir_DD(nom_dispositiu), pid_actual());
	return id;
}

int tancar(int id)
{
	//En aquest cas a més d'alliberar la entrada a la TCO, cal comprovar si el procés
	//té cap servidor asignat	
	if(server_assignat(pid_actual()))
		desassignar_server(pid_actual());

	alliberar_TCO(id, pid_actual());
}



