/*************************************************************************
 * OBD/MEMS/GPS Data Logger for Freematics ONE+
 * Data logging and file storage on microSD or SPIFFS
 *
 * Distributed under BSD license
 * Visit http://freematics.com/products/freematics-one-plus for more information
 * Developed by Stanley Huang <stanley@freematics.com.au>
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *************************************************************************/

#include <cstdint>
class FileLogger {
public:
  virtual int begin() { return 1; }

  virtual void log(int value) {
    char buf[24];
    byte len = sprintf(buf, "%d", value);
    write(buf, len);
    m_dataCount++;
  }

  virtual void logFloat(float value) {
    char buf[32];
    byte len = sprintf(buf, "%f", value);
    write(buf, len);
    m_dataCount++;
  }

  virtual uint32_t open() {
    m_dataCount = 0;
    return 0;
  }

  virtual void write(const char* buf, byte len) {
    if (m_next)
      m_next->write(buf, len);
    m_file.write((uint8_t *)buf, len);
  }

  virtual void write(char buf) {
    if (m_next)
      m_next->write(buf);
    m_file.write(buf);
  }

  virtual uint32_t size() { return m_file.position(); }
  virtual void close() {
    m_file.close();
    m_id = 0;
    m_dataCount = 0;
  }

  virtual void flush() { m_file.flush(); }
  virtual uint32_t getDataCount() { return m_dataCount; }

protected:
  int getFileID(File &root) {
    m_dataCount = 0;
    if (root) {
      int id = 0;
      File file;
      while (file = root.openNextFile()) {
        char *p = strrchr(file.name(), '/');
        unsigned int n = atoi(p ? p + 1 : file.name());
        if (n > id)
          id = n;
      }
      return id + 1;
    }
    return 0;
  }
  uint32_t m_dataTime = 0;
  uint32_t m_dataCount = 0;
  uint32_t m_id = 0;
  File m_file;
  FileLogger *m_next = 0;
};

class SDLogger : public FileLogger {
public:
  SDLogger(FileLogger *next = 0) { m_next = next; }
  int begin() {
    SPI.begin();
    if (SD.begin(PIN_SD_CS, SPI)) {
      return SD.cardSize() >> 20;
    } else {
      return -1;
    }
  }
  uint32_t open() {
    File root = SD.open("/DATA");
    m_id = getFileID(root);
    if (m_id == 0) {
      SD.mkdir("/DATA");
      m_id = 1;
    }

    char path[24];
    sprintf(path, "/DATA/%u.csv", m_id);
    Serial.print("File: ");
    Serial.println(path);
    m_dataCount = 0;
    m_size = 0;
    m_file = SD.open(path, FILE_APPEND, true);
    if (!m_file) {
      Serial.println("File error");
      m_id = 0;
    }
    return m_id;
  }

  void flush() {
    char path[24];
    sprintf(path, "/DATA/%u.csv", m_id);
    m_file.close();
    m_file = SD.open(path, FILE_APPEND);
    if (!m_file) {
      Serial.println("File error");
    }
  }
  void write(const char *buf, byte len) {
    if (m_next)
      m_next->write(buf, len);
    m_file.write((uint8_t *)buf, len);
  }

  virtual void write(char buf) {
    if (m_next)
      m_next->write(buf);
    m_file.write(buf);
  }

  void setTimestamp(uint32_t ts) { m_dataTime = ts; }

private:
  uint32_t m_size = 0;
};

// class SPIFFSLogger : public FileLogger {
// public:
//   SPIFFSLogger(FileLogger *next = 0) { m_next = next; }
//   int begin() {
//     if (SPIFFS.begin(true)) {
//       return SPIFFS.totalBytes() - SPIFFS.usedBytes();
//     }
//     return -1;
//   }
//   void write(const char *buf, byte len) {
//     if (m_next)
//       m_next->write(buf, len);
//     if (m_id == 0)
//       return;
//     if (m_file.write((uint8_t *)buf, len) != len) {
//       purge();
//       if (m_file.write((uint8_t *)buf, len) != len) {
//         Serial.println("Error writing data");
//         return;
//       }
//     }
//   }
//   uint32_t open() {
//     File root = SPIFFS.open("/");
//     m_id = getFileID(root);
//     char path[24];
//     sprintf(path, "/DATA/%u.CSV", m_id);
//     Serial.print("File: ");
//     Serial.println(path);
//     m_dataCount = 0;
//     m_file = SPIFFS.open(path, FILE_WRITE);
//     if (!m_file) {
//       Serial.println("File error");
//       m_id = 0;
//     }
//     return m_id;
//   }
//
// private:
//   void purge() {
//     // remove oldest file when unused space is insufficient
//     File root = SPIFFS.open("/");
//     File file;
//     int idx = 0;
//     while (file = root.openNextFile()) {
//       if (!strncmp(file.name(), "/DATA/", 6)) {
//         unsigned int n = atoi(file.name() + 6);
//         if (n != 0 && (idx == 0 || n < idx))
//           idx = n;
//       }
//     }
//     if (idx) {
//       m_file.close();
//       char path[32];
//       sprintf(path, "/DATA/%u.CSV", idx);
//       SPIFFS.remove(path);
//       Serial.print(path);
//       Serial.println(" removed");
//       sprintf(path, "/DATA/%u.CSV", m_id);
//       m_file = SPIFFS.open(path, FILE_APPEND);
//       if (!m_file)
//         m_id = 0;
//     }
//   }
// };
