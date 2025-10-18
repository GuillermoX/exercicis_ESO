// ------------------------------------------------------------------
// --        EXERCICI 11 EXAMEN 2a CONVOCATORIA 2021-2022          --
// --        Autor: Guillermo Pinteño			           --
// --        Corrección: NO			                   --
// ------------------------------------------------------------------

/*
Tenim un sistema un sistema domòtic per a controlar l’enllumenat d’una fàbrica.
Aquest sistema està actualment format per 8 dispositius d'il·luminació amb els seus
corresponents sensors que s‘usen per a observar la quantitat de llum que hi ha en
l’entorn dels dispositius lumínics. En aquest sistema també hi ha un temporitzador
associat a cada dispositiu que activa una interrupció pròpia quan finalitza el temps
sol·licitat d'il·luminació. Tot i que els dispositius domòtics són compartibles, no es
poden executar concurrentment dues rutines internes que utilitzin el mateix
temporitzador. Disposem de les següents funcions internes als dispositius:

 	- llegir_sensor(identificador) 
	funció síncrona ràpida d’execució que retorna un número
	que indica la intensitat lumínica que hi ha en el seu entorn.

	- obrir_llum(identificador, valor_intensitat, temps) 
	funció síncrona ràpida d’executar
	que encén el llum amb la intensitat necessària per aconseguir el valor d'intensitat que
	se li demana per paràmetre. Aquesta rutina programarà un temporitzador amb el
	valor indicat per temps (minuts) i retornarà un zero si no hi ha hagut error o un valor
	positiu en cas contrari. El temporitzador programat activarà una interrupció quan hagi
	passat el temps corresponent posant l’activitat lumínica en el seu valor mínim. I si el
	temps que se li passa per paràmetre és zero, no es programarà cap temporitzador,
	mantenint el mateix valor fins que es torni a executar una altre funció obrir_llum. Si
	el valor_intensitat es menor que la intensitat mínima, la rutina servirà per disminuir la
	intensitat lumínica durant el temps demanat, provocant durant aquest temps que la
	interrupció per menor intensitat lumínica no faci res.

	- intensitat_minima(identificador, valor_minim) 
	funció síncrona ràpida d'executar que
	actualitza un camp del descriptor de dispositiu, amb el valor mínim d’intensitat
	lumínica que es considera adequada en l’entorn. Quan la intensitat lumínica baixi
	d’aquest valor de manera natural, s’activarà una interrupció que haurà d'obrir el llum,
	posant el valor d' intensitat mínim durant un temps indefinit, això s’aconsegueix
	posant la variable temps a zero, i per tant, no fent ús del temporitzador. Si la intensitat
	lumínica inferior ha estat produïda per la rutina d’obrir_llum, no s’activarà cap
	interrupció mentre estigui activa la condició actual produïda per la funció obrir_llum.
*/


/*
 * a ) Implementeu un processos d’usuari per festejar el sistema que posin una
 * intensitat lumínica mínima de 250 lux a tots els dispositius, i després activi els dispositius
 * parells a 500 lux durant 10 minuts i posi els dispositius imparells a 0 lux durant els
 * mateixos 10 minuts. Al finalitzar aquest temps es vol llegir els valors de lluminositat en
 * tots els sensors, que hauria de ser de 250 lux. Mostrarà per pantalla quins dispositius han
 * anat bé i quins dispositius no han mostrat el valor esperat.
*/

void festejar()
{
	int ids[8]; 
	int intensitat;
	for(int i = 0; i < 8; i++)
	{
		ids[i] = obtenir_llum();
		intensitat_minima(ids[i], 250);

		intensitat = 500;			//Parells 500 lux
		if(ids[i] % 2) intensitat = 0;		//Senars 0 lux
							
		obrir_llum(ids[i], 10);			//Encendre per 10 minuts

	}

	esperar_temps(10);				//S'espera 10 minuts
	for(int i = 0; i < 8; i++)
	{
		if(llegir_sensor(ids[i]) == 250)
			printf("Dispositiu %d correcte\n", ids[i]);
		else			
			printf("Dispositiu %d incorrecte\n", ids[i]);
	}

}


// --- Primer es farà l'exercici C ja que facilitarà la feina de l'exercici B ----

/* c) Especifiqueu els camps de les diferents estructures de dades que necessitem
 * per implementar aquest sistema (iorb, dd,...) explicant cada camp qui els usa i per a què
 * s’usen. Com controlarem el funcionament de la interrupció que s'activa quan la
 * lluminositat baixa del valor mínim establert?
 */

#define NUM_LLUMS 8

typedef struct
{
	int intensitat;
	int temps;
} obrir_llum_data_t;		/* Struct que s'inserirà en el buffer del IORB en l'implementació
				 * de l'operació "obrir_llum", ja que aquesta necessita 2 params */


 typedef struct
{
	int op;			//Operació a realitzar sobre el dispositiu
	int id_disp;		//Id del dispositiu sobre el que fer la petició
	char* buffer;		//Buffer de dades (intensitat i temps)
	int mida;		//Mida del buffer
	semafor_t sem;		//Semàfor que utilitzarà el manegador per avisar al procés
	int err;		//Codi d'error de finalització

	pid_t pid;		//Identificador del procés que fa la petició
} IORB_t;


typedef struct
{
	//Control de peticions
	int estat;			//Estat del llum (SOTA_MIN_ON / SOTA_MIN_OFF)
	IORB_t* cua_pet_llum;		//Cua de peticions d'operacions
	IORB_t* iorb_act;		//IORB que s'està servint (no farà falta) 
	
	//Control del llum
	//Suposarem que tot son registres mapejats a memoria
	int* intensitat_act;		//Intensitat programada actualment
	int* intensitat_min;		//Intesitat mínima fixada
	int* intensitat_entorn;		/* Intensitat que detecta el sensor (l'actualitza 
					 * el dispositiu)*/
	int* temp;			/* Suposem que el temporitzador es programa
					 * amb aquesta variable i que es pot consultar
					 * per saber si està encés (!= 0) o apagat (== 0) */

} DD_llum_t


struct
{
	DD_llum_t* llums[NUM_LLUMS];	//DD de les llums del sistema d'enllumenat	
	int estat; 			//Estat de l'enllumenat (no farà falta)
	IORB_t* cua_pet_pend;		//Cua de peticions pendents;
	semafor_t sem_pet_pend;		//Semàfor petició pendent disponible
} DD_enllumenat;



/*
 * b) Implementeu en pseudocodi el manegador i tots els elements necessaris de
 * control per implementar les accions demanades en l‘enunciat, fent especial èmfasis en
 * tota la sincronització dels diferents elements i en l'ús de les variables dels descriptors de
 * dispositius i dels iorbs.
 */

void manegador_enllumenat()
{
	inicialitzar_enllumenat();
	while(1)
	{
		wait_k(DD_enllumenat.sem_pet_pend);	//Esperar nova petició
		IORB_t* iorb_m = cua_desencuar(DD_enllumenat.cua_pet_pend);

		int op_m = iorb_m->op;	//Obtenir operació a realitzar
		DD_llum_t* llum_m = DD_enllumenat.llums[iorb_m->id];	//Llum a tractar
					
		unsigned char despertar_proces = 0;	/* Boolèa per saber si despertar el procés
							   desde el manegador principal */
		
		if(op_m == OP_LLEGIR_SENSOR)
		{	//Realitzar la operació de lectura directament al manegador principal.
			//Això suposant que el sensor actualitza el valor en un registre mapejat.
			*(iorb_m->buffer) = *(llum_m->intensitat_entorn);
			iorb_m->mida = 4; 	//4 bytes (int)
			iorb_m->err = OK;
			despertar_proces = 1;	
		}
		else if(op_m == OP_ACT_INT_MIN)
		{	//Realitza la operació de escriptura de int.mín. a la variable del DD_llum
			*(llum_m->intensitat_minima) = (int) iorb_m->buffer;
			iorb_m->err = OK;
			desperat_proces = 1;
		}
		else if(op_m == OP_OBRIR_LLUM)
		{	//Si el temporitzador està en martxa la petició s'encua
			if(*(llum_m->temp))
			{
				cua_encuar(llum_m->cua_pet_llum, iorb_m);
			}
			else	//La petició s'atén directament
			{	//Obtenir les dades per obrir el llum
				obrir_llum_data_t obrir_data = (obrir_llum_data_t)*(iorb_m->buffer);
				
				*(llum_m->intensitat_act) = obrir_data.intensitat;
				/* Si la intensitat es programa per sota del mínim es desactiva
				 * la IRQ de detecció de intensitat sota el mínim */
				if(obrir_data.intensitat < *(llum_m->intensitat_min))
				{
					desactivar_IRQ(llum_m, IRQ_INT_SOTA_MIN);
					llum_m->estat = SOTA_MIN_ON;
				}
				
				//Si el temps a programar no és 0, s'activa el timer
				if(obrir.data.temps != 0)
					*(llum_m->temp) = obrir_data.temps;		

				despertar_proces = 1;
			}
		}
		else	//No hauria d'haver error si les operacions comproven paràmetres
		{
			iorb->err = ERROR_OP_NO_EXISTENT;
			despertar_proces = 1;
		}
		

		if(despertar_proces) 
			signal_k(iorb_m->sem);

		//El manegador continua a esperar la pròxima petició
	}
}


/* L'exercici no ho demana explícitament, però per fer entendre el funcionament del manegador
 * s'implementaran les RSI dels timer i la RSI de la IRQ de intensitat sota el mínim */

void RSI_timer_llum()
{
	
	int id = llum_invocadora_RSI();	
	DD_llum_t* llum_rsi = DD_enllumenat.llums[id];

	//Si no hi ha més peticions d'obrir llum es restaura al seu estat base
	if(cua_buida(llum_rsi->cua_pet_llum))
	{

		//S'estableix la llum actual al mínim
		*(llum_rsi->intensitat_act) = *(llum_rsi->intensitat_min);
		*(llum_rsi->temp) = 0;		//Desactivar el timer
		
		//Si la intensitat estaba sota el mínim activa la IRQ
		if(llum_rsi->estat == SOTA_MIN_ON)
		{
			llum_rsi->estat == SOTA_MIN_OFF;
			activar_IRQ(llum_rsi, IRQ_INT_SOTA_MIN);
		}
	}
	else	//Si cal tractar més peticions
	{	
		//Tractar el següent IORB com fa el manegador (es podria fer una rutina que tot
		//dos utilitzessin, però com hi ha petites diferències s'ha optat per copiar el codi
		IORB_t* iorb_rsi = cua_desencuar(llum_rsi->cua_pet_llum);	

		obrir_llum_data_t obrir_data = (obrir_llum_data_t)*(iorb_rsi->buffer);
		
		*(llum_m->intensitat_act) = obrir_data.intensitat;
		/* Si la intensitat es programa per sota del mínim (i no estaba ja en el mode 
		 * sota mínim) es desactiva la IRQ del sensor */
		if(obrir_data.intensitat < *(llum_m->intensitat_min) &&
		   llum_rsi->estat == SOTA_MIN_OFF)
		{
			desactivar_IRQ(llum_m, IRQ_INT_SOTA_MIN);
			llum_rsi->estat == SOTA_MIN_ON;
		}
		
		//Si el temps a programar no és 0, s'activa el timer
		if(obrir.data.temps != 0)
			*(llum_m->temp) = obrir_data.temps;		

		signal_k(iorb_rsi->sem);	//S'avisa que ja s'ha programat el llum

	}
}



void RSI_sota_minim()
{
	int id = llum_invocadora_RSI();
	DD_llum_t* llum_rsi = DD_enllumenat.llums[id];

	//Establir la llum actual a la mínima
	*(llum_rsi->intensitat_act) = *(llum_rsi->intensitat_min);
	*(llum_rsi->temp) = 0;		//Asegurar que el timer està desactivat;
}

