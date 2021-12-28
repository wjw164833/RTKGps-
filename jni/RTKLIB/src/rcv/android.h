#define SEC2NSEC 1E9
#define WEEK2SEC (86400*7)
#define UN (unsigned char)

static int TimeFlag=0;
double FirstBiasNanos=0.0;
long long FirstFullBiasNanos=0;
double FirstCount=0.0;//add by lg 20210609


typedef enum ReturnCodes {
  endOfFile         = -2,
  errorMsg          = -1,
  noMsg             = 0,
  observationData   = 1,
  inEphemeris       = 2,
  inSbasMsg         = 3,
  antennaPosParam   = 5,
  dgpsCorrection    = 7,
  inIonUtcParam     = 9,
  ssrMessage        = 10,
  lexMsg            = 31
} ReturnCodes;

typedef enum DeltaRangeStates{
  cycleSlip = 4,
  halfCycleResolved = 8,
  halfCycleReported = 16,
  valid             = 1,
  reset             = 2
} DeltaRangeStates;

                       /* 0 1 2 3 4    5 6 7 8    9 0 11 12 13 14 15 16 */
const int lliArray[17] = {0,0,0,0,0x01,0,0,0,0x40,0,0,0, 0, 0, 0, 0, 0x02};

#define ANDROID_CLOCKD_RECEIVED_SIZE 92
typedef struct {
  double biasNanos;
  double biasUncertaintyNanos;                
  //double driftNanosPerSecond;
 // double driftUncertaintyNanosPerSecond;
  long long fullBiasNanos;                    /* TODO */
  //int hardwareClockDiscontinuityCount;       /* TODO */
  //int leapSecond;
  long long timeNanos;                        /* TODO */
  double timeUncertaintyNanos;               /* TODO */

  int hasBiasNanos;                          /* false */
  //int hasBiasUncertaintyNanos;               /* false */
  //int hasDriftNanosPerSecond;                /* false */
  //int hasDriftUncertaintyNanosPerSecond;     /* false */
  int hasFullBiasNanos;                      /* true */
  //int hasLeapSecond;                         /* false */
  //int hasTimeUncertaintyNanos;               /* true */
} android_clockd_t;

#define ANDROID_MEASUREMENTSD_RECEIVED_SIZE 144
typedef struct {
  double accumulatedDeltaRangeMeters;                        
  int accumulatedDeltaRangeState;
  //double accumulatedDeltaRangeUncertaintyMeters;
  //double automaticGainControlLevelDbc;
  long long carrierCycles;
  float carrierFrequencyHz;
  //double carrierPhase;
  //double carrierPhaseUncertainty;
  double cn0DbHz;                                         /* TODO */
  int constellationType;                                  /* TODO */
  //int multipathIndicator;
  double pseudorangeRateMetersPerSecond;				 /* TODO */
  double pseudorangeRateUncertaintyMetersPerSecond;       /* TODO */
  long long receivedSvTimeNanos;
  long long receivedSvTimeUncertaintyNanos;                /* TODO */
  //double snrInDb;
  int state;                                              /* TODO */
  int svid;
  double timeOffsetNanos;                                 /* TODO */

  //int hasAutomaticGainControlLevelDb;
  //int hasCarrierCycles;                                   /* false */
  //int hasCarrierFrequencyHz;                              /* false */
  //int hasCarrierPhase;                                    /* false */
  //int hasCarrierPhaseUncertainty;                         /* false */
  //int hasSnrInDb;                                         /* false */

} android_measurementsd_t;

#define MAX_MEASUREMENTS 100
typedef struct {
  int n;
  android_measurementsd_t measurements[MAX_MEASUREMENTS];
} android_measurements_t;


int convertObservationData(obs_t *obs, android_clockd_t *cl, android_measurements_t *ms);

double nano2sec(long long t);
gtime_t nano2gtime(long long nanoSec);
double calcPseudoRange(gtime_t rx, gtime_t tx);

void parseClockData(android_clockd_t *cl, unsigned char **ptr);
void parseMeasurementData(android_measurements_t *ms, unsigned char **ptr);

int readInt(unsigned char **ptr);
double readDouble(unsigned char **ptr);
long long readLong(unsigned char **ptr);
float readFloat(unsigned char **ptr);
