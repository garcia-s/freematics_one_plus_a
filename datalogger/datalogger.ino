/*************************************************************************
* Vehicle Telemetry Data Logger for Freematics ONE+
*
* Developed by Stanley Huang <stanley@freematics.com.au>
* Distributed under BSD license
* Visit https://freematics.com/products/freematics-one-plus for more info
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
* THE SOFTWARE.
*************************************************************************/
#include <SPI.h>
#include <FS.h>
#include <SD.h>
#include <SPIFFS.h>
#include <httpd.h>
#include "config.h"
#include <FreematicsPlus.h>
#include "datalogger.h"

// states
#define STATE_STORE_READY 0x1
#define STATE_OBD_READY 0x2
#define STATE_GPS_FOUND 0x4
#define STATE_GPS_READY 0x8
#define STATE_CELL_GPS_FOUND 0x10
#define STATE_MEMS_READY 0x20
#define STATE_FILE_READY 0x40
#define STATE_STANDBY 0x80

void serverProcess(int timeout);
bool serverSetup();
bool serverCheckup(int wifiJoinPeriod = WIFI_JOIN_TIMEOUT);
void initMesh();

uint32_t startTime = 0;
uint32_t pidErrors = 0;
uint32_t fileid = 0;
// live data
char vin[18] = {0};
int16_t batteryVoltage = 0;

typedef struct {
  byte pid;
  byte tier;
  int value;
  uint32_t ts;
} PID_POLLING_INFO;

PID_POLLING_INFO obdData[]= {
};

float acc[3] = {0};
float gyr[3] = {0};
float mag[3] = {0};
float accBias[3];
float temp = 0;
ORIENTATION ori = {0};


GPS_DATA* gd = 0;
uint32_t lastGPStime = 0;

FreematicsESP32 sys;


class SerialDataOutput : public FileLogger
{
    void write(const char* buf, byte len)
    {
#if ENABLE_SERIAL_OUT
        Serial.print(buf);
#endif
    }


    void write(char buf)
    {
#if ENABLE_SERIAL_OUT
        Serial.write(buf);
#endif
    }
};

COBD obd;

#if STORAGE == STORAGE_SD
SDLogger store(new SerialDataOutput);
#else
DataOutputter store;
#endif


MEMS_I2C* mems = 0;

void calibrateMEMS()
{
    // MEMS data collected while sleeping
    accBias[0] = 0;
    accBias[1] = 0;
    accBias[2] = 0;
    int n;
    for (n = 0; n < 100; n++) {
      mems->read(acc);
      accBias[0] += acc[0];
      accBias[1] += acc[1];
      accBias[2] += acc[2];
      delay(10);
    }
    accBias[0] /= n;
    accBias[1] /= n;
    accBias[2] /= n;
    Serial.print("ACC Bias:");
    Serial.print(accBias[0]);
    Serial.print('/');
    Serial.print(accBias[1]);
    Serial.print('/');
    Serial.println(accBias[2]);
}

int handlerLiveData(UrlHandlerParam* param)
{
    char *buf = param->pucBuffer;
    int bufsize = param->bufSize;
    int n = snprintf(buf, bufsize, "{\"obd\":{\"vin\":\"%s\",\"battery\":%d,\"pid\":[", vin, (int)batteryVoltage);
    uint32_t t = millis();
    for (int i = 0; i < sizeof(obdData) / sizeof(obdData[0]); i++) {
        n += snprintf(buf + n, bufsize - n, "{\"pid\":%u,\"value\":%d,\"age\":%u},",
            0x100 | obdData[i].pid, obdData[i].value, t - obdData[i].ts);
    }
    n--;
    n += snprintf(buf + n, bufsize - n, "]}");
    n += snprintf(buf + n, bufsize - n, ",\"mems\":{\"acc\":[%d,%d,%d]",
        (int)((acc[0] - accBias[0]) * 100), (int)((acc[1] - accBias[1]) * 100), (int)((acc[2] - accBias[2]) * 100));
#if USE_MEMS == MEMS_9DOF || USE_MEMS == MEMS_DMP
    n += snprintf(buf + n, bufsize - n, ",\"gyro\":[%d,%d,%d]",
        (int)(gyr[0] * 100), (int)(gyr[1] * 100), (int)(gyr[2] * 100));
#endif
#if USE_MEMS == MEMS_9DOF
    n += snprintf(buf + n, bufsize - n, ",\"mag\":[%d,%d,%d]",
        (int)(mag[0] * 10000), (int)(mag[1] * 10000), (int)(mag[2] * 10000));
#endif
#if ENABLE_ORIENTATION
    n += snprintf(buf + n, bufsize - n, ",\"orientation\":{\"pitch\":\"%f\",\"roll\":\"%f\",\"yaw\":\"%f\"}",
        ori.pitch, ori.roll, ori.yaw);
#endif
    buf[n++] = '}';
    if (lastGPStime){
        n += snprintf(buf + n, bufsize - n, ",\"gps\":{\"date\":%u,\"time\":%u,\"lat\":%f,\"lng\":%f,\"alt\":%f,\"speed\":%f,\"sat\":%u,\"sentences\":%u,\"errors\":%u}",
            gd->date, gd->time, gd->lat, gd->lng, gd->alt, gd->speed, gd->sat,
            gd->sentences, gd->errors);
    } else {
        n += snprintf(buf + n, bufsize - n, ",\"gps\":{\"ready\":\"no\"}");
    }
    buf[n++] = '}';
    param->contentLength = n;
    param->contentType=HTTPFILETYPE_JSON;
    return FLAG_DATA_RAW;
}

// void listDir(fs::FS &fs, const char * dirname, uint8_t levels)
// {
//     Serial.printf("Listing directory: %s\n", dirname);
//     fs::File root = fs.open(dirname);
//     if(!root){
//         Serial.println("Failed to open directory");
//         return;
//     }
//     if(!root.isDirectory()){
//         Serial.println("Not a directory");
//         return;
//     }
//
//     fs::File file = root.openNextFile();
//     while(file){
//         if(file.isDirectory()){
//             Serial.println(file.name());
//             if(levels){
//                 listDir(fs, file.name(), levels -1);
//             }
//         } else {
//             Serial.print(file.name());
//             Serial.print(' ');
//             Serial.print(file.size());
//             Serial.println(" bytes");
//         }
//         file = root.openNextFile();
//     }
// }

class DataLogger
{
public:
    void init()
    {
        if (!checkState(STATE_GPS_FOUND)) {
            Serial.print("GNSS:");
            if (sys.gpsBeginExt(GPS_SERIAL_BAUDRATE)) {
                setState(STATE_GPS_FOUND);
                Serial.println("OK(E)");
            } else if (sys.gpsBegin()) {
                setState(STATE_GPS_FOUND);
                Serial.println("OK(I)");
            } else {
                Serial.println("NO");
            }
        }

        startTime = millis();
    }
    void logLocationData(GPS_DATA* gd)
    {


        store.log(gd->date);
        store.write(',');
        store.log(gd->time);
        store.write(',');
        store.logFloat(gd->lat);
        store.write(',');
        store.logFloat(gd->lng);
        store.write(',');
        store.log(gd->alt);
        store.write(',');
        float kph = gd->speed * 1852 / 1000;
        store.log(kph);
        store.write(',');
        store.log(gd->sat);
        store.write(',');

        // Serial.print("[GNSS] ");
        //
        // char buf[32];
        // int len = sprintf(buf, "%02u:%02u:%02u.%c",
        //     gd->time / 1000000, (gd->time % 1000000) / 10000, (gd->time % 10000) / 100, '0' + (gd->time % 100) / 10);
        // Serial.print(buf);
        // Serial.print(' ');
        // Serial.print(gd->lat, 6);
        // Serial.print(' ');
        // Serial.print(gd->lng, 6);
        // Serial.print(' ');
        // Serial.print((int)kph);
        // Serial.print("km/h");
        // if (gd->sat) {
        //     Serial.print(" SATS:");
        //     Serial.print(gd->sat);
        // }
        // Serial.println();
        //
        lastGPStime = gd->time;
        setState(STATE_GPS_READY);
    }

    void waitGPS()
    {
        int elapsed = 0;
        for (uint32_t t = millis(); millis() - t < 300000;) {
          int t1 = (millis() - t) / 1000;
          if (t1 != elapsed) {
            Serial.print("Waiting for GPS (");
            Serial.print(elapsed);
            Serial.println(")");
            elapsed = t1;
          }
          // read parsed GPS data
          if (sys.gpsGetData(&gd) && gd->sat != 0 && gd->sat != 255) {
            Serial.print("Sats:");
            Serial.println(gd->sat);
            break;
          }
        }
    }
    bool checkState(byte flags) { return (m_state & flags) == flags; }
    void setState(byte flags) { m_state |= flags; }
    void clearState(byte flags) { m_state &= ~flags; }
private:
    byte m_state = 0;
};

DataLogger logger;

void showStats()
{
    uint32_t t = millis() - startTime;
    uint32_t dataCount = store.getDataCount();
    // calculate samples per second
    float sps = (float)dataCount * 1000 / t;
    // output to serial monitor
    char buf[32];
    sprintf(buf, "%02u:%02u.%c", t / 60000, (t % 60000) / 1000, (t % 1000) / 100 + '0');
    Serial.print(buf);
    Serial.print(" | ");
    Serial.print(dataCount);
    Serial.print(" samples | ");
    Serial.print(sps, 1);
    Serial.print(" sps");
    uint32_t fileSize = store.size();
    if (fileSize > 0) {
        Serial.print(" | ");
        Serial.print(fileSize);
        Serial.print(" bytes");
        static uint8_t lastFlushCount = 0;
        uint8_t flushCount = fileSize >> 12;
        if (flushCount != lastFlushCount) {
            store.flush();
            lastFlushCount = flushCount;
            Serial.print(" (flushed)");
        }
    }
    Serial.println();
}
 

void showSysInfo()
{
  Serial.print("CPU:");
  Serial.print(ESP.getCpuFreqMHz());
  Serial.print("MHz FLASH:");
  Serial.print(ESP.getFlashChipSize() >> 20);
  Serial.println("MB");
  Serial.print("IRAM:");
  Serial.print(ESP.getHeapSize() >> 10);
  Serial.print("KB");
#if BOARD_HAS_PSRAM
  Serial.print(" PSRAM:");
  Serial.print(esp_spiram_get_size() >> 20);
  Serial.print("MB");
#endif
  Serial.println();
}


void setup()
{
    delay(500);
    Serial.begin(GPS_SERIAL_BAUDRATE);
    showSysInfo(); 

#ifdef PIN_LED
    // init LED pin
    pinMode(PIN_LED, OUTPUT);
    pinMode(PIN_LED, HIGH);
#endif
  sys.begin(false, USE_GNSS > 1);
//initMesh();
    if (!logger.checkState(STATE_MEMS_READY)) do {
        Serial.print("MEMS:");
        mems = new ICM_42627;
        byte ret = mems->begin();
        if (ret) {
            logger.setState(STATE_MEMS_READY);
            Serial.println("ICM-42627");
            break;
        }
        delete mems;
        mems = new ICM_20948_I2C;
        ret = mems->begin();
        if (ret) {
            logger.setState(STATE_MEMS_READY);
            Serial.println("ICM-20948");
            break;
        }
        delete mems;
        mems = new MPU9250;
        ret = mems->begin();
        if (ret) {
            logger.setState(STATE_MEMS_READY);
            Serial.println("MPU-9250");
            break;
        } 
        Serial.println("NO");
    } while (0);

#if STORAGE == STORAGE_SD
    Serial.print("SD:");
    int volsize = store.begin();
    if (volsize > 0) {
      Serial.print(volsize);
      Serial.println("MB");
      logger.setState(STATE_STORE_READY);
    } else {
      Serial.println("NO");
    }
#endif

#ifdef PIN_LED
    pinMode(PIN_LED, LOW);
#endif

    logger.init();
}

void loop()
{       

    uint32_t ts = millis();
    ///LOGGER HEADER
    // if file not opened, create a new file
    if (logger.checkState(STATE_STORE_READY) && !logger.checkState(STATE_FILE_READY)) {
      fileid = store.open();
      if (fileid) {
        logger.setState(STATE_FILE_READY);
        //PRINTING THE HEADER in first line
        //THIS is kinda retarded right?

          char data[] = "ts,date,time,lat,long,alt,kmh,sats,accx,accy,accz,gyrx,gyry,gyrz";
          store.write(*&data, sizeof(data));
          store.write('\n');
      }
    }

    store.log(ts);   
    store.write(',');

    if (logger.checkState(STATE_GPS_FOUND) && sys.gpsGetData(&gd) ) {
        logger.logLocationData(gd);
    } else {
        store.write(',');   
        store.write(',');
        store.write(',');
        store.write(',');
        store.write(',');
        store.write(',');
        store.write(',');
    }


    if (!logger.checkState(STATE_MEMS_READY)) {
        store.write(',');
        store.write(',');   
        store.write(',');
        store.write(',');
        store.write(',');
    } else {
        mems->read(acc, gyr, mag, &temp, &ori);
        store.log((int16_t)(acc[0] * 100));
        store.write(',');
        store.log((int16_t)(acc[1] * 100));
        store.write(',');
        store.log((int16_t)(acc[2] * 100));
        store.write(',');
        store.log((int16_t)(gyr[0] * 100));
        store.write(',');
        store.log((int16_t)(gyr[1] * 100));
        store.write(',');
        store.log((int16_t)(gyr[2] * 100));

    }

#if !ENABLE_SERIAL_OUT
    showStats();
#endif
    store.write('\n');
    if(logger.checkState(STATE_FILE_READY)) {
        store.flush();
    }

    uint32_t elapsed = millis() - ts;
    if(elapsed < MIN_LOOP_TIME) {
        delay(MIN_LOOP_TIME - elapsed);
    }
}
