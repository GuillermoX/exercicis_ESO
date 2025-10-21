// ------------------------------------------------------------------
// --        EXERCICI EXAMEN A 1a CONVOCATORIA 2024-2025           --
// --        Autor: Guillermo Pinteño			           --
// --        Corrección: NO			                   --
// ------------------------------------------------------------------

/*
Tenim una granja de setze servidors que poden ser usats en paral·lel
pels diferents usuaris. Aquests servidors utilitzen diferents acceleradors de
maquinari (com GPUs o TPUs) per a realitzar les tasques de càlcul intensiu d’una
manera més eficient. Hi ha fins a quatre acceleradors disponibles per a usar en
qualsevol dels servidors. Els codis d'usuari enviaran els treballs de càlcul a un
servidor en concret, especificant-li el mode de càlcul desitjat i el fitxer que conté
els càlculs a realitzar. Quan el servidor necessiti fer algun càlcul usarà el primer
accelerador que estigui disponible, respectant l'ordre d'arribada de les peticions.
Cada accelerador té dos tipus de modes de càlcul: precisió alta i precisió baixa. Si
en el moment de fer un càlcul, la precisió amb la que l'ha de realitzar es
incorrecte, se li ha de retornar -2 i no demanar el càlcul a l'accelerador.
Les operacions que ofereix els dispositius accelerador al codi executat pels
processos d'usuari són:

	• realitzar_càlcul(càlcul, mode): rutina sincrona que envia els càlculs per a
	ser processants, en funció del mode de càlcul que s'indica per paràmetre.

Les operacions internes del kernel que es poden fer en els acceleradors són:

	• realitzar_càlcul_K(càlcul, mode): rutina síncrona, lenta en temps
	d’execució, que envia els càlculs a l'accelerador en el mode passat per
	paràmetre. Quan s'ha acabat de realitzar el càlcul retorna 0 si tot ha anat
	bé, o -1 si ha hagut algun error en el càlcul.


Responeu les següents preguntes, justificant totes les respostes que feu i fent
èmfasis en el sincronisme i en les dades concretes del problema a resoldre.
*/


/* DUBTES ENUNCIAT:
 * - L'usuari pot especificar que servidor utilitzar o se li assigna un qualsevol?
 * - El resultat dels càlculs es rellevant o l'ignorem?
 * - Que vol dir el següent fragment: Si en el moment de fer un càlcul, la precisió
 *   amb la que l'ha de realitzar es incorrecte se li ha de retornar -2 i no demanar 
 *   el càlcul a l'accelerador. -> Que vol dir que sigui incorrecte?


/* a) Sabent que disposem d’una rutina síncrona de llibreria calcula
 * que rep dos paràmetres: el nom d’un fitxer on hi ha varis càlculs a realitzar i
 * la precisió dels càlculs amb que es volen realitzar. Escriu un programa
 * d'usuari que usant qualsevol dels servidors, enviï el fitxer calculus.dat que
 * conté dos càlculs per a ser processats a precisió alta, i el fitxer prova.dat
 * que conté cinc càlculs per a ser processats a precisió baixa. Volem
 * processar els dos fitxers de la manera més eficient i ràpida possible usant la
 * rutina de llibreria calcula.
 */

//Variables globals per poder accedir desde threads
char* fitxers[] = {"calculus.dat", "prova.dat"};
int precisio[] = {ALTA, BAIXA};

void programa_usuari_calcula()
{
	int num_fit = 2;
	thread_id tid[num_fit];

	for(int i = 0; i < num_fit, i++)
	{
		crea_thread(&tid[],calcul_thread(), i);
	}
	
	//Sincronitza amb la finalització dels threads
	for(int i = 0; i < num_fit; i++)
		espera_thread(tid[i]);
}

void calcul_thread(int i)
{
	calcula(fitxers[i], precisio[i]);
}



/* b) Com implementaries en pseudocodi la rutina de llibreria calcula.
 * Per implementar-la disposem d'una funció llegir_càlcul que llegeix un
 * càlcul d'un fitxer i una funció eof que retorna final de fitxer.
 */

void calcula(char* filepath, int precisio)
{
	FILE* f = fopen(filepath, "r");
	calcul_t calc;
	thread_id tid[];	//Suposem arrays de creixement dinàmic.
	
	int num_calc = 0;
	while(!eof(f))
	{
		calc = llegir_calcul(f);		//Llegir següent càlcul
		//Utilitzem un thread per cada càlcul per aprofitar temps.
		crea_thread(&tid[num_calc], realitzar_calcul(), calc, precisio);
		num_calc ++;
	}
	
	//Esperem tots els threads per sincronitzar-se
	for(int i = 0; i < num_calc; i++)
		esperar_thread(tid[i]);

}

// Es farà primer l'exercici D ja que facilita la feina de l'exercici C:

/* d) Especifica els camps que hauria de tenir el descriptor del
 * dispositiu i els iorbs. Quan i qui modifica cada un dels camps que has
 * especificat en les diferents estructures de dades?
 */

#define NUM_ACC_SERVER 4

typedef struct
{
	pid_t pid;
	int op;			//Suposem que s'indicarà el tipus de precisió (ALTA/BAIXA)
	char* buffer;		//Suposem que s'especifica el cálcul en forma de String
	int mida;		//Mida del buffer de caracters (encarà que té centinella)
	int err;		//Codi de retorn
	semafor_t sem;		//Semafor de finalització de petició
} IORB_t;

typedef struct
{
	int estat;			//Estat del accelerador (LLIURE/OCUPAT)
	IORB_t* cua_pet_acc;		//Cua de peticions d'un accelerador concret
	IORB_t* iorb_act;		//IORB que el accelerador està atenent
} DD_accelerador_t;

struct
{
	DD_accelerador_t* accs[NUM_ACC_SERVER];		//DDs dels acceleradors d'un servidor
	int estat;					//Estat servidor (no s'utilitzarà)
	semafor_t sem_pet_pend;				//Semafor de peticions pendents
	IORB_t* cua_pet_pend;				//Cua general de peticions

} DD_servidor;



/* c) Implementa en pseudocodi el manegador dels acceladors i tot el
 * que necessitaries per a controlar-los per a què funcionin tal com s'especifica
 * en l’enunciat.
 */


void manegador_servidor()
{
	inicialitzar_servidor(DD_servidor);
	inicialitzar_acceleradors(DD_servidor.accs);

	while(1)
	{
		wait_k(DD_servidor.sem_pet_pend);	//Esperar a que hi hagi peticions
		IORB_t* iorb_m = cua_desencuar(cua_pet_pend);
		
		int id_acc = obtenir_acc_lliure(DD_servidor.accs);
		if(id_acc == -1)	//No ha trobat accelerador lliure
		{	//Encuem la petició a l'accelerador menys saturat
			id_acc = obtenir_acc_menys_peticions(DD_servidor.accs);
			cua_encuar(DD_servidor.accs[id_acc], iorb_m);
		}
		else	//Acelerador lliure
		{
			//Crear working thread per a que faci el cálcul
			crear_thread(working_thread_acc, iorb_m, DD_servidor.accs[id_acc]);

		}
		
		//Tornar a esperar següent petició
	}
}

void working_thread_acc(IORB_t* iorb, DD_accelerador_t* acc)
{
	char* calcul;
	int precisio;

	unsigned char pet_pend;
	
	do
	{
		char* calcul = iorb->buffer;
		int precisio = iorb->op;

		if(error_precisio(precisio))	//Interpretació de l'especificació del enunciat
			iorb->err = -2;		//sobre retorn de codi d'error -2
		else
			realitzar_calcul_k(calcul, precisio);


		signal_k(iorb->sem);	//Desperta al procés que ha generat el IORB
		
		if(cua_buida(acc->cua_pet_acc)	//Si no hi ha més peticions a la cua acabar thread
			pet_pend = 0;
		else				//Si hi ha, continuar fent cálculs amb l'accelerador
		{
			iorb = cua_desencuar(acc->cua_pet_acc);
			pet_pend = 1;
		}
	
	while(pet_pend);
	
}
