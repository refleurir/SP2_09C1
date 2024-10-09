// Arduino pin assignment
#define PIN_LED  9
#define PIN_TRIG 12
#define PIN_ECHO 13

// configurable parameters
#define SND_VEL 346.0     // sound velocity at 24 celsius degree (unit: m/sec)
#define INTERVAL 25       // sampling interval (unit: msec)
#define PULSE_DURATION 10 // ultra-sound Pulse Duration (unit: usec)
#define _DIST_MIN 100     // minimum distance to be measured (unit: mm)
#define _DIST_MAX 300     // maximum distance to be measured (unit: mm)

#define TIMEOUT ((INTERVAL / 2) * 1000.0) // maximum echo waiting time (unit: usec)
#define SCALE (0.001 * 0.5 * SND_VEL)     // coefficent to convert duration to distance

#define _EMA_ALPHA 0.3    // EMA weight of new sample (range: 0 to 1)
                          // Setting EMA to 1 effectively disables EMA filter.
#define N 30               //median filter sample number

// global variables
unsigned long last_sampling_time;   // unit: msec
float dist_prev = _DIST_MAX;        // Distance last-measured
float dist_ema;                     // EMA distance
float dist_array[N];
int index = 0;

void setup() {
  // initialize GPIO pins
  pinMode(PIN_LED,OUTPUT);
  pinMode(PIN_TRIG,OUTPUT);
  pinMode(PIN_ECHO,INPUT);
  digitalWrite(PIN_TRIG, LOW);

  // initialize serial port
  Serial.begin(57600);
  
  for (int i = 0; i < N; i++) {
    dist_array[i] = _DIST_MAX;
  }

}

void loop() {
  float dist_raw;
  
  // wait until next sampling time. 
  // millis() returns the number of milliseconds since the program started. 
  //    will overflow after 50 days.
  if (millis() < last_sampling_time + INTERVAL)
    return;

  // get a distance reading from the USS
  dist_raw = USS_measure(PIN_TRIG,PIN_ECHO);
  
  if ((dist_raw == 0.0) || (dist_raw > _DIST_MAX)) {
      dist_raw = dist_prev;
      digitalWrite(PIN_LED, 1);       // LED OFF
  } else if (dist_raw < _DIST_MIN) {
      dist_raw = dist_prev;
      digitalWrite(PIN_LED, 1);       // LED OFF
  } else {    // In desired Range
      digitalWrite(PIN_LED, 0);       // LED ON      
  }

  // Modify the below line to implement the EMA equation
  dist_ema = _EMA_ALPHA * dist_raw + (1 - _EMA_ALPHA) * dist_prev;
  dist_prev = dist_ema;

  dist_array[index] = dist_raw;
  index = (index + 1) % N;
  float dist_median = median_filter(dist_array, N);

  // output the distance to the serial port
  Serial.print("Min:");   Serial.print(_DIST_MIN);
  Serial.print(",raw:");  Serial.print(dist_raw);
  Serial.print(",ema:");  Serial.print(dist_ema);
  Serial.print(",median:");Serial.print(dist_median);
  Serial.print(",Max:");  Serial.print(_DIST_MAX);
  Serial.println("");

  // update last sampling time
  last_sampling_time += INTERVAL;
}

// get a distance reading from USS. return value is in millimeter.
float USS_measure(int TRIG, int ECHO)
{
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(PULSE_DURATION);
  digitalWrite(TRIG, LOW);
  
  return pulseIn(ECHO, HIGH, TIMEOUT) * SCALE; // unit: mm
}

float median_filter(float samples[], int size) {
  float filtered[size];
  for (int i = 0; i < size; i++) {
    filtered[i] = samples[i];
  }

  for (int i = 0; i < size - 1; i++) {
    for (int j = i + 1; j < size; j++) {
      if (filtered[i] > filtered[j]) {
        float temp = filtered[i];
        filtered[i] = filtered[j];
        filtered[j] = temp;
      }
    }
  }
   if (size % 2 == 1) {
    return filtered[size / 2];
  } else {
    return (filtered[size / 2 - 1] + filtered[size / 2]) / 2.0;
  }

  }
