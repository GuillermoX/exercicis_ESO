// ------------------------------------------------------------------
// --        EXERCICI 6 EXAMEN 1a CONVOCATORIA 2019-2020           --
// --        Autor: Guillermo Pinteño			           --
// --        Corrección: NO			                   --
// ------------------------------------------------------------------

/* Tenim un sistema operatiu amb un planificador multiprogramat
 * de curt termini FCFS amb prioritats. Hi ha un manegador (que s’executa
 * amb prioritat alta) que controla un ratolí compartible de dos botons. En el
 * ratolí es genera una interrupció cada vegada que s’ha polsat un dels dos
 * botons o cada vegada que hi ha un desplaçament per l’eix X o per l’eix Y.
 * Algunes de les operacions que hi ha disponibles són:
 *
 * 	QueHaPassat(): funció síncrona que retorna 1 si s’ha premut el botó dret,
 * 	retorna 2 si és el botó esquerre, 3 si s’ha produït un desplaçament.
 *
 * 	DesplaçamentX(): funció síncrona que dibuixa el cursor i retorna 1, 0 o
 * 	-1 depenent que hagi desplaçat una unitat a la dreta, no hi ha hagut
 * 	desplaçament X o hi ha hagut desplaçament a l’esquerra.
 *
 * 	DesplaçamentY(): funció síncrona que dibuixa el cursor i retorna 1, 0 o
 * 	-1 depenent que hagi desplaçat una unitat amunt, no hi ha hagut
 * 	desplaçament Y o hi ha hagut desplaçament avall.
 *
 * 	MostrarMenuDret(): rutina síncrona que mostra el menú secundari,
 * 	aquesta rutina s’ha d’acabar quan es produeixi un altre polsament.
 *
 * 	OnClik(x,y): rutina síncrona que actualitza els valors x, y rebuts per
 * 	referència de la posició del ratolí en el moment d’execució de la
 * 	rutina.
 *
 * 	DibuixaCursor(x,y): rutina síncrona que esborra el cursor de la posició
 * 	actual i dibuixa el cursor a la posició x, y que es passa per paràmetre.
 *
 * Responeu les següents preguntes, justifiqueu totes les respostes que feu
 * i tingueu en compte les operacions que s’ofereixen.
*/

/* a) + d) Implementa en pseudocodi la rutina d’interrupció del ratolí,
 * fent especial èmfasi en l’ús dels camps del dd i dels iorb’s. */

// Estructures DD i IORB
typedef struct
{
	pid_t pid;
	int op;
	char* buffer;
	int mida;
	semafor_t sem;		// Semàfor per avisar al procés solicitant que s'ha completat E/S
	int err;		// Codi de retorn
} IORB_t;


struct
{
	int estat;
	semafor_t sem_pet_pend;		// Semàfor de peticions pendents
	IORB_t* cua_pet_pend;
	IORB_t* cua_iorb_botons;	// IORBs que esperen a que es premi un botó
	unsigned char menu_dret_activat;	// Booleà que indica si el menú dret està activat
	short x;				// Variable de la posicó x del ratolí
	short y;				// Variable de la posicó y del ratolí
	pid_t* pid_assignats;	// Array dinàmic de dispositius asignats
} DD_ratoli;

/* Suposarem que quan es refereix a que el dispositiu és compartible, això vol dir que 
 * una mateixa RSI del ratolí enviarà la mateixa informació d'un event de botó a tots els procesos.
 * D'altra banda, si un procés vol informació de la posició, el manegador ho gestionarà. La RSI
 * només actualitza els valors de posició en variables. */

void RSI_ratoli()
{
	salvar_context();
	
	int event = QueHaPasat();	// Obtenir què ha activat la RSI
	if(event == BOTO_ESQUERRE || event == BOTO_DRET)
	{	// Enviar informació als iorb que vulguin llegir event de botó
		while(!cua_buida(DD_ratoli.cua_iorb_botons))
		{
			IORB_t* iorb_rsi = cua_desencuar(DD_ratoli.cua_iorb_botons);
			iorb_rsi->err = event;		// Retorna el valor del botó
			signal_k(iorb_rsi->sem);	// Desperta al procés
		}

		if(event == BOTO_DRET)
		{	// XOR per alternar entre 0 i 1
			DD_ratoli.menu_dret_activat ^= 1;

			/* Suposem que la rutina MostrarMenuDret() és síncrona per mostrar el 
			 * menú, però no es bloqueja fins que el menú es tanqui.
			 * (Si no, s'hauria de crear un procés apart) */
			if(DD_ratoli.menu_dret_activat)
				MostrarMenuDret();
			else
				NoMostrarMenuDret();
		}
	}
	else	// Event de moviment del ratolí
	{	//Només cal actualitzar les variables de posició i redibuixar el cursor si és el càs
		int x_rsi, y_rsi;
		OnClick(&x_rsi, &y_rsi);
		unsigned char diferent = 0;

		if(DD_ratoli.x != x_rsi)
		{
			DD_ratoli.x = x_rsi;
			diferent = 1;
		}

		if(DD_ratoli.y != y_rsi)
		{	
			DD_ratoli.y = y_rsi;
			diferent = 1;
		}
		
		//Es dibuixa de nou el cursor si cambia de posició
		if(diferent)
			DibuixaCursor(x_rsi, y_rsi);
	}		
}



/* b) Quin tipus de manegador necessitem per controlar aquest
 * ratolí? Implementa’l en pseudocodi. */

void manegador()
{
	inicialitzar_dispositiu();
	DD_ratoli.menu_dret_activat = 0;
	DD_ratoli.x = 0;	//Fins que no es mogui no tindrà valors reals
	DD_ratoli.y = 0;

	while(1)
	{
		wait_k(DD_ratoli.sem_pet_pend);		//Esperar petició qualsevol
		IORB_t* iorb_m = cua_desencuar(DD_ratoli.cua_pet_pend);
		
		if(!disp_assignat(DD_ratoli.pid_assignats, iorb_m->pid))
		{
			iorb_m->err = ERR_DISP_NO_ASIGNAT;
			signal_k(iorb_m->sem);
		}
		else
		{

			if(iorb_m->op == LLEGIR_POSICIO)
			{	
				//Guardar en el buffer les coordenades
				((*short)(iorb_m->buffer))[0] = DD_ratoli.x;
				((*short)(iorb_m->buffer))[1] = DD_ratoli.y;
				iorb_m->err = COORDS_OK;
				signal_k(iorb_m->sem);		// Avisar al procés que s'ha llegit
			}
			else	//Encuar el IORB per ser atès per la RSI (ja que vol llegir botó)
				cua_encuar(DD_ratoli.cua_iorb_botons, iorb_m);
		}
	}	
}		


/* c) Si el ratolí és compartible i es vol tenir constància dels
 * processos que l’estan usant, escriu el pseudocodi de les crides al
 * sistema obrir i tancar. */

int obrir()
{
	afegir_pid_dd_ratoli(DD_ratoli, pid_actual());	//Afegeix el pid al llistat del DD_ratoli
	int id = afegir_dd_TCO(TCO, DD_ratoli);		//Afegeix el DD al TCO del procés actual
	return id;
}

int tancar(int id)
{
	int err = 0;
	if(pid_assignat(DD_ratoli, pid_actual()))
	{
		desasignar_pid(DD_ratoli, pid_actual());
		extreure_dd_TCO(TCO, DD_ratoli);
	}
	else
		err = -1;
	return err;
}


