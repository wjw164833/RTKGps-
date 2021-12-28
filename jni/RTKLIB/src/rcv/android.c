#include "rtklib.h"
#include "android.h"  
#include <stdint.h>
#include <math.h>
#define SNR_UNIT 0.001 /* SNR unit (dBHz) */
#define FRE_ERROR 0.0001E9
static unsigned char  U1(unsigned char *p) { unsigned char  u; memcpy(&u, p, 1); return u; }
static unsigned short U2(unsigned char *p) { unsigned short u; memcpy(&u, p, 2); return u; }
static unsigned int   U4(unsigned char *p) { unsigned int   u; memcpy(&u, p, 4); return u; }
static int            I4(unsigned char *p) { int            u; memcpy(&u, p, 4); return u; }
static float          R4(unsigned char *p) { float          r; memcpy(&r, p, 4); return r; }
static double         R8(unsigned char *p) { double         r; memcpy(&r, p, 8); return r; }
static long long      L8(unsigned char *p) { long long      r; memcpy(&r, p, 8); return r; }

unsigned char svid2sat(int type, unsigned char svid)
{
	switch (type) {
		case 1: return satno(SYS_GPS, svid);
		case 2: return satno(SYS_SBS, svid);
		case 3: return satno(SYS_GLO, svid);
		case 4:	return satno(SYS_QZS, svid);
		case 5:	return satno(SYS_CMP, svid);
		case 6:	return satno(SYS_GAL, svid);
	}
	return -1;
}
static int frecode(float carrierFrequencyHz, int type, uint8_t *code, double *Freq)
{
	if (type == 1)
	{
		if (fabs(carrierFrequencyHz - FREQL1) < FRE_ERROR) {
			*code = CODE_L1C;
			*Freq = FREQL1;
			return 0;
		}
		if (fabs(carrierFrequencyHz - FREQL5) < FRE_ERROR) {
			*code = CODE_L5Q;
			*Freq = FREQL5;//
			return 2;
		}
	}
	if (type == 2) {
		if (fabs(carrierFrequencyHz - FREQL1) < FRE_ERROR) {
			*code = CODE_L1C;
			*Freq = FREQL1;
			return 0;
		}
		if (fabs(carrierFrequencyHz - FREQL5) < FRE_ERROR) {
			*code = CODE_L5Q;       //
			*Freq = FREQL5;
			return 2;
		}
	}
	if (type == 3) {
		return -1;
	}
	if (type == 4) {
		if (fabs(carrierFrequencyHz - FREQL1) < FRE_ERROR) {
			*code = CODE_L1C;
			*Freq = FREQL1;
			return 0;
		}


		if (fabs(carrierFrequencyHz - FREQL5) < FRE_ERROR) {
			*code = CODE_L5Q;
			*Freq = FREQL5;//
			return 2;
		}
	}
	if (type == 5) {

		if (fabs(carrierFrequencyHz - FREQL1) < FRE_ERROR) {
			*code = CODE_L7Q;     //
			*Freq = FREQL1;
			//return 0;
		}
		else if (fabs(carrierFrequencyHz - FREQ1_CMP) < FRE_ERROR) {
			*code = CODE_L2I;
			*Freq = FREQ1_CMP;
			return 0;
		}
		else if (fabs(carrierFrequencyHz - FREQL5) < FRE_ERROR) {
			*code = CODE_L5X;   //
			*Freq = FREQL5;
			return 1;
		}
	}
	if (type == 6) {

		if (fabs(carrierFrequencyHz - FREQL1) < FRE_ERROR) {
			*code = CODE_L1C;
			*Freq = FREQL1;
			return 0;
		}
		else if (fabs(carrierFrequencyHz - FREQL5) < FRE_ERROR) {
			*code = CODE_L5X;   //
			*Freq = FREQL5;
			return 2;
		}
	}

	return -1;
}
static int obsindex(obs_t *obs, int m, int sat)
{
	int i, j;

	for (i = 0; i < m; i++) {
		if (obs->data[i].sat == (uint8_t)sat) return i;
	}
	return -1;
}
void Timetransmission(int type, long long *rcv_timenanos) {

	switch (type) {
	case 1: break;
	case 2: break;
	case 3: break;
	case 4:	break;
	case 5:
		*rcv_timenanos-=14E9;
		break;
	case 6:	break;
	}
}
int LLI(int accumulatedDeltaRangeState){
	if(accumulatedDeltaRangeState==1)  return 0;
	if(accumulatedDeltaRangeState==8)  return 0;
	if(accumulatedDeltaRangeState==16) return 0;
	if(accumulatedDeltaRangeState==9)  return 0;
	if(accumulatedDeltaRangeState==17) return 0;
	if(accumulatedDeltaRangeState==24) return 0;
	if(accumulatedDeltaRangeState==25) return 0;

	return 1;
}

/* Fill obs_t with data from android_clockd_t and andorid_measurements_t */
int convertObservationData(obs_t *obs, android_clockd_t *cl, android_measurements_t *ms) {
	int i, j, n = 0, m = 0, k = 1, fre;
	int type, sat;
	uint8_t code;
	double Freq;
	android_measurementsd_t *android_obs;
	gtime_t rcv_time, sat_time;
	long long msg_time_nanos;
	long long rcv_timenanos;
	int rcv_week;
	int index;
	double sat_tow;
	if(isnan(cl->biasNanos)) return -1;
	msg_time_nanos =  cl->timeNanos - (cl->fullBiasNanos+cl->biasNanos);
	for (i = 0; i < ms->n; i++) {
		for (j = 0; j < NFREQ + NEXOBS; j++) {
			obs->data[i].L[j] =  0.0;
            obs->data[i].P[j] =  0.0;
			obs->data[i].D[j] = 0.0;
			obs->data[i].SNR[j] =  0.0;
			obs->data[i].LLI[j] =  0.0;
			obs->data[i].code[j] = CODE_NONE;
		}

		type = ms->measurements[i].constellationType;
		if(type>6) continue;
		if(ms->measurements[i].cn0DbHz<=0) continue;
		//if ((type == 1)&&(ms->measurements[i].svid==6))printf("%f\n",ms->measurements[i].carrierFrequencyHz);
		sat = svid2sat(type, ms->measurements[i].svid);
		//if (ms->measurements[i].state==0.0||ms->measurements[i].state==NULL) continue;
		if ((fre = frecode(ms->measurements[i].carrierFrequencyHz, type, &code, &Freq)) < 0) continue;
		if ((index = obsindex(obs, m, sat)) < 0) {
			rcv_timenanos = msg_time_nanos+ms->measurements[i].timeOffsetNanos ;
			rcv_time = nano2gtime(rcv_timenanos);								//�ж������Ƿ񱻼�¼���������¼���ر���¼��λ��
			obs->data[m].time = rcv_time;
			Timetransmission(type, &rcv_timenanos);
			time2gpst(rcv_time, &rcv_week);  /* Calculate week number */
			obs->data[m].sat = (uint8_t)sat;
			obs->data[m].SNR[fre] = (uint16_t)(ms->measurements[i].cn0DbHz*1.0/SNR_UNIT+0.5);
			obs->data[m].LLI[fre] =(uint8_t)LLI(ms->measurements[i].accumulatedDeltaRangeState);//==0x01?1:0; //ms->measurements[i].state;   //lockt[sat-1][1]>0.0?1:0;
			obs->data[m].code[fre] = code;
			obs->data[m].L[fre] = ms->measurements[i].accumulatedDeltaRangeMeters / (double)(CLIGHT / Freq);
			obs->data[m].P[fre] =(double)(fmod(rcv_timenanos,WEEK2SEC*SEC2NSEC)-ms->measurements[i].receivedSvTimeNanos)*(double)(CLIGHT*1E-9); //calcPseudoRange(rcv_time, sat_time);
			obs->data[m].D[fre] = -ms->measurements[i].pseudorangeRateMetersPerSecond / (double)(CLIGHT / Freq);
			//obs->data[index].qualL[fre] = 0;
			//obs->data[m].D[fre] = ms->measurements[i].pseudorangeRateUncertaintyMetersPerSecond;
			m++;
		}
		else {
			rcv_timenanos = msg_time_nanos + ms->measurements[i].timeOffsetNanos;
			rcv_time = nano2gtime(rcv_timenanos);
			obs->data[m].time = rcv_time;
			Timetransmission(type, &rcv_timenanos);
			time2gpst(rcv_time, &rcv_week);  /* Calculate week number */
			obs->data[index].SNR[fre] = (uint16_t)(ms->measurements[i].cn0DbHz*1.0/SNR_UNIT+0.5);
			obs->data[index].LLI[fre] = (uint8_t)LLI(ms->measurements[i].accumulatedDeltaRangeState);//==0x01?1:0;//ms->measurements[i].state;
			obs->data[index].code[fre] = code;
			obs->data[index].L[fre] = ms->measurements[i].accumulatedDeltaRangeMeters / (double)(CLIGHT / Freq);
			obs->data[index].P[fre] = (double)(fmod(rcv_timenanos,WEEK2SEC*SEC2NSEC)-ms->measurements[i].receivedSvTimeNanos)*(double)CLIGHT*1E-9; //calcPseudoRange(rcv_time, sat_time);
			obs->data[index].D[fre] = -ms->measurements[i].pseudorangeRateMetersPerSecond / (double)(CLIGHT / Freq);
			//obs->data[index].qualL[fre] = ((ms->measurements[i].accumulatedDeltaRangeState&0x04)==0x04)?1:0;
			//obs->data[index].qualP[fre] = ms->measurements[i].pseudorangeRateUncertaintyMetersPerSecond;
		}
	}
	obs->n = m-1;

  return (int) observationData;
}

/* ========= ============ ========= */ 
/* ========= Calculations ========= */ 
/* ========= ============ ========= */ 
double nano2sec(long long t){
  return t/((double)SEC2NSEC);
}

gtime_t nano2gtime(long long nanoSec){
  int week;
  double sec;

  week = (int)((double)nanoSec / (double)SEC2NSEC / (double)WEEK2SEC);
  sec = ((double)nanoSec - week * WEEK2SEC * SEC2NSEC) /  (double)SEC2NSEC;

  return gpst2time(week, sec);
}

double calcPseudoRange(gtime_t rx, gtime_t tx){
  double diff = timediff(rx, tx);
  trace(5, "timediff = %f\n", diff);
  return diff * CLIGHT;
}

void Clock_Mearsurement_Data(android_clockd_t *cl, android_measurements_t *ms, uint8_t *str) {
	int i;
	android_measurementsd_t *msd;
	str = str + 6;
	cl->biasNanos=R8(str);
	str += 8;
	cl->biasUncertaintyNanos= R8(str);	
	str += 8;
	//cl->driftNanosPerSecond = R8(str);
	//str += 8;
	//cl->driftUncertaintyNanosPerSecond = R8(str);
	//str += 8;
	cl->fullBiasNanos = L8(str);
	str += 8;
	//cl->hardwareClockDiscontinuityCount = I4(str);
	//str += 4;
	//cl->leapSecond = I4(str);
	//str += 4;
	cl->timeNanos = L8(str);
	str += 8;
	cl->timeUncertaintyNanos = R8(str);
	str += 8;
	cl->hasBiasNanos = I4(str);
	str += 4;
	//cl->hasBiasUncertaintyNanos = I4(str);
	//str += 4;
	//cl->hasDriftNanosPerSecond = I4(str);
	//str += 4;
	//cl->hasDriftUncertaintyNanosPerSecond = I4(str);
	//str += 4;
    cl->hasFullBiasNanos = I4(str);
	//str += 4;
	//cl->hasLeapSecond = I4(str);
	//str += 4;
	//cl->hasTimeUncertaintyNanos = I4(str);
	str += 4;
	ms->n = I4(str);
	str += 4;

	for (i = 0; i < ms->n; i++) {
		msd = &ms->measurements[i];

		msd->accumulatedDeltaRangeMeters = R8(str);         // 相位观测值
		str += 8;
		msd->accumulatedDeltaRangeState = I4(str);
		str += 4;
	   //	msd->accumulatedDeltaRangeUncertaintyMeters = R8(str);
		//str += 8;
		//msd->automaticGainControlLevelDbc = R8(str);
		//str += 8;
		msd->carrierCycles = L8(str);
		str += 8;
		msd->carrierFrequencyHz = R4(str);
		str += 4;
		//msd->carrierPhase = R8(str);
		//str += 8;
		//msd->carrierPhaseUncertainty = R8(str);
		//str += 8;
		msd->cn0DbHz = R8(str);
		str += 8;
		msd->constellationType = I4(str);
		str += 4;
		//msd->multipathIndicator = I4(str);
		//str += 4;
		msd->pseudorangeRateMetersPerSecond = R8(str);
		str += 8;
		msd->pseudorangeRateUncertaintyMetersPerSecond = R8(str);
		str += 8;
		msd->receivedSvTimeNanos = L8(str);
		str += 8;
		msd->receivedSvTimeUncertaintyNanos = L8(str);
		str += 8;
		//msd->snrInDb = R8(str);
		//str += 8;
		msd->state = I4(str);
		str += 4;
		msd->svid = I4(str);
		str += 4;
		msd->timeOffsetNanos = R8(str);
		str += 8;
		//msd->hasAutomaticGainControlLevelDb = I4(str);
		//str += 4;
		//msd->hasCarrierCycles = I4(str);
		//str += 4;
		//msd->hasCarrierFrequencyHz = I4(str);
		//str += 4;
		//msd->hasCarrierPhase = I4(str);
		//str += 4;
		//msd->hasCarrierPhaseUncertainty = I4(str);
		//str += 4;
		//msd->hasSnrInDb = I4(str);
		//str += 4;
	}
}

//static int            I4(unsigned char *p) {int            u; memcpy(&u,p,4); return u;}
int readInt(unsigned char **ptr) {
  int val;
  memcpy(&val, *ptr, sizeof(int));

  val=I4(*ptr);

  trace(5, "parsing int: %02X%02X%02X%02X -> %d\n",
	  ((unsigned char*) *ptr)[0],
	  ((unsigned char*) *ptr)[1],
	  ((unsigned char*) *ptr)[2],
	  ((unsigned char*) *ptr)[3],
	  val);
  *ptr += sizeof(int);
  return val;
}

double readDouble(unsigned char **ptr) {
	double val;
	double val1;

	//double test = 0x7FF8000000000000;
	int i;
	unsigned char tempptr[8];
	for (i=0;i<8;i++)
	{
		tempptr[i]=*((*ptr)+(7-i));
	}
	memcpy(&val1,tempptr,sizeof(double));

	memcpy(&val, *ptr, sizeof(double));

	trace(5, "parsing double: %f\n", val);
	*ptr += sizeof(double);
	//printf("test double: %f\n", test);
	return val;
}

long long readLong(unsigned char **ptr) {
  long long val;
  memcpy(&val, *ptr, sizeof(long long));

  trace(5, "parsing long: %02X%02X%02X%02X %02X%02X%02X%02X -> %ld\n", 
      ((unsigned char*) *ptr)[0],
      ((unsigned char*) *ptr)[1],
	  ((unsigned char*) *ptr)[2],
      ((unsigned char*) *ptr)[3],
      ((unsigned char*) *ptr)[4],
	  ((unsigned char*) *ptr)[5],
	  ((unsigned char*) *ptr)[6],
      ((unsigned char*) *ptr)[7],
      val
      );
  *ptr += sizeof(long long);
  return val;
}

float readFloat(unsigned char **ptr) {
  float val;
  memcpy(&val, *ptr, sizeof(float));
  trace(5, "parsing float: %f\n", val);
  *ptr += sizeof(float);
  return val;
}
static int sync_andriod(unsigned char *buff, unsigned char data)
{
	buff[0] = buff[1]; buff[1] = data;
	return buff[0] == 0xB5 && buff[1] == 0X62;
}
static int checksum(unsigned char *buff, int len)
{
	int cka = 0, ckb = 0;
	int i;

	for (i = 2; i < len - 2; i++) {
		cka += buff[i]; ckb += cka;
	}
	return (unsigned char)(cka&0xff) == buff[len - 2] && (unsigned char)(ckb&0xff) == buff[len - 1];
}
extern int input_android(raw_t *raw, unsigned char data) {
	android_clockd_t cl;
	android_measurements_t ms;

	/* synchronize frame */
	if (raw->nbyte == 0) {
		if (!sync_andriod(raw->buff, data)) return 0;
		raw->nbyte = 2;
		return 0;
	}
	raw->buff[raw->nbyte++] = data;

	if (raw->nbyte == 6) {
		if ((raw->len = U2(raw->buff + 4) + 8) > MAXRAWLEN) {
			trace(2, "ubx length error: len=%d\n", raw->len);
			raw->nbyte = 0;
			return -1;
		}
	}
	if (raw->nbyte < 6 || raw->nbyte < raw->len) return 0;
	raw->nbyte = 0;
	if (!checksum(raw->buff, raw->len)) {
		trace(2, "ubx checksum error: len=%d\n", raw->len);
		return -1;
	}
	Clock_Mearsurement_Data(&cl, &ms, raw->buff);
	return convertObservationData(&raw->obs, &cl, &ms);

}

extern int input_androidf(raw_t *raw, FILE *fp) {
	/*int c;
	int result = noMsg;

	android_clockd_t cl;
	android_measurements_t ms;

	char str[20000] = { "0" };
	char seg[2] = ",";

	if (fgets(str, 20000, fp) == NULL) return endOfFile;
	Clock_Mearsurement_Dataf(&cl, &ms, str, seg);

	return convertObservationData(&raw->obs, &cl, &ms); */
	int i,data;

    trace(4,"input_androidf:\n");

	/* synchronize frame */
	if (raw->nbyte==0) {
		for (i=0;;i++) {
			if ((data=fgetc(fp))==EOF) return -2;
			if (sync_andriod(raw->buff,data)) break;
		}
	}
	if (fread(raw->buff+2,1,4,fp)<4) return -2;
	raw->nbyte=6;

    if ((raw->len=U2(raw->buff+4)+8)>MAXRAWLEN) {
		trace(2,"ubx length error: len=%d\n",raw->len);
        raw->nbyte=0;
        return -1;
    }
    if (fread(raw->buff+6,1,raw->len-6,fp)<(size_t)(raw->len-6)) return -2;
	raw->nbyte=0;

	 if (!checksum(raw->buff, raw->len)) {
		trace(2, "ubx checksum error: len=%d\n", raw->len);
		return -1;
	}

	android_clockd_t cl;
	android_measurements_t ms;

	Clock_Mearsurement_Data(&cl, &ms, raw->buff);
    /* decode android raw message */
	return convertObservationData(&raw->obs, &cl, &ms);
}