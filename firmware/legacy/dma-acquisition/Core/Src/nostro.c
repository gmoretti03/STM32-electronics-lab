#include <stm32h743xx.h>
#include <nostro.h>

#define N 100
#define P 20 //VOGLIO ACQUISIRE 20 DATI DI PRETRIGGER
//definisco il numero di misure che voglio fare (avendo due variabili ricorda che dovra' essere
//il doppio rispetto al numero di capionamenti che si vuole fare)


volatile uint16_t DMA_ADC_vettore[Na*(N+1)]; // dichiaro il vettore di Na*(N+1), mettendo una posizione in più per mettere il punto del trigger.
// lo moltiplico per il numero di acquisizioni che voglio fare prima di spedirlo a matlab
// volatile tells the compiler not to optimize anything that has to do with the volatile variable
// allocazione della memoria necessaria volta a leggere e scrivere i dati

uint8_t* tx_ptr;
uint16_t tx_size, tx_pos;
uint32_t buffer_posizione;

//funzione che prende gli indirizzi di memoria dei valori di calibrazione di temperatura e tensione

//funzione che 'accende' USART
void inizializza_USART(void)
{
	//con questa funzione inizializzo la USART per la trasmissione e la ricezione di dati
	USART3->CR1 |= USART_CR1_RE | USART_CR1_RXNEIE; //abilito la ricezione dell'USART e l'interrupt della ricezione del DMA
	USART3->CR1 |= USART_CR1_TE; //| USART_CR1_TCIE;
	USART3->CR1 |= USART_CR1_UE;

}

//funzione che inizializza TIM6 come periferica
void inizializza_TIM6 (void) {
	 //abilito il TIM6 e il suo interrupt
	  //TIM6 -> PSC = 48; //metto un prescaler
	  TIM6 -> ARR = 240; //quando CNT arriva a questo valore parte l'interrupt del timer
	  TIM6 -> CNT = 0; //metto per sicurezza il counter a zero
	  //TIM6 -> DIER |= TIM_DIER_UIE; //abilita l'interrupt del timer: non lo voglio abilitare perchè non lo utilizzo quindi lo tolgo
}

//funzione che abilita le impostazioni iniziali dell'ADC:
void inizializza_ADC(void)
{
	// Calibriamo ADC
	ADC3->CR &= ~ADC_CR_ADCALDIF; // calibrazione per misura non differenziale (single end)
	ADC3->CR |= ADC_CR_ADCALLIN; // calibrazione lineare
	ADC3->CR &= ~ADC_CR_ADEN; // disabilita ADC
	ADC3->CR |= ADC_CR_ADCAL; // parti calibrazione

	while (ADC3->CR & ADC_CR_ADCAL) // attendi fine calibrazione
	{}

	// Accendiamo ADC (abilitiamo)
	ADC3->ISR = ADC_ISR_ADRDY; // azzero flag scrivendogli 1
	ADC3->CR |= ADC_CR_ADEN; // accendo ADC

	while (~ADC3->ISR & ADC_ISR_ADRDY) // aspetto che si accenda
	{}

	ADC3->ISR = ADC_ISR_ADRDY; // (opzionale)

	// Abilito gli interrupt
	ADC3->IER |= ADC_IER_EOCIE; // abilito interrupt di conversione finita

	//Abilito la modalità di acquisizione continua: PER IL TRIGGER NON VA BENE
	//ADC3 -> CFGR |= ADC_CFGR_CONT;

	// Selezionare i canali e abiliatare termometro e VREFINT
	ADC3->PCSEL |= ADC_PCSEL_PCSEL_0; // | ADC_PCSEL_PCSEL_0;
	ADC3->SQR1 = (0 << ADC_SQR1_SQ1_Pos) // 0 = canale di acquisizione dell'oscilloscopio
			| (0 << ADC_SQR1_L_Pos); // la Mia sequenza e' lunga 1

	ADC3 -> CFGR |= (3<<ADC_CFGR_DMNGT_Pos);
	//sto dicendo all'ADC che sto usando la modalità di buffer circolare: quindi usa il DMA
	ADC3 -> CR |= ADC_CR_ADSTART;
	//ADC3 -> CFGR |= ADC_CFGR_OVRMOD; OVRMOD serve per non avere overrun. Ignora il problema del fatto che mi perdo
	//dati nel momento in cui acquisisce. Se c'è overrun mi blocca l'ADC e capisco che devo migliorare la velocità di elaborazione
}


//funzione che mi inizializza il DMA dell'ADC. Lo abilito all'inizio del codice, lui funziona e poi devo gestire le cose in un reparto separato
void DMA_ADC (void){
	//ADC inizia a misurare!! -> inizio a dire al DMA che deve compiere il suo lavoro
	//assegno l'indirizzo di partenza del vettore definito lì sopra al registro opportuno dal DMA.
	DMA2_Stream0 -> CR &= ~DMA_SxCR_EN;
	DMA2_Stream0 -> CR |= DMA_SxCR_CIRC;
	DMA2_Stream0 -> M0AR = (uint32_t) &DMA_ADC_vettore[volte*N];
	DMA2_Stream0 -> PAR = (uint32_t)(&ADC3->DR); //assegno al DMA l'area di memoria da gestire (QUESTO??)
	DMA2_Stream0 -> NDTR = N; //NUMERO DI MISURE DELL'ADC: quindi mi scrive il DMA solo sui pirmi N elementi
	//DMA2_Stream0 -> CR |= DMA_SxCR_TCIE; NON VOGLIO PIùGESTIRE LE COSE TRAMITE INTERUPT DEL DMA DELL'ADC
	//azzero i flag dell'interrupt del DMA: prima di abilitare il DMA devo assicurarmi che non ci siano flag attivi
	DMA2->LIFCR = 0xffffffff;
	DMA2->HIFCR = 0xffffffff;
	//ora si accende il DMA: ricorda bene che tutti i registri devono essere abilitati prima che lo si accenda
	DMA2_Stream0 -> CR |= DMA_SxCR_EN;
}

//una volta che ho finito di campionare quei dati con il DMA dell'ADC, l'ho spenta (spegnendo il timer) e quindi ho disattivato il DMA dell'ADC.
//(che è spento; ha fatto quello che doveva fare)

int trigger = 0;
int volte = 0; //prima devo impostare volte e poi accendere l'ADC

void DMA_USART (void) {
	//inizializzo il DMA dell'USART
	DMA1_Stream3 -> M0AR = (uint32_t) &DMA_ADC_vettore; //dico al DMA dove si trova il vettore che deve gestire in memoria
	DMA1_Stream3 -> PAR = (uint32_t)(&USART3->TDR);
	DMA1_Stream3 -> NDTR = Na*(N+1)*2; //NUMERO DI MISURE DELL'ADC
	USART3 -> CR3 |= USART_CR3_DMAT; //abilito il bit che serve per la trasmissione USART-DMA
	DMA1_Stream3 -> CR |= DMA_SxCR_TCIE; //abilito l'interrupt di DMA
	DMA1_Stream3 -> CR |= DMA_SxCR_EN; //accensione del DMA
	//con questa funzione mando tutto il vettore che ho creato allocando la memoria all'inizio (TUTTO)!!!!
}

int count = 0;
int stato = 0; //variabile che mi dice dove sono (sopra sotto la soglia)

void ACQUISIZIONE_MULTIPLA(void){
	if (volte < Na-1){
		DMA_ADC();
		TIM6 -> CR1 |= (TIM_CR1_CEN);
		count = 1;
		trigger = 1;
		stato = -1;
	}
	//devo anche spedirli i dati
	else if(volte == 9) {
		DMA_USART();
	}
}

void MATLAB (uint8_t c){
	// attiviamo il trigger se matlab mi manda il qualsiasi cosa (input di inizio del processo di acquisizione)
	//facendo partire il timer faccio partire anche l'ADC
	volte = 0;
	//con questo comando conifguro il TIM6 come sorgente di trigger
	ACQUISIZIONE_MULTIPLA();
}

//funzione che implementa il trigger con il pretrigger
void TRIGGER (void) {
	//if(trigger) mi controlla che la variabile trigger sia a 1

	if (trigger) {
		uint16_t V = ADC3 -> DR; //tensione misurata dall'ADC: così sono sicura di leggere solamente una volta il dato
		if (V < V_SOGLIA_Low && !((DMA1_Stream3 -> CR & DMA_SxCR_EN) == DMA_SxCR_EN_Msk) && stato != 0){
			//sono sotto la soglia più bassa (mi accerto che il mio segnale sia al di sotto) e il DMA non sta lavorando
			stato = 0;
		}
		// se il dato dell'ADC sta superando la soglia e il DMA non sta trasmettendo (non ha il flag abilitato per la trasmissione)
		if (V > V_SOGLIA  && !((DMA1_Stream3 -> CR & DMA_SxCR_EN) == DMA_SxCR_EN_Msk) && stato == 0) {
			stato = 1; //sto superando la soglia
			int punto_trigger = N - DMA2_Stream0 -> NDTR;
			DMA_ADC_vettore[N*Na+volte] = punto_trigger;
			 //sto mettendo gli ultimi Na elementi del vettore uguale all'istante di trigger
			//numero di elementi che mi mancano da trasferire per il buffer: ISTANTE DI TRIGGER: metto N- così mando già a matlab quanti dati ha già acquisito
			volte++;
			trigger = 0;
		}
	}
	else if (stato == 1 && count < 79) {
		count++; //sto contando i valori: mentre conto il DMA sta facendo il suo lavoro, memorizzando i valori dell'ADC
	}
	else if (count == 79) { // poi il 79 lo potrò scrivere meglio
		//disabilito l'ADC: per disabilitarla posso semplicemente fermare il Timer
		TIM6 -> CR1 &= ~(TIM_CR1_CEN); //count enable
		ACQUISIZIONE_MULTIPLA();
	}
	else
	{
		int aaaaaa = 0;
	}
}

