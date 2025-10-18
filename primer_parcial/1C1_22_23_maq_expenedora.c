// ------------------------------------------------------------------
// --        EXERCICI 5 EXAMEN 1a CONVOCATORIA 2022-2023           --
// --        Autor: Guillermo Pinteño			           --
// ------------------------------------------------------------------

/* Es vol programar un manegador per a controlar un dispositiu format per deu
dispensadors de productes i un únic contenidor tancat (on hi caben 30
productes com a màxim) per a la recollida de productes fins que ens demanin
que els donem tots els productes del contenidor. En cada dispensador hi ha un
producte diferent, inicialment s’han posat 100 unitats de cada producte. És a
dir, en iniciar el sistema hi haurà 100 unitats de 10 productes diferents. Cada
dispensador només pot servir una unitat del seu producte a la vegada, però els
diferents dispensadors del dispositiu han d’estar treballant en paral·lel, per tant
el dispositiu pot estar servint un màxim de deu productes al mateix moment. En
el moment que un producte arribi al contenidor final, es provoca sempre la
mateixa interrupció. També es produirà aquesta interrupció quan es treguin tots
els productes que estiguin al contenidor i es torni a tancar la porta.

Les operacions que ofereix el dispositiu als usuaris són:
 	- demanar_producte (producte): funció que demana el producte que li
	passem per paràmetre.
	- posar_producte (producte, quantitat): funció que posa al dispensador la
	quantitat del producte que passem per paràmetre.
	- agafar_productes(): funció que obra el contenidor per obtenir tots els
	productes, es fa sempre quan ja tenim tots els productes que s’han
	demanat.

Les operacions internes del kernel que es poden fer en el dispositiu són:
	- on_es_producte (producte): funció que retorna el dispensador on hi ha el
	producte sol·licitat per paràmetre. Si no queden unitats del producte
	sol·licitat, retorna NULL.
	- quantes_unitats(producte): funció que retorna el nombre d'unitats de
	producte que hi ha en el dispensador.
	- obtenir_producte(producte): funció que inicia l’obtenció d’una unitat del
	producte, genera una interrupció quan el producte arriba al contenidor
	final.
	- repondre_producte(producte, quantitat): funció que afegeix la quantitat
	d’unitats de producte a les variables internes del dispositiu.
	- donar_productes(): funció que obre el contenidor. Quan el contenidor
	estigui buit s'activarà la interrupció. Per qüestions de seguretat, abans
	d’executar aquesta funció hem d’assegurar-nos que no hi hagi cap
	dispensador servint productes.
	- quin_dispensador(): funció que s’ha d’executar dins de la interrupció i et
	retorna quin dispensador ha finalitzat el seu lliurament de producte, si la
	interrupció s’ha generat pel buidatge del contenidor, retornarà null.

Responeu les següents preguntes, justificant totes les respostes que feu i fent
èmfasis en el sincronisme i en les dades concretes del problema a resoldre. */

/* a) Escriviu un programa d’usuari que obtingui 3 productes X,Y,Z de la manera més
eficient possible. */


char* productes[] = {"X", "Y", "Z"};
sem_t semafors[];

void demanar_productes_eficient()
{

	//Suposem que demanar_producte és crida síncrona (ja que no podem pasarli semáfor)
	//Fem thread per no haver d'esperar que cada producte es demani un a un.
	for(int i = 0; i < 3; i++)
		semafor[i] = ini_sem(0);
		crear_thread(demana_productes_thread(i));
	
	//Suposem que agafar_productes() espera que tots els productes en curs caiguin.
	//Encara així no podem suposar que aquest s'executará després de que cada thread
	//hagi començat a demanar els productes.
	//Cal assegurar-se que s'executa només després d'haver demanat tots
	for(int i = 0; i < 3; i++)
		waitS(semafor[i]);
	agafar_productes();
}

void demana_productes_thread(int i)
{
	demanar_producte(productes[i]);
	signalS(semafors[i]);
}	


/* b) Quin tipus de dispositiu pot ser l’explicat? (compartible o no compartible) */
/*
 * -- RESPOSTA -- 
 *  És un dispositiu compartible, ja que com té diferents dispensadors és possible que diferents
 *  processos demanin productes diferents al mateix dispositiu alhora. De fet el procés
 *  usuari fa justament això (pero en comptes de processos diferents són threads).
 *  El que no és compartible és cada dispensador individual, ja que cadascún d'aquest només
 *  pot dispensar un dispositiu alhora (per tant només pot atendre una petició).
 */


/* -------------------------- PLANIFICACIÓ APARTATS C i D ---------------------------------
 * Abans d'implementar els exercici D i C es farà l'exercici E, ja que facilitarà la feina.

 * e) Especifiqueu quins camps hauria de tenir el descriptor de dispositiu i quins camps
 * tindran els iorbs.
 */

#define MAX_PROD_CONTENIDOR 30
#define MAX_PROD_DISPENSADOR 100
#define NUM_DISPENSADORS 10

typedef struct
{
	int operacio;		//Operació a realitzar
	char* buffer		//Buffer de dades (Serà el nom del producte)
	int mida;		/*Num de productes a reposar (hauria de ser mida buffer pero
				  al ser un String té centinella i no cal)*/
	pid_t pid;		//PID del procés solicitant
	semafor_t sem;		//Semafor privat per notificar quan s'acabi la petició
	int err;		//Retorn d'estat de finalització
} IORB_t;

//Estructura del descriptor de dispositiu d'un dispensador
typedef struct
{
	int estat;		//Estat del dispensador
	IORB_t* cua_pet_disp;	//Cua de peticions pendents del dispensador
	IORB_t* iorb;		//IORB que s'está atenent actualment
} DD_dispensador_t;

typedef struct
{
	int estat;	//Indica si el contenidor está obert
	IORB_t* iorb_cont;	//Indica el IORB que ha demanat obrir el contenidor
} DD_contenidor;


struct
{
	DD_dispensador_t dispensadors[NUM_DISPENSADORS]; //Llista dels dispensadors disponibles
	DD_contenidor contenidor;
	IORB_t* cua_pet_exp;			//Cua de peticions pendents de la máquina exp.
	semafor_t sem_pet_pend;			//Semafor per avisar de petició pendent a la cua
	int num_prod_demanats;		//Número de productes que s'han demanat i/o estan caient
} DD_expenedora;


/* c) Implementa en pseudocodi la rutina d’interrupció que s'activa al
finalitzar el lliurament del producte que tingui en compte tots els casos en
els que s’activa. */


void RSI_producte()
{	
	//Obtenir quin dispensador ha generat la interrupció
	DD_dispensador_t* disp = quin_dispensador();


	//IORB atès (ja sigui per petició de producte o per buidat de contenidor)
	IORB_t* iorb_rsi;
	
	if(disp != NULL)	//Si la interrupció ha sigut generada per la caiguda d'un producte
	{
		
		//Obtenir el IORB atès
		iorb_rsi = disp->iorb;	
		iorb_rsi->err = OK;

		/* No s'augmenta el nombre de productes demanats ja que això es farà al moment 
		 * d'encuar o iniciar l'obtenció d'un producte.
		 * Si no encara que es demanesin 30 productes, si encara aquests no han caigut
		 * es podrien demanar més */
		
		/* Si hi ha més peticions a la cua del dispensador iniciem la següent.
		 * No cal preocupar-se de si hi ha suficients productes al dispensador
		 * ja que el manegador no encuarà més peticions de les que es poden servir. */
		if(!cua_buida(disp->cua_pet_disp)
			iorb_rsi = cua_desencuar(disp->cua_pet_disp);
			disp->iorb = iorb_rsi;
			disp->estat = OCUPAT;
			obtenir_producte(disp->iorb.buffer);
		}
		else	//Actualitzar el dispensador
			disp->estat = LLIURE;			//Marquem lliure el dispensador	
			
	}
	else			//La interrupció ha sigut generada per el retirament dels productes
	{
		iorb_rsi = DD_expenedora.contenidor->iorb;	//IORB que ha demanat obrir
		DD_expenedora.contenidor.estat = TANCAT;
		DD_expenedora.num_prod_demanats = 0;	//Actualitzar numero de productes demanats
	}


	signal_k(iorb_rsi->sem);	/* Despertarà el procés que ha fet la petició
					   ja sigui per demanar producte o per agafar-los */

}


/* d) Quin tipus de manegador necessitem per controlar aquest
dispositiu? Implementa’l en pseudocodi.*/

/* RESPOSTA: Cal un Menegador del tipus Dispatcher. Com que els dispensadors compten amb 
 * interrupcions de finalització, el manegador només inicialitza/encua les peticions als
 * dispensadors corresponents. */

void manegador_maquina_expenedora()
{
	inicialitzar_dispositius();	//Inicialitza el DD de la màquina i els DD dels dispensadors	
	while(1)
	{
		wait_k(DD_expenedora.sem_pet_pend);

		
		unsigned char despertar_proces = 0;	/* Booleà per saber si al final del
							   tractament cal despertar al procés 
							   que ha fet la petició */
		//Obtenir el IORB següent						
		IORB_t* iorb_m = cua_desencuar(DD_expenedora.cua_pet_exp);
		char* prod_m = iorb_m->buffer;		//Nom del producte

		DD_dispensador_t* disp_m = on_es_producte(prod_m);

		if(iorb_m->op == DEMANAR_PROD)
		{	//Comprovar si es poden demanar més productes (no s'ha arribat al màx 
			//al contenidor
			if(DD_expenedora.num_prod_demanats == MAX_PROD_CONTENIDOR)
			{	iorb_m->err = ERROR_MAX_PROD_DEMANATS;
				despertar_proces = 1;
			}
			else if(quantes_unitats(prod_m) == 0)
			{
				iorb_m->err = ERROR_DISPENSADOR_BUIT;
				despertar_proces = 1;
			}
			else	//Si sí es pot demanar un producte més
			{		
				if(disp_m->estat == OCUPAT)
					cua_encuar(disp_m);
				else
				{
					disp_m->estat = OCUPAT;
					disp_m->iorb = iorb_m;
					/* Suposem que la rutina tmb decrementa la variable interna
					 * del nombre de productes que hi ha al dispensador*/
					obtenir_producte(prod_m);
				}
				
				//En tots dos casos s'augmenta el numero de productes demanats
				//per evitar que es puguin encuar més dels que caben al contenidor
				DD_expenedora.num_prod_demanats ++;
			}	

		}
		else if(iorb_m->op == POSAR_PROD)
		{	//Comprovar si no caben més unitats
			if(quantes_unitats(prod_m) == MAX_PROD_DISPENSADOR)
				iorb_m->err = ERROR_MAX_UNITATS_DISPENSADOR;
			else
			{	repondre_producte(prod_m, iorb->mida);
				iorb_m->m = OK;
			}

			despertar_proces = 1;
		}
		else if(iorb_m->op == AGAFAR_PROD)
		{
			/* (Es suposarà que no es pot obrir el contenidor fins que finalitzin
			 * les peticions pendents i en curs. L'alternativa és interpretar que es pot
			 * obrir mentre no hi hagi cap dispensador ocupat (pero amb posibles 
			 * peticions pendents))*/
			
			//Es comprova si està obert
			if(DD_expenedora.contenidor.estat == OBERT)
			{	/* Si un procés intenta obrir el contenidor
				   però un altre ja el te obert es retorna un error */
				iorb_m->err = ERROR_CONTENIDOR_OBERT
				despertar_proces = 1;
			}
			else	//Contenidor tancat
			{	
				//Comprovar que no hi hagi cap producte pendent o en procés de caure
				unsigned char pendent = 0;
				int i = 0;
				while(!pendent && (i < NUM_DISPENSADORS))
				{	
					pendent = (disp_m->estat == OCUPAT) ||
						   !cua_buida(disp_m->cua_pet_disp);
					i++;
				}
				
				//Si queda algun producte pendent de caure
				if(pendent)
				{
					iorb_m->err = ERROR_PROD_PENDENTS;
					despertar_proces = 1;
				}
				else	//Sí es pot obrir el contenidor
				{	
					DD_exepenedora.contenidor.estat = OBERT;
					donar_producte();
				}
			}
		}
		else	
		{	//No hi hauria d'haver error si es comprova el codi d'operacio al DOIO
			iorb_m->err = ERROR_OP_INEXISTENT;
			despertar_proces = 1;

		}
	

		if(despertar_proces)
			signal_k(iorb_m->sem);

	}

}


