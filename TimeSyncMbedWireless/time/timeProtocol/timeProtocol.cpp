/*
 * timeProtocol.c
 *
 * Created: 16/05/2016 15:54:27
 *  Author: MORAND Baptiste
 */ 
#include "timeProtocol.h"

/************************************************************************/
/* GLOBAL                                                               */
/************************************************************************/
TimeProt timeProt;
char messageHMI[200];
char messageHMICor[200];
char messageHMIDelay[200];
char messageHMIOffset[200];
Clock sumOffset;
portTickType saveDRequest=0;

portTickType saveCorrection=0;
#ifdef MASTERMODE
portTickType saveSync=0;
#endif
/************************************************************************/
/* FUNCTION                                                             */
/************************************************************************/

void correction(void){
	

	if(xTaskGetTickCount()-saveCorrection>(TIME_CORRECTION/portTICK_RATE_MS)){
			
			if(timeProt.correction.nbCorrection>2){
				saveCorrection=xTaskGetTickCount();
				if(!hardwareCorrection()){
					sofwareCorrection();
				}
			}
			
		
	}
	
}

void sofwareCorrection(){
	if(timeProt.correction.nbCorrection!=0){
	//	printf("nb corr : %d",timeProt.correction.nbCorrection);
		sendHMI("	Software Correction ");
		//interval of time
		Clock averageTime;
		Clock averageOffset;
		Clock sumOffsetl;
		float fcorr=0;
		int indice=timeProt.correction.indiceFull;
			

		sumOffsetl=sumOffset;

		
		//sign change
		if(timeProt.correction.valueCor!=sumOffsetl.sign){//if the sum of the offset change of sign error ze reset the software corection 
			indice=0;
			timeProt.correction.indiceMoySoftCor=0;
		
		}
		
		float second=(float)timeProt.correction.sumTime.second/timeProt.correction.nbCorrection;
		int secondi=(int)second;
				//calcul average time
				//printf("second : %d,%d",secondi,(int)((second-secondi)*1000.0));
				
		averageTime.second=secondi;
		averageTime.halfmillis=(timeProt.correction.sumTime.halfmillis/timeProt.correction.nbCorrection)+RTC_FREQ*1.0*((float)(second-secondi));
		//unsigned long int averageTimeli=(averageTime.second*2000)+(averageTime.halfmillis);
		//calcul average offset
		averageOffset.second=sumOffset.second/timeProt.correction.nbCorrection;
		averageOffset.halfmillis=sumOffset.halfmillis/timeProt.correction.nbCorrection;
		averageOffset.sign=sumOffset.sign;
		//display average
		/*printf("averageTime: ");
		printfClock(averageTime);
		printf("averageOffseet: ");
		printfClock(averageOffset);*/
		
		//calcul of average time in hms
		unsigned long int diffhms=averageTime.second*RTC_FREQ+averageTime.halfmillis;
		//offset in hms
		long int offset=averageOffset.second*RTC_FREQ+averageOffset.halfmillis;
		if (averageOffset.sign==false)
		{
			offset=-offset;
		}
		//printf("offset : %lu\r\n",offset);
		//printf("diffhms : %lu\r\n",diffhms);
		float correctionSoft=0.0;
		float correctionTime=0.0;
		int corr;
		int corrTime;
		int nbwait;
		
		corr=valueCor;
		nbwait=timeCorrection;
		corrTime=nbwait;		

		if(offset!=0){

					//fcorr=(float)(10*offset)/(diffhms);
					//printf("correction need %d(%d)\r\n",(int)fcorr,nbwait);	
					if(fcorr<0){
						fcorr=-fcorr;
					}
					
					while((nbwait<1000)&&(fcorr<0.5)){
						fcorr=(float)((float)VALUE_TIMER*nbwait*offset*1.0)/(diffhms);
						nbwait++;
					}
					

					
					
					//fcorr=fcorr*RTC_FREQ;	
					

				//	printf("correction need %d/10(%d)\r\n",(int)fcorr*10,nbwait);			
					int i;
					timeProt.correction.moySoftCor[timeProt.correction.indiceMoySoftCor][0]=fcorr;
					timeProt.correction.moySoftCor[timeProt.correction.indiceMoySoftCor][1]=nbwait;
					timeProt.correction.indiceMoySoftCor++;
					if(indice<MOYCORLENGTH){//if ze have enougth space to sqve the value
						indice++;
					}
					if(timeProt.correction.indiceMoySoftCor>indice){
						timeProt.correction.indiceMoySoftCor=0;
					}
					
					
					for(i=0;i<indice;i++){//go to the maximum value save
						//	printf("i : %d, value : %d\r\n",i,(int)timeProt.correction.moySoftCor[i]);
							correctionSoft+=timeProt.correction.moySoftCor[i][0];
							correctionTime+=timeProt.correction.moySoftCor[i][1];
					}
				//	printf("sumCorr : %d",(int)correctionSoft);
					
					correctionSoft=(float)(1.0*correctionSoft)/(indice*1.0);//calcul the average value of the software correction 
					correctionTime=(float)(1.0*correctionTime)/(indice*1.0);//calcul the average value of the software correction 
					corr=(int)correctionSoft;//cast to int
					corrTime=(int)correctionTime;//cast to int
					Disable_global_interrupt();
					corr=valueCor+Coef_Software_Correction*(corr-valueCor);
					Enable_global_interrupt();
					if(corr<0){
							if(correctionSoft-corr<-0.5){//calculate int the most close to the real value
								corr--;
							}
					//		printf("corr<0");
					}
					else{
							if(correctionSoft-corr>0.5){//calculate int the most close to the real value
								corr++;
							}
					//		printf("corr>0");
					}
					
					if(correctionTime-corrTime>0.5){//calculate int the most close to the real value
						corrTime++;
					}
					//	printf("moy : %d",(int)correctionSoft*1000);
					//	printf("corr: %u",corr);
					int pgcdi =pgcd(corrTime,corr);
					corrTime=corrTime/pgcdi;
					corr=corr/pgcdi;
					Disable_global_interrupt();
					
					valueCor=valueCor+Coef_Software_Correction*(corr-valueCor);
					timeCorrection=corrTime;
					timeCorr=corrTime;
					Enable_global_interrupt();
						
					
					timeProt.correction.indiceFull=indice;
		}
			
		resetSofftwareCorrection();
		sprintf(messageHMICor,"		update time corr: %d(%d) sign : %d with %d sample",(int)valueCor,corrTime,averageOffset.sign,indice);
		sendHMI(messageHMICor);
		sendHMI("	end Software correction");
	}
	
}


int pgcd( int a ,int b){
	int i=0;
	while ((!(a*b == 0))&&(i<1000)) {
		if (a>b) {
			a = a - b ;
		}
		else{			b = b - a ;		}		i++;	}	if(i>=1000){		return 1;	}	if(a==0){		return b;	}	else{		return a;	}	return a;}
void resetSofftwareCorrection(void){

		sumOffset.second=0;
		sumOffset.halfmillis=0;
		timeProt.correction.nbCorrection=0;
		timeProt.correction.sumTime.second=0;
		timeProt.correction.sumTime.halfmillis=0;
		timeProt.correction.currentTimeOffsetSync.second=0;// to not take the first value
}
bool hardwareCorrection(void){
	if(timeProt.correction.nbCorrection!=0){
		sendHMI("	Hardware correction");
		long int sumOffsetl;
		Clock averageTime={0,0,true};
		float second=(float)timeProt.correction.sumTime.second/timeProt.correction.nbCorrection;
		int secondi=(int)second;
		//calcul average time
		//printf("second : %d,%d",secondi,(int)((second-secondi)*1000.0));
		
		averageTime.second=secondi;
		averageTime.halfmillis=(timeProt.correction.sumTime.halfmillis/timeProt.correction.nbCorrection)+RTC_FREQ*1.0*((float)(second-secondi));
		unsigned long int averageTimeli=(averageTime.second*RTC_FREQ)+(averageTime.halfmillis);
	
		//printf("averageTime : %lu\r\n",averageTime);
		float Fnow=(float)RTC_FREQ*1.0/count_set_period;
		float Fafter=0;
		//calcul of the sumOffset in ms

		sumOffsetl=(sumOffset.second*RTC_FREQ)+(sumOffset.halfmillis);
		if(sumOffset.sign!=true){
			sumOffsetl=-sumOffsetl;
					
		}
			
		

		//printf("correction nb : %d",timeProt.correction.nbCorrection);
		//average per period of sync
		int nbcor=timeProt.correction.nbCorrection;
		sumOffsetl=sumOffsetl/nbcor;
		
		//printf("sumOffsetmoy :  %ld\r\n",sumOffset/2);
		sumOffsetl=sumOffsetl/(float)(averageTimeli/(RTC_FREQ*1.0));
		// average per second (in second)
		//printf("sumOffset/S :  %ld\r\n",sumOffset/2);
	

	
		//number of ms/tick
		float decPerPeriod=(float)sumOffsetl/(RTC_FREQ*1.0);
		
		if(decPerPeriod<1.0){
			Fafter=Fnow/(double)(1-decPerPeriod);
		}
		else{
			Fafter=Fnow/(double)(1+decPerPeriod);
		}
		double dcount_set_period=32000/Fafter;
		unsigned int count;
		count=(int)dcount_set_period;
		if((dcount_set_period-count)>0.5){
			count++;
		}
		if(count>RTC_FREQ){//never <1Hz of frequency
			count=RTC_FREQ;
		
		}
		if(count_set_period!=count){
			count_set_period=count;
			resetSofftwareCorrection();
			valueCor=0;
			timeProt.correction.indiceFull=0;
			timeProt.correction.indiceMoySoftCor=0;
			//printf("count : %u",(unsigned int)count);
			//rtc_count_set_compare(&rtc_instance,VALUE_TIMER,RTC_COUNT_COMPARE_0 );
		//	rtc_count_set_period(&rtc_instance,count_set_period); //MOMO
			RTC->MODE0.COMP[0].reg = count_set_period;
			unsigned int Fuse=(RTC_FREQ/count_set_period);
			unsigned int Fbest=(int)Fafter;
		
			sprintf(messageHMICor,"		Fbest: %u Hz, Fuse= %u Hz count=%d",(unsigned int)Fbest,(unsigned int)Fuse,count);
			sendHMI(messageHMICor);
			sendHMI("	end correction");
			return true;
			
		}
		else{
	

			sendHMI("	end correction");
			return false;
		}
		
	}
	return false;	
}
void stateManagement(){
	if(timeProt.state!=TIMEPROT_IDLE){
		if((xTaskGetTickCount()-timeProt.timePreviousState)>TIMOUTSTATE/portTICK_RATE_MS){//time out 
			timeProt.timePreviousState=xTaskGetTickCount();
			sendHMI("WARNING TIMEOUT arrived"); 

			switch(timeProt.state){
			#ifdef MASTERMODE
				case TIMEPROT_DELAYREQUEST:
					sync();
					//delayResponse(timeProt.previousId);
				break;
			#else
				case TIMEPROT_DELAYRESPONSE:
					delayRequest();
				break;
			#endif
				default:
					sendHMI("ERROR incorrect state");
				break;
			}
		}
	}
}
void timeProtocolTask(void){
	Disable_global_interrupt();

	#ifdef MASTERMODE
		uint8_t i ;
		for(i=0;i<=MAX_SLAVE_CLOCK;i++){
			timeProt.saveTime[i].second=0;
		}
	#else
	
	timeProt.rx.sign=true;
	timeProt.rxDelay.sign=true;
	timeProt.rxSync.sign=true;
	timeProt.tx.sign=true;
	#endif
	timeProt.correction.nbCorrection=0;

	
	sumOffset.second=0;
	sumOffset.halfmillis=0;
	sumOffset.sign=true;
	
	
	timeProt.correction.currentTimeOffsetSync.second=0;
	timeProt.correction.currentTimeOffsetSync.halfmillis=0;
	timeProt.correction.currentTimeOffsetSync.sign=true;
	
	
	timeProt.correction.previoustimeOffset.second=0;
	timeProt.correction.previoustimeOffset.halfmillis=0;
	timeProt.correction.previoustimeOffset.sign=true;
	
	
	//timeProt.correction.timeSoftCor=0;
	timeProt.correction.indiceMoySoftCor=0;
	timeProt.correction.indiceFull=0;


	timeProt.delay.second=0;
	timeProt.delay.halfmillis=0;
	timeProt.delay.sign=true;
	timeProt.waitIdentifier=false;
	timeProt.synchroTimeReceive=NULL;
	timeProt.synchroTimeSendSync=NULL;
	vSemaphoreCreateBinary(timeProt.synchroTimeReceive);
	vSemaphoreCreateBinary(timeProt.synchroTimeSendSync);
	Enable_global_interrupt();
	/*#ifdef SLAVEMODE
	delayRequest();
	#else
	sync();
	#endif
	*/
	timeProt.state=TIMEPROT_IDLE;	
	while(1){
		//sendHMI("time protocol task.");
			//pc.printf("timeprotocol task");
			sender();
			#ifdef MASTERMODE		
			if(timeProt.state!=TIMEPROT_IDLE){
			#endif
			
				if(timeProt.synchroTimeReceive!=NULL){
				
					if( xSemaphoreTake(timeProt.synchroTimeReceive,100/portTICK_RATE_MS) == pdTRUE )	{
						//vTaskDelay(100/portTICK_RATE_MS);//wait to receive alll th edata
						receiver();
					}
					
				}
				
			#ifdef MASTERMODE
			}
			
			#endif
			stateManagement();
			//correction();	
	}	 
	vTaskDelay(500/portTICK_RATE_MS);
}



void sender(void){
		#ifdef MASTERMODE
		
		if(timeProt.synchroTimeSendSync!=NULL){
			if( xSemaphoreTake(timeProt.synchroTimeSendSync,portMAX_DELAY) == pdTRUE ){//stay here until sync request
				if(timeProt.state==TIMEPROT_IDLE){
						networkFlush();
						sync();
				}
				else{
					sendHMI("WARNING sync request!");	
				}
			}
		}
			
		#endif
}
void printfClock(Clock clock){
			if(clock.sign==true){
				pc.printf("%lu s,%u\r\n",clock.second,(unsigned int)clock.halfmillis);
			}
			else{
				pc.printf("-%lu s,%u\r\n",clock.second,(unsigned int)clock.halfmillis);
			}
}
void updateClock(void){
		Clock diff;
		if(timeProt.correction.previousSignOffset==timeProt.offset.sign){
			timeProt.correction.previousSignOffset=timeProt.offset.sign;
			timeProt.correction.previoustimeOffset.second=timeProt.correction.currentTimeOffsetSync.second;
			timeProt.correction.previoustimeOffset.halfmillis=timeProt.correction.currentTimeOffsetSync.halfmillis;
			readClock(&timeProt.correction.currentTimeOffsetSync);
			if(timeProt.correction.previoustimeOffset.second!=0){
				if(sup(timeProt.correction.currentTimeOffsetSync,timeProt.correction.previoustimeOffset)){
												
					diff=subClock(timeProt.correction.currentTimeOffsetSync,timeProt.correction.previoustimeOffset);
					//printf("diff: ");
					//printfClock(diff);
					timeProt.correction.sumTime=sumClock(timeProt.correction.sumTime,diff);
					sumOffset=sumClock(sumOffset,timeProt.offset);
					/*printf("sumOffset: ");
					printfClock(sumOffset);
					printf("sumClock: ");
					printfClock(timeProt.correction.sumTime);
					
					printf("nb corrr  : %d",timeProt.correction.nbCorrection);
					*/
					timeProt.correction.nbCorrection++;
				}
				else{
					resetSofftwareCorrection();
				}
			}
		}
		else{
			resetSofftwareCorrection();
		}
		timeProt.correction.previousSignOffset=timeProt.offset.sign;
		Clock timeCopy;
		/////////////////protected
		readClock(&timeCopy);		
		timeCopy=sumClock(timeCopy,timeProt.offset);//add offset
		writeClock(timeCopy);
		/////////////////
		static bool state;
		state=stateLed;
		if(timeManage.halfmillis<(RTC_FREQ/2)){//all 500ms
			stateLed=LOW;
		}
		else{
			stateLed=HIGH;
		}
		if(state!=stateLed){
			led = stateLed;
			//	if(synchroLed!=NULL){
			//		xSemaphoreGiveFromISR( synchroLed,NULL);
			//	}
		}
		if(timeProt.offset.sign==true){
			sprintf(messageHMIOffset,"		update clock offset: +%lus,%ld[+%lus,%lu ms,~%lu us]	t: %lu,%lu",timeProt.offset.second,(long unsigned int)timeProt.offset.halfmillis,timeProt.offset.second,timeProt.offset.halfmillis/32,(timeProt.offset.halfmillis%32)*31,timeCopy.second,timeCopy.halfmillis);
		}
		else{
			sprintf(messageHMIOffset,"		update clock offset: -%lus,%ld[-%lus,%lums,~%lu us]	t: %lu,%lu",timeProt.offset.second,(long unsigned int)timeProt.offset.halfmillis,timeProt.offset.second,timeProt.offset.halfmillis/32,(timeProt.offset.halfmillis%32)*31,timeCopy.second,timeCopy.halfmillis);
		}
		sendHMI(messageHMIOffset);
}
void crcErrorsend(void){
	
	Type type=CRCERROR;
	send(type,0,NULL);
	sendHMI("	crc cmd send");
	
}
void receiver(){
			#ifdef SLAVEMODE
			Clock t2={0,0,true};
			Clock t3={0,0,true};
			Clock t2subt1={0,0,true};
			Clock t4subt3={0,0,true};
			Clock delayL={0,0,true};
			Clock timeMaster={0,0,true};	
			uint8_t i=0;
			#endif
			Data data;
		
			if(networkAvailable()){
				vTaskDelay(100);
				if(networkRead()==HEADER){
					data.id=networkRead();
					data.type=(Type)networkRead();
							switch (data.type){
									case CRCERROR:
									data.crc=networkRead();
											
											if(calculCRCID(data.type,0,NULL,data.id)!=data.crc){
												sendHMI("ERROR CRC");
												crcErrorsend();
												
											}
											else{
												sendHMI("	CRC cmd receive");
												switch (timeProt.state){

													case TIMEPROT_IDLE:
													//do nothing
											
													break;
													case TIMEPROT_SYNCSTATE:
													//not use
											
													break;
													#ifdef MASTERMODE
													case TIMEPROT_DELAYREQUEST:
														sync();
													break;
													#else
													case TIMEPROT_DELAYRESPONSE:
														delayRequest();
													break;
													#endif
												}
											}
									break;
								#ifdef SLAVEMODE
								
									case SYNC:
										if(timeProt.state==TIMEPROT_IDLE){
											sendHMI("	sync receive");
											data.length=networkRead();
											for(i=0;i<data.length;i++){
												data.data[i]=networkRead();
											}
											data.crc=networkRead();
											//networkFlush();
											if(calculCRCID(data.type,data.length,data.data,data.id)!=data.crc){
												crcErrorsend();
												sendHMI("erreur crc");
												return;
											}
											else{	
												timeProt.t1.second=data.data[0]|(data.data[1]<<8);
												timeProt.t1.halfmillis=data.data[2]|(data.data[3]<<8);
												timeProt.t1.sign=true;
												timeProt.t2.second=timeProt.rxSync.second;
												timeProt.t2.halfmillis=timeProt.rxSync.halfmillis;
												timeProt.t2.sign=true;
												//timeMaster=sumClock(timeMaster,timeProt.delay);
												//timeProt.offset=subClock(timeMaster,timeProt.rxSync);//on calcul le temps de calcul
												//updateClock();
												//sendHMI("	sync completed");
												//networkFlush();
												delayRequest();
												timeProt.state=TIMEPROT_DELAYRESPONSE;
												timeProt.timePreviousState=xTaskGetTickCount();

											}
										}
										else{
											sendHMI("ERROR sync receive but in the wrong state");	
											timeProt.state=TIMEPROT_IDLE;	
											timeProt.timePreviousState=xTaskGetTickCount();									
										}
									break;
									case DELAYRESPONSE:
										if(timeProt.state==TIMEPROT_DELAYRESPONSE){
											data.length=networkRead();
											for(i=0;i<data.length;i++){
												data.data[i]=networkRead();
											}
											data.crc=networkRead();
											//networkFlush();
											if(calculCRCID(data.type,data.length,data.data,data.id)!=data.crc){
												networkFlush();
												crcErrorsend();
												sendHMI("erreur crc");
												return;
											}
											
											sendHMI("	delay response receive");

											if(data.data[0]==IDDEVICE){
												timeProt.state=TIMEPROT_IDLE;
												timeProt.timePreviousState=xTaskGetTickCount();
												//time receive master
												timeProt.t4.second=data.data[1]|(data.data[2]<<8);
												timeProt.t4.halfmillis=data.data[3]|(data.data[4]<<8);
												timeProt.t4.sign=true;
												/*pc.printf("t1");
												printfClock(timeProt.t1);
												pc.printf("t2");
												printfClock(timeProt.t2);
												pc.printf("t3");
												printfClock(timeProt.t3);
												pc.printf("t4");
												printfClock(timeProt.t4);
												*/
												//time send master
												//t3.second=data.data[5]|(data.data[6]<<8);
												//t3.halfmillis=data.data[7]|(data.data[8]<<8);
												t2subt1=subClock(timeProt.t2,timeProt.t1);
												t4subt3=subClock(timeProt.t4,timeProt.t3);								
												delayL=sumClock(t2subt1,t4subt3);									
												timeProt.delay.second=delayL.second/2;
												timeProt.delay.halfmillis=delayL.halfmillis/2;
												
												timeProt.offset=subClock(t4subt3,t2subt1);
												timeProt.offset.second=timeProt.offset.second/2;
												timeProt.offset.halfmillis=timeProt.offset.halfmillis/2;
												updateClock();
											
												if(timeProt.delay.sign==true){
													sprintf(messageHMIDelay,"		update delay: +%lus,%u ms,~%lu us",timeProt.delay.second,timeProt.delay.halfmillis/32,(timeProt.delay.halfmillis%32)*31);
												}
												else{
													sprintf(messageHMIDelay,"		update delay: -%lus,%u ms,~%lu us",timeProt.delay.second,timeProt.delay.halfmillis/32,(timeProt.delay.halfmillis%32)*31);
												}
												sendHMI(messageHMIDelay);
												networkFlush();
												sendHMI("	delay response completed");
											}
											else{
												sendHMI("	delay response not for me");	
											}
										}
										else{
											sendHMI("ERROR delay response receive but wrong state ");
											timeProt.state=TIMEPROT_IDLE;
											timeProt.timePreviousState=xTaskGetTickCount();
											return;
										}
											
									break;
									case ACK:
									
									break;
									case PING:
									
									break;
									
								#else
									case DELAYREQUEST:
										if(timeProt.state==DELAYREQUEST){
											data.crc=networkRead();
											//printf("Type:%d , id:%d, crc:%d.",data.type, data.id,data.crc );
										/*	if(calculCRCID(data.type,0,NULL,data.id)!=data.crc){
												taskENTER_CRITICAL();
												timeProt.saveTime[data.id].second=0;
												taskEXIT_CRITICAL();
												networkFlush();
												crcErrorsend();
												sendHMI("	Erreur crc");
												return;
											}
											else{*/
												sendHMI("	delay request receive");
												delayResponse(data.id);
												timeProt.previousId=data.id;
												
												timeProt.state=TIMEPROT_IDLE;
												timeProt.timePreviousState=xTaskGetTickCount();
												//pc.printf("id:%d",data.id);
											//}
										
										}
										else{
											sendHMI("ERROR delay request receive but wrong state ");
											timeProt.state=TIMEPROT_IDLE;
											timeProt.timePreviousState=xTaskGetTickCount();
											return;
										}
									break;
								#endif
								default:
									sendHMI("	Erreur type unknow");
									networkFlush();	
									
							}		
				}
				
			}
}
#ifdef MASTERMODE
void sync(void){
	uint8_t sendP[4];
	Type type=SYNC;
	Clock timeSave;
	readClock(&timeSave);
	sendP[0]=(uint8_t)(timeSave.second&0x00FF);
	sendP[1]=(uint8_t)((timeSave.second>>8)&0x00FF);
	sendP[2]=(uint8_t)(timeSave.halfmillis&0x00FF);
	sendP[3]=(uint8_t)((timeSave.halfmillis>>8)&0x00FF);
	send(type,4,sendP);
	timeProt.state=TIMEPROT_DELAYREQUEST;
	timeProt.timePreviousState=xTaskGetTickCount();
	sendHMI("	sync send");
}
void delayResponse(uint8_t id){

	
	Type type=DELAYRESPONSE;
	uint8_t sendP[9];
	
	sendP[0]=id;
	sendP[1]=(uint8_t)(timeProt.saveTime[id].second&0x00FF);
	sendP[2]=(uint8_t)((timeProt.saveTime[id].second>>8)&0x00FF);
	sendP[3]=(uint8_t)(timeProt.saveTime[id].halfmillis&0x00FF);
	sendP[4]=(uint8_t)((timeProt.saveTime[id].halfmillis>>8)&0x00FF);
	Clock timeSave={timeManage.second,timeManage.halfmillis,true};
	timeProt.saveTime[id].second=0;
	sendP[5]=(uint8_t)(timeSave.second&0x00FF);
	sendP[6]=(uint8_t)((timeSave.second>>8)&0x00FF);
	sendP[7]=(uint8_t)(timeSave.halfmillis&0x00FF);
	sendP[8]=(uint8_t)((timeSave.halfmillis>>8)&0x00FF);
	
	send(type,9,sendP);
	sprintf(messageHMI,"	delay response send (id: %d)",id);
	sendHMI(messageHMI);
	timeProt.state=TIMEPROT_IDLE;
	timeProt.timePreviousState=xTaskGetTickCount();
}
#endif

#ifdef SLAVEMODE
void delayRequest(void){
	
	Type type=DELAYREQUEST;
	readClock(&timeProt.t3);
	send(type,0,NULL);
	sendHMI("	delay request send");
}
#endif

void send(const Type type,const uint8_t length,const uint8_t* data){
		uint8_t i;
		uint8_t j=0;
		uint8_t send[length+4];//to complete
		send[0]=HEADER;
		send[1]=IDDEVICE;	
		send[2]=type;
		if(length!=0){
			send[3]=length;//length data
			j=1;
		}
		for(i=0;i<length;i++){
			send[3+i+j]=data[i];
		}
		send[3+i+j]=calculCRC(type,length,data);
		vTaskDelay(500/portTICK_RATE_MS);
		networkTx(send,length+4+j);
}
uint8_t calculCRC(const Type type,const uint8_t length,const uint8_t* data){
	return calculCRCID(type,length,data,IDDEVICE);
}
uint8_t calculCRCID(const Type type,const uint8_t length,const uint8_t* data,const uint8_t id){
	uint8_t crc;
	uint8_t i;
	crc=type+length+id;
	for(i=0;i<length;i++){
		crc=crc+data[i];
	}
	
	return crc;
}