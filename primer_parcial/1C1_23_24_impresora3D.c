// ------------------------------------------------------------------
// --        EXERCICI 5 EXAMEN A 1a CONVOCATORIA 2023-2024         --
// --        Autor: Guillermo Pinteño			           --
// --        Corrección: NO			                   --
// ------------------------------------------------------------------



/*
 * Ens han dit que la impressió 3D és futur, per tant, hem comprat 6
 * impressores 3D iguals i les hem modificat per tal que puguem escollir el material
 * (ABS, PLA) i fins a 6 colors de cada material. Després les hem posat dins d’una caixa
 * amb una porta que només s’obra si és segur. La forma d’usar aquestes impressores
 * es primer escollir el material, després el color i finalment enviar l’arxiu STL amb el
 * disseny que volem imprimir. Totes les impressores són completament iguals i a
 * l’usuari li és igual a quina impressora es processi el seu treball sempre que s’utilitzi
 * la primera impressora que estigui lliure, mantenint l’ordre d’arribada de les peticions
 * dels usuaris.
 *
 * Les operacions que oferirem als usuaris són:
 * 	• escollir_material (material): funció que escull per imprimir el material que li
 * 	passem per paràmetre.
 *
 * 	• triar_color (color): funció que escull el color del material que passem per
 * 	paràmetre.
 *
 * 	• imprimir(fitxer.stl): funció que envia les instruccions de dins del fitxer per a ser
 * 	impreses per la impressora.
 *
 * Les operacions internes del kernel que es poden fer en el dispositiu són:
 * 	• donar_productes(): funció síncrona que obra la porta per agafar la impressió
 * 	finalitzada. Aquesta funciona tindrà una execució lenta, donat que la porta
 * 	només es podrà obrir quan sigui segur agafar la impressió 3D.
 *
 * 	• triar_color(color): funció síncrona que escull el material de la impressió, si no hi
 * 	ha el material escollit retornarà null.
 *
 * 	• triar_material(material): funció síncrona que escull el color de la impressió, si
 * 	no hi ha material del color escollit retornarà null.
 *
 * 	• enviar_fitxer(fitxer): funció síncrona que envia el fitxer que es vol imprimir, si
 * 	aquest fitxer no existeix o es produeix algun error en l’enviament retorna l’error.
 * 	Aquesta funció no retornarà fins que el contingut del fitxer hagi estat imprès o
 * 	hagi retornat error. Si durant la impressió es queda sense el material del color
 * 	escollit, engega un LED d’error i espera a que se li afegeixi material per poder
 * 	continuar des del punt on s’ha quedat.
 *
 *
 *
 * 	Responeu les següents preguntes, justificant totes les respostes que feu i fent
 * 	èmfasis en el sincronisme i en les dades concretes del problema a resoldre.
 */


/* a) Escriviu un programa d’usuari que enviï a imprimir el fitxer regal.stl
per a ser imprès en PLA de color vermell. */

void programa_usuari()
{
	if(obrir_impresora())
	{
		escollir_material(PLA);
		triar_color(VERMELL);
		imprimir("regal.stl");

		tancar_impresora();
	}
	else
		printf("Totes les impresores ocupades");

}


// Primer es fa l'exercici D per facilitar l'exercici B

/* d) Especifiqueu quins camps hauria de tenir el descriptor de dispositiu,
 * la taula de canals oberts i els camps que tindran els iorbs. */

#define NUM_IMPRESORES 6

typedef struct
{
	pid_t pid;
	int op;			// ESC_MATERIAL / TRIAR_COLOR / IMPRIMIR
	char* buffer;		// material / color / "fitxer"
	int mida;		// mida buffer
	semafor_t sem;
	int err;

} IORB_t;


typedef struct
{
	//Variables control
	int estat;			// OCUPAT / LLIURE	(però pot tindre un procés asignat)
	IORB_t* cua_pet_imp;		// Cua peticions impresora concreta
	IORB_t* iorb_act;		// IORB en tractament actualment
	pid_t pid;			// Procés que l'està utilitzant (== 0 no està utilitzada)
		
} DD_impresora_t;

struct
{
	DD_impresora_t* imps[NUM_IMPRESORES];		// DDs de les impresores
	IORB_t* cua_pet_pend;
	semafor_t sem_pet_pend;
} DD_conjunt_imp;			//DD del dispositiu que gestiona totes les impresores



/* b) Quin tipus de manegador necessitem per controlar aquests
 * dispositius? Implementa en pseudocodi el manegador i tot el que necessitis per
 * a controlar aquestes impressores. */

/* RESPOSTA: Necessitem un manegador de tipus dispatxer asíncron. Concretament les operacions d'E/S
 * 	     seràn procesades per Working Threads ja que els dispositius no compten
 * 	     amb interrupció de finalització. */

void manegador_impresores()
{
	inicialitzar_dispositius();
	while(1)
	{
		wait_k(DD_conjunt_imp.sem_pet_pend);	//Espera a que hi hagi una nova petició
		IORB_t* iorb_m = cua_desencuar(DD_conjunt_imp.cua_pet_pend);	
		
		//Comprovar si el procés que fa la petició té una impresora asignada
		DD_impresora_t* imp_m = dispositiu_assignat(iorb_m->pid);
		if(imp_m == NULL)	//No s'ha asignat cap impresora al procés
		{
			iorb_m->err = ERR_CAP_IMPRESORA_ASSIGNADA
			signal_k(iorb_m);	//Informar al procés
		}
		else
		{	//Si la impresora ja està lliure
			if(imp_m == LLIURE)
				crear_thread(w_thread_impresora(imp_m, iorb_m));
			else
				cua_encuar(imp_m->cua_pet_imp, iorb_m);
		}

		//Esperar següent petició
	}
}


void w_thread_impresora(DD_impresora_t* imp_wt, IORB_t* iorb_wt)
{
	int op;
	unsigned char pet_pend;		//Booleà per saber si queden peticions a la cua del disp
	imp_wt->estat = OCUPAT;

	do{
		op = iorb_wt->op;
		pet_pend = 0;

		if(op == ESC_MATERIAL)
		{
			int material = (int)*(iorb_wt->buffer);
			if(triar_material(material) == null)	//Selecciona el material (síncron)
				iorb_wt->err = ERR_NO_MATERIAL;
			else
				iorb_wt->err = OK;

		}
		else if(op == TRIAR_COLOR)
		{
			int color = (int)*(iorb_wt->buffer);
			if(triar_color(color) == null)		//Es selecciona el color (síncron)
				iorb_wt->err = ERR_NO_COLOR;
			else
				iorb_wt->err = OK;
		}
		else if(op == IMPRIMIR)
		{
			char* fitxer = iorb_wt->buffer;
			int err = enviar_fitxer(fitxer);	//Es manda a imprimir (síncron)
			if(err != OK)
				iorb_wt->err = ERR_IMPRESIO;
			else
				donar_productes();	//Si s'ha imprimit, s'obre la porta
				iorb_wt->err = OK;
		}

		signal_k(iorb_wt->sem);		//S'avisa al procés que ha fet la petició

		if(!cua_buida(imp_wt->cua_pet_imp))
		{
			pet_pend = 1;
			iorb_wt = cua_desencuar(imp_wt->cua_pet_imp);
		}

	}while(pet_pend)
	
	imp_wt = LLIURE;	// Fins que no atengui totes les peticions pendents no està lliure
	destruir_thread();	//Si ja no queden peticions pendents es destrueix el thread
}



/* c) Implementa en pseudocodi les crides al sistema obrir i tancar. Com
 * són les impressores (compartibles o no compartibles) Justifica la resposta.
 */

/* RESPOSTA: Les impresores han de ser NO compartibles ja que una impresora no ha d'atendre
 * peticions de diferents procesos (ja que cadascún traballa sobre el seu context d'impresió
 * independent).*/

int obrir_impresora()
{
	unsigned char trobada = 0;
	int i = 0;
	
	//Buscar impresora sense utilitzar per cap procés.
	while(!trobada && i < NUM_IMPRESORES)
	{
		trobada == DD_conjunt_imp.imp[i]->pid == 0;	//No està sent utilitzada
		i++;
	}

	if(trobada)
	{
		DD_conjunt_imp.imp[i-1]->pid = qui_soc();	//Asignar PID procés actual
	}

	return trobada;
}


int tancar_impresora()
{

	unsigned char trobada = 0;
	int i = 0;
	pid_t pid = qui_soc();
	
	//Buscar si el procés té asociada cap impresora.
	while(!trobada && i < NUM_IMPRESORES)
	{
		trobada == DD_conjunt_imp.imp[i]->pid == pid;	//Disp asignat a aquest PID
		i++;
	}

	if(trobada)
	{
		DD_conjunt_imp.imp[i-1]->pid = null;	//Marcar impresora com no utilitzada
	}

	return trobada;



}
